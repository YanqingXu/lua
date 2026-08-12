#!/usr/bin/env python3
"""Contract tests for exact-SHA GitHub release evidence verification."""

from __future__ import annotations

import copy
import hashlib
import io
import json
import os
import re
import tempfile
import unittest
import zipfile
from collections.abc import Callable
from datetime import datetime, timezone
from pathlib import Path
from unittest import mock

import verify_release_evidence as verifier
import verify_release_governance as governance
import verify_source_readiness_evidence as readiness
import write_workflow_evidence as metadata_writer


CANDIDATE_SHA = "a" * 40
OLD_SHA = "b" * 40
BASE_SHA = "c" * 40
REPOSITORY = "example/lua"
VERSION = "0.1.0-rc.1"
TAG = f"v{VERSION}"
NOW = datetime(2026, 7, 26, 12, 0, 0, tzinfo=timezone.utc)
CREATED = "2026-07-25T01:00:00Z"
UPDATED = "2026-07-25T02:00:00Z"
EXPIRES = "2026-08-24T02:00:00Z"

CI_RUN_ID = 1001
DISPATCH_RUN_ID = 2001
SCHEDULE_RUN_ID = 2002
BASE_ROOT_TREE_SHA = "d" * 40
HEAD_ROOT_TREE_SHA = "e" * 40
SHARED_CMAKE_FILE_SHA = "1" * 40
SHARED_CMAKE_TREE_SHA = "2" * 40
BASE_SRC_TREE_SHA = "3" * 40
HEAD_SRC_TREE_SHA = "4" * 40


def make_run(run_id: int, workflow: str, event: str) -> dict[str, object]:
    return {
        "id": run_id,
        "run_attempt": 2,
        "head_sha": CANDIDATE_SHA,
        "head_branch": "main",
        "event": event,
        "status": "completed",
        "conclusion": "success",
        "path": f".github/workflows/{workflow}",
        "html_url": f"https://github.com/{REPOSITORY}/actions/runs/{run_id}",
        "created_at": CREATED,
        "updated_at": UPDATED,
    }


def make_job(
    run_id: int,
    job_index: int,
    name: str,
    *,
    lint_steps: bool = False,
) -> dict[str, object]:
    job: dict[str, object] = {
        "id": run_id * 100 + job_index,
        "run_id": run_id,
        "run_attempt": 2,
        "head_sha": CANDIDATE_SHA,
        "name": name,
        "status": "completed",
        "conclusion": "success",
        "started_at": "2026-07-25T01:00:00Z",
        "completed_at": "2026-07-25T02:00:00Z",
        "html_url": f"https://github.com/{REPOSITORY}/actions/runs/{run_id}/job/{len(name)}",
    }
    if lint_steps:
        job["steps"] = [
            {
                "name": "Set up job",
                "status": "completed",
                "conclusion": "success",
            },
            {
                "name": "clang-format",
                "status": "completed",
                "conclusion": "success",
            },
            {
                "name": "Configure compile database",
                "status": "completed",
                "conclusion": "success",
            },
            {
                "name": "clang-tidy",
                "status": "completed",
                "conclusion": "success",
            },
        ]
    elif name == "Runtime and native-module soak":
        job["steps"] = [
            {
                "name": "Run runtime soak",
                "status": "completed",
                "conclusion": "success",
                "started_at": "2026-07-25T01:06:00Z",
                "completed_at": "2026-07-25T01:51:00Z",
            },
            {
                "name": "Run native-module lifecycle soak",
                "status": "completed",
                "conclusion": "success",
                "started_at": "2026-07-25T01:51:00Z",
                "completed_at": "2026-07-25T01:53:00Z",
            },
        ]
    elif name == "Long sanitizer fuzz":
        job["steps"] = [
            {
                "name": "Run long fuzz campaign",
                "status": "completed",
                "conclusion": "success",
                "started_at": "2026-07-25T01:00:00Z",
                "completed_at": "2026-07-25T02:00:00Z",
            },
        ]
    return job


def make_artifact(run_id: int, artifact_id: int, name: str) -> dict[str, object]:
    return {
        "id": artifact_id,
        "name": name,
        "size_in_bytes": 1,
        "expired": False,
        "created_at": "2026-07-25T01:54:00Z",
        "updated_at": "2026-07-25T01:54:01Z",
        "expires_at": EXPIRES,
        "digest": f"sha256:{'0' * 64}",
        "archive_download_url": (
            f"https://api.github.com/repos/{REPOSITORY}/actions/artifacts/{artifact_id}/zip"
        ),
        "workflow_run": {
            "id": run_id,
            "head_sha": CANDIDATE_SHA,
            "head_branch": "main",
        },
    }


def coverage_payloads() -> dict[str, object]:
    baseline = {
        "bytecode_verifier": (1, 239, 277),
        "c_api": (2, 1961, 2285),
        "debugger_core": (10, 2400, 3100),
        "gc_phases": (8, 1283, 1443),
        "opcode_handlers": (14, 1048, 1212),
        "parser_codegen": (29, 4732, 5150),
        "sandbox_denied_paths": (4, 1506, 1916),
    }
    filenames = {
        "bytecode_verifier": ["/repo/src/runtime/bytecode_verifier.cpp"],
        "c_api": [
            "/repo/src/api/lapi.cpp",
            "/repo/src/api/lauxlib.cpp",
        ],
        "debugger_core": [
            f"/repo/src/debugger/debugger_{index}.cpp"
            for index in range(10)
        ],
        "gc_phases": [
            f"/repo/src/gc/gc_{index}.cpp"
            for index in range(8)
        ],
        "opcode_handlers": [
            f"/repo/src/vm/vm_handlers_{index}.cpp"
            for index in range(14)
        ],
        "parser_codegen": [
            f"/repo/src/compiler/parser/parser_{index}.cpp"
            for index in range(29)
        ],
        "sandbox_denied_paths": [
            "/repo/src/runtime/sandbox_policy.hpp",
            "/repo/src/vm/state/global_state.cpp",
            "/repo/src/lib/baselib.cpp",
            "/repo/src/lib/lib_manager.cpp",
        ],
    }
    components: dict[str, object] = {}
    raw_files: list[dict[str, object]] = []
    for name, threshold in verifier.EXPECTED_COVERAGE_THRESHOLDS.items():
        file_count, covered, total = baseline[name]
        components[name] = {
            "files": file_count,
            "coveredLines": covered,
            "totalLines": total,
            "linePercent": round(100.0 * covered / total, 2),
            "minimumLinePercent": threshold,
            "thresholdPassed": True,
        }
        remaining_covered = covered
        remaining_total = total
        for index, filename in enumerate(filenames[name]):
            files_left = file_count - index
            file_covered = remaining_covered // files_left
            file_total = remaining_total // files_left
            raw_files.append(
                {
                    "filename": filename,
                    "summary": {
                        "lines": {
                            "covered": file_covered,
                            "count": file_total,
                        }
                    },
                }
            )
            remaining_covered -= file_covered
            remaining_total -= file_total
    return {
        "coverage.json": {
            "data": [{"files": raw_files}],
            "type": "llvm.coverage.json.export",
            "version": "2.0.1",
        },
        "coverage-components.json": {
            "schemaVersion": 2,
            "components": components,
            "thresholdsPassed": True,
        },
        "component-thresholds.json": {
            "schemaVersion": 1,
            "components": dict(verifier.EXPECTED_COVERAGE_THRESHOLDS),
        },
        "html/index.html": "<!doctype html><html><body>coverage</body></html>\n",
    }


BENCHMARK_METRIC_VALUES = {
    "parse_compile_mib_per_second": 10.0,
    "vm_instructions_per_second": 20_000_000.0,
    "cpp_to_lua_ns_per_call": 100.0,
    "lua_to_cpp_ns_per_call": 100.0,
    "coroutine_resume_yield_ns": 100.0,
    "table_operations_per_second": 2_000_000.0,
    "closure_upvalue_lifecycle_per_second": 200_000.0,
    "allocation_mib_per_second": 100.0,
    "gc_pause_p99_us": 1.0,
    "gc_pause_max_us": 1.0,
}


def benchmark_result(sha: str) -> dict[str, object]:
    return {
        "schema_version": 1,
        "success": True,
        "profile": "ci",
        "build_type": "Release",
        "compiler": "fixture-clang",
        "os": "Linux",
        "git_sha": sha,
        "workload": {"timing_samples": 3},
        "metrics": [
            {
                "name": name,
                "direction": direction,
                "samples": [BENCHMARK_METRIC_VALUES[name]],
                "median": BENCHMARK_METRIC_VALUES[name],
            }
            for name, (direction, _) in (
                verifier.EXPECTED_BENCHMARK_ABSOLUTE_SLOS.items()
            )
        ],
        "gc_pause_samples_us": [1.0],
    }


def benchmark_payloads() -> dict[str, object]:
    runs: list[dict[str, object]] = []
    for pair in range(3):
        revisions = ("base", "head") if pair % 2 == 0 else ("head", "base")
        for offset, revision in enumerate(revisions):
            minute = 10 + (pair * 2) + offset
            sha = BASE_SHA if revision == "base" else CANDIDATE_SHA
            runs.append(
                {
                    "pair": pair,
                    "revision": revision,
                    "sha": sha,
                    "resultPath": f"build/benchmark-comparison/{revision}-{pair + 1}.json",
                    "startedAt": f"2026-07-25T01:{minute:02d}:00Z",
                    "endedAt": f"2026-07-25T01:{minute:02d}:30Z",
                }
            )
    comparison: dict[str, object] = {
        "schemaVersion": 3,
        "success": True,
        "decision": "thresholds-passed",
        "baseSha": BASE_SHA,
        "headSha": CANDIDATE_SHA,
        "compiler": "fixture-clang",
        "os": "Linux",
        "runsPerRevision": 3,
        "minimumRunsPerRevision": 3,
        "maximumRunsPerRevision": 5,
        **verifier.EXPECTED_BENCHMARK_POLICY,
        "runtimeInputPaths": list(verifier.EXPECTED_BENCHMARK_RUNTIME_INPUTS),
        "runtimeInputDiffPaths": ["src/runtime.cpp"],
        "runtimeInputsEquivalent": False,
        "confirmationTriggered": False,
        "confirmationRecommended": False,
        "mixedFailingMetrics": [],
        "metrics": [
            {
                "name": name,
                "direction": direction,
                "base": BENCHMARK_METRIC_VALUES[name],
                "head": BENCHMARK_METRIC_VALUES[name],
                "baseSampleCount": 3,
                "headSampleCount": 3,
                "regressionRatio": 0.0,
                "pairedRunCount": 0 if name == "gc_pause_p99_us" else 3,
                "pairedRegressionRatios": (
                    [] if name == "gc_pause_p99_us" else [0.0, 0.0, 0.0]
                ),
                "pairedRunsWithinLimit": (
                    0 if name == "gc_pause_p99_us" else 3
                ),
                "pairedRunsOverLimit": 0,
                "pairedOutcomeMixed": False,
                "maximumRegressionRatio": maximum,
                "passed": True,
            }
            for name, (direction, maximum) in verifier.EXPECTED_BENCHMARK_METRICS.items()
        ],
        "observedThresholdFailures": [],
        "failures": [],
    }
    payloads: dict[str, object] = {
        "comparison.json": comparison,
        "run-order.json": {
            "schemaVersion": 2,
            "runnerPid": 1234,
            "baseSha": BASE_SHA,
            "headSha": CANDIDATE_SHA,
            "runtimeInputPaths": list(verifier.EXPECTED_BENCHMARK_RUNTIME_INPUTS),
            "runtimeInputDiffPaths": ["src/runtime.cpp"],
            "runtimeInputsEquivalent": False,
            "confirmationTriggered": False,
            "runs": runs,
        },
    }
    for revision, sha in (("base", BASE_SHA), ("head", CANDIDATE_SHA)):
        for index in range(1, 4):
            payloads[f"{revision}-{index}.json"] = benchmark_result(sha)
    return payloads


def runtime_soak_payloads() -> dict[str, object]:
    iterations = 10
    return {
        "runtime-soak.json": {
            "schema": 1,
            "status": "passed",
            "iterations": iterations,
            "states_created": 2 * iterations,
            "states_closed": 2 * iterations,
            "coroutine_cycles": 16 * iterations,
            "weak_values_collected": iterations,
            "finalizers_observed": 32 * iterations,
            "cancellation_checks": iterations,
            "max_cancellation_latency_us": 10_000,
            "max_allocator_peak_bytes": 4096,
            "duration_ms": 45 * 60 * 1000,
            "error": "",
        },
        "native-module-soak.json": {
            "schema": 1,
            "status": "passed",
            "iterations": 1000,
        },
    }


def long_fuzz_payloads() -> dict[str, object]:
    payloads: dict[str, object] = {}
    for target in verifier.EXPECTED_FUZZ_TARGETS:
        payloads[f"fuzz-artifacts/{target}.log"] = (
            "# fixture libFuzzer output\n"
            "DONE\n"
            "Done 42 runs in 600 second(s)\n"
            "stat::number_of_executed_units: 42\n"
        )
        payloads[f"fuzz-corpus/{target}/seed"] = f"{target} seed\n"
    return payloads


def artifact_payloads(artifact_name: str) -> dict[str, object]:
    if artifact_name == "component-coverage":
        return coverage_payloads()
    if artifact_name == "runtime-benchmark-evidence":
        return benchmark_payloads()
    if artifact_name == "runtime-soak-evidence":
        return runtime_soak_payloads()
    if artifact_name == "long-fuzz-evidence":
        return long_fuzz_payloads()
    raise AssertionError(f"unknown fixture artifact: {artifact_name}")


def make_archive(
    metadata: dict[str, object] | None,
    *,
    artifact_name: str,
    metadata_paths: tuple[str, ...] = ("evidence-metadata.json",),
    payloads: dict[str, object] | None = None,
) -> bytes:
    buffer = io.BytesIO()
    with zipfile.ZipFile(buffer, "w", zipfile.ZIP_DEFLATED) as zipped:
        for path, value in (
            artifact_payloads(artifact_name) if payloads is None else payloads
        ).items():
            encoded_payload = (
                json.dumps(value, sort_keys=True).encode("utf-8")
                if isinstance(value, dict)
                else str(value).encode("utf-8")
            )
            zipped.writestr(path, encoded_payload)
        if metadata is not None:
            encoded = json.dumps(metadata, sort_keys=True).encode("utf-8")
            for path in metadata_paths:
                zipped.writestr(path, encoded)
    return buffer.getvalue()


class FakeGitHub:
    def __init__(self) -> None:
        self.compare: dict[str, object] = {
            "status": "ahead",
            "ahead_by": 3,
            "behind_by": 0,
            "base_commit": {"sha": CANDIDATE_SHA},
            "merge_base_commit": {"sha": CANDIDATE_SHA},
        }
        self.benchmark_compare: dict[str, object] = {
            "status": "ahead",
            "ahead_by": 1,
            "behind_by": 0,
            "base_commit": {"sha": BASE_SHA},
            "merge_base_commit": {"sha": BASE_SHA},
        }
        self.runs: dict[tuple[str, str], list[dict[str, object]]] = {
            (verifier.CI_WORKFLOW, "push"): [
                make_run(CI_RUN_ID, verifier.CI_WORKFLOW, "push")
            ],
            (verifier.NIGHTLY_WORKFLOW, "workflow_dispatch"): [
                make_run(
                    DISPATCH_RUN_ID,
                    verifier.NIGHTLY_WORKFLOW,
                    "workflow_dispatch",
                )
            ],
            (verifier.NIGHTLY_WORKFLOW, "schedule"): [
                make_run(SCHEDULE_RUN_ID, verifier.NIGHTLY_WORKFLOW, "schedule")
            ],
        }
        self.git_commits: dict[str, dict[str, object]] = {
            BASE_SHA: {
                "sha": BASE_SHA,
                "tree": {"sha": BASE_ROOT_TREE_SHA},
            },
            CANDIDATE_SHA: {
                "sha": CANDIDATE_SHA,
                "tree": {"sha": HEAD_ROOT_TREE_SHA},
            },
        }
        self.git_trees: dict[str, dict[str, object]] = {
            BASE_ROOT_TREE_SHA: {
                "sha": BASE_ROOT_TREE_SHA,
                "truncated": False,
                "tree": [
                    {
                        "path": "CMakeLists.txt",
                        "type": "blob",
                        "sha": SHARED_CMAKE_FILE_SHA,
                    },
                    {
                        "path": "cmake",
                        "type": "tree",
                        "sha": SHARED_CMAKE_TREE_SHA,
                    },
                    {
                        "path": "src",
                        "type": "tree",
                        "sha": BASE_SRC_TREE_SHA,
                    },
                ],
            },
            HEAD_ROOT_TREE_SHA: {
                "sha": HEAD_ROOT_TREE_SHA,
                "truncated": False,
                "tree": [
                    {
                        "path": "CMakeLists.txt",
                        "type": "blob",
                        "sha": SHARED_CMAKE_FILE_SHA,
                    },
                    {
                        "path": "cmake",
                        "type": "tree",
                        "sha": SHARED_CMAKE_TREE_SHA,
                    },
                    {
                        "path": "src",
                        "type": "tree",
                        "sha": HEAD_SRC_TREE_SHA,
                    },
                ],
            },
        }
        self.jobs: dict[int, list[dict[str, object]]] = {
            CI_RUN_ID: [
                make_job(
                    CI_RUN_ID,
                    job_index,
                    name,
                    lint_steps=name == verifier.LINT_JOB,
                )
                for job_index, name in enumerate(
                    verifier.EXPECTED_CI_JOBS,
                    start=1,
                )
            ],
            DISPATCH_RUN_ID: [
                make_job(DISPATCH_RUN_ID, job_index, name)
                for job_index, name in enumerate(
                    verifier.EXPECTED_NIGHTLY_JOBS,
                    start=1,
                )
            ],
            SCHEDULE_RUN_ID: [
                make_job(SCHEDULE_RUN_ID, job_index, name)
                for job_index, name in enumerate(
                    verifier.EXPECTED_NIGHTLY_JOBS,
                    start=1,
                )
            ],
        }
        self.artifacts: dict[int, list[dict[str, object]]] = {
            CI_RUN_ID: [
                make_artifact(CI_RUN_ID, 3001, verifier.CI_ARTIFACTS[0]),
                make_artifact(CI_RUN_ID, 3002, verifier.CI_ARTIFACTS[1]),
            ],
            DISPATCH_RUN_ID: [
                make_artifact(
                    DISPATCH_RUN_ID,
                    4001,
                    verifier.NIGHTLY_ARTIFACTS[0],
                ),
                make_artifact(
                    DISPATCH_RUN_ID,
                    4002,
                    verifier.NIGHTLY_ARTIFACTS[1],
                ),
            ],
            SCHEDULE_RUN_ID: [
                make_artifact(
                    SCHEDULE_RUN_ID,
                    5001,
                    verifier.NIGHTLY_ARTIFACTS[0],
                ),
                make_artifact(
                    SCHEDULE_RUN_ID,
                    5002,
                    verifier.NIGHTLY_ARTIFACTS[1],
                ),
            ],
        }
        self.downloads: dict[str, bytes] = {}
        for run_id, artifacts in self.artifacts.items():
            for artifact in artifacts:
                artifact_name = str(artifact["name"])
                metadata = self.make_metadata(run_id, artifact_name)
                self.set_archive(
                    artifact,
                    make_archive(metadata, artifact_name=artifact_name),
                )

    def make_metadata(self, run_id: int, artifact_name: str) -> dict[str, object]:
        if run_id == CI_RUN_ID:
            workflow = verifier.CI_WORKFLOW
            event = "push"
        elif run_id == DISPATCH_RUN_ID:
            workflow = verifier.NIGHTLY_WORKFLOW
            event = "workflow_dispatch"
        elif run_id == SCHEDULE_RUN_ID:
            workflow = verifier.NIGHTLY_WORKFLOW
            event = "schedule"
        else:
            raise AssertionError(f"unknown fixture run: {run_id}")
        if artifact_name in verifier.CI_ARTIFACTS:
            parameters: dict[str, object] = {}
        elif artifact_name == "runtime-soak-evidence":
            parameters = {
                "soak_minutes": 45,
                "native_module_iterations": 1000,
            }
        elif artifact_name == "long-fuzz-evidence":
            parameters = {
                "fuzz_seconds_per_target": 600,
                "fuzz_targets": list(verifier.EXPECTED_FUZZ_TARGETS),
            }
        else:
            raise AssertionError(f"unknown fixture artifact: {artifact_name}")
        return {
            "schema": verifier.WORKFLOW_EVIDENCE_SCHEMA,
            "kind": artifact_name,
            "repository": REPOSITORY,
            "candidate_sha": CANDIDATE_SHA,
            "run_id": run_id,
            "run_attempt": 2,
            "event": event,
            "workflow_ref": (
                f"{REPOSITORY}/.github/workflows/{workflow}@refs/heads/main"
            ),
            "job": verifier.EXPECTED_ARTIFACT_JOBS[artifact_name],
            "result": "passed",
            "created_at": "2026-07-25T01:53:00Z",
            "parameters": parameters,
        }

    def artifact(self, run_id: int, artifact_name: str) -> dict[str, object]:
        return next(
            artifact
            for artifact in self.artifacts[run_id]
            if artifact["name"] == artifact_name
        )

    def set_archive(self, artifact: dict[str, object], archive: bytes) -> None:
        artifact["size_in_bytes"] = len(archive)
        artifact["digest"] = f"sha256:{hashlib.sha256(archive).hexdigest()}"
        self.downloads[str(artifact["archive_download_url"])] = archive

    def rewrite_metadata(
        self,
        run_id: int,
        artifact_name: str,
        update: Callable[[dict[str, object]], None],
        *,
        metadata_paths: tuple[str, ...] = ("evidence-metadata.json",),
    ) -> None:
        metadata = self.make_metadata(run_id, artifact_name)
        update(metadata)
        self.set_archive(
            self.artifact(run_id, artifact_name),
            make_archive(
                metadata,
                artifact_name=artifact_name,
                metadata_paths=metadata_paths,
            ),
        )

    def rewrite_payload(
        self,
        run_id: int,
        artifact_name: str,
        update: Callable[[dict[str, object]], None],
    ) -> None:
        payloads = artifact_payloads(artifact_name)
        update(payloads)
        self.set_archive(
            self.artifact(run_id, artifact_name),
            make_archive(
                self.make_metadata(run_id, artifact_name),
                artifact_name=artifact_name,
                payloads=payloads,
            ),
        )

    @staticmethod
    def _page(items: list[dict[str, object]], params: object, key: str) -> dict[str, object]:
        query = params if isinstance(params, dict) else {}
        page = int(query.get("page", 1))
        per_page = int(query.get("per_page", 100))
        start = (page - 1) * per_page
        return {
            "total_count": len(items),
            key: copy.deepcopy(items[start : start + per_page]),
        }

    def get(self, path: str, params: object = None) -> object:
        compare_path = f"repos/{REPOSITORY}/compare/{CANDIDATE_SHA}...main"
        if path == compare_path:
            return copy.deepcopy(self.compare)
        benchmark_compare_path = (
            f"repos/{REPOSITORY}/compare/{BASE_SHA}...{CANDIDATE_SHA}"
        )
        if path == benchmark_compare_path:
            return copy.deepcopy(self.benchmark_compare)

        commit_match = re.fullmatch(
            rf"repos/{re.escape(REPOSITORY)}/git/commits/([0-9a-f]{{40}})",
            path,
        )
        if commit_match is not None:
            try:
                return copy.deepcopy(self.git_commits[commit_match.group(1)])
            except KeyError as error:
                raise verifier.EvidenceError(
                    f"missing fake Git commit: {commit_match.group(1)}"
                ) from error

        tree_match = re.fullmatch(
            rf"repos/{re.escape(REPOSITORY)}/git/trees/([0-9a-f]{{40}})",
            path,
        )
        if tree_match is not None:
            try:
                return copy.deepcopy(self.git_trees[tree_match.group(1)])
            except KeyError as error:
                raise verifier.EvidenceError(
                    f"missing fake Git tree: {tree_match.group(1)}"
                ) from error

        workflow_match = re.fullmatch(
            rf"repos/{re.escape(REPOSITORY)}/actions/workflows/([^/]+)/runs",
            path,
        )
        if workflow_match is not None:
            query = params if isinstance(params, dict) else {}
            event = str(query.get("event", ""))
            workflow = workflow_match.group(1)
            return self._page(
                self.runs.get((workflow, event), []),
                params,
                "workflow_runs",
            )

        jobs_match = re.fullmatch(
            rf"repos/{re.escape(REPOSITORY)}/actions/runs/(\d+)/attempts/(\d+)/jobs",
            path,
        )
        if jobs_match is not None:
            run_id = int(jobs_match.group(1))
            return self._page(self.jobs.get(run_id, []), params, "jobs")

        artifacts_match = re.fullmatch(
            rf"repos/{re.escape(REPOSITORY)}/actions/runs/(\d+)/artifacts",
            path,
        )
        if artifacts_match is not None:
            run_id = int(artifacts_match.group(1))
            return self._page(
                self.artifacts.get(run_id, []),
                params,
                "artifacts",
            )
        raise verifier.EvidenceError(f"unexpected fake API path: {path}")

    def download(self, url: str) -> bytes:
        try:
            return self.downloads[url]
        except KeyError as error:
            raise verifier.EvidenceError(f"fake artifact download unavailable: {url}") from error


def valid_release_notes() -> str:
    return f"""
# Lua C++ 0.1.0 Runtime Preview

## Highlights

This release provides the Lua 5.1 source and public C API compatibility layer,
the installed static and shared CMake SDK, configured runtime limits, and the
reference isolated worker. The candidate remains a Runtime Preview with the
documented allocator and native callback boundaries.

## Compatibility

The SDK version is 0.1.0 and the shared-library ABI is 0. Default state
creation retains unrestricted Lua 5.1 behavior. Resource limits and the
sandbox are selected explicitly through the configured-state API.

## Known limitations

The callback allocator does not account for every process allocation. Native
callbacks must cooperate with execution cancellation or run behind a process
boundary. The sandbox does not constrain a malicious host or native code, and
macOS process memory enforcement remains the deployer's responsibility.

## Verification

CI, manual nightly, and scheduled nightly evidence is bound to one immutable
commit by the runtime evidence manifest. Coverage is stored in
`{verifier.CI_ARTIFACTS[0]}` and benchmark results in
`{verifier.CI_ARTIFACTS[1]}`. Runtime and native-module soak results are stored
in `{verifier.NIGHTLY_ARTIFACTS[1]}`, while sanitizer fuzz results and corpus
are stored in `{verifier.NIGHTLY_ARTIFACTS[0]}`.

Platform package names, SBOM filenames, and actual package SHA-256 values are
generated and validated after packaging, then appended to the GitHub release
body without modifying this tracked narrative.
"""


def valid_governance_evidence(
    *,
    approved: bool = True,
) -> dict[str, object]:
    if not approved:
        return governance.build_governance_evidence(
            event="workflow_dispatch",
            repository=REPOSITORY,
            candidate_sha=CANDIDATE_SHA,
            tag=None,
            version=VERSION,
            attestation_json=None,
            now=NOW,
        )
    controls = {name: "enforced" for name in governance.CONTROL_NAMES}
    attestation = {
        "schema": governance.ATTESTATION_SCHEMA,
        "repository": REPOSITORY,
        "candidate_sha": CANDIDATE_SHA,
        "tag": TAG,
        "version": VERSION,
        "decision": "protected-ruleset",
        "approved_by": "release-owner",
        "independent_reviewer": "independent-reviewer",
        "approved_at": "2026-07-25T12:00:00Z",
        "expires_at": "2026-08-25T12:00:00Z",
        "record_url": "https://github.com/example/lua/issues/6#issuecomment-1",
        "controls": controls,
        "risk_acceptance": None,
        "compensating_controls": [],
    }
    return governance.build_governance_evidence(
        event="push",
        repository=REPOSITORY,
        candidate_sha=CANDIDATE_SHA,
        tag=TAG,
        version=VERSION,
        attestation_json=json.dumps(
            attestation,
            sort_keys=True,
            separators=(",", ":"),
        ),
        now=NOW,
    )


def valid_source_readiness_evidence() -> dict[str, object]:
    return {
        "schema": readiness.EVIDENCE_SCHEMA,
        "generated_at": "2026-07-26T11:59:00Z",
        "repository": REPOSITORY,
        "candidate_sha": CANDIDATE_SHA,
        "version": VERSION,
        "project_version": "0.1.0",
        "abi_version": 0,
        "checks": {name: "passed" for name in readiness.CHECK_NAMES},
    }


def verify(api: FakeGitHub) -> dict[str, object]:
    return verifier.verify_release_evidence(
        api,
        REPOSITORY,
        CANDIDATE_SHA,
        valid_release_notes(),
        valid_governance_evidence(),
        valid_source_readiness_evidence(),
        expected_tag=TAG,
        expected_version=VERSION,
        now=NOW,
    )


class ReleaseEvidenceTests(unittest.TestCase):
    def test_green_exact_sha_produces_complete_manifest(self) -> None:
        manifest = verify(FakeGitHub())

        self.assertEqual(verifier.MANIFEST_SCHEMA, manifest["schema"])
        self.assertEqual(CANDIDATE_SHA, manifest["candidate_sha"])
        self.assertEqual(VERSION, manifest["version"])
        self.assertEqual(0, manifest["abi_version"])
        self.assertEqual(
            "passed",
            manifest["source_readiness"]["checks"]["public_api_contract"],
        )
        self.assertEqual("approved", manifest["governance"]["publication"])
        self.assertEqual(
            "protected-ruleset",
            manifest["governance"]["attestation"]["decision"],
        )
        self.assertEqual("2026-07-26T12:00:00Z", manifest["generated_at"])
        self.assertEqual(
            {
                "ci_push",
                "nightly_schedule",
                "nightly_workflow_dispatch",
            },
            set(manifest["runs"]),
        )
        ci = manifest["runs"]["ci_push"]
        self.assertEqual(CI_RUN_ID, ci["id"])
        self.assertEqual(2, ci["attempt"])
        self.assertEqual(17, len(ci["jobs"]))
        self.assertEqual(
            ["component-coverage", "runtime-benchmark-evidence"],
            [artifact["name"] for artifact in ci["artifacts"]],
        )
        benchmark = next(
            artifact
            for artifact in ci["artifacts"]
            if artifact["name"] == "runtime-benchmark-evidence"
        )
        authoritative_inputs = benchmark["payload_evidence"][
            "authoritative_runtime_inputs"
        ]
        self.assertEqual("github-git-root-tree", authoritative_inputs["source"])
        self.assertFalse(authoritative_inputs["equivalent"])
        self.assertEqual(
            "ahead",
            authoritative_inputs["base_ancestry"]["status"],
        )
        self.assertTrue(
            benchmark["payload_evidence"]["metric_regressions_recomputed"]
        )
        self.assertTrue(benchmark["payload_evidence"]["absolute_slo"]["passed"])
        for run in manifest["runs"].values():
            self.assertEqual(CANDIDATE_SHA, run["head_sha"])
            for artifact in run["artifacts"]:
                self.assertRegex(artifact["digest"], r"^sha256:[0-9a-f]{64}$")
                self.assertIn("created_at", artifact)
                self.assertIn("expires_at", artifact)
                self.assertIn("payload_evidence", artifact)
                self.assertEqual(
                    artifact["name"],
                    artifact["workflow_evidence"]["kind"],
                )
                self.assertEqual(
                    run["id"],
                    artifact["workflow_evidence"]["run_id"],
                )
                self.assertEqual(
                    run["attempt"],
                    artifact["workflow_evidence"]["run_attempt"],
                )
        self.assertFalse(manifest["release_notes"]["package_checksums_checked"])

    def test_manual_candidate_manifest_preserves_non_publishing_governance(self) -> None:
        manifest = verifier.verify_release_evidence(
            FakeGitHub(),
            REPOSITORY,
            CANDIDATE_SHA,
            valid_release_notes(),
            valid_governance_evidence(approved=False),
            valid_source_readiness_evidence(),
            expected_tag=None,
            expected_version=VERSION,
            now=NOW,
        )
        self.assertEqual("candidate-only", manifest["governance"]["publication"])
        self.assertIsNone(manifest["governance"]["attestation"])

    def test_governance_is_revalidated_before_remote_evidence(self) -> None:
        evidence = valid_governance_evidence()
        evidence["attestation"]["candidate_sha"] = OLD_SHA
        with self.assertRaisesRegex(
            verifier.EvidenceError,
            "governance evidence is invalid",
        ):
            verifier.verify_release_evidence(
                FakeGitHub(),
                REPOSITORY,
                CANDIDATE_SHA,
                valid_release_notes(),
                evidence,
                valid_source_readiness_evidence(),
                expected_tag=TAG,
                expected_version=VERSION,
                now=NOW,
            )
    def test_writer_and_verifier_metadata_contracts_are_identical(self) -> None:
        api = FakeGitHub()
        generated_at = datetime(2026, 7, 25, 1, 49, 0, tzinfo=timezone.utc)
        for run_id, artifacts in api.artifacts.items():
            if run_id == CI_RUN_ID:
                workflow = verifier.CI_WORKFLOW
                event = "push"
            elif run_id == DISPATCH_RUN_ID:
                workflow = verifier.NIGHTLY_WORKFLOW
                event = "workflow_dispatch"
            else:
                workflow = verifier.NIGHTLY_WORKFLOW
                event = "schedule"
            for artifact in artifacts:
                kind = str(artifact["name"])
                parameters = api.make_metadata(run_id, kind)["parameters"]
                metadata = metadata_writer.build_evidence(
                    kind,
                    parameters,
                    {
                        "GITHUB_REPOSITORY": REPOSITORY,
                        "GITHUB_SHA": CANDIDATE_SHA,
                        "GITHUB_RUN_ID": str(run_id),
                        "GITHUB_RUN_ATTEMPT": "2",
                        "GITHUB_EVENT_NAME": event,
                        "GITHUB_WORKFLOW_REF": (
                            f"{REPOSITORY}/.github/workflows/{workflow}"
                            "@refs/heads/main"
                        ),
                        "GITHUB_JOB": verifier.EXPECTED_ARTIFACT_JOBS[kind],
                    },
                    now=generated_at,
                )
                api.set_archive(
                    artifact,
                    make_archive(metadata, artifact_name=kind),
                )

        manifest = verify(api)
        self.assertEqual(
            metadata_writer.SCHEMA,
            manifest["runs"]["ci_push"]["artifacts"][0]["workflow_evidence"]["schema"],
        )

    def test_ci_job_failure_is_rejected(self) -> None:
        api = FakeGitHub()
        api.jobs[CI_RUN_ID][0]["conclusion"] = "failure"
        with self.assertRaisesRegex(verifier.EvidenceError, "required job .*not successful"):
            verify(api)

    def test_nightly_from_old_sha_is_rejected(self) -> None:
        api = FakeGitHub()
        api.runs[(verifier.NIGHTLY_WORKFLOW, "workflow_dispatch")][0][
            "head_sha"
        ] = OLD_SHA
        with self.assertRaisesRegex(verifier.EvidenceError, "no completed nightly.yml workflow_dispatch"):
            verify(api)

    def test_missing_scheduled_nightly_is_rejected(self) -> None:
        api = FakeGitHub()
        api.runs[(verifier.NIGHTLY_WORKFLOW, "schedule")] = []
        with self.assertRaisesRegex(verifier.EvidenceError, "no completed nightly.yml schedule"):
            verify(api)

    def test_missing_artifact_is_rejected(self) -> None:
        api = FakeGitHub()
        api.artifacts[CI_RUN_ID] = [
            artifact
            for artifact in api.artifacts[CI_RUN_ID]
            if artifact["name"] != "component-coverage"
        ]
        with self.assertRaisesRegex(verifier.EvidenceError, "component-coverage is missing"):
            verify(api)

    def test_expired_artifact_is_rejected(self) -> None:
        api = FakeGitHub()
        api.artifacts[DISPATCH_RUN_ID][0]["expired"] = True
        with self.assertRaisesRegex(verifier.EvidenceError, "artifact long-fuzz-evidence is expired"):
            verify(api)

    def test_artifact_without_sha256_digest_is_rejected(self) -> None:
        api = FakeGitHub()
        api.artifacts[SCHEDULE_RUN_ID][0]["digest"] = None
        with self.assertRaisesRegex(verifier.EvidenceError, "digest is missing"):
            verify(api)

    def test_artifact_from_other_sha_is_rejected(self) -> None:
        api = FakeGitHub()
        api.artifacts[CI_RUN_ID][0]["workflow_run"]["head_sha"] = OLD_SHA
        with self.assertRaisesRegex(verifier.EvidenceError, "not bound to the selected exact-SHA run"):
            verify(api)

    def test_artifact_download_failure_is_rejected(self) -> None:
        api = FakeGitHub()
        artifact = api.artifact(CI_RUN_ID, "component-coverage")
        del api.downloads[str(artifact["archive_download_url"])]
        with self.assertRaisesRegex(verifier.EvidenceError, "download unavailable"):
            verify(api)

    def test_downloaded_zip_digest_mismatch_is_rejected(self) -> None:
        api = FakeGitHub()
        artifact = api.artifact(CI_RUN_ID, "component-coverage")
        url = str(artifact["archive_download_url"])
        archive = api.downloads[url]
        api.downloads[url] = bytes([archive[0] ^ 0x1]) + archive[1:]
        with self.assertRaisesRegex(verifier.EvidenceError, "ZIP digest does not match"):
            verify(api)

    def test_missing_or_duplicate_workflow_metadata_is_rejected(self) -> None:
        for metadata_paths, expected in (
            ((), "exactly one evidence-metadata.json"),
            (
                ("first/evidence-metadata.json", "second/evidence-metadata.json"),
                "exactly one evidence-metadata.json",
            ),
        ):
            with self.subTest(metadata_paths=metadata_paths):
                api = FakeGitHub()
                artifact = api.artifact(CI_RUN_ID, "component-coverage")
                metadata = (
                    None
                    if not metadata_paths
                    else api.make_metadata(CI_RUN_ID, "component-coverage")
                )
                api.set_archive(
                    artifact,
                    make_archive(
                        metadata,
                        artifact_name="component-coverage",
                        metadata_paths=metadata_paths,
                    ),
                )
                with self.assertRaisesRegex(verifier.EvidenceError, expected):
                    verify(api)

    def test_metadata_only_archive_is_rejected(self) -> None:
        cases = (
            (CI_RUN_ID, "component-coverage"),
            (CI_RUN_ID, "runtime-benchmark-evidence"),
            (DISPATCH_RUN_ID, "runtime-soak-evidence"),
            (SCHEDULE_RUN_ID, "long-fuzz-evidence"),
        )
        for run_id, artifact_name in cases:
            with self.subTest(artifact=artifact_name):
                api = FakeGitHub()
                artifact = api.artifact(run_id, artifact_name)
                api.set_archive(
                    artifact,
                    make_archive(
                        api.make_metadata(run_id, artifact_name),
                        artifact_name=artifact_name,
                        payloads={},
                    ),
                )
                with self.assertRaisesRegex(
                    verifier.EvidenceError,
                    "missing required payload file|payload file set mismatch|fuzz log set mismatch",
                ):
                    verify(api)

    def test_machine_result_payloads_are_enforced(self) -> None:
        cases: tuple[
            tuple[
                int,
                str,
                Callable[[dict[str, object]], object],
                str,
            ],
            ...,
        ] = (
            (
                CI_RUN_ID,
                "component-coverage",
                lambda payloads: payloads["coverage-components.json"].__setitem__(
                    "thresholdsPassed",
                    False,
                ),
                "coverage thresholds did not pass",
            ),
            (
                CI_RUN_ID,
                "runtime-benchmark-evidence",
                lambda payloads: payloads["comparison.json"].__setitem__(
                    "headSha",
                    OLD_SHA,
                ),
                "head SHA does not match candidate",
            ),
            (
                DISPATCH_RUN_ID,
                "runtime-soak-evidence",
                lambda payloads: payloads["native-module-soak.json"].__setitem__(
                    "iterations",
                    999,
                ),
                "fewer than 1000 iterations",
            ),
            (
                SCHEDULE_RUN_ID,
                "long-fuzz-evidence",
                lambda payloads: payloads.pop("fuzz-artifacts/parser.log"),
                "fuzz log set mismatch",
            ),
        )
        for run_id, artifact_name, update, expected in cases:
            with self.subTest(artifact=artifact_name):
                api = FakeGitHub()
                api.rewrite_payload(run_id, artifact_name, update)
                with self.assertRaisesRegex(verifier.EvidenceError, expected):
                    verify(api)

    def test_coverage_raw_evidence_policy_and_scope_are_enforced(self) -> None:
        def forge_summary(payloads: dict[str, object]) -> None:
            component = payloads["coverage-components.json"]["components"][
                "bytecode_verifier"
            ]
            component["coveredLines"] += 1

        def forge_raw_export(payloads: dict[str, object]) -> None:
            raw_files = payloads["coverage.json"]["data"][0]["files"]
            bytecode = next(
                file_report
                for file_report in raw_files
                if file_report["filename"].endswith("bytecode_verifier.cpp")
            )
            bytecode["summary"]["lines"]["covered"] -= 1

        def lower_policy(payloads: dict[str, object]) -> None:
            payloads["component-thresholds.json"]["components"][
                "bytecode_verifier"
            ] = 1.0

        def collapse_scope(payloads: dict[str, object]) -> None:
            raw_files = payloads["coverage.json"]["data"][0]["files"]
            payloads["coverage.json"]["data"][0]["files"] = [
                file_report
                for file_report in raw_files
                if not file_report["filename"].endswith("bytecode_verifier.cpp")
            ]
            payloads["coverage.json"]["data"][0]["files"].append(
                {
                    "filename": "/repo/src/runtime/bytecode_verifier.cpp",
                    "summary": {
                        "lines": {
                            "covered": 1,
                            "count": 1,
                        }
                    },
                }
            )

        cases = (
            (forge_summary, "summary contradicts raw coverage export"),
            (forge_raw_export, "summary contradicts raw coverage export"),
            (lower_policy, "threshold policy for bytecode_verifier was changed"),
            (collapse_scope, "raw scope collapsed"),
        )
        for update, expected in cases:
            with self.subTest(expected=expected):
                api = FakeGitHub()
                api.rewrite_payload(
                    CI_RUN_ID,
                    "component-coverage",
                    update,
                )
                with self.assertRaisesRegex(verifier.EvidenceError, expected):
                    verify(api)

    def test_benchmark_cannot_claim_pass_when_raw_results_regress(self) -> None:
        api = FakeGitHub()

        def update(payloads: dict[str, object]) -> None:
            for revision in ("base", "head"):
                for index in range(1, 4):
                    report = payloads[f"{revision}-{index}.json"]
                    for metric in report["metrics"]:
                        if metric["name"] == "gc_pause_p99_us":
                            value = 1.0 if revision == "base" else 1.0e300
                        elif metric["direction"] == "higher":
                            value = 1.0 if revision == "base" else 1.0e-300
                        else:
                            value = 1.0 if revision == "base" else 1.0e300
                        metric["samples"] = [value]
                        metric["median"] = value
                    report["gc_pause_samples_us"] = [
                        1.0 if revision == "base" else 1.0e300
                    ]

        api.rewrite_payload(
            CI_RUN_ID,
            "runtime-benchmark-evidence",
            update,
        )
        with self.assertRaisesRegex(
            verifier.EvidenceError,
            "absolute SLO|does not match raw machine evidence|contradicts raw evidence",
        ):
            verify(api)

    def test_benchmark_head_results_must_pass_absolute_slo(self) -> None:
        api = FakeGitHub()

        def violate_absolute_slo(payloads: dict[str, object]) -> None:
            for index in range(1, 4):
                report = payloads[f"head-{index}.json"]
                metric = next(
                    metric
                    for metric in report["metrics"]
                    if metric["name"] == "parse_compile_mib_per_second"
                )
                metric["samples"] = [0.5]
                metric["median"] = 0.5

        api.rewrite_payload(
            CI_RUN_ID,
            "runtime-benchmark-evidence",
            violate_absolute_slo,
        )
        with self.assertRaisesRegex(
            verifier.EvidenceError,
            "absolute SLO metric parse_compile_mib_per_second.*below minimum",
        ):
            verify(api)

    def test_benchmark_base_must_be_strict_candidate_ancestor(self) -> None:
        api = FakeGitHub()
        api.benchmark_compare.update(
            {
                "status": "diverged",
                "ahead_by": 1,
                "behind_by": 1,
                "merge_base_commit": {"sha": OLD_SHA},
            }
        )
        with self.assertRaisesRegex(
            verifier.EvidenceError,
            "base SHA is not a strict ancestor",
        ):
            verify(api)

    def test_benchmark_runtime_input_equivalence_uses_authoritative_git_trees(
        self,
    ) -> None:
        api = FakeGitHub()

        def claim_equivalence(payloads: dict[str, object]) -> None:
            comparison = payloads["comparison.json"]
            comparison["runtimeInputsEquivalent"] = True
            comparison["runtimeInputDiffPaths"] = []
            comparison["decision"] = "equivalent-runtime-inputs"
            payloads["run-order.json"]["runtimeInputsEquivalent"] = True
            payloads["run-order.json"]["runtimeInputDiffPaths"] = []

        api.rewrite_payload(
            CI_RUN_ID,
            "runtime-benchmark-evidence",
            claim_equivalence,
        )
        with self.assertRaisesRegex(
            verifier.EvidenceError,
            "contradicts authoritative Git trees",
        ):
            verify(api)

        api = FakeGitHub()
        head_src = next(
            entry
            for entry in api.git_trees[HEAD_ROOT_TREE_SHA]["tree"]
            if entry["path"] == "src"
        )
        head_src["sha"] = BASE_SRC_TREE_SHA
        api.rewrite_payload(
            CI_RUN_ID,
            "runtime-benchmark-evidence",
            claim_equivalence,
        )
        manifest = verify(api)
        benchmark = next(
            artifact
            for artifact in manifest["runs"]["ci_push"]["artifacts"]
            if artifact["name"] == "runtime-benchmark-evidence"
        )
        self.assertTrue(
            benchmark["payload_evidence"]["authoritative_runtime_inputs"][
                "equivalent"
            ]
        )

    def test_benchmark_git_object_validation_is_fail_closed(self) -> None:
        def remove_src(api: FakeGitHub) -> None:
            api.git_trees[HEAD_ROOT_TREE_SHA]["tree"] = [
                entry
                for entry in api.git_trees[HEAD_ROOT_TREE_SHA]["tree"]
                if entry["path"] != "src"
            ]

        def duplicate_src(api: FakeGitHub) -> None:
            api.git_trees[HEAD_ROOT_TREE_SHA]["tree"].append(
                {
                    "path": "src",
                    "type": "tree",
                    "sha": HEAD_SRC_TREE_SHA,
                }
            )

        def wrong_type(api: FakeGitHub) -> None:
            src = next(
                entry
                for entry in api.git_trees[HEAD_ROOT_TREE_SHA]["tree"]
                if entry["path"] == "src"
            )
            src["type"] = "blob"

        def malformed_tree_sha(api: FakeGitHub) -> None:
            api.git_commits[CANDIDATE_SHA]["tree"]["sha"] = "not-a-sha"

        def unavailable_commit(api: FakeGitHub) -> None:
            del api.git_commits[CANDIDATE_SHA]

        cases = (
            (remove_src, "missing runtime input path"),
            (duplicate_src, "duplicate path"),
            (wrong_type, "is not a tree"),
            (malformed_tree_sha, "not a full 40-character commit SHA"),
            (unavailable_commit, "missing fake Git commit"),
        )
        for mutate, expected in cases:
            with self.subTest(expected=expected):
                api = FakeGitHub()
                mutate(api)
                with self.assertRaisesRegex(verifier.EvidenceError, expected):
                    verify(api)

    def test_reported_duration_cannot_replace_authoritative_step_duration(self) -> None:
        cases = (
            (
                DISPATCH_RUN_ID,
                "Runtime and native-module soak",
                "Run runtime soak",
                "2026-07-25T01:07:00Z",
                "shorter than 2700 seconds",
            ),
            (
                SCHEDULE_RUN_ID,
                "Long sanitizer fuzz",
                "Run long fuzz campaign",
                "2026-07-25T01:01:00Z",
                "shorter than 3600 seconds",
            ),
        )
        for run_id, job_name, step_name, completed_at, expected in cases:
            with self.subTest(job=job_name):
                api = FakeGitHub()
                job = next(
                    job
                    for job in api.jobs[run_id]
                    if job["name"] == job_name
                )
                step = next(
                    step
                    for step in job["steps"]
                    if step["name"] == step_name
                )
                step["completed_at"] = completed_at
                with self.assertRaisesRegex(verifier.EvidenceError, expected):
                    verify(api)

    def test_workflow_metadata_binding_mismatches_are_rejected(self) -> None:
        cases = (
            ("schema", "wrong/schema", "schema mismatch"),
            ("kind", "runtime-benchmark-evidence", "kind mismatch"),
            ("repository", "other/repository", "repository mismatch"),
            ("candidate_sha", OLD_SHA, "SHA mismatch"),
            ("run_id", 9999, "run ID mismatch"),
            ("run_attempt", 1, "run attempt mismatch"),
            ("event", "schedule", "event mismatch"),
            ("workflow_ref", "wrong/ref", "workflow reference mismatch"),
            ("job", "wrong-job", "job mismatch"),
            ("result", "failed", "did not pass"),
            ("created_at", "2026-07-25T01:55:00Z", "timestamp is inconsistent"),
        )
        for field, value, expected in cases:
            with self.subTest(field=field):
                api = FakeGitHub()

                def update(metadata: dict[str, object]) -> None:
                    metadata[field] = value

                api.rewrite_metadata(
                    CI_RUN_ID,
                    "component-coverage",
                    update,
                )
                with self.assertRaisesRegex(verifier.EvidenceError, expected):
                    verify(api)

    def test_nightly_parameter_thresholds_and_targets_are_enforced(self) -> None:
        cases = (
            (
                DISPATCH_RUN_ID,
                "runtime-soak-evidence",
                "soak_minutes",
                44,
                "shorter than 45 minutes",
            ),
            (
                DISPATCH_RUN_ID,
                "runtime-soak-evidence",
                "native_module_iterations",
                999,
                "fewer than 1000 iterations",
            ),
            (
                SCHEDULE_RUN_ID,
                "long-fuzz-evidence",
                "fuzz_seconds_per_target",
                599,
                "shorter than 600 seconds",
            ),
            (
                SCHEDULE_RUN_ID,
                "long-fuzz-evidence",
                "fuzz_targets",
                list(verifier.EXPECTED_FUZZ_TARGETS[:-1]),
                "target set or order is incomplete",
            ),
        )
        for run_id, artifact_name, parameter, value, expected in cases:
            with self.subTest(artifact=artifact_name, parameter=parameter):
                api = FakeGitHub()

                def update(metadata: dict[str, object]) -> None:
                    parameters = metadata["parameters"]
                    parameters[parameter] = value

                api.rewrite_metadata(run_id, artifact_name, update)
                with self.assertRaisesRegex(verifier.EvidenceError, expected):
                    verify(api)

    def test_nightly_parameters_are_bound_to_machine_and_step_results(self) -> None:
        cases = (
            (
                DISPATCH_RUN_ID,
                "runtime-soak-evidence",
                "soak_minutes",
                46,
                "authoritative job step is shorter",
            ),
            (
                DISPATCH_RUN_ID,
                "runtime-soak-evidence",
                "native_module_iterations",
                1001,
                "does not match its declared iteration count",
            ),
            (
                SCHEDULE_RUN_ID,
                "long-fuzz-evidence",
                "fuzz_seconds_per_target",
                601,
                "authoritative job step is shorter",
            ),
        )
        for run_id, artifact_name, parameter, value, expected in cases:
            with self.subTest(artifact=artifact_name, parameter=parameter):
                api = FakeGitHub()

                def update(metadata: dict[str, object]) -> None:
                    parameters = metadata["parameters"]
                    parameters[parameter] = value

                api.rewrite_metadata(run_id, artifact_name, update)
                with self.assertRaisesRegex(verifier.EvidenceError, expected):
                    verify(api)

    def test_non_main_ancestor_is_rejected(self) -> None:
        api = FakeGitHub()
        api.compare.update(
            {
                "status": "diverged",
                "ahead_by": 2,
                "behind_by": 1,
                "merge_base_commit": {"sha": OLD_SHA},
            }
        )
        with self.assertRaisesRegex(verifier.EvidenceError, "not an ancestor of main"):
            verify(api)

    def test_skipped_lint_step_is_rejected(self) -> None:
        api = FakeGitHub()
        lint = next(
            job
            for job in api.jobs[CI_RUN_ID]
            if job["name"] == verifier.LINT_JOB
        )
        tidy = next(step for step in lint["steps"] if step["name"] == "clang-tidy")
        tidy["conclusion"] = "skipped"
        with self.assertRaisesRegex(verifier.EvidenceError, "clang-tidy was not executed successfully"):
            verify(api)

    def test_release_notes_template_is_rejected_without_checksum_cycle(self) -> None:
        template = """
# Candidate release notes

UNPUBLISHED TEMPLATE TODO: add the candidate SHA, CI run, nightly run, artifacts,
platform packages, SBOM, and checksums after all release jobs finish.
"""
        with self.assertRaisesRegex(verifier.EvidenceError, "placeholder"):
            verifier.validate_release_notes_narrative(
                template,
                expected_project_version="0.1.0",
                expected_abi_version=0,
            )

        # A stable narrative is accepted before SHA, run URLs, and package checksums exist.
        narrative = valid_release_notes()
        self.assertNotIn(CANDIDATE_SHA, narrative)
        self.assertNotIn("https://github.com/", narrative)
        verifier.validate_release_notes_narrative(
            narrative,
            expected_project_version="0.1.0",
            expected_abi_version=0,
        )

    def test_release_notes_reject_commit_sha_and_actions_run_url(self) -> None:
        cases = (
            (
                f"\nCandidate commit: {OLD_SHA}\n",
                "must not contain a full commit SHA",
            ),
            (
                "\nCI: https://github.com/example/lua/actions/runs/123456789\n",
                "must not contain a GitHub Actions run URL",
            ),
        )
        for suffix, expected in cases:
            with self.subTest(suffix=suffix):
                with self.assertRaisesRegex(verifier.EvidenceError, expected):
                    verifier.validate_release_notes_narrative(
                        valid_release_notes() + suffix,
                        expected_project_version="0.1.0",
                        expected_abi_version=0,
                    )

    def test_tracked_release_notes_are_stable_and_valid(self) -> None:
        repository_root = Path(__file__).resolve().parents[1]
        narrative = (
            repository_root / "docs/release/rc-notes-0.1.0.md"
        ).read_text(encoding="utf-8-sig")
        verifier.validate_release_notes_narrative(
            narrative,
            expected_project_version="0.1.0",
            expected_abi_version=0,
        )

    def test_release_notes_version_and_abi_must_match_source_readiness(self) -> None:
        cases = (
            (
                valid_release_notes().replace("0.1.0", "9.9.9"),
                "title version mismatch",
            ),
            (
                valid_release_notes().replace(
                    "shared-library ABI is 0",
                    "shared-library ABI is 99",
                ),
                "ABI version mismatch",
            ),
        )
        for narrative, expected in cases:
            with self.subTest(expected=expected):
                with self.assertRaisesRegex(verifier.EvidenceError, expected):
                    verifier.validate_release_notes_narrative(
                        narrative,
                        expected_project_version="0.1.0",
                        expected_abi_version=0,
                    )

    def test_api_failure_is_fail_closed(self) -> None:
        class UnavailableApi:
            def get(self, path: str, params: object = None) -> object:
                raise verifier.EvidenceError(f"API unavailable: {path}")

        with self.assertRaisesRegex(verifier.EvidenceError, "API unavailable"):
            verifier.verify_release_evidence(
                UnavailableApi(),
                REPOSITORY,
                CANDIDATE_SHA,
                valid_release_notes(),
                valid_governance_evidence(),
                valid_source_readiness_evidence(),
                expected_tag=TAG,
                expected_version=VERSION,
                now=NOW,
            )

    def test_rest_client_rejects_cross_origin_archive_url(self) -> None:
        client = verifier.GitHubRestClient("fixture-token")
        with self.assertRaisesRegex(verifier.EvidenceError, "outside the configured GitHub API origin"):
            client.download("https://example.invalid/artifact.zip")

    def test_cli_reads_environment_and_writes_manifest_only_on_success(self) -> None:
        api = FakeGitHub()
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            notes = root / "notes.md"
            output = root / "evidence.json"
            governance_path = root / "governance-evidence.json"
            readiness_path = root / "source-readiness-evidence.json"
            notes.write_text(valid_release_notes(), encoding="utf-8")
            governance_path.write_text(
                json.dumps(valid_governance_evidence()),
                encoding="utf-8",
            )
            readiness_path.write_text(
                json.dumps(valid_source_readiness_evidence()),
                encoding="utf-8",
            )
            with mock.patch.dict(
                os.environ,
                {
                    "GITHUB_TOKEN": "fixture-token",
                    "GITHUB_REPOSITORY": REPOSITORY,
                    "GITHUB_SHA": CANDIDATE_SHA,
                },
                clear=True,
            ):
                result = verifier.main(
                    [
                        "--release-notes",
                        str(notes),
                        "--governance-evidence",
                        str(governance_path),
                        "--expected-tag",
                        TAG,
                        "--expected-version",
                        VERSION,
                        "--source-readiness-evidence",
                        str(readiness_path),
                        "--output",
                        str(output),
                    ],
                    client=api,
                    now=NOW,
                )
            self.assertEqual(0, result)
            self.assertEqual(CANDIDATE_SHA, json.loads(output.read_text())["candidate_sha"])

            api.jobs[CI_RUN_ID][0]["conclusion"] = "cancelled"
            result = verifier.main(
                [
                    "--sha",
                    CANDIDATE_SHA,
                    "--repository",
                    REPOSITORY,
                    "--token",
                    "fixture-token",
                    "--release-notes",
                    str(notes),
                    "--governance-evidence",
                    str(governance_path),
                    "--expected-tag",
                    TAG,
                    "--expected-version",
                    VERSION,
                    "--source-readiness-evidence",
                    str(readiness_path),
                    "--output",
                    str(output),
                ],
                client=api,
                now=NOW,
            )
            self.assertEqual(1, result)
            self.assertFalse(output.exists(), "failed verification left a stale manifest")


if __name__ == "__main__":
    unittest.main()
