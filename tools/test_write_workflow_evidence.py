#!/usr/bin/env python3
"""Contract tests for workflow artifact evidence metadata."""

from __future__ import annotations

import json
import io
import re
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from datetime import datetime, timezone
from pathlib import Path
from unittest import mock

import write_workflow_evidence as evidence
import verify_release_evidence as release_evidence


NOW = datetime(2026, 7, 26, 8, 9, 10, tzinfo=timezone.utc)
CANDIDATE_SHA = "ABCDEF0123456789ABCDEF0123456789ABCDEF01"


def github_environment(kind: str = "runtime-soak-evidence") -> dict[str, str]:
    workflow, job = evidence.KIND_CONTEXT[kind]
    return {
        "GITHUB_REPOSITORY": "example/lua",
        "GITHUB_SHA": CANDIDATE_SHA,
        "GITHUB_RUN_ID": "123456",
        "GITHUB_RUN_ATTEMPT": "2",
        "GITHUB_EVENT_NAME": "workflow_dispatch",
        "GITHUB_WORKFLOW_REF": f"example/lua/.github/workflows/{workflow}@refs/heads/main",
        "GITHUB_JOB": job,
    }


class WorkflowEvidenceTests(unittest.TestCase):
    def test_debugger_build_modes_match_ci_and_release_boundaries(self) -> None:
        repository = Path(__file__).resolve().parents[1]
        ci = (repository / ".github/workflows/ci.yml").read_text(encoding="utf-8")
        nightly = (repository / ".github/workflows/nightly.yml").read_text(encoding="utf-8")
        release = (repository / ".github/workflows/release.yml").read_text(encoding="utf-8")

        ci_fuzz = ci[ci.index("  linux-fuzzers:") : ci.index("  portability:")]
        ci_allocator = ci[ci.index("  allocator-failure-contract:") : ci.index("  linux-fuzzers:")]
        ci_coverage = ci[ci.index("  linux-coverage:") : ci.index("  linux-runtime-benchmark:")]
        ci_benchmark = ci[ci.index("  linux-runtime-benchmark:") :]
        nightly_fuzz = nightly[nightly.index("  long-fuzz:") :]

        self.assertIn("-DLUA_CPP_BUILD_DEBUGGER=ON", ci_fuzz)
        self.assertIn("-DLUA_CPP_BUILD_DEBUGGER=ON", ci_coverage)
        self.assertIn("-DLUA_CPP_BUILD_DEBUGGER=ON", nightly_fuzz)
        self.assertEqual(ci_benchmark.count("-DLUA_CPP_BUILD_DEBUGGER=OFF"), 2)
        self.assertEqual(release.count("-DLUA_CPP_BUILD_DEBUGGER=OFF"), 2)
        self.assertIn("cmake -S . -B build/allocator -A x64", ci_allocator)
        self.assertIn('build/allocator/Debug/lua_test.exe', ci_allocator)

    def test_fuzz_target_policy_matches_workflows_cmake_and_release_verifier(self) -> None:
        repository = Path(__file__).resolve().parents[1]
        expected = evidence.LONG_FUZZ_TARGETS
        self.assertEqual(expected, release_evidence.EXPECTED_FUZZ_TARGETS)

        cmake = (repository / "CMakeLists.txt").read_text(encoding="utf-8")
        cmake_base = re.search(r"set\(LUA_FUZZ_TARGETS ([^)]+)\)", cmake)
        cmake_debugger = re.search(r"list\(APPEND LUA_FUZZ_TARGETS ([^)]+)\)", cmake)
        self.assertIsNotNone(cmake_base)
        self.assertIsNotNone(cmake_debugger)
        cmake_targets = tuple(cmake_base.group(1).split()) + tuple(cmake_debugger.group(1).split())
        self.assertEqual(cmake_targets, expected)

        for relative_path in (".github/workflows/ci.yml", ".github/workflows/nightly.yml"):
            workflow = (repository / relative_path).read_text(encoding="utf-8")
            shell_match = re.search(r"targets=\(([^)]+)\)", workflow)
            self.assertIsNotNone(shell_match, relative_path)
            self.assertEqual(tuple(shell_match.group(1).split()), expected, relative_path)
            for target in expected:
                self.assertIn(f"fuzz_{target}", workflow, relative_path)

        nightly = (repository / ".github/workflows/nightly.yml").read_text(encoding="utf-8")
        metadata_match = re.search(r"--parameter 'fuzz_targets=(\[[^']+\])'", nightly)
        self.assertIsNotNone(metadata_match)
        self.assertEqual(tuple(json.loads(metadata_match.group(1))), expected)

        checklist = (repository / "docs/release/release-checklist.md").read_text(encoding="utf-8")
        for target in expected:
            self.assertIn(f"`{target}`", checklist)

    def test_nightly_long_fuzz_timeout_covers_the_maximum_campaign(self) -> None:
        repository = Path(__file__).resolve().parents[1]
        nightly = (repository / ".github/workflows/nightly.yml").read_text(encoding="utf-8")
        long_fuzz = nightly[nightly.index("  long-fuzz:") :]

        timeout_match = re.search(r"^    timeout-minutes: (\d+)$", long_fuzz, re.MULTILINE)
        maximum_match = re.search(
            r"FUZZ_SECONDS_PER_TARGET <= (\d+)", long_fuzz
        )
        targets_match = re.search(r"targets=\(([^)]+)\)", long_fuzz)
        input_block = nightly[
            nightly.index("      fuzz_seconds_per_target:") :
            nightly.index("      native_module_iterations:")
        ]
        default_match = re.search(r'^        default: "(\d+)"$', input_block, re.MULTILINE)

        self.assertIsNotNone(timeout_match)
        self.assertIsNotNone(maximum_match)
        self.assertIsNotNone(targets_match)
        self.assertIsNotNone(default_match)

        timeout_seconds = int(timeout_match.group(1)) * 60
        maximum_seconds_per_target = int(maximum_match.group(1))
        targets = tuple(targets_match.group(1).split())
        setup_and_upload_budget_seconds = 30 * 60

        self.assertEqual(int(default_match.group(1)), 600)
        self.assertEqual(maximum_seconds_per_target, 1200)
        self.assertEqual(targets, evidence.LONG_FUZZ_TARGETS)
        self.assertGreater(
            timeout_seconds,
            len(targets) * maximum_seconds_per_target + setup_and_upload_budget_seconds,
        )
        self.assertEqual(
            release_evidence.TIMED_JOB_STEPS["Long sanitizer fuzz"][1],
            len(targets) * 600,
        )

    def test_builds_exact_sha_payload(self) -> None:
        payload = evidence.build_evidence(
            "runtime-soak-evidence",
            {
                "soak_minutes": 45,
                "native_module_iterations": 1000,
            },
            github_environment(),
            now=NOW,
        )

        self.assertEqual(
            payload,
            {
                "schema": "lua-cpp.workflow-evidence/v1",
                "kind": "runtime-soak-evidence",
                "repository": "example/lua",
                "candidate_sha": CANDIDATE_SHA.lower(),
                "run_id": 123456,
                "run_attempt": 2,
                "event": "workflow_dispatch",
                "workflow_ref": "example/lua/.github/workflows/nightly.yml@refs/heads/main",
                "job": "runtime-soak",
                "result": "passed",
                "created_at": "2026-07-26T08:09:10Z",
                "parameters": {
                    "soak_minutes": 45,
                    "native_module_iterations": 1000,
                },
            },
        )

    def test_parses_json_parameters_and_rejects_duplicate_keys(self) -> None:
        parameters = evidence.parse_parameters(
            [
                "fuzz_seconds_per_target=600",
                'fuzz_targets=["undump","bytecode_verifier","parser","stdlib_numeric_arguments",'
                '"remote_protocol","debugger_expression"]',
            ]
        )
        self.assertEqual(parameters["fuzz_seconds_per_target"], 600)
        self.assertEqual(parameters["fuzz_targets"], list(evidence.LONG_FUZZ_TARGETS))

        with self.assertRaisesRegex(evidence.EvidenceError, "duplicate parameter key"):
            evidence.parse_parameters(["duration=10", "duration=20"])

    def test_rejects_invalid_parameter_syntax_and_json(self) -> None:
        invalid_items = (
            ["missing-separator"],
            ["Uppercase=1"],
            ["empty="],
            ["value=not-json"],
            ['value={"key":1,"key":2}'],
        )
        for items in invalid_items:
            with self.subTest(items=items), self.assertRaises(evidence.EvidenceError):
                evidence.parse_parameters(items)

    def test_rejects_every_missing_github_field(self) -> None:
        parameters = {
            "soak_minutes": 45,
            "native_module_iterations": 1000,
        }
        for field in github_environment():
            environment = github_environment()
            del environment[field]
            with self.subTest(field=field), self.assertRaisesRegex(
                evidence.EvidenceError,
                field,
            ):
                evidence.build_evidence(
                    "runtime-soak-evidence",
                    parameters,
                    environment,
                    now=NOW,
                )

    def test_rejects_invalid_sha_and_run_identity(self) -> None:
        cases = {
            "GITHUB_SHA": ("f" * 39, "not-hex-" + "0" * 32),
            "GITHUB_RUN_ID": ("0", "-1", "1.5"),
            "GITHUB_RUN_ATTEMPT": ("0", "attempt-two"),
            "GITHUB_EVENT_NAME": ("workflow dispatch", " push"),
            "GITHUB_JOB": ("runtime soak", ""),
        }
        parameters = {
            "soak_minutes": 45,
            "native_module_iterations": 1000,
        }
        for field, invalid_values in cases.items():
            for invalid_value in invalid_values:
                environment = github_environment()
                environment[field] = invalid_value
                with self.subTest(field=field, value=invalid_value), self.assertRaises(
                    evidence.EvidenceError
                ):
                    evidence.build_evidence(
                        "runtime-soak-evidence",
                        parameters,
                        environment,
                        now=NOW,
                    )

    def test_rejects_foreign_or_malformed_workflow_ref(self) -> None:
        parameters = {
            "soak_minutes": 45,
            "native_module_iterations": 1000,
        }
        for workflow_ref in (
            "other/lua/.github/workflows/nightly.yml@refs/heads/main",
            "example/lua/nightly.yml@refs/heads/main",
            "example/lua/.github/workflows/nightly.yml",
            "example/lua/.github/workflows/nightly.yml@refs/heads/main with-space",
        ):
            environment = github_environment()
            environment["GITHUB_WORKFLOW_REF"] = workflow_ref
            with self.subTest(workflow_ref=workflow_ref), self.assertRaises(
                evidence.EvidenceError
            ):
                evidence.build_evidence(
                    "runtime-soak-evidence",
                    parameters,
                    environment,
                    now=NOW,
                )

    def test_runtime_soak_requires_positive_integer_parameters(self) -> None:
        valid = {
            "soak_minutes": 45,
            "native_module_iterations": 1000,
        }
        invalid_parameters = (
            {"native_module_iterations": 1000},
            {"soak_minutes": 45},
            {"soak_minutes": "45", "native_module_iterations": 1000},
            {"soak_minutes": 45, "native_module_iterations": 0},
            {"soak_minutes": True, "native_module_iterations": 1000},
            {
                "soak_minutes": 45,
                "native_module_iterations": 1000,
                "extra": 1,
            },
        )
        for parameters in invalid_parameters:
            with self.subTest(parameters=parameters), self.assertRaises(
                evidence.EvidenceError
            ):
                evidence.build_evidence(
                    "runtime-soak-evidence",
                    parameters,
                    github_environment(),
                    now=NOW,
                )

        payload = evidence.build_evidence(
            "runtime-soak-evidence",
            valid,
            github_environment(),
            now=NOW,
        )
        self.assertEqual(payload["parameters"], valid)

    def test_long_fuzz_requires_duration_and_exact_targets(self) -> None:
        valid = {
            "fuzz_seconds_per_target": 600,
            "fuzz_targets": list(evidence.LONG_FUZZ_TARGETS),
        }
        invalid_parameters = (
            {"fuzz_targets": list(evidence.LONG_FUZZ_TARGETS)},
            {
                "fuzz_seconds_per_target": 0,
                "fuzz_targets": list(evidence.LONG_FUZZ_TARGETS),
            },
            {
                "fuzz_seconds_per_target": 600,
                "fuzz_targets": list(evidence.LONG_FUZZ_TARGETS[:3]),
            },
            {
                "fuzz_seconds_per_target": 600,
                "fuzz_targets": ["undump"] * len(evidence.LONG_FUZZ_TARGETS),
            },
            {
                "fuzz_seconds_per_target": 600,
                "fuzz_targets": list(evidence.LONG_FUZZ_TARGETS),
                "extra": 1,
            },
        )
        for parameters in invalid_parameters:
            with self.subTest(parameters=parameters), self.assertRaises(
                evidence.EvidenceError
            ):
                evidence.build_evidence(
                    "long-fuzz-evidence",
                    parameters,
                    github_environment("long-fuzz-evidence"),
                    now=NOW,
                )

        payload = evidence.build_evidence(
            "long-fuzz-evidence",
            valid,
            github_environment("long-fuzz-evidence"),
            now=NOW,
        )
        self.assertEqual(payload["parameters"], valid)

    def test_rejects_unsupported_kind_and_non_finite_json(self) -> None:
        with self.assertRaisesRegex(evidence.EvidenceError, "unsupported"):
            evidence.build_evidence(
                "unknown-kind",
                {},
                github_environment(),
                now=NOW,
            )
        with self.assertRaisesRegex(evidence.EvidenceError, "finite JSON"):
            evidence.build_evidence(
                "component-coverage",
                {"ratio": float("nan")},
                github_environment("component-coverage"),
                now=NOW,
            )

    def test_rejects_kind_from_wrong_workflow_or_job(self) -> None:
        environment = github_environment("component-coverage")
        environment["GITHUB_JOB"] = "linux-runtime-benchmark"
        with self.assertRaisesRegex(evidence.EvidenceError, "job linux-coverage"):
            evidence.build_evidence(
                "component-coverage",
                {},
                environment,
                now=NOW,
            )

        environment = github_environment("component-coverage")
        environment["GITHUB_WORKFLOW_REF"] = (
            "example/lua/.github/workflows/nightly.yml@refs/heads/main"
        )
        with self.assertRaisesRegex(evidence.EvidenceError, "ci.yml"):
            evidence.build_evidence(
                "component-coverage",
                {},
                environment,
                now=NOW,
            )

    def test_ci_artifacts_reject_nonempty_parameters(self) -> None:
        for kind in ("component-coverage", "runtime-benchmark-evidence"):
            with self.subTest(kind=kind), self.assertRaisesRegex(
                evidence.EvidenceError,
                "parameters must be empty",
            ):
                evidence.build_evidence(
                    kind,
                    {"unexpected": 1},
                    github_environment(kind),
                    now=NOW,
                )

    def test_atomic_write_uses_required_basename_and_replaces_payload(self) -> None:
        payload = evidence.build_evidence(
            "component-coverage",
            {},
            github_environment("component-coverage"),
            now=NOW,
        )
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            output = root / evidence.EVIDENCE_BASENAME
            output.write_text("stale", encoding="utf-8")

            evidence.atomic_write_json(output, payload)

            self.assertEqual(json.loads(output.read_text(encoding="utf-8")), payload)
            self.assertEqual(
                list(root.glob(f".{evidence.EVIDENCE_BASENAME}.*.tmp")),
                [],
            )
            with self.assertRaisesRegex(evidence.EvidenceError, "basename"):
                evidence.atomic_write_json(root / "wrong-name.json", payload)

    def test_atomic_write_preserves_existing_file_when_replace_fails(self) -> None:
        payload = evidence.build_evidence(
            "runtime-benchmark-evidence",
            {},
            github_environment("runtime-benchmark-evidence"),
            now=NOW,
        )
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            output = root / evidence.EVIDENCE_BASENAME
            output.write_text("original", encoding="utf-8")

            with mock.patch.object(evidence.os, "replace", side_effect=OSError("replace failed")):
                with self.assertRaisesRegex(OSError, "replace failed"):
                    evidence.atomic_write_json(output, payload)

            self.assertEqual(output.read_text(encoding="utf-8"), "original")
            self.assertEqual(
                list(root.glob(f".{evidence.EVIDENCE_BASENAME}.*.tmp")),
                [],
            )

    def test_main_does_not_replace_output_after_validation_failure(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            output = Path(temporary_directory) / evidence.EVIDENCE_BASENAME
            output.write_text("original", encoding="utf-8")
            environment = github_environment()
            environment["GITHUB_SHA"] = "bad"

            stderr = io.StringIO()
            with redirect_stderr(stderr):
                exit_code = evidence.main(
                    [
                        "--kind",
                        "component-coverage",
                        "--output",
                        str(output),
                    ],
                    environment=environment,
                )

            self.assertEqual(exit_code, 2)
            self.assertIn("GITHUB_SHA", stderr.getvalue())
            self.assertEqual(output.read_text(encoding="utf-8"), "original")

    def test_main_writes_typed_long_fuzz_payload(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            output = Path(temporary_directory) / evidence.EVIDENCE_BASENAME
            stdout = io.StringIO()
            with redirect_stdout(stdout):
                exit_code = evidence.main(
                    [
                        "--kind",
                        "long-fuzz-evidence",
                        "--output",
                        str(output),
                        "--parameter",
                        "fuzz_seconds_per_target=600",
                        "--parameter",
                        'fuzz_targets=["undump","bytecode_verifier","parser",'
                        '"stdlib_numeric_arguments","remote_protocol","debugger_expression"]',
                    ],
                    environment=github_environment("long-fuzz-evidence"),
                )

            self.assertEqual(exit_code, 0)
            payload = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(payload["kind"], "long-fuzz-evidence")
            self.assertEqual(payload["parameters"]["fuzz_seconds_per_target"], 600)
            self.assertEqual(
                payload["parameters"]["fuzz_targets"],
                list(evidence.LONG_FUZZ_TARGETS),
            )


if __name__ == "__main__":
    unittest.main()
