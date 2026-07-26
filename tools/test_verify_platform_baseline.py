#!/usr/bin/env python3
"""Negative and positive contracts for verify_platform_baseline.py."""

from __future__ import annotations

import copy
import json
import subprocess
import tempfile
import unittest
from pathlib import Path

import verify_platform_baseline as verifier


ROOT = Path(__file__).resolve().parents[1]
POLICY_PATH = ROOT / "docs" / "release" / "platform-baseline.json"


def load_policy() -> dict:
    return verifier.load_json(POLICY_PATH)


def evidence_for(policy: dict, rid: str) -> dict:
    baseline = policy["releaseRids"][rid]
    builder = baseline["builder"]
    runtime = baseline["minimumRuntime"]
    host_version = builder.get("hostVersionMinimum", "6.8.0")
    return {
        "schema": verifier.EVIDENCE_SCHEMA,
        "rid": rid,
        "runner": builder["runner"],
        "host": {
            "system": builder["hostSystem"],
            "version": host_version,
            "distributionId": builder.get("distributionId", ""),
            "distributionVersion": builder.get("distributionVersion", ""),
        },
        "target": {
            "system": builder["hostSystem"],
            "processor": baseline["architecture"],
            "pointerSize": baseline["pointerSize"],
        },
        "toolchain": {
            "generator": builder["generator"],
            "compilerId": builder["compilerId"],
            "compilerVersion": builder["compilerVersionMinimum"],
            "cmakeVersion": policy["language"]["cmakeMinimum"],
        },
        "runtime": {
            "msvcRuntime": builder.get("msvcRuntime", ""),
            "libc": runtime.get("libc", ""),
            "libcVersion": runtime.get("libcVersion", ""),
            "deploymentTarget": runtime.get("deploymentTarget", ""),
        },
    }


class PlatformBaselineTests(unittest.TestCase):
    def test_canonical_policy(self) -> None:
        verifier.validate_policy(load_policy())

    def test_all_release_rid_evidence(self) -> None:
        policy = load_policy()
        for rid in verifier.SUPPORTED_RIDS:
            with self.subTest(rid=rid):
                verifier.validate_evidence(policy, evidence_for(policy, rid), rid)

    def test_duplicate_policy_key_is_rejected(self) -> None:
        raw = POLICY_PATH.read_text(encoding="utf-8")
        duplicated = raw.replace(
            '"schema": "lua-cpp.platform-baseline/v1",',
            '"schema": "lua-cpp.platform-baseline/v1",\n'
            '  "schema": "lua-cpp.platform-baseline/v1",',
            1,
        )
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "duplicate.json"
            path.write_text(duplicated, encoding="utf-8")
            with self.assertRaisesRegex(verifier.BaselineError, "duplicate JSON key"):
                verifier.load_json(path)

    def test_extra_rid_is_rejected(self) -> None:
        policy = load_policy()
        policy["releaseRids"]["linux-arm64"] = copy.deepcopy(
            policy["releaseRids"]["linux-x64"]
        )
        with self.assertRaisesRegex(verifier.BaselineError, "exactly"):
            verifier.validate_policy(policy)

    def test_missing_mingw_decision_is_rejected(self) -> None:
        policy = load_policy()
        policy["unsupportedTargets"] = [
            value for value in policy["unsupportedTargets"] if value["id"] != "mingw"
        ]
        with self.assertRaisesRegex(verifier.BaselineError, "unsupported target set"):
            verifier.validate_policy(policy)

    def test_wrong_runner_is_rejected(self) -> None:
        policy = load_policy()
        evidence = evidence_for(policy, "linux-x64")
        evidence["runner"] = "ubuntu-latest"
        with self.assertRaisesRegex(verifier.BaselineError, "pinned baseline"):
            verifier.validate_evidence(policy, evidence, "linux-x64")

    def test_wrong_distribution_is_rejected(self) -> None:
        policy = load_policy()
        evidence = evidence_for(policy, "linux-x64")
        evidence["host"]["distributionVersion"] = "26.04"
        with self.assertRaisesRegex(verifier.BaselineError, "exactly"):
            verifier.validate_evidence(policy, evidence, "linux-x64")

    def test_old_compiler_is_rejected(self) -> None:
        policy = load_policy()
        evidence = evidence_for(policy, "windows-x64")
        evidence["toolchain"]["compilerVersion"] = "19.39"
        with self.assertRaisesRegex(verifier.BaselineError, "below minimum"):
            verifier.validate_evidence(policy, evidence, "windows-x64")

    def test_future_compiler_major_is_rejected(self) -> None:
        policy = load_policy()
        evidence = evidence_for(policy, "windows-x64")
        evidence["toolchain"]["compilerVersion"] = "20.0"
        with self.assertRaisesRegex(verifier.BaselineError, "not below"):
            verifier.validate_evidence(policy, evidence, "windows-x64")

    def test_32_bit_target_is_rejected(self) -> None:
        policy = load_policy()
        evidence = evidence_for(policy, "linux-x64")
        evidence["target"]["pointerSize"] = 4
        with self.assertRaisesRegex(verifier.BaselineError, "pointer size"):
            verifier.validate_evidence(policy, evidence, "linux-x64")

    def test_wrong_linux_libc_is_rejected(self) -> None:
        policy = load_policy()
        evidence = evidence_for(policy, "linux-x64")
        evidence["runtime"]["libc"] = "musl"
        with self.assertRaisesRegex(verifier.BaselineError, "libc family"):
            verifier.validate_evidence(policy, evidence, "linux-x64")

    def test_wrong_macos_deployment_target_is_rejected(self) -> None:
        policy = load_policy()
        evidence = evidence_for(policy, "macos-arm64")
        evidence["runtime"]["deploymentTarget"] = "15.0"
        with self.assertRaisesRegex(verifier.BaselineError, "deployment target"):
            verifier.validate_evidence(policy, evidence, "macos-arm64")

    def test_static_windows_crt_is_rejected(self) -> None:
        policy = load_policy()
        evidence = evidence_for(policy, "windows-x64")
        evidence["runtime"]["msvcRuntime"] = "MultiThreaded"
        with self.assertRaisesRegex(verifier.BaselineError, "dynamic release CRT"):
            verifier.validate_evidence(policy, evidence, "windows-x64")

    def test_evidence_rid_mismatch_is_rejected(self) -> None:
        policy = load_policy()
        evidence = evidence_for(policy, "windows-x64")
        with self.assertRaisesRegex(verifier.BaselineError, "does not match"):
            verifier.validate_evidence(policy, evidence, "linux-x64")

    def test_cli_requires_evidence_and_rid_together(self) -> None:
        self.assertEqual(
            verifier.main(["--policy", str(POLICY_PATH), "--expected-rid", "linux-x64"]),
            1,
        )

    def test_windows_dynamic_runtime_report(self) -> None:
        runtime = load_policy()["releaseRids"]["windows-x64"]["minimumRuntime"]
        verifier.validate_windows_binary(
            runtime,
            """
              KERNEL32.dll
              MSVCP140.dll
              VCRUNTIME140.dll
              api-ms-win-crt-runtime-l1-1-0.dll
            """,
        )
        with self.assertRaisesRegex(verifier.BaselineError, "debug CRT"):
            verifier.validate_windows_binary(
                runtime,
                """
                  MSVCP140.dll
                  VCRUNTIME140.dll
                  VCRUNTIME140D.dll
                  ucrtbase.dll
                """,
            )

    def test_linux_symbol_ceiling_report(self) -> None:
        runtime = load_policy()["releaseRids"]["linux-x64"]["minimumRuntime"]
        dynamic = """
          (NEEDED) Shared library: [libstdc++.so.6]
          (NEEDED) Shared library: [libgcc_s.so.1]
          (NEEDED) Shared library: [libc.so.6]
        """
        versions = """
          Name: GLIBC_2.39
          Name: GLIBCXX_3.4.33
          Name: CXXABI_1.3.15
        """
        verifier.validate_linux_binary(runtime, dynamic, versions)
        with self.assertRaisesRegex(verifier.BaselineError, "above"):
            verifier.validate_linux_binary(
                runtime,
                dynamic,
                versions.replace("GLIBCXX_3.4.33", "GLIBCXX_3.4.34"),
            )

    def test_macos_dependency_and_minos_report(self) -> None:
        runtime = load_policy()["releaseRids"]["macos-arm64"]["minimumRuntime"]
        dependencies = """
build/liblua_public_api.0.1.0.dylib:
    @rpath/liblua_public_api.0.dylib (compatibility version 0.0.0, current version 0.1.0)
    /usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 1900.0.0)
    /usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1351.0.0)
        """
        verifier.validate_macos_binary(
            runtime,
            dependencies,
            "      cmd LC_BUILD_VERSION\n    minos 14.0\n",
        )
        with self.assertRaisesRegex(verifier.BaselineError, "does not equal"):
            verifier.validate_macos_binary(
                runtime,
                dependencies,
                "      cmd LC_BUILD_VERSION\n    minos 15.0\n",
            )

    def test_strict_json_round_trip_fixture(self) -> None:
        policy = load_policy()
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "policy.json"
            path.write_text(json.dumps(policy), encoding="utf-8")
            verifier.validate_policy(verifier.load_json(path))

    def test_cmake_module_writes_canonical_evidence(self) -> None:
        policy = load_policy()
        with tempfile.TemporaryDirectory() as directory:
            evidence_path = Path(directory) / "platform-evidence.json"
            completed = subprocess.run(
                [
                    "cmake",
                    f"-DPROJECT_SOURCE_DIR={ROOT}",
                    f"-DEVIDENCE_OUTPUT={evidence_path}",
                    "-P",
                    str(ROOT / "tests/cmake/test_platform_baseline_module.cmake"),
                ],
                check=False,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                encoding="utf-8",
                errors="replace",
            )
            self.assertEqual(
                completed.returncode,
                0,
                msg=f"{completed.stdout}\n{completed.stderr}",
            )
            verifier.validate_evidence(
                policy, verifier.load_json(evidence_path), "windows-x64"
            )

    def test_cmake_module_rejects_mingw_at_configure_time(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            completed = subprocess.run(
                [
                    "cmake",
                    f"-DPROJECT_SOURCE_DIR={ROOT}",
                    f"-DEVIDENCE_OUTPUT={Path(directory) / 'evidence.json'}",
                    "-DTEST_MINGW=ON",
                    "-P",
                    str(ROOT / "tests/cmake/test_platform_baseline_module.cmake"),
                ],
                check=False,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                encoding="utf-8",
                errors="replace",
            )
            self.assertNotEqual(completed.returncode, 0)
            self.assertIn("MinGW is not supported", completed.stderr)

    def test_cmake_module_rejects_static_msvc_runtime(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            completed = subprocess.run(
                [
                    "cmake",
                    f"-DPROJECT_SOURCE_DIR={ROOT}",
                    f"-DEVIDENCE_OUTPUT={Path(directory) / 'evidence.json'}",
                    "-DTEST_MSVC_RUNTIME=MultiThreaded",
                    "-P",
                    str(ROOT / "tests/cmake/test_platform_baseline_module.cmake"),
                ],
                check=False,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                encoding="utf-8",
                errors="replace",
            )
            self.assertNotEqual(completed.returncode, 0)
            self.assertIn("dynamic UCRT/MSVC v143", completed.stderr)


if __name__ == "__main__":
    unittest.main()
