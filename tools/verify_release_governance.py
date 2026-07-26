#!/usr/bin/env python3
"""Normalize and strictly validate release-governance evidence.

Tag publication consumes one repository variable,
``LUA_RELEASE_GOVERNANCE_ATTESTATION``.  Its value is a single-line JSON
attestation bound to one repository, candidate commit, tag, and version.  A
manual workflow dispatch deliberately produces non-publishing
``candidate-only`` evidence instead of treating any repository-wide boolean as
approval.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
import tempfile
import urllib.parse
from collections.abc import Mapping, Sequence
from datetime import datetime, timedelta, timezone
from pathlib import Path
from typing import Any


TOOL_VERSION = "1.0.0"
ATTESTATION_SCHEMA = "lua-cpp.release-governance-attestation/v1"
EVIDENCE_SCHEMA = "lua-cpp.release-governance-evidence/v1"
DECISIONS = {"protected-ruleset", "time-limited-waiver"}
CONTROL_NAMES = (
    "required_ci_checks",
    "branch_must_be_up_to_date",
    "default_branch_force_push_blocked",
    "default_branch_deletion_blocked",
    "tag_creation_restricted",
    "tag_deletion_restricted",
)
CONTROL_STATES = {"enforced", "waived"}
ATTESTATION_FIELDS = {
    "schema",
    "repository",
    "candidate_sha",
    "tag",
    "version",
    "decision",
    "approved_by",
    "independent_reviewer",
    "approved_at",
    "expires_at",
    "record_url",
    "controls",
    "risk_acceptance",
    "compensating_controls",
}
EVIDENCE_FIELDS = {
    "schema",
    "generated_at",
    "event",
    "publication",
    "repository",
    "candidate_sha",
    "tag",
    "version",
    "attestation",
}

_REPOSITORY_RE = re.compile(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+")
_SHA_RE = re.compile(r"[0-9a-f]{40}")
_VERSION_RE = re.compile(r"[0-9]+\.[0-9]+\.[0-9]+(?:-rc\.[0-9]+)?")
_LOGIN_RE = re.compile(r"[A-Za-z0-9](?:[A-Za-z0-9-]{0,37}[A-Za-z0-9])?")
_UTC_TIMESTAMP_RE = re.compile(
    r"(?:[0-9]{4})-(?:[0-9]{2})-(?:[0-9]{2})"
    r"T(?:[0-9]{2}):(?:[0-9]{2}):(?:[0-9]{2})Z"
)


class GovernanceError(RuntimeError):
    """Raised when governance input is missing, ambiguous, or invalid."""


def _reject_constant(value: str) -> object:
    raise GovernanceError(f"non-standard JSON value is forbidden: {value}")


def _unique_object(pairs: list[tuple[str, object]]) -> dict[str, object]:
    result: dict[str, object] = {}
    for key, value in pairs:
        if key in result:
            raise GovernanceError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def parse_attestation_json(raw: str) -> Mapping[str, Any]:
    """Parse a single-line strict JSON object while rejecting duplicate keys."""

    if not isinstance(raw, str) or not raw:
        raise GovernanceError("governance attestation JSON is missing")
    if raw != raw.strip() or "\r" in raw or "\n" in raw:
        raise GovernanceError("governance attestation must be one trimmed JSON line")
    try:
        parsed = json.loads(
            raw,
            object_pairs_hook=_unique_object,
            parse_constant=_reject_constant,
        )
    except GovernanceError:
        raise
    except (TypeError, ValueError, json.JSONDecodeError) as error:
        raise GovernanceError("governance attestation is not strict JSON") from error
    if not isinstance(parsed, Mapping):
        raise GovernanceError("governance attestation root must be an object")
    return parsed


def parse_evidence_json(raw: str) -> Mapping[str, Any]:
    """Parse a governance-evidence document while rejecting duplicate keys."""

    if not isinstance(raw, str) or not raw.strip():
        raise GovernanceError("governance evidence JSON is missing")
    try:
        parsed = json.loads(
            raw,
            object_pairs_hook=_unique_object,
            parse_constant=_reject_constant,
        )
    except GovernanceError:
        raise
    except (TypeError, ValueError, json.JSONDecodeError) as error:
        raise GovernanceError("governance evidence is not strict JSON") from error
    if not isinstance(parsed, Mapping):
        raise GovernanceError("governance evidence root must be an object")
    return parsed


def _expect_fields(value: Mapping[str, Any], expected: set[str], label: str) -> None:
    actual = set(value)
    if actual != expected:
        raise GovernanceError(
            f"{label} fields mismatch; "
            f"missing={sorted(expected - actual)}, extra={sorted(actual - expected)}"
        )


def _string(value: object, label: str) -> str:
    if not isinstance(value, str) or not value or value != value.strip():
        raise GovernanceError(f"{label} must be a non-empty trimmed string")
    return value


def _repository(value: object, label: str) -> str:
    result = _string(value, label)
    if _REPOSITORY_RE.fullmatch(result) is None:
        raise GovernanceError(f"{label} must use the owner/name form")
    return result


def _sha(value: object, label: str) -> str:
    result = _string(value, label)
    if _SHA_RE.fullmatch(result) is None:
        raise GovernanceError(f"{label} must be a lowercase full 40-character SHA")
    return result


def _version(value: object, label: str) -> str:
    result = _string(value, label)
    if _VERSION_RE.fullmatch(result) is None:
        raise GovernanceError(f"{label} is not a supported release version")
    return result


def _login(value: object, label: str) -> str:
    result = _string(value, label)
    if _LOGIN_RE.fullmatch(result) is None:
        raise GovernanceError(f"{label} is not a GitHub login")
    return result


def _timestamp(value: object, label: str) -> datetime:
    text = _string(value, label)
    if _UTC_TIMESTAMP_RE.fullmatch(text) is None:
        raise GovernanceError(f"{label} must be canonical UTC YYYY-MM-DDTHH:MM:SSZ")
    try:
        parsed = datetime.strptime(text, "%Y-%m-%dT%H:%M:%SZ").replace(
            tzinfo=timezone.utc
        )
    except ValueError as error:
        raise GovernanceError(f"{label} is not a valid UTC timestamp") from error
    return parsed


def _format_timestamp(value: datetime) -> str:
    return value.astimezone(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def _record_url(value: object, repository: str) -> str:
    result = _string(value, "attestation.record_url")
    parsed = urllib.parse.urlsplit(result)
    try:
        port = parsed.port
    except ValueError as error:
        raise GovernanceError("attestation.record_url has an invalid port") from error
    if (
        parsed.scheme != "https"
        or parsed.hostname is None
        or parsed.hostname.lower() != "github.com"
        or parsed.username is not None
        or parsed.password is not None
        or port not in (None, 443)
        or not parsed.path.startswith(f"/{repository}/")
    ):
        raise GovernanceError(
            "attestation.record_url must be an HTTPS github.com URL for the repository"
        )
    return result


def validate_attestation(
    attestation: Mapping[str, Any],
    *,
    expected_repository: str,
    expected_sha: str,
    expected_tag: str,
    expected_version: str,
    now: datetime | None = None,
) -> dict[str, object]:
    """Validate and return a normalized, exact-scope governance attestation."""

    if not isinstance(attestation, Mapping):
        raise GovernanceError("governance attestation must be an object")
    _expect_fields(attestation, ATTESTATION_FIELDS, "governance attestation")

    repository = _repository(
        attestation.get("repository"), "attestation.repository"
    )
    candidate_sha = _sha(
        attestation.get("candidate_sha"), "attestation.candidate_sha"
    )
    version = _version(attestation.get("version"), "attestation.version")
    tag = _string(attestation.get("tag"), "attestation.tag")
    if tag != f"v{version}":
        raise GovernanceError("attestation.tag must equal v + attestation.version")

    expected_repository = _repository(expected_repository, "expected repository")
    expected_sha = _sha(expected_sha, "expected candidate SHA")
    expected_version = _version(expected_version, "expected version")
    expected_tag = _string(expected_tag, "expected tag")
    if expected_tag != f"v{expected_version}":
        raise GovernanceError("expected tag must equal v + expected version")
    if repository != expected_repository:
        raise GovernanceError("attestation repository does not match the workflow")
    if candidate_sha != expected_sha:
        raise GovernanceError("attestation candidate SHA does not match the workflow")
    if tag != expected_tag:
        raise GovernanceError("attestation tag does not match the workflow")
    if version != expected_version:
        raise GovernanceError("attestation version does not match the workflow")
    if attestation.get("schema") != ATTESTATION_SCHEMA:
        raise GovernanceError("governance attestation schema mismatch")

    decision = _string(attestation.get("decision"), "attestation.decision")
    if decision not in DECISIONS:
        raise GovernanceError("attestation.decision is unsupported")
    approved_by = _login(
        attestation.get("approved_by"), "attestation.approved_by"
    )
    reviewer = _login(
        attestation.get("independent_reviewer"),
        "attestation.independent_reviewer",
    )
    if approved_by.casefold() == reviewer.casefold():
        raise GovernanceError("approved_by and independent reviewer must be different")

    approved_at = _timestamp(
        attestation.get("approved_at"), "attestation.approved_at"
    )
    expires_at = _timestamp(
        attestation.get("expires_at"), "attestation.expires_at"
    )
    current = now or datetime.now(timezone.utc)
    if current.tzinfo is None:
        raise GovernanceError("validation time must include a timezone")
    current = current.astimezone(timezone.utc)
    if approved_at > current:
        raise GovernanceError("governance approval is in the future")
    if expires_at <= current:
        raise GovernanceError("governance approval is expired")
    if expires_at <= approved_at:
        raise GovernanceError("governance expiry must be after approval")

    record_url = _record_url(attestation.get("record_url"), repository)
    raw_controls = attestation.get("controls")
    if not isinstance(raw_controls, Mapping):
        raise GovernanceError("attestation.controls must be an object")
    _expect_fields(raw_controls, set(CONTROL_NAMES), "attestation.controls")
    controls: dict[str, str] = {}
    for name in CONTROL_NAMES:
        state = _string(raw_controls.get(name), f"attestation.controls.{name}")
        if state not in CONTROL_STATES:
            raise GovernanceError(
                f"attestation.controls.{name} must be enforced or waived"
            )
        controls[name] = state

    risk_acceptance = attestation.get("risk_acceptance")
    raw_compensating = attestation.get("compensating_controls")
    if not isinstance(raw_compensating, list):
        raise GovernanceError("attestation.compensating_controls must be an array")
    compensating: list[str] = []
    for index, value in enumerate(raw_compensating):
        item = _string(
            value,
            f"attestation.compensating_controls[{index}]",
        )
        if item in compensating:
            raise GovernanceError("compensating controls must be unique")
        compensating.append(item)

    if decision == "protected-ruleset":
        if any(state != "enforced" for state in controls.values()):
            raise GovernanceError("protected-ruleset requires all controls enforced")
        if risk_acceptance is not None or compensating:
            raise GovernanceError(
                "protected-ruleset must not carry waiver risk or compensating controls"
            )
    else:
        if not isinstance(risk_acceptance, str) or not risk_acceptance.strip():
            raise GovernanceError(
                "time-limited-waiver requires non-empty risk acceptance"
            )
        risk_acceptance = risk_acceptance.strip()
        if not compensating:
            raise GovernanceError(
                "time-limited-waiver requires compensating controls"
            )
        if "waived" not in controls.values():
            raise GovernanceError(
                "time-limited-waiver requires at least one waived control"
            )
        if expires_at - approved_at > timedelta(days=30):
            raise GovernanceError("time-limited-waiver may not exceed 30 days")

    return {
        "schema": ATTESTATION_SCHEMA,
        "repository": repository,
        "candidate_sha": candidate_sha,
        "tag": tag,
        "version": version,
        "decision": decision,
        "approved_by": approved_by,
        "independent_reviewer": reviewer,
        "approved_at": _format_timestamp(approved_at),
        "expires_at": _format_timestamp(expires_at),
        "record_url": record_url,
        "controls": controls,
        "risk_acceptance": risk_acceptance,
        "compensating_controls": compensating,
    }


def build_governance_evidence(
    *,
    event: str,
    repository: str,
    candidate_sha: str,
    tag: str | None,
    version: str,
    attestation_json: str | None,
    now: datetime | None = None,
) -> dict[str, object]:
    """Create approved tag evidence or explicit candidate-only evidence."""

    repository = _repository(repository, "repository")
    candidate_sha = _sha(candidate_sha, "candidate SHA")
    version = _version(version, "version")
    current = now or datetime.now(timezone.utc)
    if current.tzinfo is None:
        raise GovernanceError("evidence time must include a timezone")
    current = current.astimezone(timezone.utc)

    if event == "push":
        if tag is None:
            raise GovernanceError("tag push requires a tag")
        normalized_tag = _string(tag, "tag")
        if normalized_tag != f"v{version}":
            raise GovernanceError("tag must equal v + version")
        if attestation_json is None:
            raise GovernanceError("tag push requires governance attestation JSON")
        attestation = validate_attestation(
            parse_attestation_json(attestation_json),
            expected_repository=repository,
            expected_sha=candidate_sha,
            expected_tag=normalized_tag,
            expected_version=version,
            now=current,
        )
        publication = "approved"
    elif event == "workflow_dispatch":
        if tag not in (None, ""):
            raise GovernanceError("manual candidate evidence must not claim a release tag")
        normalized_tag = None
        attestation = None
        publication = "candidate-only"
    else:
        raise GovernanceError(f"unsupported workflow event: {event}")

    return {
        "schema": EVIDENCE_SCHEMA,
        "generated_at": _format_timestamp(current),
        "event": event,
        "publication": publication,
        "repository": repository,
        "candidate_sha": candidate_sha,
        "tag": normalized_tag,
        "version": version,
        "attestation": attestation,
    }


def validate_governance_evidence(
    evidence: Mapping[str, Any],
    *,
    expected_repository: str,
    expected_sha: str,
    expected_tag: str | None,
    expected_version: str,
    now: datetime | None = None,
    require_approved: bool = False,
) -> dict[str, object]:
    """Deeply validate normalized evidence at its point of consumption."""

    if not isinstance(evidence, Mapping):
        raise GovernanceError("governance evidence must be an object")
    _expect_fields(evidence, EVIDENCE_FIELDS, "governance evidence")
    if evidence.get("schema") != EVIDENCE_SCHEMA:
        raise GovernanceError("governance evidence schema mismatch")

    repository = _repository(evidence.get("repository"), "evidence.repository")
    candidate_sha = _sha(
        evidence.get("candidate_sha"), "evidence.candidate_sha"
    )
    version = _version(evidence.get("version"), "evidence.version")
    expected_repository = _repository(expected_repository, "expected repository")
    expected_sha = _sha(expected_sha, "expected candidate SHA")
    expected_version = _version(expected_version, "expected version")
    if repository != expected_repository:
        raise GovernanceError("governance evidence repository mismatch")
    if candidate_sha != expected_sha:
        raise GovernanceError("governance evidence candidate SHA mismatch")
    if version != expected_version:
        raise GovernanceError("governance evidence version mismatch")

    generated_at = _timestamp(
        evidence.get("generated_at"), "evidence.generated_at"
    )
    current = now or datetime.now(timezone.utc)
    if current.tzinfo is None:
        raise GovernanceError("validation time must include a timezone")
    current = current.astimezone(timezone.utc)
    if generated_at > current:
        raise GovernanceError("governance evidence was generated in the future")

    event = _string(evidence.get("event"), "evidence.event")
    publication = _string(evidence.get("publication"), "evidence.publication")
    raw_tag = evidence.get("tag")
    raw_attestation = evidence.get("attestation")
    if require_approved and publication != "approved":
        raise GovernanceError("release publication requires approved governance evidence")
    if publication == "approved":
        if event != "push":
            raise GovernanceError("approved governance evidence must come from tag push")
        if expected_tag is None:
            raise GovernanceError("approved governance evidence requires expected tag")
        tag = _string(raw_tag, "evidence.tag")
        if tag != expected_tag or tag != f"v{version}":
            raise GovernanceError("governance evidence tag mismatch")
        if not isinstance(raw_attestation, Mapping):
            raise GovernanceError("approved governance evidence lacks attestation")
        attestation = validate_attestation(
            raw_attestation,
            expected_repository=repository,
            expected_sha=candidate_sha,
            expected_tag=tag,
            expected_version=version,
            now=current,
        )
        approved_at = _timestamp(
            attestation["approved_at"],
            "attestation.approved_at",
        )
        expires_at = _timestamp(
            attestation["expires_at"],
            "attestation.expires_at",
        )
        if approved_at > generated_at:
            raise GovernanceError(
                "governance evidence predates its approval"
            )
        if expires_at <= generated_at:
            raise GovernanceError(
                "governance evidence was generated after approval expiry"
            )
    elif publication == "candidate-only":
        if event != "workflow_dispatch":
            raise GovernanceError(
                "candidate-only governance evidence must come from workflow_dispatch"
            )
        if expected_tag not in (None, ""):
            raise GovernanceError("candidate-only governance evidence cannot bind a tag")
        if raw_tag is not None or raw_attestation is not None:
            raise GovernanceError(
                "candidate-only governance evidence must not contain tag approval"
            )
        tag = None
        attestation = None
    else:
        raise GovernanceError("governance evidence publication is unsupported")

    return {
        "schema": EVIDENCE_SCHEMA,
        "generated_at": _format_timestamp(generated_at),
        "event": event,
        "publication": publication,
        "repository": repository,
        "candidate_sha": candidate_sha,
        "tag": tag,
        "version": version,
        "attestation": attestation,
    }


def _write_atomic(path: Path, value: Mapping[str, object]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(
        mode="w",
        encoding="utf-8",
        newline="\n",
        dir=path.parent,
        prefix=f".{path.name}.",
        suffix=".tmp",
        delete=False,
    ) as temporary:
        json.dump(value, temporary, indent=2, sort_keys=True)
        temporary.write("\n")
        temporary_path = Path(temporary.name)
    os.replace(temporary_path, path)


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--event", default=os.environ.get("GITHUB_EVENT_NAME"))
    parser.add_argument("--repository", default=os.environ.get("GITHUB_REPOSITORY"))
    parser.add_argument("--sha", default=os.environ.get("GITHUB_SHA"))
    parser.add_argument("--tag")
    parser.add_argument("--version", required=True)
    parser.add_argument(
        "--attestation-json",
        default=os.environ.get("LUA_RELEASE_GOVERNANCE_ATTESTATION"),
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("build/governance-evidence.json"),
    )
    return parser


def main(
    argv: Sequence[str] | None = None,
    *,
    now: datetime | None = None,
) -> int:
    args = _parser().parse_args(argv)
    try:
        args.output.unlink(missing_ok=True)
        if not args.event:
            raise GovernanceError("workflow event is required")
        if not args.repository:
            raise GovernanceError("repository is required")
        if not args.sha:
            raise GovernanceError("candidate SHA is required")
        evidence = build_governance_evidence(
            event=args.event,
            repository=args.repository,
            candidate_sha=args.sha,
            tag=args.tag,
            version=args.version,
            attestation_json=args.attestation_json,
            now=now,
        )
        _write_atomic(args.output, evidence)
    except Exception as error:  # Every malformed/missing input fails closed.
        print(f"release governance verification failed: {error}", file=sys.stderr)
        return 1
    print(
        f"Release governance normalized as {evidence['publication']} "
        f"for {evidence['candidate_sha']}; evidence written to {args.output}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
