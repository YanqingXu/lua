#!/usr/bin/env python3
"""Strict validator for normalized release source-readiness evidence."""

from __future__ import annotations

import json
import re
from collections.abc import Mapping
from datetime import datetime, timezone
from typing import Any


EVIDENCE_SCHEMA = "lua-cpp.source-readiness-evidence/v1"
EVIDENCE_FIELDS = {
    "schema",
    "generated_at",
    "repository",
    "candidate_sha",
    "version",
    "project_version",
    "abi_version",
    "checks",
}
CHECK_NAMES = {
    "source_version_consistent",
    "required_files_present",
    "worktree_clean",
    "public_api_contract",
}

_REPOSITORY_RE = re.compile(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+")
_SHA_RE = re.compile(r"[0-9a-f]{40}")
_VERSION_RE = re.compile(r"[0-9]+\.[0-9]+\.[0-9]+(?:-rc\.[0-9]+)?")
_PROJECT_VERSION_RE = re.compile(r"[0-9]+\.[0-9]+\.[0-9]+")
_UTC_TIMESTAMP_RE = re.compile(
    r"(?:[0-9]{4})-(?:[0-9]{2})-(?:[0-9]{2})"
    r"T(?:[0-9]{2}):(?:[0-9]{2}):(?:[0-9]{2})Z"
)


class SourceReadinessError(RuntimeError):
    """Raised when source-readiness evidence is incomplete or inconsistent."""


def _reject_constant(value: str) -> object:
    raise SourceReadinessError(f"non-standard JSON value is forbidden: {value}")


def _unique_object(pairs: list[tuple[str, object]]) -> dict[str, object]:
    result: dict[str, object] = {}
    for key, value in pairs:
        if key in result:
            raise SourceReadinessError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def parse_source_readiness_json(raw: str) -> Mapping[str, Any]:
    if not isinstance(raw, str) or not raw.strip():
        raise SourceReadinessError("source-readiness evidence JSON is missing")
    try:
        parsed = json.loads(
            raw,
            object_pairs_hook=_unique_object,
            parse_constant=_reject_constant,
        )
    except SourceReadinessError:
        raise
    except (TypeError, ValueError, json.JSONDecodeError) as error:
        raise SourceReadinessError(
            "source-readiness evidence is not strict JSON"
        ) from error
    if not isinstance(parsed, Mapping):
        raise SourceReadinessError("source-readiness evidence root must be an object")
    return parsed


def _expect_fields(value: Mapping[str, Any], expected: set[str], label: str) -> None:
    actual = set(value)
    if actual != expected:
        raise SourceReadinessError(
            f"{label} fields mismatch; "
            f"missing={sorted(expected - actual)}, extra={sorted(actual - expected)}"
        )


def _string(value: object, label: str) -> str:
    if not isinstance(value, str) or not value or value != value.strip():
        raise SourceReadinessError(f"{label} must be a non-empty trimmed string")
    return value


def _timestamp(value: object, label: str) -> datetime:
    text = _string(value, label)
    if _UTC_TIMESTAMP_RE.fullmatch(text) is None:
        raise SourceReadinessError(
            f"{label} must be canonical UTC YYYY-MM-DDTHH:MM:SSZ"
        )
    try:
        return datetime.strptime(text, "%Y-%m-%dT%H:%M:%SZ").replace(
            tzinfo=timezone.utc
        )
    except ValueError as error:
        raise SourceReadinessError(f"{label} is not a valid UTC timestamp") from error


def validate_source_readiness_evidence(
    evidence: Mapping[str, Any],
    *,
    expected_repository: str,
    expected_sha: str,
    expected_version: str,
    now: datetime | None = None,
) -> dict[str, object]:
    """Validate source identity, version/ABI binding, and all readiness checks."""

    if not isinstance(evidence, Mapping):
        raise SourceReadinessError("source-readiness evidence must be an object")
    _expect_fields(evidence, EVIDENCE_FIELDS, "source-readiness evidence")
    if evidence.get("schema") != EVIDENCE_SCHEMA:
        raise SourceReadinessError("source-readiness evidence schema mismatch")

    repository = _string(evidence.get("repository"), "source readiness repository")
    if _REPOSITORY_RE.fullmatch(repository) is None:
        raise SourceReadinessError("source readiness repository must use owner/name")
    candidate_sha = _string(
        evidence.get("candidate_sha"),
        "source readiness candidate SHA",
    )
    if _SHA_RE.fullmatch(candidate_sha) is None:
        raise SourceReadinessError(
            "source readiness candidate SHA must be lowercase full SHA"
        )
    version = _string(evidence.get("version"), "source readiness version")
    project_version = _string(
        evidence.get("project_version"),
        "source readiness project version",
    )
    if _VERSION_RE.fullmatch(version) is None:
        raise SourceReadinessError("source readiness version is unsupported")
    if _PROJECT_VERSION_RE.fullmatch(project_version) is None:
        raise SourceReadinessError("source readiness project version is invalid")
    if re.sub(r"-rc\.[0-9]+$", "", version) != project_version:
        raise SourceReadinessError(
            "source readiness version does not match project version"
        )

    if repository != expected_repository:
        raise SourceReadinessError("source readiness repository mismatch")
    if candidate_sha != expected_sha:
        raise SourceReadinessError("source readiness candidate SHA mismatch")
    if version != expected_version:
        raise SourceReadinessError("source readiness version mismatch")

    abi_version = evidence.get("abi_version")
    if (
        isinstance(abi_version, bool)
        or not isinstance(abi_version, int)
        or abi_version < 0
    ):
        raise SourceReadinessError(
            "source readiness ABI version must be a non-negative integer"
        )

    generated_at = _timestamp(
        evidence.get("generated_at"),
        "source readiness generated_at",
    )
    current = now or datetime.now(timezone.utc)
    if current.tzinfo is None:
        raise SourceReadinessError("source readiness validation time has no timezone")
    current = current.astimezone(timezone.utc)
    if generated_at > current:
        raise SourceReadinessError("source-readiness evidence was generated in the future")

    raw_checks = evidence.get("checks")
    if not isinstance(raw_checks, Mapping):
        raise SourceReadinessError("source readiness checks must be an object")
    _expect_fields(raw_checks, CHECK_NAMES, "source readiness checks")
    checks: dict[str, str] = {}
    for name in sorted(CHECK_NAMES):
        if raw_checks.get(name) != "passed":
            raise SourceReadinessError(f"source readiness check did not pass: {name}")
        checks[name] = "passed"

    return {
        "schema": EVIDENCE_SCHEMA,
        "generated_at": generated_at.strftime("%Y-%m-%dT%H:%M:%SZ"),
        "repository": repository,
        "candidate_sha": candidate_sha,
        "version": version,
        "project_version": project_version,
        "abi_version": abi_version,
        "checks": checks,
    }
