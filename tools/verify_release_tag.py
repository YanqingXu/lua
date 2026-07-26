#!/usr/bin/env python3
"""Fail-closed verification that a release ref is an annotated tag for one commit."""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
import urllib.error
import urllib.parse
import urllib.request
from collections.abc import Mapping
from typing import Any, Protocol


TOOL_VERSION = "1.0.0"
MAX_TAG_DEPTH = 8
_REPOSITORY_RE = re.compile(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+")
_SHA_RE = re.compile(r"[0-9a-f]{40}")


class TagIdentityError(RuntimeError):
    """Raised when the release tag identity cannot be proven."""


class RestApi(Protocol):
    def get(self, path: str) -> object:
        """Return decoded JSON for one GitHub REST request."""


class GitHubRestClient:
    """Minimal standard-library GitHub REST client."""

    def __init__(
        self,
        token: str,
        api_url: str = "https://api.github.com",
        timeout_seconds: float = 30.0,
    ):
        if not token:
            raise TagIdentityError("GitHub token is required")
        self._token = token
        self._api_url = api_url.rstrip("/")
        self._timeout_seconds = timeout_seconds

    def get(self, path: str) -> object:
        request = urllib.request.Request(
            f"{self._api_url}/{path.lstrip('/')}",
            headers={
                "Accept": "application/vnd.github+json",
                "Authorization": f"Bearer {self._token}",
                "User-Agent": f"lua-cpp-release-tag/{TOOL_VERSION}",
                "X-GitHub-Api-Version": "2022-11-28",
            },
        )
        try:
            with urllib.request.urlopen(
                request,
                timeout=self._timeout_seconds,
            ) as response:
                payload = response.read()
        except urllib.error.HTTPError as error:
            raise TagIdentityError(
                f"GitHub API returned HTTP {error.code} for {path}"
            ) from error
        except (urllib.error.URLError, TimeoutError, OSError) as error:
            raise TagIdentityError(
                f"GitHub API request failed for {path}: {error}"
            ) from error

        try:
            return json.loads(payload.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError) as error:
            raise TagIdentityError(
                f"GitHub API returned invalid JSON for {path}"
            ) from error


def _mapping(value: object, label: str) -> Mapping[str, Any]:
    if not isinstance(value, Mapping):
        raise TagIdentityError(f"{label} is not a JSON object")
    return value


def _string(value: object, label: str) -> str:
    if not isinstance(value, str) or not value:
        raise TagIdentityError(f"{label} is missing")
    return value


def _sha(value: object, label: str) -> str:
    text = _string(value, label)
    if _SHA_RE.fullmatch(text) is None:
        raise TagIdentityError(
            f"{label} is not a lowercase 40-character Git object SHA"
        )
    return text


def _repository(value: str) -> str:
    if _REPOSITORY_RE.fullmatch(value) is None:
        raise TagIdentityError("repository must be in owner/name form")
    return value


def _tag_name(value: str) -> str:
    if (
        not value
        or value.startswith("/")
        or value.endswith("/")
        or value.startswith("refs/")
        or any(character.isspace() or ord(character) < 0x20 for character in value)
    ):
        raise TagIdentityError("tag name is invalid")
    return value


def verify_release_tag(
    api: RestApi,
    repository: str,
    tag_name: str,
    expected_commit_sha: str,
    *,
    max_tag_depth: int = MAX_TAG_DEPTH,
) -> str:
    """Return the commit SHA after proving an annotated tag's immutable identity."""

    repository = _repository(repository)
    tag_name = _tag_name(tag_name)
    expected_commit_sha = _sha(expected_commit_sha, "expected commit SHA")
    if isinstance(max_tag_depth, bool) or not isinstance(max_tag_depth, int):
        raise TagIdentityError("maximum tag depth must be a positive integer")
    if max_tag_depth <= 0:
        raise TagIdentityError("maximum tag depth must be a positive integer")

    encoded_tag = urllib.parse.quote(tag_name, safe="")
    reference = _mapping(
        api.get(f"/repos/{repository}/git/ref/tags/{encoded_tag}"),
        "release tag ref",
    )
    if _string(reference.get("ref"), "release tag ref name") != (
        f"refs/tags/{tag_name}"
    ):
        raise TagIdentityError("release tag ref name does not match the requested tag")
    reference_object = _mapping(
        reference.get("object"),
        "release tag ref object",
    )
    reference_type = _string(
        reference_object.get("type"),
        "release tag ref object type",
    )
    if reference_type != "tag":
        raise TagIdentityError(
            "release ref is not an annotated tag (lightweight tags are rejected)"
        )

    current_tag_sha = _sha(
        reference_object.get("sha"),
        "release tag ref object SHA",
    )
    seen_tag_shas: set[str] = set()

    for depth in range(max_tag_depth):
        if current_tag_sha in seen_tag_shas:
            raise TagIdentityError("annotated tag chain contains a cycle")
        seen_tag_shas.add(current_tag_sha)

        tag_object = _mapping(
            api.get(f"/repos/{repository}/git/tags/{current_tag_sha}"),
            f"annotated tag object at depth {depth + 1}",
        )
        returned_tag_sha = _sha(
            tag_object.get("sha"),
            f"annotated tag object SHA at depth {depth + 1}",
        )
        if returned_tag_sha != current_tag_sha:
            raise TagIdentityError(
                f"annotated tag object SHA mismatch at depth {depth + 1}"
            )
        object_tag_name = _string(
            tag_object.get("tag"),
            f"annotated tag name at depth {depth + 1}",
        )
        if depth == 0 and object_tag_name != tag_name:
            raise TagIdentityError(
                "top-level annotated tag name does not match the requested tag"
            )

        target = _mapping(
            tag_object.get("object"),
            f"annotated tag target at depth {depth + 1}",
        )
        target_type = _string(
            target.get("type"),
            f"annotated tag target type at depth {depth + 1}",
        )
        target_sha = _sha(
            target.get("sha"),
            f"annotated tag target SHA at depth {depth + 1}",
        )
        if target_type == "commit":
            if target_sha != expected_commit_sha:
                raise TagIdentityError(
                    "annotated release tag resolves to a different candidate commit"
                )
            return target_sha
        if target_type != "tag":
            raise TagIdentityError(
                f"annotated tag target type is unsupported at depth {depth + 1}"
            )
        current_tag_sha = target_sha

    raise TagIdentityError(
        f"annotated tag chain exceeds the maximum depth of {max_tag_depth}"
    )


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Verify that a GitHub release ref is an annotated tag resolving to "
            "the exact evidence-approved candidate commit."
        )
    )
    parser.add_argument(
        "--repository",
        default=os.environ.get("GITHUB_REPOSITORY"),
        help="GitHub repository in owner/name form (default: GITHUB_REPOSITORY)",
    )
    parser.add_argument(
        "--tag",
        default=os.environ.get("GITHUB_REF_NAME"),
        help="release tag name (default: GITHUB_REF_NAME)",
    )
    parser.add_argument(
        "--expected-sha",
        required=True,
        help="exact evidence-approved candidate commit SHA",
    )
    parser.add_argument(
        "--token",
        default=os.environ.get("GITHUB_TOKEN"),
        help=argparse.SUPPRESS,
    )
    return parser.parse_args(argv)


def main(
    argv: list[str] | None = None,
    *,
    client: RestApi | None = None,
) -> int:
    args = parse_args(argv)
    try:
        if not args.repository:
            raise TagIdentityError(
                "repository is required via --repository or GITHUB_REPOSITORY"
            )
        if not args.tag:
            raise TagIdentityError("tag is required via --tag or GITHUB_REF_NAME")
        if client is None:
            client = GitHubRestClient(args.token or "")
        commit_sha = verify_release_tag(
            client,
            args.repository,
            args.tag,
            args.expected_sha,
        )
    except TagIdentityError as error:
        print(f"release tag verification failed: {error}", file=sys.stderr)
        return 1

    print(
        f"verified annotated release tag {args.tag} resolves to {commit_sha}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
