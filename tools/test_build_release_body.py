#!/usr/bin/env python3
"""Contract tests for the final release manifest consumer."""

from __future__ import annotations

import copy
import hashlib
import json
import shutil
import tempfile
import unittest
from pathlib import Path

import build_release_body as consumer
import test_validate_release_artifacts as package_fixture
import test_verify_release_evidence as verifier_fixture
import verify_release_evidence as verifier


REPOSITORY = "example/lua"
CANDIDATE_SHA = "a" * 40
VERSION = verifier_fixture.VERSION
TAG = verifier_fixture.TAG
NOW = verifier_fixture.NOW
BASE_SHA = "b" * 40
CREATED = "2026-07-26T01:00:00Z"
UPDATED = "2026-07-26T02:00:00Z"
EXPIRES = "2026-08-26T02:00:00Z"


def make_job(run_id: int, index: int, name: str) -> dict[str, object]:
    job: dict[str, object] = {
        "id": run_id * 100 + index,
        "name": name,
        "url": f"https://github.com/{REPOSITORY}/actions/runs/{run_id}/job/{index}",
        "started_at": "2026-07-26T01:00:00Z",
        "completed_at": UPDATED,
    }
    policy = verifier.TIMED_JOB_STEPS.get(name)
    if policy is not None:
        step_name, seconds = policy
        if name == "Runtime and native-module soak":
            started_at = "2026-07-26T01:05:00Z"
            completed_at = "2026-07-26T01:50:00Z"
        else:
            started_at = "2026-07-26T01:00:00Z"
            completed_at = "2026-07-26T02:00:00Z"
        job["required_timed_steps"] = [
            {
                "name": step_name,
                "started_at": started_at,
                "completed_at": completed_at,
                "duration_seconds": float(seconds),
            }
        ]
    return job


def coverage_payload() -> dict[str, object]:
    return {
        "schema_version": 2,
        "thresholds_passed": True,
        "line_percent": {
            name: threshold + 1.0
            for name, threshold in verifier.EXPECTED_COVERAGE_THRESHOLDS.items()
        },
        "html_index": "html/index.html",
        "raw_coverage": {
            "type": "llvm.coverage.json.export",
            "version": "2.0.1",
            "recomputed": True,
        },
        "threshold_policy": "component-thresholds.json",
        "minimum_scope": copy.deepcopy(verifier.MINIMUM_COVERAGE_SCOPE),
    }


def benchmark_payload() -> dict[str, object]:
    paths: dict[str, object] = {}
    base_objects = ("c" * 40, "d" * 40, "e" * 40)
    head_objects = ("f" * 40, "0" * 40, "1" * 40)
    for index, path in enumerate(verifier.EXPECTED_BENCHMARK_RUNTIME_INPUTS):
        paths[path] = {
            "type": "blob" if path == "CMakeLists.txt" else "tree",
            "base_sha": base_objects[index],
            "head_sha": head_objects[index],
            "equivalent": False,
        }
    metrics = {
        name: {
            "direction": direction,
            "base": 100.0,
            "head": 100.0,
            "regression_ratio": 0.0,
            "maximum_regression_ratio": limit,
            "passed": True,
            "base_sample_count": 9,
            "head_sample_count": 9,
            "paired_run_count": 0 if name == "gc_pause_p99_us" else 3,
        }
        for name, (direction, limit) in verifier.EXPECTED_BENCHMARK_METRICS.items()
    }
    absolute_policy_metrics = {
        name: {"direction": direction, "threshold": threshold}
        for name, (direction, threshold) in (
            verifier.EXPECTED_BENCHMARK_ABSOLUTE_SLOS.items()
        )
    }
    absolute_result = {
        name: {
            "direction": direction,
            "threshold": threshold,
            "actual": threshold,
            "passed": True,
        }
        for name, (direction, threshold) in (
            verifier.EXPECTED_BENCHMARK_ABSOLUTE_SLOS.items()
        )
    }
    return {
        "schema_version": 3,
        "success": True,
        "decision": "thresholds-passed",
        "base_sha": BASE_SHA,
        "head_sha": CANDIDATE_SHA,
        "runs_per_revision": 3,
        "confirmation_triggered": False,
        "runtime_inputs_equivalent": False,
        "metric_regressions_recomputed": True,
        "metrics": metrics,
        "observed_failure_metrics": [],
        "absolute_slo": {
            "policy": {
                **verifier.EXPECTED_BENCHMARK_ABSOLUTE_SCOPE,
                "metrics": absolute_policy_metrics,
            },
            "head_results": [copy.deepcopy(absolute_result) for _ in range(3)],
            "passed": True,
        },
        "authoritative_runtime_inputs": {
            "source": "github-git-root-tree",
            "equivalent": False,
            "base_commit_sha": BASE_SHA,
            "head_commit_sha": CANDIDATE_SHA,
            "base_root_tree_sha": "c" * 40,
            "head_root_tree_sha": "d" * 40,
            "base_ancestry": {
                "status": "ahead",
                "ahead_by": 1,
                "behind_by": 0,
                "base_sha": BASE_SHA,
                "head_sha": CANDIDATE_SHA,
                "merge_base_sha": BASE_SHA,
            },
            "paths": paths,
        },
    }


def runtime_payload() -> dict[str, object]:
    return {
        "schema_version": 1,
        "status": "passed",
        "duration_ms": 45 * 60 * 1000,
        "iterations": 10,
        "native_module_iterations": 1000,
    }


def fuzz_payload() -> dict[str, object]:
    return {
        "targets": {
            target: {"reported_seconds": 600, "executions": 42}
            for target in verifier.EXPECTED_FUZZ_TARGETS
        }
    }


def workflow_parameters(name: str) -> dict[str, object]:
    if name in verifier.CI_ARTIFACTS:
        return {}
    if name == "runtime-soak-evidence":
        return {"soak_minutes": 45, "native_module_iterations": 1000}
    if name == "long-fuzz-evidence":
        return {
            "fuzz_seconds_per_target": 600,
            "fuzz_targets": list(verifier.EXPECTED_FUZZ_TARGETS),
        }
    raise AssertionError(f"unknown artifact: {name}")


def artifact_payload(name: str) -> dict[str, object]:
    if name == "component-coverage":
        return coverage_payload()
    if name == "runtime-benchmark-evidence":
        return benchmark_payload()
    if name == "runtime-soak-evidence":
        return runtime_payload()
    if name == "long-fuzz-evidence":
        return fuzz_payload()
    raise AssertionError(f"unknown artifact: {name}")


def make_artifact(
    *,
    run_id: int,
    attempt: int,
    workflow: str,
    event: str,
    artifact_id: int,
    name: str,
) -> dict[str, object]:
    return {
        "id": artifact_id,
        "name": name,
        "digest": f"sha256:{artifact_id:064x}",
        "size_in_bytes": 4096,
        "archive_download_url": (
            f"https://api.github.com/repos/{REPOSITORY}/actions/artifacts/{artifact_id}/zip"
        ),
        "created_at": "2026-07-26T02:01:00Z",
        "updated_at": "2026-07-26T02:01:01Z",
        "expires_at": EXPIRES,
        "workflow_evidence": {
            "schema": verifier.WORKFLOW_EVIDENCE_SCHEMA,
            "kind": name,
            "repository": REPOSITORY,
            "candidate_sha": CANDIDATE_SHA,
            "run_id": run_id,
            "run_attempt": attempt,
            "event": event,
            "workflow_ref": (
                f"{REPOSITORY}/.github/workflows/{workflow}@refs/heads/main"
            ),
            "job": verifier.EXPECTED_ARTIFACT_JOBS[name],
            "result": "passed",
            "created_at": "2026-07-26T01:59:00Z",
            "parameters": workflow_parameters(name),
        },
        "payload_evidence": artifact_payload(name),
    }


def make_run(
    *,
    run_id: int,
    workflow: str,
    event: str,
    job_names: tuple[str, ...],
    artifact_names: tuple[str, ...],
) -> dict[str, object]:
    attempt = 2
    return {
        "workflow": workflow,
        "event": event,
        "id": run_id,
        "attempt": attempt,
        "url": f"https://github.com/{REPOSITORY}/actions/runs/{run_id}",
        "head_sha": CANDIDATE_SHA,
        "head_branch": "main",
        "status": "completed",
        "conclusion": "success",
        "created_at": CREATED,
        "updated_at": UPDATED,
        "jobs": [
            make_job(run_id, index, name)
            for index, name in enumerate(job_names, start=1)
        ],
        "artifacts": [
            make_artifact(
                run_id=run_id,
                attempt=attempt,
                workflow=workflow,
                event=event,
                artifact_id=run_id * 10 + index,
                name=name,
            )
            for index, name in enumerate(artifact_names, start=1)
        ],
    }


def valid_manifest() -> dict[str, object]:
    return {
        "schema": verifier.MANIFEST_SCHEMA,
        "generated_at": "2026-07-26T03:00:00Z",
        "repository": REPOSITORY,
        "candidate_sha": CANDIDATE_SHA,
        "version": VERSION,
        "abi_version": 0,
        "governance": verifier_fixture.valid_governance_evidence(),
        "source_readiness": verifier_fixture.valid_source_readiness_evidence(),
        "main_history": {
            "branch": "main",
            "compare_status": "ahead",
            "commits_after_candidate": 2,
        },
        "tool": {
            "name": "tools/verify_release_evidence.py",
            "version": verifier.TOOL_VERSION,
        },
        "runs": {
            "ci_push": make_run(
                run_id=1001,
                workflow="ci.yml",
                event="push",
                job_names=verifier.EXPECTED_CI_JOBS,
                artifact_names=verifier.CI_ARTIFACTS,
            ),
            "nightly_workflow_dispatch": make_run(
                run_id=2001,
                workflow="nightly.yml",
                event="workflow_dispatch",
                job_names=verifier.EXPECTED_NIGHTLY_JOBS,
                artifact_names=verifier.NIGHTLY_ARTIFACTS,
            ),
            "nightly_schedule": make_run(
                run_id=2002,
                workflow="nightly.yml",
                event="schedule",
                job_names=verifier.EXPECTED_NIGHTLY_JOBS,
                artifact_names=verifier.NIGHTLY_ARTIFACTS,
            ),
        },
        "release_notes": {
            "narrative_placeholders_checked": True,
            "package_checksums_checked": False,
            "package_checksums_policy": "deferred until packages exist",
        },
    }


def tracked_narrative() -> str:
    return (
        Path(__file__).resolve().parents[1] / "docs/release/rc-notes-0.1.0.md"
    ).read_text(encoding="utf-8-sig")


def write_release_assets(root: Path, manifest: dict[str, object]) -> tuple[Path, Path]:
    manifest_path = root / "release-evidence.json"
    manifest_path.write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    for rid in ("windows-x64", "linux-x64", "macos-arm64"):
        with tempfile.TemporaryDirectory() as temporary:
            assets = package_fixture.create_release_asset_set(
                Path(temporary),
                version=VERSION,
                rid=rid,
                commit=CANDIDATE_SHA,
            )
            for key in ("archive", "sbom", "manifest", "checksums"):
                shutil.copy2(assets[key], root / assets[key].name)
    checksum_path = rewrite_global_checksums(root)
    return manifest_path, checksum_path


def rewrite_global_checksums(root: Path) -> Path:
    asset_paths = sorted(
        path for path in root.iterdir() if path.is_file() and path.name != "SHA256SUMS"
    )
    checksum_path = root / "SHA256SUMS"
    checksum_path.write_text(
        "".join(
            f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.name}\n"
            for path in asset_paths
        ),
        encoding="ascii",
    )
    return checksum_path


class ReleaseBodyConsumerTests(unittest.TestCase):
    def validate(self, manifest: dict[str, object]) -> None:
        consumer.validate_release_manifest(
            manifest,
            expected_repository=REPOSITORY,
            expected_sha=CANDIDATE_SHA,
            expected_tag=TAG,
            expected_version=VERSION,
            now=NOW,
        )

    def test_green_manifest_and_body_include_substantive_evidence(self) -> None:
        manifest = valid_manifest()
        self.validate(manifest)
        body = consumer.build_release_body(
            manifest,
            expected_repository=REPOSITORY,
            expected_sha=CANDIDATE_SHA,
            expected_tag=TAG,
            expected_version=VERSION,
            narrative=tracked_narrative(),
            checksum_text=f"{'1' * 64}  fixture.zip",
            now=NOW,
        )
        self.assertIn(CANDIDATE_SHA, body)
        self.assertIn("independently recomputed coverage", body)
        self.assertIn("runtime inputs bound to authoritative Git trees", body)
        self.assertIn("machine target results", body)
        self.assertIn("Governance: `protected-ruleset`", body)
        self.assertIn("independently reviewed", body)

    def test_helper_to_verifier_to_body_chain_is_deeply_validated(self) -> None:
        manifest = verifier_fixture.verify(verifier_fixture.FakeGitHub())
        consumer.validate_release_manifest(
            manifest,
            expected_repository=verifier_fixture.REPOSITORY,
            expected_sha=verifier_fixture.CANDIDATE_SHA,
            expected_tag=verifier_fixture.TAG,
            expected_version=verifier_fixture.VERSION,
            now=verifier_fixture.NOW,
        )
        benchmark = next(
            artifact
            for artifact in manifest["runs"]["ci_push"]["artifacts"]
            if artifact["name"] == "runtime-benchmark-evidence"
        )["payload_evidence"]
        self.assertEqual(
            set(verifier.EXPECTED_BENCHMARK_METRICS),
            set(benchmark["metrics"]),
        )
        self.assertEqual([], benchmark["observed_failure_metrics"])
        self.assertTrue(benchmark["absolute_slo"]["passed"])
        self.assertEqual(
            verifier_fixture.BASE_SHA,
            benchmark["authoritative_runtime_inputs"]["base_ancestry"][
                "merge_base_sha"
            ],
        )
        self.assertEqual(
            "release-owner",
            manifest["governance"]["attestation"]["approved_by"],
        )

        mutations = (
            (
                lambda payload: payload["metrics"].pop(
                    next(iter(verifier.EXPECTED_BENCHMARK_METRICS))
                ),
                "metric set mismatch",
            ),
            (
                lambda payload: payload["observed_failure_metrics"].append(
                    next(iter(verifier.EXPECTED_BENCHMARK_METRICS))
                ),
                "observed failure metric set mismatch",
            ),
            (
                lambda payload: payload["absolute_slo"].__setitem__("passed", False),
                "absolute SLO did not pass",
            ),
            (
                lambda payload: payload["authoritative_runtime_inputs"][
                    "base_ancestry"
                ].__setitem__("merge_base_sha", "f" * 40),
                "base ancestry mismatch",
            ),
        )
        for mutate, expected in mutations:
            with self.subTest(expected=expected):
                mutated = copy.deepcopy(manifest)
                payload = next(
                    artifact
                    for artifact in mutated["runs"]["ci_push"]["artifacts"]
                    if artifact["name"] == "runtime-benchmark-evidence"
                )["payload_evidence"]
                mutate(payload)
                with self.assertRaisesRegex(consumer.ReleaseBodyError, expected):
                    consumer.validate_release_manifest(
                        mutated,
                        expected_repository=verifier_fixture.REPOSITORY,
                        expected_sha=verifier_fixture.CANDIDATE_SHA,
                        expected_tag=verifier_fixture.TAG,
                        expected_version=verifier_fixture.VERSION,
                        now=verifier_fixture.NOW,
                    )

    def test_candidate_only_or_mutated_governance_is_rejected_for_body(self) -> None:
        cases = (
            (
                lambda manifest: manifest.__setitem__(
                    "governance",
                    verifier_fixture.valid_governance_evidence(approved=False),
                ),
                "requires approved governance evidence",
            ),
            (
                lambda manifest: manifest["governance"]["attestation"].__setitem__(
                    "independent_reviewer",
                    manifest["governance"]["attestation"]["approved_by"],
                ),
                "must be different",
            ),
            (
                lambda manifest: manifest["governance"]["attestation"].__setitem__(
                    "expires_at",
                    "2026-07-26T12:00:00Z",
                ),
                "is expired",
            ),
        )
        for mutate, expected in cases:
            with self.subTest(expected=expected):
                manifest = valid_manifest()
                mutate(manifest)
                with self.assertRaisesRegex(consumer.ReleaseBodyError, expected):
                    self.validate(manifest)

    def test_missing_payload_evidence_is_rejected(self) -> None:
        manifest = valid_manifest()
        del manifest["runs"]["ci_push"]["artifacts"][0]["payload_evidence"]
        with self.assertRaisesRegex(consumer.ReleaseBodyError, "payload_evidence"):
            self.validate(manifest)

    def test_artifact_must_still_be_live_when_release_body_is_consumed(self) -> None:
        manifest = valid_manifest()
        manifest["runs"]["ci_push"]["artifacts"][0][
            "expires_at"
        ] = "2026-07-26T12:00:00Z"
        with self.assertRaisesRegex(
            consumer.ReleaseBodyError,
            "expired at release publication",
        ):
            self.validate(manifest)

    def test_missing_authoritative_timed_step_is_rejected(self) -> None:
        manifest = valid_manifest()
        nightly = manifest["runs"]["nightly_workflow_dispatch"]
        timed_job = next(
            job
            for job in nightly["jobs"]
            if job["name"] == "Runtime and native-module soak"
        )
        del timed_job["required_timed_steps"]
        with self.assertRaisesRegex(consumer.ReleaseBodyError, "required_timed_steps"):
            self.validate(manifest)

    def test_top_level_identity_and_ancestry_mismatches_are_rejected(self) -> None:
        cases = (
            (
                lambda manifest: manifest.__setitem__("repository", "other/repository"),
                "repository mismatch",
            ),
            (
                lambda manifest: manifest["tool"].__setitem__("version", "99.0.0"),
                "verifier identity mismatch",
            ),
            (
                lambda manifest: manifest["main_history"].__setitem__(
                    "compare_status", "diverged"
                ),
                "not proven to be in main history",
            ),
            (
                lambda manifest: manifest.__setitem__("candidate_sha", "f" * 40),
                "manifest SHA mismatch",
            ),
            (
                lambda manifest: manifest.__setitem__("version", "0.1.0"),
                "manifest version mismatch",
            ),
            (
                lambda manifest: manifest.__setitem__("abi_version", 1),
                "ABI version does not match source readiness",
            ),
        )
        for mutate, expected in cases:
            with self.subTest(expected=expected):
                manifest = valid_manifest()
                mutate(manifest)
                with self.assertRaisesRegex(consumer.ReleaseBodyError, expected):
                    self.validate(manifest)

    def test_run_sha_and_artifact_set_mismatches_are_rejected(self) -> None:
        manifest = valid_manifest()
        manifest["runs"]["ci_push"]["head_sha"] = "f" * 40
        with self.assertRaisesRegex(consumer.ReleaseBodyError, "run ci_push binding mismatch"):
            self.validate(manifest)

        manifest = valid_manifest()
        manifest["runs"]["nightly_schedule"]["artifacts"].pop()
        with self.assertRaisesRegex(consumer.ReleaseBodyError, "artifact set mismatch"):
            self.validate(manifest)

    def test_checksum_index_is_bound_to_all_assets(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            manifest_path, checksums = write_release_assets(root, valid_manifest())
            text = consumer.validate_release_checksums(
                checksums,
                manifest_path=manifest_path,
                expected_version=VERSION,
                expected_sha=CANDIDATE_SHA,
            )
            self.assertIn("release-evidence.json", text)
            (root / "lua-cpp-0.1.0-rc.1-linux-x64.zip").write_bytes(b"tampered")
            with self.assertRaisesRegex(consumer.ReleaseBodyError, "checksum mismatch"):
                consumer.validate_release_checksums(
                    checksums,
                    manifest_path=manifest_path,
                    expected_version=VERSION,
                    expected_sha=CANDIDATE_SHA,
                )

    def test_package_semantics_reject_rehashed_tampering(self) -> None:
        def mutate_manifest(root: Path, rid: str, field: str, value: object) -> None:
            path = root / f"lua-cpp-{VERSION}-{rid}.manifest.json"
            payload = json.loads(path.read_text(encoding="utf-8"))
            payload[field] = value
            path.write_text(json.dumps(payload), encoding="utf-8")

        def rewrite_package_checksums(root: Path, rid: str) -> None:
            stem = f"lua-cpp-{VERSION}-{rid}"
            archive = root / f"{stem}.zip"
            sbom = root / f"{stem}.spdx.json"
            (root / f"{stem}.SHA256SUMS").write_text(
                f"{hashlib.sha256(archive.read_bytes()).hexdigest()}  {archive.name}\n"
                f"{hashlib.sha256(sbom.read_bytes()).hexdigest()}  {sbom.name}\n",
                encoding="ascii",
            )

        with tempfile.TemporaryDirectory() as temporary:
            seed = Path(temporary) / "seed"
            seed.mkdir()
            write_release_assets(seed, valid_manifest())

            def wrong_version(root: Path) -> None:
                mutate_manifest(root, "windows-x64", "version", "9.9.9")

            def wrong_commit(root: Path) -> None:
                mutate_manifest(root, "linux-x64", "commit", "f" * 40)

            def duplicate_rid(root: Path) -> None:
                mutate_manifest(
                    root,
                    "linux-x64",
                    "runtimeIdentifier",
                    "windows-x64",
                )

            def wrong_filename(root: Path) -> None:
                source = root / f"lua-cpp-{VERSION}-windows-x64.zip"
                source.rename(root / "wrong-windows-x64.zip")

            def fake_checksum(root: Path) -> None:
                path = root / f"lua-cpp-{VERSION}-linux-x64.SHA256SUMS"
                path.write_text(
                    f"{'0' * 64}  lua-cpp-{VERSION}-linux-x64.zip\n",
                    encoding="ascii",
                )

            def non_zip(root: Path) -> None:
                (root / f"lua-cpp-{VERSION}-macos-arm64.zip").write_bytes(
                    b"not a zip"
                )
                rewrite_package_checksums(root, "macos-arm64")

            def mismatched_sbom(root: Path) -> None:
                path = root / f"lua-cpp-{VERSION}-windows-x64.spdx.json"
                path.write_text("{}\n", encoding="utf-8")
                rewrite_package_checksums(root, "windows-x64")

            def missing_rid(root: Path) -> None:
                for suffix in (".zip", ".spdx.json", ".manifest.json", ".SHA256SUMS"):
                    (root / f"lua-cpp-{VERSION}-macos-arm64{suffix}").unlink()

            def extra_file(root: Path) -> None:
                (root / "lua-cpp-extra.zip").write_bytes(b"extra")

            cases = {
                "wrong version": wrong_version,
                "wrong commit": wrong_commit,
                "duplicate RID": duplicate_rid,
                "wrong filename": wrong_filename,
                "fake per-package checksum": fake_checksum,
                "non-ZIP archive": non_zip,
                "internal/external SBOM mismatch": mismatched_sbom,
                "missing RID": missing_rid,
                "extra file": extra_file,
            }
            for index, (label, mutate) in enumerate(cases.items()):
                with self.subTest(label=label):
                    root = Path(temporary) / f"case-{index}"
                    shutil.copytree(seed, root)
                    mutate(root)
                    checksums = rewrite_global_checksums(root)
                    with self.assertRaises(consumer.ReleaseBodyError):
                        consumer.validate_release_checksums(
                            checksums,
                            manifest_path=root / "release-evidence.json",
                            expected_version=VERSION,
                            expected_sha=CANDIDATE_SHA,
                        )

    def test_cli_writes_body_only_after_complete_validation(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            release = root / "release"
            release.mkdir()
            manifest_path, checksums = write_release_assets(release, valid_manifest())
            notes = Path(__file__).resolve().parents[1] / "docs/release/rc-notes-0.1.0.md"
            output = root / "release-body.md"
            result = consumer.main(
                [
                    "--manifest",
                    str(manifest_path),
                    "--release-notes",
                    str(notes),
                    "--checksums",
                    str(checksums),
                    "--output",
                    str(output),
                    "--expected-repository",
                    REPOSITORY,
                    "--expected-sha",
                    CANDIDATE_SHA,
                    "--expected-tag",
                    TAG,
                    "--expected-version",
                    VERSION,
                ],
                now=NOW,
            )
            self.assertEqual(0, result)
            self.assertIn("Published asset SHA-256", output.read_text(encoding="utf-8"))

            output.write_text("stale body\n", encoding="utf-8")
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            del manifest["runs"]["nightly_schedule"]["artifacts"][0]["payload_evidence"]
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            checksums.write_text(
                checksums.read_text(encoding="ascii").replace(
                    next(
                        line
                        for line in checksums.read_text(encoding="ascii").splitlines()
                        if line.endswith("  release-evidence.json")
                    ).split("  ", 1)[0],
                    hashlib.sha256(manifest_path.read_bytes()).hexdigest(),
                ),
                encoding="ascii",
            )
            result = consumer.main(
                [
                    "--manifest",
                    str(manifest_path),
                    "--release-notes",
                    str(notes),
                    "--checksums",
                    str(checksums),
                    "--output",
                    str(output),
                    "--expected-repository",
                    REPOSITORY,
                    "--expected-sha",
                    CANDIDATE_SHA,
                    "--expected-tag",
                    TAG,
                    "--expected-version",
                    VERSION,
                ],
                now=NOW,
            )
            self.assertEqual(1, result)
            self.assertFalse(output.exists(), "failed consumer left a stale release body")


if __name__ == "__main__":
    unittest.main()
