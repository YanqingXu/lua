#!/usr/bin/env python3
"""Validate the release platform policy and CMake-produced platform evidence."""

from __future__ import annotations

import argparse
import json
import re
import shutil
import subprocess
import sys
import os
from pathlib import Path
from typing import Any


POLICY_SCHEMA = "lua-cpp.platform-baseline/v1"
EVIDENCE_SCHEMA = "lua-cpp.platform-evidence/v1"
SUPPORTED_RIDS = ("windows-x64", "linux-x64", "macos-arm64")
VERSION_RE = re.compile(r"^[0-9]+(?:\.[0-9]+)*$")
ARCHITECTURE_ALIASES = {
    "windows-x64": {"amd64", "x64", "x86_64", "x86-64"},
    "linux-x64": {"amd64", "x64", "x86_64", "x86-64"},
    "macos-arm64": {"aarch64", "arm64"},
}


class BaselineError(ValueError):
    """Raised when policy or evidence is not canonical and self-consistent."""


def _reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise BaselineError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def load_json(path: Path) -> dict[str, Any]:
    try:
        raw = path.read_text(encoding="utf-8")
    except OSError as error:
        raise BaselineError(f"cannot read {path}: {error}") from error
    try:
        value = json.loads(
            raw,
            object_pairs_hook=_reject_duplicate_keys,
            parse_constant=lambda token: (_ for _ in ()).throw(
                BaselineError(f"non-standard JSON number: {token}")
            ),
        )
    except (json.JSONDecodeError, UnicodeDecodeError) as error:
        raise BaselineError(f"{path} is not strict UTF-8 JSON: {error}") from error
    if not isinstance(value, dict):
        raise BaselineError(f"{path} root must be an object")
    return value


def _exact_fields(value: dict[str, Any], expected: set[str], label: str) -> None:
    actual = set(value)
    if actual != expected:
        missing = sorted(expected - actual)
        extra = sorted(actual - expected)
        raise BaselineError(f"{label} field mismatch; missing={missing}, extra={extra}")


def _nonempty_string(value: Any, label: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise BaselineError(f"{label} must be a non-empty string")
    return value


def _version(value: Any, label: str) -> tuple[int, ...]:
    text = _nonempty_string(value, label)
    if not VERSION_RE.fullmatch(text):
        raise BaselineError(f"{label} must be a dotted numeric version")
    return tuple(int(part) for part in text.split("."))


def _version_at_least(actual: str, minimum: str, label: str) -> None:
    if _version(actual, label) < _version(minimum, f"{label} minimum"):
        raise BaselineError(f"{label} {actual} is below minimum {minimum}")


def _version_below(actual: str, maximum: str, label: str) -> None:
    if _version(actual, label) >= _version(maximum, f"{label} maximum"):
        raise BaselineError(f"{label} {actual} is not below {maximum}")


def validate_policy(policy: dict[str, Any]) -> None:
    _exact_fields(
        policy,
        {
            "schema",
            "appliesTo",
            "language",
            "releaseRids",
            "ciOnlyTargets",
            "unsupportedTargets",
        },
        "policy",
    )
    if policy["schema"] != POLICY_SCHEMA:
        raise BaselineError(f"policy schema must be {POLICY_SCHEMA}")
    _nonempty_string(policy["appliesTo"], "appliesTo")

    language = policy["language"]
    if not isinstance(language, dict):
        raise BaselineError("language must be an object")
    _exact_fields(language, {"c", "cxx", "cmakeMinimum"}, "language")
    if language["c"] != "C99" or language["cxx"] != "C++23":
        raise BaselineError("language baseline must remain C99/C++23 for 0.1.x")
    _version(language["cmakeMinimum"], "language.cmakeMinimum")

    rids = policy["releaseRids"]
    if not isinstance(rids, dict) or tuple(rids) != SUPPORTED_RIDS:
        raise BaselineError(
            "releaseRids must contain exactly windows-x64, linux-x64, macos-arm64 in canonical order"
        )
    for rid, entry in rids.items():
        _validate_rid_policy(rid, entry)

    ci_only = policy["ciOnlyTargets"]
    if ci_only != ["linux-arm64"]:
        raise BaselineError("ciOnlyTargets must explicitly contain only linux-arm64")

    unsupported = policy["unsupportedTargets"]
    if not isinstance(unsupported, list) or not unsupported:
        raise BaselineError("unsupportedTargets must be a non-empty array")
    expected_unsupported = {"mingw", "windows-x86", "linux-musl", "macos-x64"}
    actual_unsupported: set[str] = set()
    for index, item in enumerate(unsupported):
        if not isinstance(item, dict):
            raise BaselineError(f"unsupportedTargets[{index}] must be an object")
        _exact_fields(item, {"id", "reason"}, f"unsupportedTargets[{index}]")
        item_id = _nonempty_string(item["id"], f"unsupportedTargets[{index}].id")
        _nonempty_string(item["reason"], f"unsupportedTargets[{index}].reason")
        if item_id in actual_unsupported:
            raise BaselineError(f"duplicate unsupported target: {item_id}")
        actual_unsupported.add(item_id)
    if actual_unsupported != expected_unsupported:
        raise BaselineError(
            f"unsupported target set mismatch: {sorted(actual_unsupported)}"
        )


def _validate_rid_policy(rid: str, entry: Any) -> None:
    if not isinstance(entry, dict):
        raise BaselineError(f"releaseRids.{rid} must be an object")
    _exact_fields(
        entry,
        {"architecture", "pointerSize", "minimumRuntime", "builder"},
        f"releaseRids.{rid}",
    )
    architecture = _nonempty_string(
        entry["architecture"], f"releaseRids.{rid}.architecture"
    ).lower()
    if architecture not in ARCHITECTURE_ALIASES[rid]:
        raise BaselineError(f"releaseRids.{rid}.architecture is inconsistent")
    if entry["pointerSize"] != 8 or isinstance(entry["pointerSize"], bool):
        raise BaselineError(f"releaseRids.{rid}.pointerSize must be 8")

    builder = entry["builder"]
    if not isinstance(builder, dict):
        raise BaselineError(f"releaseRids.{rid}.builder must be an object")
    builder_fields = {
        "runner",
        "hostSystem",
        "generator",
        "compilerId",
        "compilerVersionMinimum",
        "compilerVersionMaximumExclusive",
    }
    if rid == "linux-x64":
        builder_fields |= {"distributionId", "distributionVersion"}
    else:
        builder_fields.add("hostVersionMinimum")
    if rid == "windows-x64":
        builder_fields.add("msvcRuntime")
    _exact_fields(builder, builder_fields, f"releaseRids.{rid}.builder")
    for key in builder_fields - {
        "compilerVersionMinimum",
        "compilerVersionMaximumExclusive",
        "hostVersionMinimum",
        "distributionVersion",
    }:
        _nonempty_string(builder[key], f"releaseRids.{rid}.builder.{key}")
    minimum = _version(
        builder["compilerVersionMinimum"],
        f"releaseRids.{rid}.builder.compilerVersionMinimum",
    )
    maximum = _version(
        builder["compilerVersionMaximumExclusive"],
        f"releaseRids.{rid}.builder.compilerVersionMaximumExclusive",
    )
    if minimum >= maximum:
        raise BaselineError(f"releaseRids.{rid} compiler range is empty")
    if "hostVersionMinimum" in builder:
        _version(
            builder["hostVersionMinimum"],
            f"releaseRids.{rid}.builder.hostVersionMinimum",
        )
    if "distributionVersion" in builder:
        _version(
            builder["distributionVersion"],
            f"releaseRids.{rid}.builder.distributionVersion",
        )

    runtime = entry["minimumRuntime"]
    if not isinstance(runtime, dict):
        raise BaselineError(f"releaseRids.{rid}.minimumRuntime must be an object")
    runtime_fields = {"os", "osVersion"}
    if rid == "windows-x64":
        runtime_fields |= {
            "crt",
            "requiredDynamicDependencies",
            "forbiddenDynamicDependencies",
        }
    elif rid == "linux-x64":
        runtime_fields |= {
            "libc",
            "libcVersion",
            "glibcSymbolMaximum",
            "libstdcxxSymbolMaximum",
            "cxxabiSymbolMaximum",
            "requiredDynamicDependencies",
        }
    else:
        runtime_fields |= {
            "deploymentTarget",
            "crt",
            "requiredDynamicDependencies",
            "allowedDynamicDependencyPrefixes",
        }
    _exact_fields(runtime, runtime_fields, f"releaseRids.{rid}.minimumRuntime")
    array_fields = {
        "requiredDynamicDependencies",
        "forbiddenDynamicDependencies",
        "allowedDynamicDependencyPrefixes",
    }
    for key in runtime_fields - array_fields:
        _nonempty_string(runtime[key], f"releaseRids.{rid}.minimumRuntime.{key}")
    for key in runtime_fields & array_fields:
        values = runtime[key]
        if (
            not isinstance(values, list)
            or not values
            or len(values) != len(set(values))
            or any(not isinstance(value, str) or not value for value in values)
        ):
            raise BaselineError(
                f"releaseRids.{rid}.minimumRuntime.{key} must be a unique non-empty string array"
            )
    _version(runtime["osVersion"], f"releaseRids.{rid}.minimumRuntime.osVersion")
    if rid == "linux-x64":
        _version(
            runtime["libcVersion"],
            f"releaseRids.{rid}.minimumRuntime.libcVersion",
        )
        for field, prefix in (
            ("glibcSymbolMaximum", "GLIBC"),
            ("libstdcxxSymbolMaximum", "GLIBCXX"),
            ("cxxabiSymbolMaximum", "CXXABI"),
        ):
            if not re.fullmatch(
                rf"{prefix}_[0-9]+(?:\.[0-9]+)+", runtime[field]
            ):
                raise BaselineError(f"Linux {prefix} symbol ceiling is not canonical")
    if rid == "macos-arm64":
        _version(
            runtime["deploymentTarget"],
            f"releaseRids.{rid}.minimumRuntime.deploymentTarget",
        )
        if runtime["deploymentTarget"] != runtime["osVersion"]:
            raise BaselineError("macOS deployment target must equal the runtime floor")


def _run_tool(arguments: list[str]) -> str:
    try:
        completed = subprocess.run(
            arguments,
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            encoding="utf-8",
            errors="replace",
        )
    except OSError as error:
        raise BaselineError(f"cannot execute {arguments[0]}: {error}") from error
    if completed.returncode != 0:
        raise BaselineError(
            f"{arguments[0]} failed with exit {completed.returncode}: "
            f"{completed.stderr.strip()}"
        )
    return completed.stdout


def _normalize_dependencies(dependencies: set[str]) -> dict[str, str]:
    return {dependency.lower(): dependency for dependency in dependencies}


def _find_windows_analysis_tool() -> tuple[str, str]:
    dumpbin = shutil.which("dumpbin")
    if dumpbin:
        return "dumpbin", dumpbin
    llvm_readobj = shutil.which("llvm-readobj")
    if llvm_readobj:
        return "llvm-readobj", llvm_readobj

    program_files_x86 = os.environ.get("ProgramFiles(x86)", "")
    if program_files_x86:
        vswhere = (
            Path(program_files_x86)
            / "Microsoft Visual Studio"
            / "Installer"
            / "vswhere.exe"
        )
        if vswhere.is_file():
            output = _run_tool(
                [
                    str(vswhere),
                    "-latest",
                    "-products",
                    "*",
                    "-requires",
                    "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                    "-find",
                    r"VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe",
                ]
            )
            candidates = [
                line.strip() for line in output.splitlines() if line.strip()
            ]
            if candidates and Path(candidates[-1]).is_file():
                return "dumpbin", candidates[-1]
    raise BaselineError(
        "dumpbin or llvm-readobj is required for Windows dependency validation"
    )


def validate_windows_binary(runtime: dict[str, Any], dependency_output: str) -> None:
    dependencies = set(
        re.findall(
            r"(?im)^\s*(?:Name:\s*)?([A-Za-z0-9_.-]+\.dll)\s*$",
            dependency_output,
        )
    )
    if not dependencies:
        raise BaselineError("Windows dependency report contains no imported DLLs")
    normalized = _normalize_dependencies(dependencies)
    required = {
        value.lower() for value in runtime["requiredDynamicDependencies"]
    }
    missing = sorted(required - set(normalized))
    if missing:
        raise BaselineError(f"Windows shared library is missing CRT imports: {missing}")
    forbidden = {
        value.lower() for value in runtime["forbiddenDynamicDependencies"]
    }
    present_forbidden = sorted(forbidden & set(normalized))
    if present_forbidden:
        raise BaselineError(
            f"Windows shared library imports debug CRT components: {present_forbidden}"
        )
    if not any(
        dependency == "ucrtbase.dll"
        or dependency.startswith("api-ms-win-crt-")
        for dependency in normalized
    ):
        raise BaselineError("Windows shared library has no dynamic UCRT import")


def _symbol_version(value: str, prefix: str) -> tuple[int, ...]:
    expected_prefix = f"{prefix}_"
    if not value.startswith(expected_prefix):
        raise BaselineError(f"invalid {prefix} symbol version: {value}")
    return _version(value[len(expected_prefix) :], f"{prefix} symbol version")


def validate_linux_binary(
    runtime: dict[str, Any], dynamic_output: str, version_output: str
) -> None:
    dependencies = set(
        re.findall(r"\(NEEDED\).*?Shared library: \[([^\]]+)\]", dynamic_output)
    )
    required = set(runtime["requiredDynamicDependencies"])
    missing = sorted(required - dependencies)
    if missing:
        raise BaselineError(f"Linux shared library is missing runtime imports: {missing}")

    versions = set(
        re.findall(
            r"\b((?:GLIBC|GLIBCXX|CXXABI)_[0-9]+(?:\.[0-9]+)+)\b",
            version_output,
        )
    )
    for prefix, policy_field in (
        ("GLIBC", "glibcSymbolMaximum"),
        ("GLIBCXX", "libstdcxxSymbolMaximum"),
        ("CXXABI", "cxxabiSymbolMaximum"),
    ):
        family = [value for value in versions if value.startswith(f"{prefix}_")]
        if not family:
            raise BaselineError(f"Linux shared library has no {prefix} symbol evidence")
        maximum = _symbol_version(runtime[policy_field], prefix)
        observed = max(_symbol_version(value, prefix) for value in family)
        if observed > maximum:
            raise BaselineError(
                f"Linux shared library requires {prefix}_{'.'.join(map(str, observed))}, "
                f"above {runtime[policy_field]}"
            )


def validate_macos_binary(
    runtime: dict[str, Any], dependency_output: str, load_command_output: str
) -> None:
    dependency_lines = dependency_output.splitlines()[1:]
    dependencies: set[str] = set()
    for line in dependency_lines:
        match = re.match(r"^\s*(\S+)\s+\(compatibility version", line)
        if match:
            dependencies.add(match.group(1))
    required = set(runtime["requiredDynamicDependencies"])
    missing = sorted(required - dependencies)
    if missing:
        raise BaselineError(f"macOS shared library is missing runtime imports: {missing}")
    prefixes = tuple(runtime["allowedDynamicDependencyPrefixes"])
    unexpected = sorted(
        dependency
        for dependency in dependencies
        if not dependency.startswith(prefixes)
    )
    if unexpected:
        raise BaselineError(
            f"macOS shared library has unexpected dynamic dependencies: {unexpected}"
        )
    minimum_versions = re.findall(
        r"(?m)^\s*minos\s+([0-9]+(?:\.[0-9]+)+)\s*$",
        load_command_output,
    )
    if not minimum_versions:
        raise BaselineError("macOS shared library has no LC_BUILD_VERSION minos")
    expected = runtime["deploymentTarget"]
    if any(version != expected for version in minimum_versions):
        raise BaselineError(
            f"macOS shared library minos {minimum_versions} does not equal {expected}"
        )


def validate_shared_library(
    policy: dict[str, Any], rid: str, shared_library: Path
) -> None:
    try:
        if not shared_library.is_file() or shared_library.is_symlink():
            raise BaselineError(
                f"shared library must be one regular non-symlink file: {shared_library}"
            )
    except OSError as error:
        raise BaselineError(f"cannot inspect shared library {shared_library}: {error}") from error
    runtime = policy["releaseRids"][rid]["minimumRuntime"]
    if rid == "windows-x64":
        tool_kind, tool = _find_windows_analysis_tool()
        if tool_kind == "dumpbin":
            output = _run_tool([tool, "/dependents", str(shared_library)])
        else:
            output = _run_tool([tool, "--coff-imports", str(shared_library)])
        validate_windows_binary(runtime, output)
    elif rid == "linux-x64":
        readelf = shutil.which("readelf")
        if not readelf:
            raise BaselineError("readelf is required for Linux dependency validation")
        dynamic_output = _run_tool([readelf, "--dynamic", str(shared_library)])
        version_output = _run_tool([readelf, "--version-info", str(shared_library)])
        validate_linux_binary(runtime, dynamic_output, version_output)
    else:
        otool = shutil.which("otool")
        if not otool:
            raise BaselineError("otool is required for macOS dependency validation")
        dependency_output = _run_tool([otool, "-L", str(shared_library)])
        load_command_output = _run_tool([otool, "-l", str(shared_library)])
        validate_macos_binary(runtime, dependency_output, load_command_output)


def validate_evidence(
    policy: dict[str, Any], evidence: dict[str, Any], expected_rid: str
) -> None:
    validate_policy(policy)
    if expected_rid not in SUPPORTED_RIDS:
        raise BaselineError(f"unsupported expected RID: {expected_rid}")
    _exact_fields(
        evidence,
        {
            "schema",
            "rid",
            "runner",
            "host",
            "target",
            "toolchain",
            "runtime",
        },
        "evidence",
    )
    if evidence["schema"] != EVIDENCE_SCHEMA:
        raise BaselineError(f"evidence schema must be {EVIDENCE_SCHEMA}")
    if evidence["rid"] != expected_rid:
        raise BaselineError(
            f"evidence RID {evidence['rid']!r} does not match {expected_rid!r}"
        )
    baseline = policy["releaseRids"][expected_rid]
    builder = baseline["builder"]
    runtime_policy = baseline["minimumRuntime"]

    if evidence["runner"] != builder["runner"]:
        raise BaselineError("release runner does not match the pinned baseline")

    host = evidence["host"]
    if not isinstance(host, dict):
        raise BaselineError("evidence.host must be an object")
    host_fields = {"system", "version", "distributionId", "distributionVersion"}
    _exact_fields(host, host_fields, "evidence.host")
    for key in host_fields:
        if not isinstance(host[key], str):
            raise BaselineError(f"evidence.host.{key} must be a string")
    if host["system"] != builder["hostSystem"]:
        raise BaselineError("build host system does not match the baseline")
    if expected_rid == "linux-x64":
        if (
            host["distributionId"] != builder["distributionId"]
            or host["distributionVersion"] != builder["distributionVersion"]
        ):
            raise BaselineError("Linux build distribution must exactly match the baseline")
    else:
        _version_at_least(
            host["version"], builder["hostVersionMinimum"], "build host version"
        )
        if host["distributionId"] or host["distributionVersion"]:
            raise BaselineError("non-Linux evidence must not claim a distribution")

    target = evidence["target"]
    if not isinstance(target, dict):
        raise BaselineError("evidence.target must be an object")
    _exact_fields(target, {"system", "processor", "pointerSize"}, "evidence.target")
    if target["system"] != builder["hostSystem"]:
        raise BaselineError("cross-compiled release packages are not accepted")
    processor = _nonempty_string(target["processor"], "evidence.target.processor").lower()
    if processor not in ARCHITECTURE_ALIASES[expected_rid]:
        raise BaselineError("target processor does not match the RID")
    if target["pointerSize"] != baseline["pointerSize"] or isinstance(
        target["pointerSize"], bool
    ):
        raise BaselineError("target pointer size does not match the RID")

    toolchain = evidence["toolchain"]
    if not isinstance(toolchain, dict):
        raise BaselineError("evidence.toolchain must be an object")
    _exact_fields(
        toolchain,
        {"generator", "compilerId", "compilerVersion", "cmakeVersion"},
        "evidence.toolchain",
    )
    if toolchain["generator"] != builder["generator"]:
        raise BaselineError("generator does not match the release baseline")
    if toolchain["compilerId"] != builder["compilerId"]:
        raise BaselineError("compiler family does not match the release baseline")
    compiler_version = _nonempty_string(
        toolchain["compilerVersion"], "evidence.toolchain.compilerVersion"
    )
    _version_at_least(
        compiler_version,
        builder["compilerVersionMinimum"],
        "compiler version",
    )
    _version_below(
        compiler_version,
        builder["compilerVersionMaximumExclusive"],
        "compiler version",
    )
    _version_at_least(
        _nonempty_string(toolchain["cmakeVersion"], "evidence.toolchain.cmakeVersion"),
        policy["language"]["cmakeMinimum"],
        "CMake version",
    )

    runtime = evidence["runtime"]
    if not isinstance(runtime, dict):
        raise BaselineError("evidence.runtime must be an object")
    _exact_fields(
        runtime,
        {"msvcRuntime", "libc", "libcVersion", "deploymentTarget"},
        "evidence.runtime",
    )
    for key, value in runtime.items():
        if not isinstance(value, str):
            raise BaselineError(f"evidence.runtime.{key} must be a string")
    if expected_rid == "windows-x64":
        if runtime["msvcRuntime"] != builder["msvcRuntime"]:
            raise BaselineError("MSVC runtime must be the dynamic release CRT")
        if runtime["libc"] or runtime["libcVersion"] or runtime["deploymentTarget"]:
            raise BaselineError("Windows evidence contains foreign runtime fields")
    elif expected_rid == "linux-x64":
        if runtime["libc"] != runtime_policy["libc"]:
            raise BaselineError("Linux libc family does not match the baseline")
        if runtime["libcVersion"] != runtime_policy["libcVersion"]:
            raise BaselineError(
                "Linux release must be built on the exact glibc baseline"
            )
        if runtime["msvcRuntime"] or runtime["deploymentTarget"]:
            raise BaselineError("Linux evidence contains foreign runtime fields")
    else:
        if runtime["deploymentTarget"] != runtime_policy["deploymentTarget"]:
            raise BaselineError("macOS deployment target does not match the baseline")
        if runtime["msvcRuntime"] or runtime["libc"] or runtime["libcVersion"]:
            raise BaselineError("macOS evidence contains foreign runtime fields")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--policy", type=Path, default=Path("docs/release/platform-baseline.json")
    )
    parser.add_argument("--evidence", type=Path)
    parser.add_argument("--expected-rid", choices=SUPPORTED_RIDS)
    parser.add_argument("--shared-library", type=Path)
    arguments = parser.parse_args(argv)
    try:
        policy = load_json(arguments.policy)
        validate_policy(policy)
        if (arguments.evidence is None) != (arguments.expected_rid is None):
            raise BaselineError(
                "--evidence and --expected-rid must be provided together"
            )
        if arguments.evidence is not None:
            evidence = load_json(arguments.evidence)
            validate_evidence(policy, evidence, arguments.expected_rid)
            if arguments.shared_library is None:
                raise BaselineError(
                    "--shared-library is required with release platform evidence"
                )
            validate_shared_library(
                policy, arguments.expected_rid, arguments.shared_library
            )
        elif arguments.shared_library is not None:
            raise BaselineError("--shared-library requires release platform evidence")
    except BaselineError as error:
        print(f"platform baseline validation failed: {error}", file=sys.stderr)
        return 1
    if arguments.evidence is None:
        print("platform baseline policy validated")
    else:
        print(f"platform baseline evidence validated for {arguments.expected_rid}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
