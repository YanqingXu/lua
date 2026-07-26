#!/usr/bin/env python3
"""Write fail-closed, exact-SHA metadata inside a workflow artifact."""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
import tempfile
from collections.abc import Mapping, Sequence
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


SCHEMA = "lua-cpp.workflow-evidence/v1"
EVIDENCE_BASENAME = "evidence-metadata.json"
ALLOWED_KINDS = frozenset(
    {
        "component-coverage",
        "runtime-benchmark-evidence",
        "runtime-soak-evidence",
        "long-fuzz-evidence",
    }
)
LONG_FUZZ_TARGETS = (
    "undump",
    "bytecode_verifier",
    "parser",
    "stdlib_numeric_arguments",
)
KIND_CONTEXT = {
    "component-coverage": ("ci.yml", "linux-coverage"),
    "runtime-benchmark-evidence": ("ci.yml", "linux-runtime-benchmark"),
    "runtime-soak-evidence": ("nightly.yml", "runtime-soak"),
    "long-fuzz-evidence": ("nightly.yml", "long-fuzz"),
}

_SHA_PATTERN = re.compile(r"^[0-9a-fA-F]{40}$")
_REPOSITORY_PATTERN = re.compile(r"^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$")
_EVENT_PATTERN = re.compile(r"^[A-Za-z0-9_]+$")
_JOB_PATTERN = re.compile(r"^[A-Za-z0-9_.-]+$")
_PARAMETER_KEY_PATTERN = re.compile(r"^[a-z][a-z0-9_]*$")


class EvidenceError(ValueError):
    """Raised when workflow evidence cannot be trusted."""


def _required_env(environment: Mapping[str, str], name: str) -> str:
    value = environment.get(name, "")
    if not value or value.strip() != value:
        raise EvidenceError(f"{name} is required and must not contain surrounding whitespace")
    return value


def _positive_integer(value: str, name: str) -> int:
    if not value.isascii() or not value.isdecimal():
        raise EvidenceError(f"{name} must be a positive decimal integer")
    parsed = int(value)
    if parsed <= 0:
        raise EvidenceError(f"{name} must be greater than zero")
    return parsed


def _json_object(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise EvidenceError(f"JSON parameter object contains duplicate key: {key}")
        result[key] = value
    return result


def parse_parameters(items: Sequence[str]) -> dict[str, Any]:
    """Parse unique NAME=JSON command-line parameters."""

    parameters: dict[str, Any] = {}
    for item in items:
        key, separator, raw_value = item.partition("=")
        if not separator or not key or not raw_value:
            raise EvidenceError("each --parameter must use NAME=JSON")
        if _PARAMETER_KEY_PATTERN.fullmatch(key) is None:
            raise EvidenceError(f"invalid parameter key: {key}")
        if key in parameters:
            raise EvidenceError(f"duplicate parameter key: {key}")
        try:
            parameters[key] = json.loads(raw_value, object_pairs_hook=_json_object)
        except json.JSONDecodeError as exc:
            raise EvidenceError(f"parameter {key} is not valid JSON: {exc.msg}") from exc
    return parameters


def _validate_parameters(kind: str, parameters: Mapping[str, Any]) -> dict[str, Any]:
    validated = dict(parameters)
    for key in validated:
        if _PARAMETER_KEY_PATTERN.fullmatch(key) is None:
            raise EvidenceError(f"invalid parameter key: {key}")

    try:
        json.dumps(validated, allow_nan=False)
    except (TypeError, ValueError) as exc:
        raise EvidenceError(f"parameters must contain only finite JSON values: {exc}") from exc

    if kind in ("component-coverage", "runtime-benchmark-evidence") and validated:
        raise EvidenceError(f"{kind} parameters must be empty")

    if kind == "runtime-soak-evidence":
        expected_keys = {"soak_minutes", "native_module_iterations"}
        if set(validated) != expected_keys:
            raise EvidenceError(f"{kind} requires exactly: {', '.join(sorted(expected_keys))}")
        for key in expected_keys:
            value = validated[key]
            if type(value) is not int or value <= 0:
                raise EvidenceError(f"{key} must be a positive JSON integer")

    if kind == "long-fuzz-evidence":
        expected_keys = {"fuzz_seconds_per_target", "fuzz_targets"}
        if set(validated) != expected_keys:
            raise EvidenceError(f"{kind} requires exactly: {', '.join(sorted(expected_keys))}")

        seconds = validated.get("fuzz_seconds_per_target")
        if type(seconds) is not int or seconds <= 0:
            raise EvidenceError("fuzz_seconds_per_target must be a positive JSON integer")

        targets = validated.get("fuzz_targets")
        if not isinstance(targets, list) or tuple(targets) != LONG_FUZZ_TARGETS:
            expected = ", ".join(LONG_FUZZ_TARGETS)
            raise EvidenceError(
                f"long-fuzz fuzz_targets must be the ordered four-target set: {expected}"
            )

    return validated


def build_evidence(
    kind: str,
    parameters: Mapping[str, Any],
    environment: Mapping[str, str],
    *,
    now: datetime | None = None,
) -> dict[str, Any]:
    """Validate inputs and construct the workflow evidence payload."""

    if kind not in ALLOWED_KINDS:
        raise EvidenceError(f"unsupported evidence kind: {kind}")

    repository = _required_env(environment, "GITHUB_REPOSITORY")
    if _REPOSITORY_PATTERN.fullmatch(repository) is None:
        raise EvidenceError("GITHUB_REPOSITORY must have the form owner/repository")

    candidate_sha = _required_env(environment, "GITHUB_SHA")
    if _SHA_PATTERN.fullmatch(candidate_sha) is None:
        raise EvidenceError("GITHUB_SHA must be exactly 40 hexadecimal characters")

    run_id = _positive_integer(_required_env(environment, "GITHUB_RUN_ID"), "GITHUB_RUN_ID")
    run_attempt = _positive_integer(
        _required_env(environment, "GITHUB_RUN_ATTEMPT"),
        "GITHUB_RUN_ATTEMPT",
    )

    event = _required_env(environment, "GITHUB_EVENT_NAME")
    if _EVENT_PATTERN.fullmatch(event) is None:
        raise EvidenceError("GITHUB_EVENT_NAME contains invalid characters")

    workflow_ref = _required_env(environment, "GITHUB_WORKFLOW_REF")
    workflow_prefix = f"{repository}/.github/workflows/"
    if (
        not workflow_ref.startswith(workflow_prefix)
        or workflow_ref.count("@") != 1
        or workflow_ref.endswith("@")
        or any(character.isspace() for character in workflow_ref)
    ):
        raise EvidenceError("GITHUB_WORKFLOW_REF does not identify this repository workflow")

    job = _required_env(environment, "GITHUB_JOB")
    if _JOB_PATTERN.fullmatch(job) is None:
        raise EvidenceError("GITHUB_JOB contains invalid characters")

    expected_workflow, expected_job = KIND_CONTEXT[kind]
    workflow_identity = workflow_ref.split("@", maxsplit=1)[0]
    expected_identity = f"{workflow_prefix}{expected_workflow}"
    if workflow_identity != expected_identity:
        raise EvidenceError(f"{kind} must be generated by {expected_workflow}")
    if job != expected_job:
        raise EvidenceError(f"{kind} must be generated by job {expected_job}")

    timestamp = now if now is not None else datetime.now(timezone.utc)
    if timestamp.tzinfo is None or timestamp.utcoffset() is None:
        raise EvidenceError("created_at source must be timezone-aware")
    created_at = (
        timestamp.astimezone(timezone.utc)
        .replace(microsecond=0)
        .isoformat()
        .replace("+00:00", "Z")
    )

    return {
        "schema": SCHEMA,
        "kind": kind,
        "repository": repository,
        "candidate_sha": candidate_sha.lower(),
        "run_id": run_id,
        "run_attempt": run_attempt,
        "event": event,
        "workflow_ref": workflow_ref,
        "job": job,
        "result": "passed",
        "created_at": created_at,
        "parameters": _validate_parameters(kind, parameters),
    }


def atomic_write_json(output: Path, payload: Mapping[str, Any]) -> None:
    """Atomically replace the evidence file after the payload is complete."""

    if output.name != EVIDENCE_BASENAME:
        raise EvidenceError(f"output basename must be {EVIDENCE_BASENAME}")

    output.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary_name = tempfile.mkstemp(
        dir=output.parent,
        prefix=f".{EVIDENCE_BASENAME}.",
        suffix=".tmp",
    )
    temporary_path = Path(temporary_name)
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8", newline="\n") as stream:
            json.dump(
                payload,
                stream,
                indent=2,
                sort_keys=True,
                ensure_ascii=False,
                allow_nan=False,
            )
            stream.write("\n")
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary_path, output)
    except Exception:
        temporary_path.unlink(missing_ok=True)
        raise


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--kind", required=True, choices=sorted(ALLOWED_KINDS))
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument(
        "--parameter",
        action="append",
        default=[],
        metavar="NAME=JSON",
        help="record a unique, JSON-encoded workflow parameter",
    )
    return parser


def main(
    argv: Sequence[str] | None = None,
    *,
    environment: Mapping[str, str] | None = None,
) -> int:
    parser = _parser()
    arguments = parser.parse_args(argv)
    try:
        parameters = parse_parameters(arguments.parameter)
        payload = build_evidence(
            arguments.kind,
            parameters,
            os.environ if environment is None else environment,
        )
        atomic_write_json(arguments.output, payload)
    except (EvidenceError, OSError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2

    print(f"Workflow evidence written: {arguments.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
