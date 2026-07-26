#!/usr/bin/env python3
"""Contract tests for the annotated release-tag identity gate."""

from __future__ import annotations

import unittest
from pathlib import Path

import verify_release_tag as verifier


REPOSITORY = "example/lua-cpp"
TAG_NAME = "v0.1.0-rc.1"
COMMIT_SHA = "a" * 40
FIRST_TAG_SHA = "b" * 40
SECOND_TAG_SHA = "c" * 40


class FakeGitHub:
    def __init__(self) -> None:
        self.reference: object = {
            "ref": f"refs/tags/{TAG_NAME}",
            "object": {
                "type": "tag",
                "sha": FIRST_TAG_SHA,
            },
        }
        self.tags: dict[str, object] = {
            FIRST_TAG_SHA: {
                "sha": FIRST_TAG_SHA,
                "tag": TAG_NAME,
                "object": {
                    "type": "tag",
                    "sha": SECOND_TAG_SHA,
                },
            },
            SECOND_TAG_SHA: {
                "sha": SECOND_TAG_SHA,
                "tag": "release-candidate-signed",
                "object": {
                    "type": "commit",
                    "sha": COMMIT_SHA,
                },
            },
        }
        self.fail_path: str | None = None

    def get(self, path: str) -> object:
        if path == self.fail_path:
            raise verifier.TagIdentityError(f"API unavailable: {path}")
        reference_path = f"/repos/{REPOSITORY}/git/ref/tags/{TAG_NAME}"
        if path == reference_path:
            return self.reference
        prefix = f"/repos/{REPOSITORY}/git/tags/"
        if path.startswith(prefix):
            sha = path.removeprefix(prefix)
            if sha not in self.tags:
                raise verifier.TagIdentityError(f"missing fake tag object: {sha}")
            return self.tags[sha]
        raise verifier.TagIdentityError(f"unexpected fake API path: {path}")


class VerifyReleaseTagTests(unittest.TestCase):
    def test_nested_annotated_tag_resolves_to_exact_candidate(self) -> None:
        result = verifier.verify_release_tag(
            FakeGitHub(),
            REPOSITORY,
            TAG_NAME,
            COMMIT_SHA,
        )
        self.assertEqual(COMMIT_SHA, result)

    def test_lightweight_tag_is_rejected(self) -> None:
        api = FakeGitHub()
        api.reference["object"]["type"] = "commit"
        api.reference["object"]["sha"] = COMMIT_SHA
        with self.assertRaisesRegex(
            verifier.TagIdentityError,
            "lightweight tags are rejected",
        ):
            verifier.verify_release_tag(api, REPOSITORY, TAG_NAME, COMMIT_SHA)

    def test_wrong_candidate_commit_is_rejected(self) -> None:
        api = FakeGitHub()
        api.tags[SECOND_TAG_SHA]["object"]["sha"] = "d" * 40
        with self.assertRaisesRegex(
            verifier.TagIdentityError,
            "different candidate commit",
        ):
            verifier.verify_release_tag(api, REPOSITORY, TAG_NAME, COMMIT_SHA)

    def test_malformed_api_fields_are_rejected(self) -> None:
        cases = (
            (
                lambda api: api.reference.__setitem__("ref", f"refs/tags/{TAG_NAME}-other"),
                "ref name does not match",
            ),
            (
                lambda api: api.reference["object"].__setitem__("sha", "short"),
                "not a lowercase 40-character",
            ),
            (
                lambda api: api.tags[FIRST_TAG_SHA].__setitem__(
                    "sha",
                    "d" * 40,
                ),
                "object SHA mismatch",
            ),
            (
                lambda api: api.tags[FIRST_TAG_SHA].__setitem__("tag", ""),
                "annotated tag name.*missing",
            ),
            (
                lambda api: api.tags[FIRST_TAG_SHA]["object"].__setitem__(
                    "type",
                    "tree",
                ),
                "target type is unsupported",
            ),
            (
                lambda api: api.tags[FIRST_TAG_SHA].__setitem__("object", []),
                "target at depth 1 is not a JSON object",
            ),
        )
        for mutate, expected in cases:
            with self.subTest(expected=expected):
                api = FakeGitHub()
                mutate(api)
                with self.assertRaisesRegex(verifier.TagIdentityError, expected):
                    verifier.verify_release_tag(
                        api,
                        REPOSITORY,
                        TAG_NAME,
                        COMMIT_SHA,
                    )

    def test_annotated_tag_cycle_is_rejected(self) -> None:
        api = FakeGitHub()
        api.tags[SECOND_TAG_SHA]["object"] = {
            "type": "tag",
            "sha": FIRST_TAG_SHA,
        }
        with self.assertRaisesRegex(verifier.TagIdentityError, "contains a cycle"):
            verifier.verify_release_tag(api, REPOSITORY, TAG_NAME, COMMIT_SHA)

    def test_annotated_tag_depth_is_bounded(self) -> None:
        api = FakeGitHub()
        api.tags = {}
        tag_shas = [f"{value:040x}" for value in range(1, 10)]
        api.reference["object"]["sha"] = tag_shas[0]
        for index, sha in enumerate(tag_shas):
            api.tags[sha] = {
                "sha": sha,
                "tag": TAG_NAME if index == 0 else f"nested-{index}",
                "object": {
                    "type": "commit" if index == len(tag_shas) - 1 else "tag",
                    "sha": (
                        COMMIT_SHA
                        if index == len(tag_shas) - 1
                        else tag_shas[index + 1]
                    ),
                },
            }
        with self.assertRaisesRegex(
            verifier.TagIdentityError,
            "exceeds the maximum depth of 8",
        ):
            verifier.verify_release_tag(api, REPOSITORY, TAG_NAME, COMMIT_SHA)

    def test_api_error_is_fail_closed(self) -> None:
        api = FakeGitHub()
        api.fail_path = f"/repos/{REPOSITORY}/git/tags/{SECOND_TAG_SHA}"
        with self.assertRaisesRegex(verifier.TagIdentityError, "API unavailable"):
            verifier.verify_release_tag(api, REPOSITORY, TAG_NAME, COMMIT_SHA)

    def test_cli_returns_failure_on_verification_error(self) -> None:
        api = FakeGitHub()
        api.reference["object"]["type"] = "commit"
        result = verifier.main(
            [
                "--repository",
                REPOSITORY,
                "--tag",
                TAG_NAME,
                "--expected-sha",
                COMMIT_SHA,
            ],
            client=api,
        )
        self.assertEqual(1, result)

    def test_workflow_rechecks_tag_immediately_before_release_creation(self) -> None:
        repository_root = Path(__file__).resolve().parents[1]
        workflow = (
            repository_root / ".github/workflows/release.yml"
        ).read_text(encoding="utf-8")
        create_step_marker = "      - name: Create GitHub release\n"
        self.assertEqual(1, workflow.count(create_step_marker))
        create_step = workflow.split(create_step_marker, maxsplit=1)[1]
        verify_command = "          python3 tools/verify_release_tag.py \\\n"
        publish_command = '          gh "${args[@]}"\n'
        self.assertEqual(1, create_step.count(verify_command))
        self.assertEqual(1, create_step.count(publish_command))
        verify_position = create_step.index(verify_command)
        publish_position = create_step.index(publish_command)
        self.assertLess(verify_position, publish_position)
        intervening_lines = create_step[
            verify_position + len(verify_command) : publish_position
        ].splitlines()
        self.assertEqual(
            [
                '            --repository "$GITHUB_REPOSITORY" \\',
                '            --tag "$GITHUB_REF_NAME" \\',
                '            --expected-sha "$CANDIDATE_SHA"',
            ],
            intervening_lines,
        )


if __name__ == "__main__":
    unittest.main()
