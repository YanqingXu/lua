#!/usr/bin/env python3
"""Validate a release-evidence manifest and build the final release body."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import re
import sys
import tempfile
import urllib.parse
from collections.abc import Mapping, Sequence
from datetime import datetime, timezone
from pathlib import Path, PurePath
from typing import Any

from validate_release_artifacts import (
    ReleaseArtifactError,
    SUPPORTED_RELEASE_RIDS,
    validate_release_artifacts,
)
from verify_release_evidence import (
    ARTIFACT_TIMED_JOBS,
    CI_ARTIFACTS,
    EXPECTED_ARTIFACT_JOBS,
    EXPECTED_BENCHMARK_ABSOLUTE_SCOPE,
    EXPECTED_BENCHMARK_ABSOLUTE_SLOS,
    EXPECTED_BENCHMARK_METRICS,
    EXPECTED_BENCHMARK_RUNTIME_INPUTS,
    EXPECTED_CI_JOBS,
    EXPECTED_COVERAGE_THRESHOLDS,
    EXPECTED_FUZZ_TARGETS,
    EXPECTED_NIGHTLY_JOBS,
    MAIN_BRANCH,
    MANIFEST_SCHEMA,
    MINIMUM_COVERAGE_SCOPE,
    NIGHTLY_ARTIFACTS,
    TIMED_JOB_STEPS,
    TOOL_VERSION,
    WORKFLOW_EVIDENCE_FIELDS,
    WORKFLOW_EVIDENCE_SCHEMA,
    validate_release_notes_narrative,
)
from verify_release_governance import (
    GovernanceError,
    validate_governance_evidence,
)
from verify_source_readiness_evidence import (
    SourceReadinessError,
    validate_source_readiness_evidence,
)


EXPECTED_VERIFIER_NAME = "tools/verify_release_evidence.py"
EXPECTED_MANIFEST_FIELDS = {
    "schema",
    "generated_at",
    "repository",
    "candidate_sha",
    "version",
    "abi_version",
    "governance",
    "source_readiness",
    "main_history",
    "tool",
    "runs",
    "release_notes",
}
EXPECTED_RUN_FIELDS = {
    "workflow",
    "event",
    "id",
    "attempt",
    "url",
    "head_sha",
    "head_branch",
    "status",
    "conclusion",
    "created_at",
    "updated_at",
    "jobs",
    "artifacts",
}
EXPECTED_ARTIFACT_FIELDS = {
    "id",
    "name",
    "digest",
    "size_in_bytes",
    "archive_download_url",
    "created_at",
    "updated_at",
    "expires_at",
    "workflow_evidence",
    "payload_evidence",
}
RUN_POLICIES = {
    "ci_push": {
        "label": "CI push",
        "workflow": "ci.yml",
        "event": "push",
        "jobs": EXPECTED_CI_JOBS,
        "artifacts": CI_ARTIFACTS,
    },
    "nightly_workflow_dispatch": {
        "label": "Manual nightly",
        "workflow": "nightly.yml",
        "event": "workflow_dispatch",
        "jobs": EXPECTED_NIGHTLY_JOBS,
        "artifacts": NIGHTLY_ARTIFACTS,
    },
    "nightly_schedule": {
        "label": "Scheduled nightly",
        "workflow": "nightly.yml",
        "event": "schedule",
        "jobs": EXPECTED_NIGHTLY_JOBS,
        "artifacts": NIGHTLY_ARTIFACTS,
    },
}

_SHA_RE = re.compile(r"[0-9a-f]{40}")
_DIGEST_RE = re.compile(r"sha256:[0-9a-f]{64}")
_REPOSITORY_RE = re.compile(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+")
_CHECKSUM_RE = re.compile(r"([0-9a-f]{64})  ([A-Za-z0-9][A-Za-z0-9._+-]*)")
_PACKAGE_SUFFIXES = (".zip", ".spdx.json", ".manifest.json", ".SHA256SUMS")


class ReleaseBodyError(RuntimeError):
    """Raised when the verified manifest or release assets are inconsistent."""


def _mapping(value: object, label: str) -> Mapping[str, Any]:
    if not isinstance(value, Mapping):
        raise ReleaseBodyError(f"{label} is not an object")
    return value


def _sequence(value: object, label: str) -> Sequence[object]:
    if not isinstance(value, list):
        raise ReleaseBodyError(f"{label} is not an array")
    return value


def _string(value: object, label: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise ReleaseBodyError(f"{label} is missing")
    return value


def _integer(value: object, label: str, *, minimum: int = 1) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or value < minimum:
        raise ReleaseBodyError(f"{label} must be an integer >= {minimum}")
    return value


def _number(value: object, label: str, *, minimum: float | None = None) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise ReleaseBodyError(f"{label} is not numeric")
    result = float(value)
    if not math.isfinite(result):
        raise ReleaseBodyError(f"{label} is not finite")
    if minimum is not None and result < minimum:
        raise ReleaseBodyError(f"{label} is less than {minimum}")
    return result


def _boolean(value: object, label: str) -> bool:
    if type(value) is not bool:
        raise ReleaseBodyError(f"{label} is not a boolean")
    return value


def _sha(value: object, label: str) -> str:
    result = _string(value, label).lower()
    if _SHA_RE.fullmatch(result) is None:
        raise ReleaseBodyError(f"{label} is not a full 40-character SHA")
    return result


def _timestamp(value: object, label: str) -> datetime:
    text = _string(value, label)
    try:
        parsed = datetime.fromisoformat(text[:-1] + "+00:00" if text.endswith("Z") else text)
    except ValueError as error:
        raise ReleaseBodyError(f"{label} is not an ISO-8601 timestamp") from error
    if parsed.tzinfo is None:
        raise ReleaseBodyError(f"{label} has no timezone")
    return parsed.astimezone(timezone.utc)


def _url(value: object, label: str) -> str:
    text = _string(value, label)
    parsed = urllib.parse.urlsplit(text)
    if parsed.scheme != "https" or not parsed.netloc:
        raise ReleaseBodyError(f"{label} is not an absolute HTTPS URL")
    return text


def _expect_fields(value: Mapping[str, Any], expected: set[str], label: str) -> None:
    actual = set(value)
    if actual != expected:
        raise ReleaseBodyError(
            f"{label} field set mismatch; "
            f"missing={sorted(expected - actual)}, extra={sorted(actual - expected)}"
        )


def _json_object_without_duplicates(pairs: list[tuple[str, object]]) -> dict[str, object]:
    result: dict[str, object] = {}
    for key, value in pairs:
        if key in result:
            raise ReleaseBodyError(f"JSON contains duplicate key: {key}")
        result[key] = value
    return result


def _reject_json_constant(value: str) -> object:
    raise ReleaseBodyError(f"JSON contains non-standard constant: {value}")


def _load_json(path: Path, label: str) -> Mapping[str, Any]:
    try:
        text = path.read_text(encoding="utf-8-sig")
        value = json.loads(
            text,
            object_pairs_hook=_json_object_without_duplicates,
            parse_constant=_reject_json_constant,
        )
    except ReleaseBodyError:
        raise
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as error:
        raise ReleaseBodyError(f"{label} is not readable UTF-8 JSON: {path}") from error
    return _mapping(value, label)


def _validate_main_history(value: object) -> None:
    history = _mapping(value, "main_history")
    _expect_fields(
        history,
        {"branch", "compare_status", "commits_after_candidate"},
        "main_history",
    )
    if _string(history.get("branch"), "main_history.branch") != MAIN_BRANCH:
        raise ReleaseBodyError("main_history branch is not main")
    status = _string(history.get("compare_status"), "main_history.compare_status")
    commits_after = _integer(
        history.get("commits_after_candidate"),
        "main_history.commits_after_candidate",
        minimum=0,
    )
    if status == "identical":
        if commits_after != 0:
            raise ReleaseBodyError("identical main_history has commits after the candidate")
    elif status == "ahead":
        if commits_after == 0:
            raise ReleaseBodyError("ahead main_history has no commits after the candidate")
    else:
        raise ReleaseBodyError("candidate is not proven to be in main history")


def _validate_timed_steps(job: Mapping[str, Any], job_name: str) -> float | None:
    policy = TIMED_JOB_STEPS.get(job_name)
    if policy is None:
        if "required_timed_steps" in job:
            raise ReleaseBodyError(f"untimed job {job_name} unexpectedly contains timed evidence")
        return None

    required_name, minimum_seconds = policy
    steps = _sequence(job.get("required_timed_steps"), f"job {job_name} timed steps")
    if len(steps) != 1:
        raise ReleaseBodyError(f"job {job_name} must contain exactly one timed step")
    step = _mapping(steps[0], f"job {job_name} timed step")
    _expect_fields(
        step,
        {"name", "started_at", "completed_at", "duration_seconds"},
        f"job {job_name} timed step",
    )
    if _string(step.get("name"), f"job {job_name} timed step name") != required_name:
        raise ReleaseBodyError(f"job {job_name} timed step name mismatch")
    started = _timestamp(step.get("started_at"), f"job {job_name} timed step started_at")
    completed = _timestamp(step.get("completed_at"), f"job {job_name} timed step completed_at")
    if completed < started:
        raise ReleaseBodyError(f"job {job_name} timed step timestamps are inconsistent")
    duration = _number(
        step.get("duration_seconds"),
        f"job {job_name} timed step duration",
        minimum=float(minimum_seconds),
    )
    measured = (completed - started).total_seconds()
    if abs(duration - measured) > 1e-6:
        raise ReleaseBodyError(f"job {job_name} timed step duration contradicts timestamps")
    return duration


def _validate_jobs(value: object, expected_names: Sequence[str]) -> dict[str, Mapping[str, Any]]:
    jobs = _sequence(value, "run jobs")
    by_name: dict[str, Mapping[str, Any]] = {}
    ids: set[int] = set()
    for index, raw_job in enumerate(jobs):
        job = _mapping(raw_job, f"run job {index}")
        name = _string(job.get("name"), f"run job {index} name")
        expected_fields = {"id", "name", "url", "started_at", "completed_at"}
        if name in TIMED_JOB_STEPS:
            expected_fields.add("required_timed_steps")
        _expect_fields(job, expected_fields, f"job {name}")
        if name in by_name:
            raise ReleaseBodyError(f"run contains duplicate job: {name}")
        job_id = _integer(job.get("id"), f"job {name} id")
        if job_id in ids:
            raise ReleaseBodyError(f"run contains duplicate job ID: {job_id}")
        ids.add(job_id)
        _url(job.get("url"), f"job {name} URL")
        started = _timestamp(job.get("started_at"), f"job {name} started_at")
        completed = _timestamp(job.get("completed_at"), f"job {name} completed_at")
        if completed < started:
            raise ReleaseBodyError(f"job {name} timestamps are inconsistent")
        _validate_timed_steps(job, name)
        by_name[name] = job
    if set(by_name) != set(expected_names):
        raise ReleaseBodyError(
            "run job set mismatch; "
            f"missing={sorted(set(expected_names) - set(by_name))}, "
            f"extra={sorted(set(by_name) - set(expected_names))}"
        )
    return by_name


def _validate_workflow_evidence(
    value: object,
    *,
    artifact_name: str,
    repository: str,
    candidate_sha: str,
    run: Mapping[str, Any],
) -> Mapping[str, Any]:
    evidence = _mapping(value, f"artifact {artifact_name} workflow_evidence")
    _expect_fields(evidence, set(WORKFLOW_EVIDENCE_FIELDS), "workflow_evidence")
    expected_scalars = {
        "schema": WORKFLOW_EVIDENCE_SCHEMA,
        "kind": artifact_name,
        "repository": repository,
        "candidate_sha": candidate_sha,
        "run_id": run["id"],
        "run_attempt": run["attempt"],
        "event": run["event"],
        "workflow_ref": (
            f"{repository}/.github/workflows/{run['workflow']}@refs/heads/{MAIN_BRANCH}"
        ),
        "job": EXPECTED_ARTIFACT_JOBS[artifact_name],
        "result": "passed",
    }
    for field, expected in expected_scalars.items():
        if evidence.get(field) != expected:
            raise ReleaseBodyError(
                f"artifact {artifact_name} workflow_evidence {field} mismatch"
            )
    _timestamp(evidence.get("created_at"), f"artifact {artifact_name} evidence created_at")
    parameters = _mapping(
        evidence.get("parameters"),
        f"artifact {artifact_name} workflow parameters",
    )
    if artifact_name in CI_ARTIFACTS:
        if parameters:
            raise ReleaseBodyError(f"artifact {artifact_name} CI parameters are not empty")
    elif artifact_name == "runtime-soak-evidence":
        _expect_fields(
            parameters,
            {"soak_minutes", "native_module_iterations"},
            "runtime soak parameters",
        )
        _integer(parameters.get("soak_minutes"), "soak_minutes", minimum=45)
        _integer(
            parameters.get("native_module_iterations"),
            "native_module_iterations",
            minimum=1000,
        )
    elif artifact_name == "long-fuzz-evidence":
        _expect_fields(
            parameters,
            {"fuzz_seconds_per_target", "fuzz_targets"},
            "long fuzz parameters",
        )
        _integer(
            parameters.get("fuzz_seconds_per_target"),
            "fuzz_seconds_per_target",
            minimum=600,
        )
        targets = tuple(
            _string(target, f"fuzz target {index}")
            for index, target in enumerate(
                _sequence(parameters.get("fuzz_targets"), "fuzz_targets")
            )
        )
        if targets != tuple(EXPECTED_FUZZ_TARGETS):
            raise ReleaseBodyError("long fuzz target policy mismatch")
    return evidence


def _validate_coverage_payload(value: object) -> None:
    payload = _mapping(value, "component coverage payload_evidence")
    _expect_fields(
        payload,
        {
            "schema_version",
            "thresholds_passed",
            "line_percent",
            "html_index",
            "raw_coverage",
            "threshold_policy",
            "minimum_scope",
        },
        "component coverage payload_evidence",
    )
    if payload.get("schema_version") != 2 or payload.get("thresholds_passed") is not True:
        raise ReleaseBodyError("component coverage payload did not pass schema v2 policy")
    percentages = _mapping(payload.get("line_percent"), "component coverage line_percent")
    if set(percentages) != set(EXPECTED_COVERAGE_THRESHOLDS):
        raise ReleaseBodyError("component coverage component set mismatch")
    for name, threshold in EXPECTED_COVERAGE_THRESHOLDS.items():
        percent = _number(percentages.get(name), f"component coverage {name}")
        if percent < threshold or percent > 100.0:
            raise ReleaseBodyError(f"component coverage {name} violates release threshold")
    if payload.get("html_index") != "html/index.html":
        raise ReleaseBodyError("component coverage HTML index mismatch")
    raw = _mapping(payload.get("raw_coverage"), "component coverage raw_coverage")
    _expect_fields(raw, {"type", "version", "recomputed"}, "component coverage raw_coverage")
    if (
        raw.get("type") != "llvm.coverage.json.export"
        or not isinstance(raw.get("version"), str)
        or not raw["version"]
        or raw.get("recomputed") is not True
    ):
        raise ReleaseBodyError("component coverage raw evidence was not recomputed")
    if payload.get("threshold_policy") != "component-thresholds.json":
        raise ReleaseBodyError("component coverage threshold policy mismatch")
    if payload.get("minimum_scope") != MINIMUM_COVERAGE_SCOPE:
        raise ReleaseBodyError("component coverage minimum scope policy mismatch")


def _validate_benchmark_payload(value: object, candidate_sha: str) -> None:
    payload = _mapping(value, "runtime benchmark payload_evidence")
    _expect_fields(
        payload,
        {
            "schema_version",
            "success",
            "decision",
            "base_sha",
            "head_sha",
            "runs_per_revision",
            "confirmation_triggered",
            "runtime_inputs_equivalent",
            "metric_regressions_recomputed",
            "metrics",
            "observed_failure_metrics",
            "absolute_slo",
            "authoritative_runtime_inputs",
        },
        "runtime benchmark payload_evidence",
    )
    if payload.get("schema_version") != 3 or payload.get("success") is not True:
        raise ReleaseBodyError("runtime benchmark payload did not pass schema v3 policy")
    base_sha = _sha(payload.get("base_sha"), "runtime benchmark base_sha")
    head_sha = _sha(payload.get("head_sha"), "runtime benchmark head_sha")
    if head_sha != candidate_sha or base_sha == head_sha:
        raise ReleaseBodyError("runtime benchmark revision binding mismatch")
    runs = _integer(payload.get("runs_per_revision"), "runtime benchmark runs")
    if runs not in (3, 5):
        raise ReleaseBodyError("runtime benchmark run count is not three or five")
    confirmation = _boolean(
        payload.get("confirmation_triggered"),
        "runtime benchmark confirmation_triggered",
    )
    if confirmation != (runs == 5):
        raise ReleaseBodyError("runtime benchmark confirmation state mismatch")
    equivalent = _boolean(
        payload.get("runtime_inputs_equivalent"),
        "runtime benchmark runtime_inputs_equivalent",
    )
    expected_decision = "equivalent-runtime-inputs" if equivalent else "thresholds-passed"
    if payload.get("decision") != expected_decision:
        raise ReleaseBodyError("runtime benchmark decision mismatch")
    if payload.get("metric_regressions_recomputed") is not True:
        raise ReleaseBodyError("runtime benchmark metrics were not independently recomputed")

    metrics = _mapping(payload.get("metrics"), "runtime benchmark metrics")
    if set(metrics) != set(EXPECTED_BENCHMARK_METRICS):
        raise ReleaseBodyError("runtime benchmark metric set mismatch")
    computed_failures: list[str] = []
    for name, (expected_direction, expected_limit) in EXPECTED_BENCHMARK_METRICS.items():
        metric = _mapping(metrics.get(name), f"runtime benchmark metric {name}")
        _expect_fields(
            metric,
            {
                "direction",
                "base",
                "head",
                "regression_ratio",
                "maximum_regression_ratio",
                "passed",
                "base_sample_count",
                "head_sample_count",
                "paired_run_count",
            },
            f"runtime benchmark metric {name}",
        )
        if metric.get("direction") != expected_direction:
            raise ReleaseBodyError(f"runtime benchmark metric {name} direction mismatch")
        base_value = _number(metric.get("base"), f"runtime benchmark metric {name} base", minimum=0.0)
        head_value = _number(metric.get("head"), f"runtime benchmark metric {name} head", minimum=0.0)
        if base_value == 0.0 or head_value == 0.0:
            raise ReleaseBodyError(f"runtime benchmark metric {name} is not positive")
        regression = _number(
            metric.get("regression_ratio"),
            f"runtime benchmark metric {name} regression_ratio",
        )
        limit = _number(
            metric.get("maximum_regression_ratio"),
            f"runtime benchmark metric {name} maximum_regression_ratio",
            minimum=0.0,
        )
        if not math.isclose(limit, expected_limit, rel_tol=1e-12, abs_tol=1e-12):
            raise ReleaseBodyError(f"runtime benchmark metric {name} policy mismatch")
        passed = _boolean(metric.get("passed"), f"runtime benchmark metric {name} passed")
        if passed != (regression <= limit):
            raise ReleaseBodyError(f"runtime benchmark metric {name} pass result mismatch")
        base_samples = _integer(
            metric.get("base_sample_count"),
            f"runtime benchmark metric {name} base_sample_count",
        )
        head_samples = _integer(
            metric.get("head_sample_count"),
            f"runtime benchmark metric {name} head_sample_count",
        )
        if base_samples != head_samples or base_samples < runs:
            raise ReleaseBodyError(f"runtime benchmark metric {name} sample count mismatch")
        paired_runs = _integer(
            metric.get("paired_run_count"),
            f"runtime benchmark metric {name} paired_run_count",
            minimum=0,
        )
        expected_paired_runs = 0 if name == "gc_pause_p99_us" else runs
        if paired_runs != expected_paired_runs:
            raise ReleaseBodyError(f"runtime benchmark metric {name} paired run count mismatch")
        if not passed:
            computed_failures.append(name)

    observed_failures = _sequence(
        payload.get("observed_failure_metrics"),
        "runtime benchmark observed_failure_metrics",
    )
    if list(observed_failures) != computed_failures:
        raise ReleaseBodyError("runtime benchmark observed failure metric set mismatch")
    if not equivalent and computed_failures:
        raise ReleaseBodyError("runtime benchmark thresholds-passed decision has failures")

    absolute_slo = _mapping(payload.get("absolute_slo"), "runtime benchmark absolute_slo")
    _expect_fields(
        absolute_slo,
        {"policy", "head_results", "passed"},
        "runtime benchmark absolute_slo",
    )
    if absolute_slo.get("passed") is not True:
        raise ReleaseBodyError("runtime benchmark absolute SLO did not pass")
    absolute_policy = _mapping(
        absolute_slo.get("policy"),
        "runtime benchmark absolute SLO policy",
    )
    expected_policy_fields = set(EXPECTED_BENCHMARK_ABSOLUTE_SCOPE) | {"metrics"}
    _expect_fields(
        absolute_policy,
        expected_policy_fields,
        "runtime benchmark absolute SLO policy",
    )
    for name, expected in EXPECTED_BENCHMARK_ABSOLUTE_SCOPE.items():
        if absolute_policy.get(name) != expected:
            raise ReleaseBodyError(f"runtime benchmark absolute SLO policy {name} mismatch")
    absolute_metrics = _mapping(
        absolute_policy.get("metrics"),
        "runtime benchmark absolute SLO policy metrics",
    )
    if set(absolute_metrics) != set(EXPECTED_BENCHMARK_ABSOLUTE_SLOS):
        raise ReleaseBodyError("runtime benchmark absolute SLO metric policy set mismatch")
    for name, (direction, threshold) in EXPECTED_BENCHMARK_ABSOLUTE_SLOS.items():
        policy = _mapping(
            absolute_metrics.get(name),
            f"runtime benchmark absolute SLO policy metric {name}",
        )
        _expect_fields(policy, {"direction", "threshold"}, f"absolute SLO policy metric {name}")
        if policy.get("direction") != direction or not math.isclose(
            _number(policy.get("threshold"), f"absolute SLO policy metric {name} threshold"),
            threshold,
            rel_tol=1e-12,
            abs_tol=1e-12,
        ):
            raise ReleaseBodyError(f"runtime benchmark absolute SLO policy metric {name} mismatch")
    head_results = _sequence(
        absolute_slo.get("head_results"),
        "runtime benchmark absolute SLO head_results",
    )
    if len(head_results) != runs:
        raise ReleaseBodyError("runtime benchmark absolute SLO head result count mismatch")
    for index, raw_result in enumerate(head_results):
        result = _mapping(raw_result, f"runtime benchmark absolute SLO head result {index}")
        if set(result) != set(EXPECTED_BENCHMARK_ABSOLUTE_SLOS):
            raise ReleaseBodyError("runtime benchmark absolute SLO head metric set mismatch")
        for name, (direction, threshold) in EXPECTED_BENCHMARK_ABSOLUTE_SLOS.items():
            evidence = _mapping(
                result.get(name),
                f"runtime benchmark absolute SLO head result {index} metric {name}",
            )
            _expect_fields(
                evidence,
                {"direction", "threshold", "actual", "passed"},
                f"absolute SLO head result {index} metric {name}",
            )
            actual = _number(
                evidence.get("actual"),
                f"absolute SLO head result {index} metric {name} actual",
                minimum=0.0,
            )
            metric_passed = actual >= threshold if direction == "higher" else actual <= threshold
            if (
                evidence.get("direction") != direction
                or not math.isclose(
                    _number(
                        evidence.get("threshold"),
                        f"absolute SLO head result {index} metric {name} threshold",
                    ),
                    threshold,
                    rel_tol=1e-12,
                    abs_tol=1e-12,
                )
                or evidence.get("passed") is not True
                or not metric_passed
            ):
                raise ReleaseBodyError(
                    f"runtime benchmark absolute SLO head result {index} metric {name} mismatch"
                )

    authoritative = _mapping(
        payload.get("authoritative_runtime_inputs"),
        "runtime benchmark authoritative_runtime_inputs",
    )
    _expect_fields(
        authoritative,
        {
            "source",
            "equivalent",
            "base_commit_sha",
            "head_commit_sha",
            "base_root_tree_sha",
            "head_root_tree_sha",
            "base_ancestry",
            "paths",
        },
        "runtime benchmark authoritative_runtime_inputs",
    )
    if (
        authoritative.get("source") != "github-git-root-tree"
        or authoritative.get("equivalent") is not equivalent
        or _sha(authoritative.get("base_commit_sha"), "benchmark authoritative base SHA")
        != base_sha
        or _sha(authoritative.get("head_commit_sha"), "benchmark authoritative head SHA")
        != head_sha
    ):
        raise ReleaseBodyError("runtime benchmark authoritative Git binding mismatch")
    _sha(authoritative.get("base_root_tree_sha"), "benchmark base root tree SHA")
    _sha(authoritative.get("head_root_tree_sha"), "benchmark head root tree SHA")
    ancestry = _mapping(
        authoritative.get("base_ancestry"),
        "runtime benchmark base ancestry",
    )
    _expect_fields(
        ancestry,
        {
            "status",
            "ahead_by",
            "behind_by",
            "base_sha",
            "head_sha",
            "merge_base_sha",
        },
        "runtime benchmark base ancestry",
    )
    if (
        ancestry.get("status") != "ahead"
        or _integer(ancestry.get("ahead_by"), "benchmark ancestry ahead_by") <= 0
        or _integer(
            ancestry.get("behind_by"),
            "benchmark ancestry behind_by",
            minimum=0,
        )
        != 0
        or _sha(ancestry.get("base_sha"), "benchmark ancestry base_sha") != base_sha
        or _sha(ancestry.get("head_sha"), "benchmark ancestry head_sha") != head_sha
        or _sha(ancestry.get("merge_base_sha"), "benchmark ancestry merge_base_sha")
        != base_sha
    ):
        raise ReleaseBodyError("runtime benchmark base ancestry mismatch")
    paths = _mapping(authoritative.get("paths"), "benchmark authoritative paths")
    if set(paths) != set(EXPECTED_BENCHMARK_RUNTIME_INPUTS):
        raise ReleaseBodyError("runtime benchmark authoritative path set mismatch")
    computed_equivalent = True
    for path in EXPECTED_BENCHMARK_RUNTIME_INPUTS:
        entry = _mapping(paths.get(path), f"benchmark authoritative path {path}")
        _expect_fields(entry, {"type", "base_sha", "head_sha", "equivalent"}, f"path {path}")
        expected_type = "blob" if path == "CMakeLists.txt" else "tree"
        if entry.get("type") != expected_type:
            raise ReleaseBodyError(f"runtime benchmark path {path} type mismatch")
        path_equivalent = (
            _sha(entry.get("base_sha"), f"benchmark path {path} base SHA")
            == _sha(entry.get("head_sha"), f"benchmark path {path} head SHA")
        )
        if entry.get("equivalent") is not path_equivalent:
            raise ReleaseBodyError(f"runtime benchmark path {path} equivalence mismatch")
        computed_equivalent = computed_equivalent and path_equivalent
    if computed_equivalent != equivalent:
        raise ReleaseBodyError("runtime benchmark aggregate input equivalence mismatch")


def _validate_runtime_payload(
    value: object,
    parameters: Mapping[str, Any],
    jobs: Mapping[str, Mapping[str, Any]],
) -> None:
    payload = _mapping(value, "runtime soak payload_evidence")
    _expect_fields(
        payload,
        {
            "schema_version",
            "status",
            "duration_ms",
            "iterations",
            "native_module_iterations",
        },
        "runtime soak payload_evidence",
    )
    if payload.get("schema_version") != 1 or payload.get("status") != "passed":
        raise ReleaseBodyError("runtime soak payload did not pass schema v1 policy")
    duration_ms = _integer(payload.get("duration_ms"), "runtime soak duration_ms")
    _integer(payload.get("iterations"), "runtime soak iterations")
    native_iterations = _integer(
        payload.get("native_module_iterations"),
        "runtime soak native_module_iterations",
    )
    soak_minutes = _integer(parameters.get("soak_minutes"), "runtime soak soak_minutes")
    declared_native = _integer(
        parameters.get("native_module_iterations"),
        "runtime soak declared native iterations",
    )
    if native_iterations != declared_native or duration_ms < soak_minutes * 60 * 1000:
        raise ReleaseBodyError("runtime soak payload does not match workflow parameters")
    job_name = ARTIFACT_TIMED_JOBS["runtime-soak-evidence"]
    authoritative = _validate_timed_steps(jobs[job_name], job_name)
    if authoritative is None or authoritative < soak_minutes * 60:
        raise ReleaseBodyError("runtime soak authoritative step is too short")
    if duration_ms > (authoritative + 30) * 1000:
        raise ReleaseBodyError("runtime soak machine duration exceeds authoritative step")


def _validate_fuzz_payload(
    value: object,
    parameters: Mapping[str, Any],
    jobs: Mapping[str, Mapping[str, Any]],
) -> None:
    payload = _mapping(value, "long fuzz payload_evidence")
    _expect_fields(payload, {"targets"}, "long fuzz payload_evidence")
    targets = _mapping(payload.get("targets"), "long fuzz targets")
    if set(targets) != set(EXPECTED_FUZZ_TARGETS):
        raise ReleaseBodyError("long fuzz payload target set mismatch")
    seconds_per_target = _integer(
        parameters.get("fuzz_seconds_per_target"),
        "long fuzz declared seconds",
    )
    for target in EXPECTED_FUZZ_TARGETS:
        result = _mapping(targets.get(target), f"long fuzz target {target}")
        _expect_fields(result, {"reported_seconds", "executions"}, f"long fuzz target {target}")
        _integer(
            result.get("reported_seconds"),
            f"long fuzz target {target} reported_seconds",
            minimum=seconds_per_target,
        )
        _integer(result.get("executions"), f"long fuzz target {target} executions")
    job_name = ARTIFACT_TIMED_JOBS["long-fuzz-evidence"]
    authoritative = _validate_timed_steps(jobs[job_name], job_name)
    required = seconds_per_target * len(EXPECTED_FUZZ_TARGETS)
    if authoritative is None or authoritative < required:
        raise ReleaseBodyError("long fuzz authoritative step is too short")


def _validate_artifacts(
    value: object,
    *,
    expected_names: Sequence[str],
    repository: str,
    candidate_sha: str,
    run: Mapping[str, Any],
    jobs: Mapping[str, Mapping[str, Any]],
    now: datetime,
) -> dict[str, Mapping[str, Any]]:
    artifacts = _sequence(value, f"run {run['id']} artifacts")
    by_name: dict[str, Mapping[str, Any]] = {}
    ids: set[int] = set()
    for index, raw_artifact in enumerate(artifacts):
        artifact = _mapping(raw_artifact, f"run artifact {index}")
        _expect_fields(artifact, EXPECTED_ARTIFACT_FIELDS, "release evidence artifact")
        name = _string(artifact.get("name"), f"run artifact {index} name")
        if name in by_name:
            raise ReleaseBodyError(f"run contains duplicate artifact: {name}")
        artifact_id = _integer(artifact.get("id"), f"artifact {name} id")
        if artifact_id in ids:
            raise ReleaseBodyError(f"run contains duplicate artifact ID: {artifact_id}")
        ids.add(artifact_id)
        digest = _string(artifact.get("digest"), f"artifact {name} digest").lower()
        if _DIGEST_RE.fullmatch(digest) is None:
            raise ReleaseBodyError(f"artifact {name} digest is invalid")
        _integer(artifact.get("size_in_bytes"), f"artifact {name} size")
        _url(artifact.get("archive_download_url"), f"artifact {name} archive URL")
        created = _timestamp(artifact.get("created_at"), f"artifact {name} created_at")
        updated = _timestamp(artifact.get("updated_at"), f"artifact {name} updated_at")
        expires = _timestamp(artifact.get("expires_at"), f"artifact {name} expires_at")
        if updated < created or expires <= updated:
            raise ReleaseBodyError(f"artifact {name} timestamps are inconsistent")
        if expires <= now:
            raise ReleaseBodyError(f"artifact {name} is expired at release publication")
        workflow = _validate_workflow_evidence(
            artifact.get("workflow_evidence"),
            artifact_name=name,
            repository=repository,
            candidate_sha=candidate_sha,
            run=run,
        )
        parameters = _mapping(workflow.get("parameters"), f"artifact {name} parameters")
        payload = artifact.get("payload_evidence")
        if name == "component-coverage":
            _validate_coverage_payload(payload)
        elif name == "runtime-benchmark-evidence":
            _validate_benchmark_payload(payload, candidate_sha)
        elif name == "runtime-soak-evidence":
            _validate_runtime_payload(payload, parameters, jobs)
        elif name == "long-fuzz-evidence":
            _validate_fuzz_payload(payload, parameters, jobs)
        else:
            raise ReleaseBodyError(f"artifact {name} has no release-body policy")
        by_name[name] = artifact
    if set(by_name) != set(expected_names):
        raise ReleaseBodyError(
            "run artifact set mismatch; "
            f"missing={sorted(set(expected_names) - set(by_name))}, "
            f"extra={sorted(set(by_name) - set(expected_names))}"
        )
    return by_name


def validate_release_manifest(
    manifest: Mapping[str, Any],
    *,
    expected_repository: str,
    expected_sha: str,
    expected_tag: str,
    expected_version: str,
    now: datetime | None = None,
) -> Mapping[str, Any]:
    """Deeply validate the normalized output of verify_release_evidence.py."""

    _expect_fields(manifest, EXPECTED_MANIFEST_FIELDS, "release evidence manifest")
    if manifest.get("schema") != MANIFEST_SCHEMA:
        raise ReleaseBodyError("release evidence manifest schema mismatch")
    current = now or datetime.now(timezone.utc)
    if current.tzinfo is None:
        raise ReleaseBodyError("release manifest validation time has no timezone")
    current = current.astimezone(timezone.utc)
    if _REPOSITORY_RE.fullmatch(expected_repository) is None:
        raise ReleaseBodyError("expected repository must use owner/name form")
    if manifest.get("repository") != expected_repository:
        raise ReleaseBodyError("release evidence manifest repository mismatch")
    candidate_sha = _sha(manifest.get("candidate_sha"), "manifest candidate_sha")
    if candidate_sha != _sha(expected_sha, "expected candidate SHA"):
        raise ReleaseBodyError("release evidence manifest SHA mismatch")
    version = _string(manifest.get("version"), "manifest version")
    if version != expected_version:
        raise ReleaseBodyError("release evidence manifest version mismatch")
    abi_version = _integer(
        manifest.get("abi_version"),
        "manifest ABI version",
        minimum=0,
    )
    _timestamp(manifest.get("generated_at"), "manifest generated_at")
    try:
        validate_governance_evidence(
            _mapping(manifest.get("governance"), "manifest governance"),
            expected_repository=expected_repository,
            expected_sha=candidate_sha,
            expected_tag=expected_tag,
            expected_version=expected_version,
            now=current,
            require_approved=True,
        )
    except GovernanceError as error:
        raise ReleaseBodyError(
            f"release governance evidence is invalid: {error}"
        ) from error
    try:
        source_readiness = validate_source_readiness_evidence(
            _mapping(
                manifest.get("source_readiness"),
                "manifest source_readiness",
            ),
            expected_repository=expected_repository,
            expected_sha=candidate_sha,
            expected_version=expected_version,
            now=current,
        )
    except SourceReadinessError as error:
        raise ReleaseBodyError(
            f"release source-readiness evidence is invalid: {error}"
        ) from error
    if source_readiness["abi_version"] != abi_version:
        raise ReleaseBodyError(
            "release evidence manifest ABI version does not match source readiness"
        )
    _validate_main_history(manifest.get("main_history"))

    tool = _mapping(manifest.get("tool"), "manifest tool")
    _expect_fields(tool, {"name", "version"}, "manifest tool")
    if tool.get("name") != EXPECTED_VERIFIER_NAME or tool.get("version") != TOOL_VERSION:
        raise ReleaseBodyError("release evidence verifier identity mismatch")

    notes = _mapping(manifest.get("release_notes"), "manifest release_notes")
    _expect_fields(
        notes,
        {
            "narrative_placeholders_checked",
            "package_checksums_checked",
            "package_checksums_policy",
        },
        "manifest release_notes",
    )
    if (
        notes.get("narrative_placeholders_checked") is not True
        or notes.get("package_checksums_checked") is not False
        or notes.get("package_checksums_policy") != "deferred until packages exist"
    ):
        raise ReleaseBodyError("release evidence release-notes policy mismatch")

    runs = _mapping(manifest.get("runs"), "manifest runs")
    if set(runs) != set(RUN_POLICIES):
        raise ReleaseBodyError("release evidence run set mismatch")
    global_job_ids: set[int] = set()
    global_artifact_ids: set[int] = set()
    for key, policy in RUN_POLICIES.items():
        run = _mapping(runs.get(key), f"manifest run {key}")
        _expect_fields(run, EXPECTED_RUN_FIELDS, f"manifest run {key}")
        if (
            run.get("workflow") != policy["workflow"]
            or run.get("event") != policy["event"]
            or run.get("head_sha") != candidate_sha
            or run.get("head_branch") != MAIN_BRANCH
            or run.get("status") != "completed"
            or run.get("conclusion") != "success"
        ):
            raise ReleaseBodyError(f"release evidence run {key} binding mismatch")
        _integer(run.get("id"), f"run {key} id")
        _integer(run.get("attempt"), f"run {key} attempt")
        _url(run.get("url"), f"run {key} URL")
        created = _timestamp(run.get("created_at"), f"run {key} created_at")
        updated = _timestamp(run.get("updated_at"), f"run {key} updated_at")
        if updated < created:
            raise ReleaseBodyError(f"release evidence run {key} timestamps are inconsistent")
        jobs = _validate_jobs(run.get("jobs"), policy["jobs"])
        artifacts = _validate_artifacts(
            run.get("artifacts"),
            expected_names=policy["artifacts"],
            repository=expected_repository,
            candidate_sha=candidate_sha,
            run=run,
            jobs=jobs,
            now=current,
        )
        for job in jobs.values():
            job_id = int(job["id"])
            if job_id in global_job_ids:
                raise ReleaseBodyError(f"duplicate job ID across runs: {job_id}")
            global_job_ids.add(job_id)
        for artifact in artifacts.values():
            artifact_id = int(artifact["id"])
            if artifact_id in global_artifact_ids:
                raise ReleaseBodyError(f"duplicate artifact ID across runs: {artifact_id}")
            global_artifact_ids.add(artifact_id)
    return manifest


def validate_release_checksums(
    checksum_path: Path,
    *,
    manifest_path: Path,
    expected_version: str,
    expected_sha: str,
) -> str:
    """Validate the exact release asset set and every package payload."""

    try:
        text = checksum_path.read_text(encoding="ascii")
    except (OSError, UnicodeDecodeError) as error:
        raise ReleaseBodyError(f"release checksum index is not readable ASCII: {checksum_path}") from error
    lines = text.splitlines()
    if not lines:
        raise ReleaseBodyError("release checksum index is empty")
    entries: dict[str, str] = {}
    for index, line in enumerate(lines, start=1):
        match = _CHECKSUM_RE.fullmatch(line)
        if match is None:
            raise ReleaseBodyError(f"release checksum line {index} is malformed")
        digest, name = match.groups()
        if name == checksum_path.name or PurePath(name).name != name:
            raise ReleaseBodyError(f"release checksum line {index} has an unsafe filename")
        if name in entries:
            raise ReleaseBodyError(f"release checksum index contains duplicate filename: {name}")
        entries[name] = digest

    release_directory = checksum_path.resolve().parent
    expected_sha = _sha(expected_sha, "expected package candidate SHA")
    expected_package_names = {
        f"lua-cpp-{expected_version}-{rid}{suffix}"
        for rid in SUPPORTED_RELEASE_RIDS
        for suffix in _PACKAGE_SUFFIXES
    }
    expected_files = expected_package_names | {"release-evidence.json"}
    actual_files: dict[str, Path] = {}
    unexpected_entries: list[str] = []
    for path in release_directory.iterdir():
        if path.resolve() == checksum_path.resolve():
            continue
        if not path.is_file() or path.is_symlink():
            unexpected_entries.append(path.name)
            continue
        actual_files[path.name] = path
    if unexpected_entries:
        raise ReleaseBodyError(
            f"release directory contains non-regular assets: {sorted(unexpected_entries)}"
        )
    if set(actual_files) != expected_files:
        raise ReleaseBodyError(
            "release asset file set mismatch; "
            f"missing={sorted(expected_files - set(actual_files))}, "
            f"extra={sorted(set(actual_files) - expected_files)}"
        )
    if set(entries) != set(actual_files):
        raise ReleaseBodyError(
            "release checksum file set mismatch; "
            f"missing={sorted(set(actual_files) - set(entries))}, "
            f"extra={sorted(set(entries) - set(actual_files))}"
        )
    if manifest_path.resolve() != (release_directory / "release-evidence.json").resolve():
        raise ReleaseBodyError("release evidence manifest is not the published asset")
    if "release-evidence.json" not in entries:
        raise ReleaseBodyError("release checksum index omits release-evidence.json")
    for name, expected_digest in entries.items():
        digest = hashlib.sha256(actual_files[name].read_bytes()).hexdigest()
        if digest != expected_digest:
            raise ReleaseBodyError(f"release asset checksum mismatch: {name}")

    for rid in SUPPORTED_RELEASE_RIDS:
        stem = f"lua-cpp-{expected_version}-{rid}"
        try:
            validate_release_artifacts(
                archive=release_directory / f"{stem}.zip",
                sbom=release_directory / f"{stem}.spdx.json",
                checksums=release_directory / f"{stem}.SHA256SUMS",
                manifest=release_directory / f"{stem}.manifest.json",
                expected_version=expected_version,
                expected_rid=rid,
                expected_commit=expected_sha,
            )
        except ReleaseArtifactError as error:
            raise ReleaseBodyError(
                f"release package {rid} failed deep validation: {error}"
            ) from error
    return text.rstrip()


def build_release_body(
    manifest: Mapping[str, Any],
    *,
    expected_repository: str,
    expected_sha: str,
    expected_tag: str,
    expected_version: str,
    narrative: str,
    checksum_text: str,
    now: datetime | None = None,
) -> str:
    """Return the final Markdown body after all inputs have been validated."""

    validate_release_manifest(
        manifest,
        expected_repository=expected_repository,
        expected_sha=expected_sha,
        expected_tag=expected_tag,
        expected_version=expected_version,
        now=now,
    )
    source_readiness = _mapping(
        manifest["source_readiness"],
        "manifest source_readiness",
    )
    validate_release_notes_narrative(
        narrative,
        expected_project_version=_string(
            source_readiness.get("project_version"),
            "manifest source readiness project version",
        ),
        expected_abi_version=_integer(
            source_readiness.get("abi_version"),
            "manifest source readiness ABI version",
            minimum=0,
        ),
    )
    candidate_sha = str(manifest["candidate_sha"])
    governance = _mapping(manifest["governance"], "manifest governance")
    attestation = _mapping(
        governance["attestation"],
        "manifest governance attestation",
    )
    summary = [
        "",
        "## Exact-SHA execution evidence",
        "",
        f"- Candidate commit: `{candidate_sha}`",
        f"- Release version / ABI: `{manifest['version']}` / `{manifest['abi_version']}`",
        f"- Evidence schema: `{manifest['schema']}`",
        f"- Manifest generated: `{manifest['generated_at']}`",
        (
            f"- Governance: `{attestation['decision']}` approved by "
            f"`{attestation['approved_by']}` and independently reviewed by "
            f"`{attestation['independent_reviewer']}`; expires "
            f"`{attestation['expires_at']}`; "
            f"[record]({attestation['record_url']})"
        ),
    ]
    runs = _mapping(manifest["runs"], "manifest runs")
    for key, policy in RUN_POLICIES.items():
        run = _mapping(runs[key], f"manifest run {key}")
        summary.append(
            f"- {policy['label']}: [run {run['id']} attempt {run['attempt']}]"
            f"({run['url']})"
        )
        for raw_artifact in _sequence(run["artifacts"], f"run {key} artifacts"):
            artifact = _mapping(raw_artifact, f"run {key} artifact")
            summary.append(
                f"  - `{artifact['name']}`: artifact `{artifact['id']}`, "
                f"`{artifact['digest']}`, expires `{artifact['expires_at']}`"
            )
            workflow = _mapping(artifact["workflow_evidence"], "artifact workflow_evidence")
            parameters = _mapping(workflow["parameters"], "artifact parameters")
            if parameters:
                summary.append(
                    "    - parameters: `"
                    + json.dumps(parameters, sort_keys=True, separators=(",", ":"))
                    + "`"
                )
            payload = _mapping(artifact["payload_evidence"], "artifact payload_evidence")
            if artifact["name"] == "component-coverage":
                summary.append(
                    "    - independently recomputed coverage: `"
                    + json.dumps(
                        payload["line_percent"],
                        sort_keys=True,
                        separators=(",", ":"),
                    )
                    + "`"
                )
            elif artifact["name"] == "runtime-benchmark-evidence":
                summary.append(
                    f"    - recomputed benchmark: `{payload['decision']}`, "
                    f"`{payload['runs_per_revision']}` runs/revision; "
                    "runtime inputs bound to authoritative Git trees"
                )
            elif artifact["name"] == "runtime-soak-evidence":
                summary.append(
                    f"    - machine results: `{payload['duration_ms']}` ms runtime, "
                    f"`{payload['native_module_iterations']}` native-module iterations"
                )
            elif artifact["name"] == "long-fuzz-evidence":
                summary.append(
                    "    - machine target results: `"
                    + json.dumps(payload["targets"], sort_keys=True, separators=(",", ":"))
                    + "`"
                )
    summary.extend(
        [
            "- Published machine-readable manifest: `release-evidence.json`",
            "",
            "## Published asset SHA-256",
            "",
            "`SHA256SUMS` intentionally excludes itself; the release body is "
            "generated after hashing and is not a release asset.",
            "",
            "```text",
            checksum_text,
            "```",
            "",
        ]
    )
    return narrative.rstrip() + "\n" + "\n".join(summary)


def _write_atomic(path: Path, text: str) -> None:
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
        temporary.write(text)
        temporary.write("\n")
        temporary_path = Path(temporary.name)
    os.replace(temporary_path, path)


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--release-notes", type=Path, required=True)
    parser.add_argument("--checksums", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument(
        "--expected-repository",
        default=os.environ.get("GITHUB_REPOSITORY"),
    )
    parser.add_argument(
        "--expected-sha",
        default=os.environ.get("CANDIDATE_SHA"),
    )
    parser.add_argument(
        "--expected-tag",
        default=os.environ.get("GITHUB_REF_NAME"),
    )
    parser.add_argument(
        "--expected-version",
        default=os.environ.get("RELEASE_VERSION"),
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
        if not args.expected_repository:
            raise ReleaseBodyError(
                "expected repository is required via --expected-repository or GITHUB_REPOSITORY"
            )
        if not args.expected_sha:
            raise ReleaseBodyError(
                "expected SHA is required via --expected-sha or CANDIDATE_SHA"
            )
        if not args.expected_tag:
            raise ReleaseBodyError(
                "expected tag is required via --expected-tag or GITHUB_REF_NAME"
            )
        if not args.expected_version:
            raise ReleaseBodyError(
                "expected version is required via --expected-version or RELEASE_VERSION"
            )
        manifest = _load_json(args.manifest, "release evidence manifest")
        try:
            narrative = args.release_notes.read_text(encoding="utf-8-sig")
        except (OSError, UnicodeDecodeError) as error:
            raise ReleaseBodyError("release notes are not readable UTF-8") from error
        checksum_text = validate_release_checksums(
            args.checksums,
            manifest_path=args.manifest,
            expected_version=args.expected_version,
            expected_sha=args.expected_sha,
        )
        body = build_release_body(
            manifest,
            expected_repository=args.expected_repository,
            expected_sha=args.expected_sha,
            expected_tag=args.expected_tag,
            expected_version=args.expected_version,
            narrative=narrative,
            checksum_text=checksum_text,
            now=now,
        )
        _write_atomic(args.output, body)
    except Exception as error:  # Fail closed for parsing, policy, and filesystem errors.
        print(f"release body generation failed: {error}", file=sys.stderr)
        return 1
    print(f"Release body written to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
