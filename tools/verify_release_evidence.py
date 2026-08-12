#!/usr/bin/env python3
"""Verify exact-SHA GitHub Actions evidence before release packaging.

This verifier intentionally checks remote execution evidence only. Package
checksums are produced after packaging and must be validated by the package
validator; requiring them here would create a verifier -> package -> verifier
self-reference.
"""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import math
import os
import re
import stat
import sys
import tempfile
import urllib.error
import urllib.parse
import urllib.request
import zipfile
from collections.abc import Iterable, Mapping, Sequence
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath
from typing import Any, Protocol

from verify_release_governance import (
    GovernanceError,
    parse_evidence_json,
    validate_governance_evidence,
)
from verify_source_readiness_evidence import (
    SourceReadinessError,
    parse_source_readiness_json,
    validate_source_readiness_evidence,
)

TOOL_VERSION = "2.0.0"
MANIFEST_SCHEMA = "lua-cpp.release-evidence/v2"
CI_WORKFLOW = "ci.yml"
NIGHTLY_WORKFLOW = "nightly.yml"
MAIN_BRANCH = "main"

EXPECTED_CI_JOBS = (
    "Allocator failure contract (ubuntu-latest)",
    "Allocator failure contract (windows-latest)",
    "Linux Clang (Debug)",
    "Linux Clang (Release)",
    "Linux Clang (address)",
    "Linux Clang (thread)",
    "Linux Clang (undefined)",
    "Linux GCC (Debug)",
    "Linux GCC (Release)",
    "Linux clang-format / clang-tidy",
    "Linux component coverage",
    "Linux libFuzzer security boundaries",
    "Linux runtime benchmark contract",
    "Portability (Linux ARM64)",
    "Portability (macOS ARM64)",
    "Windows MSBuild (Debug)",
    "Windows MSBuild (Release)",
)
EXPECTED_NIGHTLY_JOBS = (
    "Long sanitizer fuzz",
    "Runtime and native-module soak",
    "Worker fault matrix (linux-x64)",
    "Worker fault matrix (windows-x64)",
)
CI_ARTIFACTS = (
    "component-coverage",
    "runtime-benchmark-evidence",
)
NIGHTLY_ARTIFACTS = (
    "long-fuzz-evidence",
    "runtime-soak-evidence",
)
WORKFLOW_EVIDENCE_SCHEMA = "lua-cpp.workflow-evidence/v1"
EXPECTED_ARTIFACT_JOBS = {
    "component-coverage": "linux-coverage",
    "runtime-benchmark-evidence": "linux-runtime-benchmark",
    "runtime-soak-evidence": "runtime-soak",
    "long-fuzz-evidence": "long-fuzz",
}
EXPECTED_FUZZ_TARGETS = (
    "undump",
    "bytecode_verifier",
    "parser",
    "stdlib_numeric_arguments",
    "remote_protocol",
    "debugger_expression",
)
WORKFLOW_EVIDENCE_FIELDS = {
    "schema",
    "kind",
    "repository",
    "candidate_sha",
    "run_id",
    "run_attempt",
    "event",
    "workflow_ref",
    "job",
    "result",
    "created_at",
    "parameters",
}
LINT_JOB = "Linux clang-format / clang-tidy"
LINT_STEPS = ("clang-format", "clang-tidy")
REQUIRED_NOTES_ARTIFACTS = (*CI_ARTIFACTS, *NIGHTLY_ARTIFACTS)
REQUIRED_NOTES_EVIDENCE_TERMS = ("CI", "nightly", "SHA-256", "SBOM")
EXPECTED_COVERAGE_THRESHOLDS = {
    "bytecode_verifier": 84.0,
    "c_api": 83.0,
    "debugger_core": 75.0,
    "gc_phases": 86.0,
    "opcode_handlers": 84.0,
    "parser_codegen": 90.0,
    "sandbox_denied_paths": 76.0,
}
EXPECTED_COVERAGE_COMPONENT_PATTERNS = {
    "parser_codegen": (
        "/src/compiler/parser/",
        "/src/compiler/lexer/",
        "/src/compiler/codegen/",
    ),
    "opcode_handlers": (
        "/src/vm/vm_handlers",
        "/src/vm/vm_loop.cpp",
        "/src/vm/vm.cpp",
    ),
    "gc_phases": ("/src/gc/",),
    "c_api": ("/src/api/",),
    "bytecode_verifier": ("/src/runtime/bytecode_verifier.cpp",),
    "debugger_core": ("/src/debugger/",),
    "sandbox_denied_paths": (
        "/src/runtime/sandbox_policy.hpp",
        "/src/vm/state/global_state.cpp",
        "/src/lib/baselib.cpp",
        "/src/lib/lib_manager.cpp",
    ),
}
MINIMUM_COVERAGE_SCOPE = {
    "bytecode_verifier": {"files": 1, "total_lines": 200},
    "c_api": {"files": 2, "total_lines": 1800},
    "debugger_core": {"files": 8, "total_lines": 3000},
    "gc_phases": {"files": 6, "total_lines": 1100},
    "opcode_handlers": {"files": 10, "total_lines": 900},
    "parser_codegen": {"files": 20, "total_lines": 4000},
    "sandbox_denied_paths": {"files": 3, "total_lines": 1400},
}
EXPECTED_LLVM_COVERAGE_EXPORT_VERSION = "2.0.1"
EXPECTED_BENCHMARK_METRICS = {
    "vm_instructions_per_second": ("higher", 0.20),
    "cpp_to_lua_ns_per_call": ("lower", 0.20),
    "lua_to_cpp_ns_per_call": ("lower", 0.20),
    "coroutine_resume_yield_ns": ("lower", 0.20),
    "closure_upvalue_lifecycle_per_second": ("higher", 0.25),
    "gc_pause_p99_us": ("lower", 0.50),
}
EXPECTED_BENCHMARK_ABSOLUTE_SLOS = {
    "parse_compile_mib_per_second": ("higher", 1.0),
    "vm_instructions_per_second": ("higher", 10_000_000.0),
    "cpp_to_lua_ns_per_call": ("lower", 2_000.0),
    "lua_to_cpp_ns_per_call": ("lower", 1_500.0),
    "coroutine_resume_yield_ns": ("lower", 2_500.0),
    "table_operations_per_second": ("higher", 1_000_000.0),
    "closure_upvalue_lifecycle_per_second": ("higher", 100_000.0),
    "allocation_mib_per_second": ("higher", 50.0),
    "gc_pause_p99_us": ("lower", 250.0),
    "gc_pause_max_us": ("lower", 5_000.0),
}
EXPECTED_BENCHMARK_ABSOLUTE_SCOPE = {
    "schema_version": 1,
    "profile": "ci",
    "build_type": "Release",
    "runner": "Linux x86_64 hosted or better",
}
EXPECTED_BENCHMARK_RUNTIME_INPUTS = ("CMakeLists.txt", "cmake", "src")
EXPECTED_BENCHMARK_POLICY = {
    "executionOrder": "alternating-base-head-on-the-same-runner",
    "sampleAggregation": "median-of-run-medians",
    "regressionAggregation": "median-of-paired-run-regressions",
    "gcPauseAggregation": "pooled-nearest-rank-p99",
    "noisePolicy": "confirm-mixed-paired-threshold-outcomes",
}
TIMED_JOB_STEPS = {
    "Runtime and native-module soak": ("Run runtime soak", 45 * 60),
    "Long sanitizer fuzz": ("Run long fuzz campaign", len(EXPECTED_FUZZ_TARGETS) * 600),
}
ARTIFACT_TIMED_JOBS = {
    "runtime-soak-evidence": "Runtime and native-module soak",
    "long-fuzz-evidence": "Long sanitizer fuzz",
}
MAX_ARCHIVE_ENTRIES = 100_000
MAX_ARCHIVE_UNCOMPRESSED_BYTES = 4 * 1024 * 1024 * 1024
MAX_ARCHIVE_FILE_BYTES = 512 * 1024 * 1024
MAX_JSON_BYTES = 16 * 1024 * 1024
MAX_FUZZ_LOG_BYTES = 32 * 1024 * 1024

_SHA_RE = re.compile(r"[0-9a-f]{40}")
_DIGEST_RE = re.compile(r"sha256:([0-9a-f]{64})", re.IGNORECASE)
_REPOSITORY_RE = re.compile(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+")
_EMBEDDED_SHA_RE = re.compile(r"(?<![0-9a-f])[0-9a-f]{40}(?![0-9a-f])", re.IGNORECASE)
_ACTIONS_RUN_URL_RE = re.compile(
    r"https?://(?:www\.)?github\.com/[^/\s]+/[^/\s]+/actions/runs/\d+"
    r"(?:[/?#][^\s)]*)?",
    re.IGNORECASE,
)
_FUZZ_DONE_RE = re.compile(r"\bDone\s+\d+\s+runs?\s+in\s+(\d+)\s+second\(s\)", re.IGNORECASE)
_FUZZ_EXECUTIONS_RE = re.compile(r"stat::number_of_executed_units:\s*(\d+)")
_PLACEHOLDER_PATTERNS = (
    re.compile(r"\b(?:TODO|TBD|FIXME|PLACEHOLDER|UNPUBLISHED)\b", re.IGNORECASE),
    re.compile(r"(?:待补|未发布|占位|发布说明模板|正式\s*RC\s*说明必须补入)", re.IGNORECASE),
    re.compile(r"\{\{[^{}\n]+\}\}"),
    re.compile(r"\$\{[^{}\n]+\}"),
    re.compile(
        r"<(?:candidate[-_ ]?sha|sha|commit|ci[-_ ]?run|nightly[-_ ]?run|artifact)[^>\n]*>",
        re.IGNORECASE,
    ),
    re.compile(r"\[(?:TODO|TBD|INSERT|PLACEHOLDER)[^\]\n]*\]", re.IGNORECASE),
    re.compile(r"(?<![0-9A-Za-z])(?:0{40}|x{40}|\?{40})(?![0-9A-Za-z])", re.IGNORECASE),
)


class EvidenceError(RuntimeError):
    """Raised when required evidence is missing, ambiguous, or invalid."""


class RestApi(Protocol):
    def get(self, path: str, params: Mapping[str, object] | None = None) -> object:
        """Return decoded JSON for one GitHub REST request."""

    def download(self, url: str) -> bytes:
        """Download one authenticated artifact archive."""


class _SafeRedirectHandler(urllib.request.HTTPRedirectHandler):
    """Avoid forwarding the GitHub token to a cross-origin artifact URL."""

    def redirect_request(
        self,
        request: urllib.request.Request,
        file_pointer: Any,
        code: int,
        message: str,
        headers: Any,
        new_url: str,
    ) -> urllib.request.Request | None:
        redirected = super().redirect_request(
            request,
            file_pointer,
            code,
            message,
            headers,
            new_url,
        )
        if redirected is not None:
            old_origin = urllib.parse.urlsplit(request.full_url)[:2]
            new_origin = urllib.parse.urlsplit(new_url)[:2]
            if old_origin != new_origin:
                redirected.remove_header("Authorization")
                redirected.remove_header("X-GitHub-Api-Version")
        return redirected


class GitHubRestClient:
    """Small standard-library GitHub REST client."""

    def __init__(
        self,
        token: str,
        api_url: str = "https://api.github.com",
        timeout_seconds: float = 30.0,
        max_download_bytes: int = 1024 * 1024 * 1024,
    ):
        if not token:
            raise EvidenceError("GitHub token is required")
        self._token = token
        self._api_url = api_url.rstrip("/")
        self._timeout_seconds = timeout_seconds
        self._max_download_bytes = max_download_bytes
        self._opener = urllib.request.build_opener(_SafeRedirectHandler())

    def get(self, path: str, params: Mapping[str, object] | None = None) -> object:
        url = f"{self._api_url}/{path.lstrip('/')}"
        if params:
            url = f"{url}?{urllib.parse.urlencode(params)}"
        request = urllib.request.Request(
            url,
            headers={
                "Accept": "application/vnd.github+json",
                "Authorization": f"Bearer {self._token}",
                "User-Agent": f"lua-cpp-release-evidence/{TOOL_VERSION}",
                "X-GitHub-Api-Version": "2022-11-28",
            },
        )
        try:
            with self._opener.open(request, timeout=self._timeout_seconds) as response:
                payload = response.read()
        except urllib.error.HTTPError as error:
            raise EvidenceError(f"GitHub API returned HTTP {error.code} for {path}") from error
        except (urllib.error.URLError, TimeoutError, OSError) as error:
            raise EvidenceError(f"GitHub API request failed for {path}: {error}") from error
        try:
            return json.loads(payload.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError) as error:
            raise EvidenceError(f"GitHub API returned invalid JSON for {path}") from error

    def download(self, url: str) -> bytes:
        parsed_url = urllib.parse.urlsplit(url)
        api_origin = urllib.parse.urlsplit(self._api_url)[:2]
        if parsed_url[:2] != api_origin:
            raise EvidenceError("artifact archive URL is outside the configured GitHub API origin")
        request = urllib.request.Request(
            url,
            headers={
                "Accept": "application/vnd.github+json",
                "Authorization": f"Bearer {self._token}",
                "User-Agent": f"lua-cpp-release-evidence/{TOOL_VERSION}",
                "X-GitHub-Api-Version": "2022-11-28",
            },
        )
        try:
            with self._opener.open(request, timeout=self._timeout_seconds) as response:
                content_length = response.headers.get("Content-Length")
                if content_length is not None and int(content_length) > self._max_download_bytes:
                    raise EvidenceError("artifact archive exceeds the configured download limit")
                payload = response.read(self._max_download_bytes + 1)
        except EvidenceError:
            raise
        except urllib.error.HTTPError as error:
            raise EvidenceError(f"artifact download returned HTTP {error.code}") from error
        except (urllib.error.URLError, TimeoutError, OSError, ValueError) as error:
            raise EvidenceError(f"artifact download failed: {error}") from error
        if len(payload) > self._max_download_bytes:
            raise EvidenceError("artifact archive exceeds the configured download limit")
        return payload


def _mapping(value: object, label: str) -> Mapping[str, Any]:
    if not isinstance(value, Mapping):
        raise EvidenceError(f"{label} is not a JSON object")
    return value


def _sequence(value: object, label: str) -> Sequence[object]:
    if not isinstance(value, list):
        raise EvidenceError(f"{label} is not a JSON array")
    return value


def _integer(value: object, label: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or value <= 0:
        raise EvidenceError(f"{label} is not a positive integer")
    return value


def _nonnegative_integer(value: object, label: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or value < 0:
        raise EvidenceError(f"{label} is not a non-negative integer")
    return value


def _string(value: object, label: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise EvidenceError(f"{label} is missing")
    return value


def _boolean(value: object, label: str) -> bool:
    if type(value) is not bool:
        raise EvidenceError(f"{label} is not a boolean")
    return value


def _number(value: object, label: str) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise EvidenceError(f"{label} is not numeric")
    result = float(value)
    if not math.isfinite(result):
        raise EvidenceError(f"{label} is not finite")
    return result


def _timestamp(value: object, label: str) -> datetime:
    text = _string(value, label)
    try:
        parsed = datetime.fromisoformat(text[:-1] + "+00:00" if text.endswith("Z") else text)
    except ValueError as error:
        raise EvidenceError(f"{label} is not an ISO-8601 timestamp") from error
    if parsed.tzinfo is None:
        raise EvidenceError(f"{label} has no timezone")
    return parsed.astimezone(timezone.utc)


def _format_timestamp(value: datetime) -> str:
    return value.astimezone(timezone.utc).isoformat(timespec="seconds").replace("+00:00", "Z")


def _sha(value: object, label: str) -> str:
    text = _string(value, label).lower()
    if _SHA_RE.fullmatch(text) is None:
        raise EvidenceError(f"{label} is not a full 40-character commit SHA")
    return text


def _url(value: object, label: str) -> str:
    text = _string(value, label)
    parsed = urllib.parse.urlparse(text)
    if parsed.scheme not in ("http", "https") or not parsed.netloc:
        raise EvidenceError(f"{label} is not an absolute HTTP URL")
    return text


def _json_object_without_duplicates(pairs: list[tuple[str, object]]) -> dict[str, object]:
    result: dict[str, object] = {}
    for key, value in pairs:
        if key in result:
            raise EvidenceError(f"evidence JSON contains duplicate key: {key}")
        result[key] = value
    return result


def _finite_json_float(value: str) -> float:
    parsed = float(value)
    if not math.isfinite(parsed):
        raise EvidenceError("evidence JSON contains a non-finite number")
    return parsed


def _reject_json_constant(value: str) -> object:
    raise EvidenceError(f"evidence JSON contains a non-standard constant: {value}")


def _decode_json_bytes(payload: bytes, label: str) -> Mapping[str, Any]:
    try:
        payload = json.loads(
            payload.decode("utf-8-sig"),
            object_pairs_hook=_json_object_without_duplicates,
            parse_float=_finite_json_float,
            parse_constant=_reject_json_constant,
        )
    except EvidenceError:
        raise
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise EvidenceError(f"{label} is not valid UTF-8 JSON") from error
    return _mapping(payload, label)


def _catalog_zip_files(
    zipped: zipfile.ZipFile,
    artifact_name: str,
) -> dict[str, zipfile.ZipInfo]:
    entries = zipped.infolist()
    if len(entries) > MAX_ARCHIVE_ENTRIES:
        raise EvidenceError(f"artifact {artifact_name} contains too many ZIP entries")

    files: dict[str, zipfile.ZipInfo] = {}
    seen_paths: set[str] = set()
    total_uncompressed = 0
    for info in entries:
        raw_name = info.filename
        if (
            not raw_name
            or "\x00" in raw_name
            or "\\" in raw_name
            or raw_name.startswith("/")
            or re.match(r"^[A-Za-z]:", raw_name) is not None
        ):
            raise EvidenceError(f"artifact {artifact_name} contains an unsafe ZIP path")
        without_trailing_slash = raw_name[:-1] if raw_name.endswith("/") else raw_name
        parts = without_trailing_slash.split("/")
        if not parts or any(part in ("", ".", "..") for part in parts):
            raise EvidenceError(f"artifact {artifact_name} contains an unsafe ZIP path")
        normalized = PurePosixPath(*parts).as_posix()
        if normalized != without_trailing_slash:
            raise EvidenceError(f"artifact {artifact_name} contains a non-canonical ZIP path")
        path_key = normalized.casefold()
        if path_key in seen_paths:
            raise EvidenceError(f"artifact {artifact_name} contains duplicate ZIP path: {normalized}")
        seen_paths.add(path_key)
        if info.flag_bits & 0x1:
            raise EvidenceError(f"artifact {artifact_name} contains an encrypted ZIP entry")
        unix_mode = (info.external_attr >> 16) & 0xFFFF
        if unix_mode and stat.S_ISLNK(unix_mode):
            raise EvidenceError(f"artifact {artifact_name} contains a symbolic-link ZIP entry")
        if info.file_size < 0 or info.file_size > MAX_ARCHIVE_FILE_BYTES:
            raise EvidenceError(f"artifact {artifact_name} contains an oversized ZIP entry")
        total_uncompressed += info.file_size
        if total_uncompressed > MAX_ARCHIVE_UNCOMPRESSED_BYTES:
            raise EvidenceError(f"artifact {artifact_name} has excessive uncompressed size")
        if not info.is_dir():
            files[normalized] = info
    return files


def _read_zip_file(
    zipped: zipfile.ZipFile,
    files: Mapping[str, zipfile.ZipInfo],
    path: str,
    artifact_name: str,
    *,
    maximum_bytes: int,
) -> bytes:
    info = files.get(path)
    if info is None:
        raise EvidenceError(f"artifact {artifact_name} is missing required payload file: {path}")
    if info.file_size <= 0:
        raise EvidenceError(f"artifact {artifact_name} payload file is empty: {path}")
    if info.file_size > maximum_bytes:
        raise EvidenceError(f"artifact {artifact_name} payload file is too large: {path}")
    try:
        payload = zipped.read(info)
    except (KeyError, RuntimeError, OSError, zipfile.BadZipFile) as error:
        raise EvidenceError(f"artifact {artifact_name} payload file is unreadable: {path}") from error
    if len(payload) != info.file_size:
        raise EvidenceError(f"artifact {artifact_name} payload file has an inconsistent size: {path}")
    return payload


def _read_zip_json(
    zipped: zipfile.ZipFile,
    files: Mapping[str, zipfile.ZipInfo],
    path: str,
    artifact_name: str,
) -> Mapping[str, Any]:
    return _decode_json_bytes(
        _read_zip_file(
            zipped,
            files,
            path,
            artifact_name,
            maximum_bytes=MAX_JSON_BYTES,
        ),
        f"artifact {artifact_name} payload {path}",
    )


def _require_exact_file_set(
    files: Mapping[str, zipfile.ZipInfo],
    expected: set[str],
    artifact_name: str,
) -> None:
    actual = set(files)
    if actual != expected:
        missing = sorted(expected - actual)
        extra = sorted(actual - expected)
        raise EvidenceError(
            f"artifact {artifact_name} payload file set mismatch; "
            f"missing={missing}, extra={extra}"
        )


def _coverage_components_from_raw(
    report: Mapping[str, Any],
) -> tuple[dict[str, dict[str, int | float]], str]:
    if set(report) != {"data", "type", "version"}:
        raise EvidenceError("raw coverage export field set mismatch")
    if _string(report.get("type"), "raw coverage export type") != "llvm.coverage.json.export":
        raise EvidenceError("raw coverage export type mismatch")
    version = _string(report.get("version"), "raw coverage export version")
    if version != EXPECTED_LLVM_COVERAGE_EXPORT_VERSION:
        raise EvidenceError("raw coverage export version mismatch")
    records = _sequence(report.get("data"), "raw coverage export data")
    if not records:
        raise EvidenceError("raw coverage export has no data")

    files: list[dict[str, int | str]] = []
    seen_filenames: set[str] = set()
    for record_index, raw_record in enumerate(records):
        record = _mapping(raw_record, f"raw coverage export data {record_index}")
        raw_files = _sequence(
            record.get("files"),
            f"raw coverage export data {record_index} files",
        )
        for file_index, raw_file in enumerate(raw_files):
            file_report = _mapping(
                raw_file,
                f"raw coverage export data {record_index} file {file_index}",
            )
            filename = _string(
                file_report.get("filename"),
                f"raw coverage export data {record_index} file {file_index} filename",
            ).replace("\\", "/")
            filename_key = filename.casefold()
            if filename_key in seen_filenames:
                raise EvidenceError(f"raw coverage export contains duplicate file: {filename}")
            seen_filenames.add(filename_key)
            summary = _mapping(
                file_report.get("summary"),
                f"raw coverage export file {filename} summary",
            )
            lines = _mapping(
                summary.get("lines"),
                f"raw coverage export file {filename} line summary",
            )
            total = _nonnegative_integer(
                lines.get("count"),
                f"raw coverage export file {filename} total lines",
            )
            covered = _nonnegative_integer(
                lines.get("covered"),
                f"raw coverage export file {filename} covered lines",
            )
            if covered > total:
                raise EvidenceError(
                    f"raw coverage export file {filename} covered lines exceed total"
                )
            files.append(
                {
                    "filename": filename,
                    "covered": covered,
                    "total": total,
                }
            )
    if not files:
        raise EvidenceError("raw coverage export has no files")

    components: dict[str, dict[str, int | float]] = {}
    for name, patterns in EXPECTED_COVERAGE_COMPONENT_PATTERNS.items():
        matched = [
            file_report
            for file_report in files
            if any(
                pattern in str(file_report["filename"])
                for pattern in patterns
            )
        ]
        file_count = len(matched)
        covered = sum(int(file_report["covered"]) for file_report in matched)
        total = sum(int(file_report["total"]) for file_report in matched)
        scope = MINIMUM_COVERAGE_SCOPE[name]
        if (
            file_count < scope["files"]
            or total < scope["total_lines"]
        ):
            raise EvidenceError(
                f"component coverage {name} raw scope collapsed "
                f"(files={file_count}, totalLines={total})"
            )
        components[name] = {
            "files": file_count,
            "coveredLines": covered,
            "totalLines": total,
            "linePercent": round(100.0 * covered / total, 2),
        }
    return components, version


def _validated_coverage_policy(
    policy: Mapping[str, Any],
) -> dict[str, float]:
    if set(policy) != {"schemaVersion", "components"} or policy.get("schemaVersion") != 1:
        raise EvidenceError("component coverage threshold policy schema mismatch")
    raw_components = _mapping(
        policy.get("components"),
        "component coverage threshold policy components",
    )
    if set(raw_components) != set(EXPECTED_COVERAGE_THRESHOLDS):
        raise EvidenceError("component coverage threshold policy component set mismatch")
    thresholds: dict[str, float] = {}
    for name, expected in EXPECTED_COVERAGE_THRESHOLDS.items():
        threshold = _number(
            raw_components.get(name),
            f"component coverage threshold policy {name}",
        )
        if abs(threshold - expected) > 1.0e-9:
            raise EvidenceError(
                f"component coverage threshold policy for {name} was changed"
            )
        thresholds[name] = threshold
    return thresholds


def _validated_coverage_payload(
    zipped: zipfile.ZipFile,
    files: Mapping[str, zipfile.ZipInfo],
    artifact_name: str,
) -> dict[str, object]:
    allowed = {
        path
        for path in files
        if path
        in {
            "evidence-metadata.json",
            "coverage.json",
            "coverage-components.json",
            "component-thresholds.json",
        }
        or path.startswith("html/")
    }
    if allowed != set(files):
        raise EvidenceError(f"artifact {artifact_name} contains an unexpected payload path")
    index_bytes = _read_zip_file(
        zipped,
        files,
        "html/index.html",
        artifact_name,
        maximum_bytes=4 * 1024 * 1024,
    )
    if b"<html" not in index_bytes[:4096].lower() and b"<!doctype html" not in index_bytes[:4096].lower():
        raise EvidenceError(f"artifact {artifact_name} HTML index is not recognizable")

    raw_report = _read_zip_json(
        zipped,
        files,
        "coverage.json",
        artifact_name,
    )
    recomputed_components, raw_version = _coverage_components_from_raw(raw_report)
    policy = _read_zip_json(
        zipped,
        files,
        "component-thresholds.json",
        artifact_name,
    )
    thresholds = _validated_coverage_policy(policy)
    report = _read_zip_json(
        zipped,
        files,
        "coverage-components.json",
        artifact_name,
    )
    if set(report) != {"schemaVersion", "components", "thresholdsPassed"}:
        raise EvidenceError("component coverage report field set mismatch")
    if report.get("schemaVersion") != 2:
        raise EvidenceError("component coverage report schemaVersion must be 2")
    if _boolean(report.get("thresholdsPassed"), "component coverage thresholdsPassed") is not True:
        raise EvidenceError("component coverage thresholds did not pass")
    components = _mapping(report.get("components"), "component coverage components")
    if set(components) != set(EXPECTED_COVERAGE_THRESHOLDS):
        raise EvidenceError("component coverage report component set mismatch")

    summary: dict[str, float] = {}
    for name, threshold in thresholds.items():
        metrics = _mapping(components.get(name), f"component coverage {name}")
        expected_fields = {
            "files",
            "coveredLines",
            "totalLines",
            "linePercent",
            "minimumLinePercent",
            "thresholdPassed",
        }
        if set(metrics) != expected_fields:
            raise EvidenceError(f"component coverage {name} field set mismatch")
        file_count = _integer(metrics.get("files"), f"component coverage {name} files")
        covered = _nonnegative_integer(
            metrics.get("coveredLines"),
            f"component coverage {name} coveredLines",
        )
        total = _integer(metrics.get("totalLines"), f"component coverage {name} totalLines")
        if covered > total:
            raise EvidenceError(f"component coverage {name} covered lines exceed total lines")
        percent = _number(metrics.get("linePercent"), f"component coverage {name} linePercent")
        minimum = _number(
            metrics.get("minimumLinePercent"),
            f"component coverage {name} minimumLinePercent",
        )
        if abs(minimum - threshold) > 1e-9:
            raise EvidenceError(f"component coverage {name} threshold differs from release policy")
        recomputed = recomputed_components[name]
        if (
            file_count != recomputed["files"]
            or covered != recomputed["coveredLines"]
            or total != recomputed["totalLines"]
        ):
            raise EvidenceError(
                f"component coverage {name} summary contradicts raw coverage export"
            )
        expected_percent = float(recomputed["linePercent"])
        if abs(percent - expected_percent) > 0.011:
            raise EvidenceError(
                f"component coverage {name} percentage contradicts raw coverage export"
            )
        expected_passed = percent >= minimum
        if _boolean(
            metrics.get("thresholdPassed"),
            f"component coverage {name} thresholdPassed",
        ) != expected_passed:
            raise EvidenceError(
                f"component coverage {name} threshold result is inconsistent"
            )
        if not expected_passed:
            raise EvidenceError(f"component coverage {name} threshold did not pass")
        if file_count <= 0:
            raise EvidenceError(f"component coverage {name} has no source files")
        summary[name] = percent
    return {
        "schema_version": 2,
        "thresholds_passed": True,
        "line_percent": summary,
        "html_index": "html/index.html",
        "raw_coverage": {
            "type": "llvm.coverage.json.export",
            "version": raw_version,
            "recomputed": True,
        },
        "threshold_policy": "component-thresholds.json",
        "minimum_scope": MINIMUM_COVERAGE_SCOPE,
    }


def _string_array(value: object, label: str) -> list[str]:
    return [_string(item, f"{label}[{index}]") for index, item in enumerate(_sequence(value, label))]


def _median(values: Sequence[float], label: str) -> float:
    if not values:
        raise EvidenceError(f"{label} cannot be computed from an empty sample set")
    ordered = sorted(values)
    middle = len(ordered) // 2
    if len(ordered) % 2:
        return ordered[middle]
    result = (ordered[middle - 1] + ordered[middle]) / 2.0
    if not math.isfinite(result):
        raise EvidenceError(f"{label} is not finite")
    return result


def _nearest_rank(values: Sequence[float], percentile: float, label: str) -> float:
    if not values:
        raise EvidenceError(f"{label} cannot be computed from an empty sample set")
    index = math.ceil(percentile * len(values)) - 1
    return sorted(values)[index]


def _require_nearly_equal(value: object, expected: float, label: str) -> float:
    actual = _number(value, label)
    scale = max(1.0, abs(actual), abs(expected))
    if abs(actual - expected) > 1.0e-9 * scale:
        raise EvidenceError(
            f"{label} does not match raw machine evidence "
            f"(actual={actual}, expected={expected})"
        )
    return actual


def _regression_ratio(base: float, head: float, direction: str, label: str) -> float:
    result = (
        (base - head) / base
        if direction == "higher"
        else (head - base) / base
    )
    if not math.isfinite(result):
        raise EvidenceError(f"{label} is not finite")
    return result


def _validated_benchmark_result(
    report: Mapping[str, Any],
    *,
    label: str,
    expected_sha: str,
    enforce_absolute_slo: bool,
) -> dict[str, object]:
    if report.get("schema_version") != 1:
        raise EvidenceError(f"{label} schema_version must be 1")
    if _boolean(report.get("success"), f"{label} success") is not True:
        raise EvidenceError(f"{label} did not pass")
    if _string(report.get("profile"), f"{label} profile") != "ci":
        raise EvidenceError(f"{label} is not the ci profile")
    if _string(report.get("build_type"), f"{label} build_type") != "Release":
        raise EvidenceError(f"{label} is not a Release build")
    if _sha(report.get("git_sha"), f"{label} git_sha") != expected_sha:
        raise EvidenceError(f"{label} SHA binding mismatch")
    compiler = _string(report.get("compiler"), f"{label} compiler")
    operating_system = _string(report.get("os"), f"{label} os")
    if compiler.lower() == "unknown" or operating_system.lower() == "unknown":
        raise EvidenceError(f"{label} has unknown machine identity")
    workload = dict(_mapping(report.get("workload"), f"{label} workload"))
    if not workload:
        raise EvidenceError(f"{label} workload is empty")

    raw_metrics = _sequence(report.get("metrics"), f"{label} metrics")
    metrics: dict[str, Mapping[str, Any]] = {}
    for index, raw_metric in enumerate(raw_metrics):
        metric = _mapping(raw_metric, f"{label} metric {index}")
        name = _string(metric.get("name"), f"{label} metric {index} name")
        if name in metrics:
            raise EvidenceError(f"{label} contains duplicate metric: {name}")
        metrics[name] = metric
    required_directions = {
        name: direction
        for name, (direction, _) in EXPECTED_BENCHMARK_ABSOLUTE_SLOS.items()
    }
    required_directions.update(
        {
            name: direction
            for name, (direction, _) in EXPECTED_BENCHMARK_METRICS.items()
        }
    )
    metric_evidence: dict[str, dict[str, object]] = {}
    for name, direction in required_directions.items():
        metric = metrics.get(name)
        if metric is None:
            raise EvidenceError(f"{label} is missing required metric: {name}")
        if _string(metric.get("direction"), f"{label} metric {name} direction") != direction:
            raise EvidenceError(f"{label} metric {name} direction mismatch")
        samples = _sequence(metric.get("samples"), f"{label} metric {name} samples")
        if not samples:
            raise EvidenceError(f"{label} metric {name} has no samples")
        numeric_samples: list[float] = []
        for sample_index, sample in enumerate(samples):
            numeric_sample = _number(
                sample,
                f"{label} metric {name} sample {sample_index}",
            )
            if numeric_sample <= 0:
                raise EvidenceError(f"{label} metric {name} has a non-positive sample")
            numeric_samples.append(numeric_sample)
        recomputed_median = _median(
            numeric_samples,
            f"{label} metric {name} median",
        )
        reported_median = _require_nearly_equal(
            metric.get("median"),
            recomputed_median,
            f"{label} metric {name} median",
        )
        if reported_median <= 0:
            raise EvidenceError(f"{label} metric {name} has a non-positive median")
        metric_evidence[name] = {
            "median": recomputed_median,
            "samples": numeric_samples,
        }

    pause_samples = _sequence(report.get("gc_pause_samples_us"), f"{label} gc_pause_samples_us")
    if not pause_samples:
        raise EvidenceError(f"{label} has no GC pause samples")
    numeric_pause_samples: list[float] = []
    positive_pause_samples = 0
    for index, sample in enumerate(pause_samples):
        pause = _number(sample, f"{label} GC pause sample {index}")
        if pause < 0:
            raise EvidenceError(f"{label} has a negative GC pause sample")
        if pause > 0:
            positive_pause_samples += 1
        numeric_pause_samples.append(pause)
    if positive_pause_samples == 0:
        raise EvidenceError(f"{label} has no measurable GC pause sample")
    expected_gc_p99 = _nearest_rank(
        numeric_pause_samples,
        0.99,
        f"{label} GC pause P99",
    )
    _require_nearly_equal(
        metric_evidence["gc_pause_p99_us"]["median"],
        expected_gc_p99,
        f"{label} gc_pause_p99_us median",
    )
    _require_nearly_equal(
        metric_evidence["gc_pause_max_us"]["median"],
        max(numeric_pause_samples),
        f"{label} gc_pause_max_us median",
    )

    absolute_slo_evidence: dict[str, dict[str, object]] = {}
    if enforce_absolute_slo:
        if operating_system != "Linux":
            raise EvidenceError(f"{label} absolute SLO evidence is not from Linux")
        for name, (direction, threshold) in EXPECTED_BENCHMARK_ABSOLUTE_SLOS.items():
            actual = _number(
                metric_evidence[name]["median"],
                f"{label} absolute SLO metric {name}",
            )
            passed = actual >= threshold if direction == "higher" else actual <= threshold
            if not passed:
                comparison = "below minimum" if direction == "higher" else "above maximum"
                raise EvidenceError(
                    f"{label} absolute SLO metric {name} is {actual}, "
                    f"{comparison} {threshold}"
                )
            absolute_slo_evidence[name] = {
                "direction": direction,
                "threshold": threshold,
                "actual": actual,
                "passed": True,
            }
    return {
        "compiler": compiler,
        "os": operating_system,
        "workload": workload,
        "metrics": metric_evidence,
        "gc_pause_samples_us": numeric_pause_samples,
        "absolute_slo": absolute_slo_evidence,
    }


def _benchmark_run_metric_value(
    report: Mapping[str, object],
    name: str,
    label: str,
) -> float:
    metrics = _mapping(report.get("metrics"), f"{label} metrics")
    metric = _mapping(metrics.get(name), f"{label} metric {name}")
    return _number(metric.get("median"), f"{label} metric {name} median")


def _aggregate_benchmark_metric(
    reports: Sequence[Mapping[str, object]],
    name: str,
    label: str,
) -> tuple[float, int]:
    if name == "gc_pause_p99_us":
        pause_samples: list[float] = []
        for index, report in enumerate(reports):
            raw_samples = _sequence(
                report.get("gc_pause_samples_us"),
                f"{label} result {index} GC pause samples",
            )
            pause_samples.extend(
                _number(
                    sample,
                    f"{label} result {index} GC pause sample {sample_index}",
                )
                for sample_index, sample in enumerate(raw_samples)
            )
        return (
            _nearest_rank(pause_samples, 0.99, f"{label} pooled GC pause P99"),
            len(pause_samples),
        )

    run_medians = [
        _benchmark_run_metric_value(
            report,
            name,
            f"{label} result {index}",
        )
        for index, report in enumerate(reports)
    ]
    return _median(run_medians, f"{label} metric {name} aggregate"), len(run_medians)


def _validated_benchmark_payload(
    zipped: zipfile.ZipFile,
    files: Mapping[str, zipfile.ZipInfo],
    artifact_name: str,
    candidate_sha: str,
) -> dict[str, object]:
    comparison = _read_zip_json(zipped, files, "comparison.json", artifact_name)
    if comparison.get("schemaVersion") != 3:
        raise EvidenceError("runtime benchmark comparison schemaVersion must be 3")
    if _boolean(comparison.get("success"), "runtime benchmark comparison success") is not True:
        raise EvidenceError("runtime benchmark comparison did not pass")
    base_sha = _sha(comparison.get("baseSha"), "runtime benchmark comparison baseSha")
    head_sha = _sha(comparison.get("headSha"), "runtime benchmark comparison headSha")
    if head_sha != candidate_sha:
        raise EvidenceError("runtime benchmark comparison head SHA does not match candidate")
    if base_sha == head_sha:
        raise EvidenceError("runtime benchmark comparison base and head SHAs are identical")
    runs_per_revision = _integer(
        comparison.get("runsPerRevision"),
        "runtime benchmark runsPerRevision",
    )
    if runs_per_revision not in (3, 5):
        raise EvidenceError("runtime benchmark must contain three or five runs per revision")
    if comparison.get("minimumRunsPerRevision") != 3 or comparison.get("maximumRunsPerRevision") != 5:
        raise EvidenceError("runtime benchmark run-count policy mismatch")
    for field, expected in EXPECTED_BENCHMARK_POLICY.items():
        if _string(comparison.get(field), f"runtime benchmark {field}") != expected:
            raise EvidenceError(f"runtime benchmark {field} policy mismatch")
    runtime_inputs = tuple(
        _string_array(comparison.get("runtimeInputPaths"), "runtime benchmark runtimeInputPaths")
    )
    if runtime_inputs != EXPECTED_BENCHMARK_RUNTIME_INPUTS:
        raise EvidenceError("runtime benchmark runtime input policy mismatch")
    runtime_inputs_equivalent = _boolean(
        comparison.get("runtimeInputsEquivalent"),
        "runtime benchmark runtimeInputsEquivalent",
    )
    runtime_input_diff_paths = tuple(
        _string_array(
            comparison.get("runtimeInputDiffPaths"),
            "runtime benchmark runtimeInputDiffPaths",
        )
    )
    if runtime_inputs_equivalent == bool(runtime_input_diff_paths):
        raise EvidenceError("runtime benchmark input equivalence contradicts its diff paths")
    confirmation_triggered = _boolean(
        comparison.get("confirmationTriggered"),
        "runtime benchmark confirmationTriggered",
    )
    confirmation_recommended = _boolean(
        comparison.get("confirmationRecommended"),
        "runtime benchmark confirmationRecommended",
    )
    if confirmation_triggered != (runs_per_revision == 5):
        raise EvidenceError("runtime benchmark confirmation state contradicts run count")
    failures = _sequence(comparison.get("failures"), "runtime benchmark failures")
    if failures:
        raise EvidenceError("runtime benchmark comparison contains effective failures")

    raw_metrics = _sequence(comparison.get("metrics"), "runtime benchmark comparison metrics")
    metrics: dict[str, Mapping[str, Any]] = {}
    metric_order: list[str] = []
    for index, raw_metric in enumerate(raw_metrics):
        metric = _mapping(raw_metric, f"runtime benchmark comparison metric {index}")
        name = _string(metric.get("name"), f"runtime benchmark comparison metric {index} name")
        if name in metrics:
            raise EvidenceError(f"runtime benchmark comparison contains duplicate metric: {name}")
        metrics[name] = metric
        metric_order.append(name)
    if tuple(metric_order) != tuple(EXPECTED_BENCHMARK_METRICS):
        raise EvidenceError("runtime benchmark comparison metric set or order mismatch")
    for name, (direction, limit) in EXPECTED_BENCHMARK_METRICS.items():
        metric = metrics[name]
        expected_metric_fields = {
            "name",
            "direction",
            "base",
            "head",
            "baseSampleCount",
            "headSampleCount",
            "regressionRatio",
            "pairedRunCount",
            "pairedRegressionRatios",
            "pairedRunsWithinLimit",
            "pairedRunsOverLimit",
            "pairedOutcomeMixed",
            "maximumRegressionRatio",
            "passed",
        }
        if set(metric) != expected_metric_fields:
            raise EvidenceError(f"runtime benchmark metric {name} field set mismatch")
        if _string(metric.get("direction"), f"runtime benchmark metric {name} direction") != direction:
            raise EvidenceError(f"runtime benchmark metric {name} direction mismatch")
        actual_limit = _number(
            metric.get("maximumRegressionRatio"),
            f"runtime benchmark metric {name} maximumRegressionRatio",
        )
        if abs(actual_limit - limit) > 1e-12:
            raise EvidenceError(f"runtime benchmark metric {name} limit mismatch")

    expected_files = {
        "evidence-metadata.json",
        "comparison.json",
        "run-order.json",
        *{f"base-{index}.json" for index in range(1, runs_per_revision + 1)},
        *{f"head-{index}.json" for index in range(1, runs_per_revision + 1)},
    }
    if confirmation_triggered:
        expected_files.add("comparison-initial.json")
    _require_exact_file_set(files, expected_files, artifact_name)

    run_order = _read_zip_json(zipped, files, "run-order.json", artifact_name)
    if run_order.get("schemaVersion") != 2:
        raise EvidenceError("runtime benchmark run-order schemaVersion must be 2")
    if _sha(run_order.get("baseSha"), "runtime benchmark run-order baseSha") != base_sha:
        raise EvidenceError("runtime benchmark run-order base SHA mismatch")
    if _sha(run_order.get("headSha"), "runtime benchmark run-order headSha") != head_sha:
        raise EvidenceError("runtime benchmark run-order head SHA mismatch")
    _integer(run_order.get("runnerPid"), "runtime benchmark run-order runnerPid")
    if tuple(
        _string_array(run_order.get("runtimeInputPaths"), "runtime benchmark run-order runtimeInputPaths")
    ) != EXPECTED_BENCHMARK_RUNTIME_INPUTS:
        raise EvidenceError("runtime benchmark run-order runtime input policy mismatch")
    run_order_diff_paths = tuple(
        _string_array(
            run_order.get("runtimeInputDiffPaths"),
            "runtime benchmark run-order runtimeInputDiffPaths",
        )
    )
    if run_order_diff_paths != runtime_input_diff_paths:
        raise EvidenceError("runtime benchmark run-order runtime input diff mismatch")
    if _boolean(
        run_order.get("runtimeInputsEquivalent"),
        "runtime benchmark run-order runtimeInputsEquivalent",
    ) != runtime_inputs_equivalent:
        raise EvidenceError("runtime benchmark run-order input equivalence mismatch")
    if _boolean(
        run_order.get("confirmationTriggered"),
        "runtime benchmark run-order confirmationTriggered",
    ) != confirmation_triggered:
        raise EvidenceError("runtime benchmark run-order confirmation state mismatch")
    runs = _sequence(run_order.get("runs"), "runtime benchmark run-order runs")
    if len(runs) != 2 * runs_per_revision:
        raise EvidenceError("runtime benchmark run-order has the wrong execution count")
    previous_start: datetime | None = None
    for pair in range(runs_per_revision):
        expected_revisions = ("base", "head") if pair % 2 == 0 else ("head", "base")
        for offset, expected_revision in enumerate(expected_revisions):
            index = 2 * pair + offset
            run = _mapping(runs[index], f"runtime benchmark run-order run {index}")
            if run.get("pair") != pair:
                raise EvidenceError("runtime benchmark run-order pair index mismatch")
            revision = _string(run.get("revision"), f"runtime benchmark run-order run {index} revision")
            if revision != expected_revision:
                raise EvidenceError("runtime benchmark run-order does not alternate base/head")
            expected_sha = base_sha if revision == "base" else head_sha
            if _sha(run.get("sha"), f"runtime benchmark run-order run {index} sha") != expected_sha:
                raise EvidenceError("runtime benchmark run-order revision SHA mismatch")
            result_path = _string(
                run.get("resultPath"),
                f"runtime benchmark run-order run {index} resultPath",
            )
            expected_basename = f"{revision}-{pair + 1}.json"
            if result_path.replace("\\", "/").rsplit("/", 1)[-1] != expected_basename:
                raise EvidenceError("runtime benchmark run-order result path mismatch")
            started = _timestamp(
                run.get("startedAt"),
                f"runtime benchmark run-order run {index} startedAt",
            )
            ended = _timestamp(
                run.get("endedAt"),
                f"runtime benchmark run-order run {index} endedAt",
            )
            if ended < started or (previous_start is not None and started < previous_start):
                raise EvidenceError("runtime benchmark run-order timestamps are inconsistent")
            previous_start = started

    compiler = _string(comparison.get("compiler"), "runtime benchmark compiler")
    operating_system = _string(comparison.get("os"), "runtime benchmark os")
    reference_workload: dict[str, object] | None = None
    results_by_revision: dict[str, list[Mapping[str, object]]] = {
        "base": [],
        "head": [],
    }
    for revision, expected_sha in (("base", base_sha), ("head", head_sha)):
        for index in range(1, runs_per_revision + 1):
            path = f"{revision}-{index}.json"
            result_summary = _validated_benchmark_result(
                _read_zip_json(zipped, files, path, artifact_name),
                label=f"runtime benchmark {path}",
                expected_sha=expected_sha,
                enforce_absolute_slo=revision == "head",
            )
            if result_summary["compiler"] != compiler or result_summary["os"] != operating_system:
                raise EvidenceError("runtime benchmark result platform differs from comparison")
            workload = result_summary["workload"]
            if reference_workload is None:
                reference_workload = workload
            elif workload != reference_workload:
                raise EvidenceError("runtime benchmark result workloads differ")
            results_by_revision[revision].append(result_summary)

    failing_metrics: list[str] = []
    mixed_failing_metrics: list[str] = []
    recomputed_metrics: dict[str, dict[str, object]] = {}
    for name, (direction, limit) in EXPECTED_BENCHMARK_METRICS.items():
        metric = metrics[name]
        base_value, base_sample_count = _aggregate_benchmark_metric(
            results_by_revision["base"],
            name,
            "runtime benchmark base",
        )
        head_value, head_sample_count = _aggregate_benchmark_metric(
            results_by_revision["head"],
            name,
            "runtime benchmark head",
        )
        if base_value <= 0 or head_value <= 0:
            raise EvidenceError(f"runtime benchmark metric {name} is not positive")

        if name == "gc_pause_p99_us":
            regression = _regression_ratio(
                base_value,
                head_value,
                direction,
                f"runtime benchmark metric {name} regression",
            )
            paired_ratios: list[float] = []
        else:
            paired_ratios = []
            for index, (base_result, head_result) in enumerate(
                zip(
                    results_by_revision["base"],
                    results_by_revision["head"],
                    strict=True,
                )
            ):
                base_run_value = _benchmark_run_metric_value(
                    base_result,
                    name,
                    f"runtime benchmark base result {index}",
                )
                head_run_value = _benchmark_run_metric_value(
                    head_result,
                    name,
                    f"runtime benchmark head result {index}",
                )
                paired_ratios.append(
                    _regression_ratio(
                        base_run_value,
                        head_run_value,
                        direction,
                        f"runtime benchmark metric {name} pair {index} regression",
                    )
                )
            regression = _median(
                paired_ratios,
                f"runtime benchmark metric {name} paired regression",
            )

        paired_within_limit = sum(ratio <= limit for ratio in paired_ratios)
        paired_over_limit = len(paired_ratios) - paired_within_limit
        paired_outcome_mixed = (
            paired_within_limit > 0 and paired_over_limit > 0
        )
        computed_passed = regression <= limit
        if not computed_passed:
            failing_metrics.append(name)
            if paired_outcome_mixed:
                mixed_failing_metrics.append(name)

        _require_nearly_equal(
            metric.get("base"),
            base_value,
            f"runtime benchmark metric {name} base",
        )
        _require_nearly_equal(
            metric.get("head"),
            head_value,
            f"runtime benchmark metric {name} head",
        )
        if (
            _nonnegative_integer(
                metric.get("baseSampleCount"),
                f"runtime benchmark metric {name} baseSampleCount",
            )
            != base_sample_count
            or _nonnegative_integer(
                metric.get("headSampleCount"),
                f"runtime benchmark metric {name} headSampleCount",
            )
            != head_sample_count
        ):
            raise EvidenceError(f"runtime benchmark metric {name} sample count mismatch")
        _require_nearly_equal(
            metric.get("regressionRatio"),
            regression,
            f"runtime benchmark metric {name} regressionRatio",
        )
        if _nonnegative_integer(
            metric.get("pairedRunCount"),
            f"runtime benchmark metric {name} pairedRunCount",
        ) != len(paired_ratios):
            raise EvidenceError(f"runtime benchmark metric {name} paired run count mismatch")
        reported_paired_ratios = _sequence(
            metric.get("pairedRegressionRatios"),
            f"runtime benchmark metric {name} pairedRegressionRatios",
        )
        sorted_paired_ratios = sorted(paired_ratios)
        if len(reported_paired_ratios) != len(sorted_paired_ratios):
            raise EvidenceError(f"runtime benchmark metric {name} paired ratio count mismatch")
        for index, expected_ratio in enumerate(sorted_paired_ratios):
            _require_nearly_equal(
                reported_paired_ratios[index],
                expected_ratio,
                f"runtime benchmark metric {name} paired ratio {index}",
            )
        if (
            _nonnegative_integer(
                metric.get("pairedRunsWithinLimit"),
                f"runtime benchmark metric {name} pairedRunsWithinLimit",
            )
            != paired_within_limit
            or _nonnegative_integer(
                metric.get("pairedRunsOverLimit"),
                f"runtime benchmark metric {name} pairedRunsOverLimit",
            )
            != paired_over_limit
            or _boolean(
                metric.get("pairedOutcomeMixed"),
                f"runtime benchmark metric {name} pairedOutcomeMixed",
            )
            != paired_outcome_mixed
        ):
            raise EvidenceError(f"runtime benchmark metric {name} paired outcome mismatch")
        if _boolean(
            metric.get("passed"),
            f"runtime benchmark metric {name} passed",
        ) != computed_passed:
            raise EvidenceError(
                f"runtime benchmark metric {name} pass result contradicts raw evidence"
            )
        recomputed_metrics[name] = {
            "direction": direction,
            "base": base_value,
            "head": head_value,
            "regression_ratio": regression,
            "maximum_regression_ratio": limit,
            "passed": computed_passed,
            "base_sample_count": base_sample_count,
            "head_sample_count": head_sample_count,
            "paired_run_count": len(paired_ratios),
        }

    computed_confirmation_recommended = (
        not runtime_inputs_equivalent
        and runs_per_revision == 3
        and bool(mixed_failing_metrics)
    )
    if confirmation_recommended != computed_confirmation_recommended:
        raise EvidenceError("runtime benchmark confirmation recommendation is inconsistent")
    if confirmation_recommended:
        raise EvidenceError("runtime benchmark still recommends confirmation")

    observed_failures = _sequence(
        comparison.get("observedThresholdFailures"),
        "runtime benchmark observedThresholdFailures",
    )
    if len(observed_failures) != len(failing_metrics):
        raise EvidenceError("runtime benchmark observed threshold failure count mismatch")
    for index, failure in enumerate(observed_failures):
        _string(failure, f"runtime benchmark observed threshold failure {index}")
    reported_mixed_failures = tuple(
        _string_array(
            comparison.get("mixedFailingMetrics"),
            "runtime benchmark mixedFailingMetrics",
        )
    )
    if reported_mixed_failures != tuple(mixed_failing_metrics):
        raise EvidenceError("runtime benchmark mixed failing metric set mismatch")

    if runtime_inputs_equivalent:
        expected_decision = "equivalent-runtime-inputs"
    elif not failing_metrics:
        expected_decision = "thresholds-passed"
    elif computed_confirmation_recommended:
        expected_decision = "confirmation-required"
    else:
        expected_decision = "thresholds-failed"
    if _string(comparison.get("decision"), "runtime benchmark decision") != expected_decision:
        raise EvidenceError("runtime benchmark decision is inconsistent with raw evidence")
    if expected_decision not in {"equivalent-runtime-inputs", "thresholds-passed"}:
        raise EvidenceError("runtime benchmark raw evidence does not pass release policy")

    if confirmation_triggered:
        initial = _read_zip_json(
            zipped,
            files,
            "comparison-initial.json",
            artifact_name,
        )
        if initial.get("schemaVersion") != 3:
            raise EvidenceError("runtime benchmark initial comparison schemaVersion must be 3")
        if _sha(initial.get("baseSha"), "runtime benchmark initial baseSha") != base_sha:
            raise EvidenceError("runtime benchmark initial comparison base SHA mismatch")
        if _sha(initial.get("headSha"), "runtime benchmark initial headSha") != head_sha:
            raise EvidenceError("runtime benchmark initial comparison head SHA mismatch")

    return {
        "schema_version": 3,
        "success": True,
        "decision": expected_decision,
        "base_sha": base_sha,
        "head_sha": head_sha,
        "runs_per_revision": runs_per_revision,
        "confirmation_triggered": confirmation_triggered,
        "runtime_inputs_equivalent": runtime_inputs_equivalent,
        "metric_regressions_recomputed": True,
        "metrics": recomputed_metrics,
        "observed_failure_metrics": failing_metrics,
        "absolute_slo": {
            "policy": {
                **EXPECTED_BENCHMARK_ABSOLUTE_SCOPE,
                "metrics": {
                    name: {
                        "direction": direction,
                        "threshold": threshold,
                    }
                    for name, (direction, threshold) in (
                        EXPECTED_BENCHMARK_ABSOLUTE_SLOS.items()
                    )
                },
            },
            "head_results": [
                _mapping(
                    result.get("absolute_slo"),
                    f"runtime benchmark head result {index} absolute SLO",
                )
                for index, result in enumerate(results_by_revision["head"])
            ],
            "passed": True,
        },
    }


def _validated_runtime_soak_payload(
    zipped: zipfile.ZipFile,
    files: Mapping[str, zipfile.ZipInfo],
    artifact_name: str,
) -> dict[str, object]:
    _require_exact_file_set(
        files,
        {
            "evidence-metadata.json",
            "runtime-soak.json",
            "native-module-soak.json",
        },
        artifact_name,
    )
    runtime = _read_zip_json(zipped, files, "runtime-soak.json", artifact_name)
    expected_runtime_fields = {
        "schema",
        "status",
        "iterations",
        "states_created",
        "states_closed",
        "coroutine_cycles",
        "weak_values_collected",
        "finalizers_observed",
        "cancellation_checks",
        "max_cancellation_latency_us",
        "max_allocator_peak_bytes",
        "duration_ms",
        "error",
    }
    if set(runtime) != expected_runtime_fields or runtime.get("schema") != 1:
        raise EvidenceError("runtime soak result schema or field set mismatch")
    if _string(runtime.get("status"), "runtime soak status") != "passed":
        raise EvidenceError("runtime soak result did not pass")
    if runtime.get("error") != "":
        raise EvidenceError("runtime soak result contains an error")
    iterations = _integer(runtime.get("iterations"), "runtime soak iterations")
    states_created = _integer(runtime.get("states_created"), "runtime soak states_created")
    states_closed = _integer(runtime.get("states_closed"), "runtime soak states_closed")
    coroutine_cycles = _integer(
        runtime.get("coroutine_cycles"),
        "runtime soak coroutine_cycles",
    )
    weak_values = _integer(
        runtime.get("weak_values_collected"),
        "runtime soak weak_values_collected",
    )
    finalizers = _integer(
        runtime.get("finalizers_observed"),
        "runtime soak finalizers_observed",
    )
    cancellation_checks = _integer(
        runtime.get("cancellation_checks"),
        "runtime soak cancellation_checks",
    )
    cancellation_latency = _nonnegative_integer(
        runtime.get("max_cancellation_latency_us"),
        "runtime soak max_cancellation_latency_us",
    )
    allocator_peak = _integer(
        runtime.get("max_allocator_peak_bytes"),
        "runtime soak max_allocator_peak_bytes",
    )
    duration_ms = _integer(runtime.get("duration_ms"), "runtime soak duration_ms")
    if states_created != 2 * iterations or states_closed != states_created:
        raise EvidenceError("runtime soak state lifecycle counts are inconsistent")
    if (
        coroutine_cycles != 16 * iterations
        or weak_values != iterations
        or finalizers != 32 * iterations
        or cancellation_checks != iterations
    ):
        raise EvidenceError("runtime soak workload counters are inconsistent")
    if cancellation_latency > 250_000:
        raise EvidenceError("runtime soak cancellation latency exceeds the release policy")
    if allocator_peak <= 0:
        raise EvidenceError("runtime soak allocator evidence is empty")
    if duration_ms < 45 * 60 * 1000:
        raise EvidenceError("runtime soak measured duration is shorter than 45 minutes")

    native = _read_zip_json(zipped, files, "native-module-soak.json", artifact_name)
    if set(native) != {"schema", "status", "iterations"} or native.get("schema") != 1:
        raise EvidenceError("native-module soak result schema or field set mismatch")
    if _string(native.get("status"), "native-module soak status") != "passed":
        raise EvidenceError("native-module soak result did not pass")
    native_iterations = _integer(
        native.get("iterations"),
        "native-module soak iterations",
    )
    if native_iterations < 1000:
        raise EvidenceError("native-module soak result has fewer than 1000 iterations")
    return {
        "schema_version": 1,
        "status": "passed",
        "duration_ms": duration_ms,
        "iterations": iterations,
        "native_module_iterations": native_iterations,
    }


def _validated_long_fuzz_payload(
    zipped: zipfile.ZipFile,
    files: Mapping[str, zipfile.ZipInfo],
    artifact_name: str,
) -> dict[str, object]:
    allowed = {
        path
        for path in files
        if path == "evidence-metadata.json"
        or path.startswith("fuzz-artifacts/")
        or path.startswith("fuzz-corpus/")
    }
    if allowed != set(files):
        raise EvidenceError(f"artifact {artifact_name} contains an unexpected payload path")
    required_logs = {f"fuzz-artifacts/{target}.log" for target in EXPECTED_FUZZ_TARGETS}
    actual_artifact_files = {path for path in files if path.startswith("fuzz-artifacts/")}
    if actual_artifact_files != required_logs:
        missing = sorted(required_logs - actual_artifact_files)
        extra = sorted(actual_artifact_files - required_logs)
        raise EvidenceError(
            f"artifact {artifact_name} fuzz log set mismatch; missing={missing}, extra={extra}"
        )

    target_evidence: dict[str, dict[str, int]] = {}
    for target in EXPECTED_FUZZ_TARGETS:
        corpus_prefix = f"fuzz-corpus/{target}/"
        corpus_files = [
            info
            for path, info in files.items()
            if path.startswith(corpus_prefix)
        ]
        if not corpus_files or not any(info.file_size > 0 for info in corpus_files):
            raise EvidenceError(f"artifact {artifact_name} has no corpus evidence for {target}")
        log_path = f"fuzz-artifacts/{target}.log"
        log_bytes = _read_zip_file(
            zipped,
            files,
            log_path,
            artifact_name,
            maximum_bytes=MAX_FUZZ_LOG_BYTES,
        )
        try:
            log = log_bytes.decode("utf-8")
        except UnicodeDecodeError as error:
            raise EvidenceError(f"artifact {artifact_name} fuzz log is not UTF-8: {target}") from error
        lowered = log.lower()
        for marker in (
            "error: addresssanitizer",
            "summary: addresssanitizer",
            "summary: undefinedbehaviorsanitizer",
            "runtime error:",
        ):
            if marker in lowered:
                raise EvidenceError(f"artifact {artifact_name} fuzz log reports sanitizer failure: {target}")
        duration_match = _FUZZ_DONE_RE.search(log)
        executions_match = _FUZZ_EXECUTIONS_RE.search(log)
        if duration_match is None or executions_match is None or "DONE" not in log:
            raise EvidenceError(f"artifact {artifact_name} fuzz log is incomplete: {target}")
        reported_seconds = int(duration_match.group(1))
        executions = int(executions_match.group(1))
        if reported_seconds <= 0 or executions <= 0:
            raise EvidenceError(f"artifact {artifact_name} fuzz log has empty results: {target}")
        target_evidence[target] = {
            "reported_seconds": reported_seconds,
            "executions": executions,
        }
    return {
        "targets": target_evidence,
    }


def _read_artifact_archive(
    archive: bytes,
    artifact_name: str,
    candidate_sha: str,
) -> tuple[Mapping[str, Any], dict[str, object]]:
    try:
        with zipfile.ZipFile(io.BytesIO(archive)) as zipped:
            files = _catalog_zip_files(zipped, artifact_name)
            metadata_matches = [
                path for path in files if PurePosixPath(path).name == "evidence-metadata.json"
            ]
            if len(metadata_matches) != 1:
                raise EvidenceError(
                    f"artifact {artifact_name} must contain exactly one "
                    "evidence-metadata.json basename"
                )
            if metadata_matches[0] != "evidence-metadata.json":
                raise EvidenceError(
                    f"artifact {artifact_name} evidence-metadata.json must be at the ZIP root"
                )
            metadata = _decode_json_bytes(
                _read_zip_file(
                    zipped,
                    files,
                    "evidence-metadata.json",
                    artifact_name,
                    maximum_bytes=1024 * 1024,
                ),
                f"artifact {artifact_name} evidence metadata",
            )
            if artifact_name == "component-coverage":
                payload = _validated_coverage_payload(zipped, files, artifact_name)
            elif artifact_name == "runtime-benchmark-evidence":
                payload = _validated_benchmark_payload(
                    zipped,
                    files,
                    artifact_name,
                    candidate_sha,
                )
            elif artifact_name == "runtime-soak-evidence":
                payload = _validated_runtime_soak_payload(zipped, files, artifact_name)
            elif artifact_name == "long-fuzz-evidence":
                payload = _validated_long_fuzz_payload(zipped, files, artifact_name)
            else:
                raise EvidenceError(f"artifact {artifact_name} has no payload policy")
    except EvidenceError:
        raise
    except (zipfile.BadZipFile, KeyError, RuntimeError, OSError) as error:
        raise EvidenceError(f"artifact {artifact_name} is not a readable ZIP archive") from error
    return metadata, payload


def _validated_workflow_parameters(
    artifact_name: str,
    value: object,
) -> dict[str, object]:
    parameters = _mapping(value, f"artifact {artifact_name} metadata.parameters")
    if artifact_name in CI_ARTIFACTS:
        if parameters:
            raise EvidenceError(f"artifact {artifact_name} CI parameters must be empty")
        return {}
    if artifact_name == "runtime-soak-evidence":
        if set(parameters) != {"soak_minutes", "native_module_iterations"}:
            raise EvidenceError("runtime soak metadata parameter set is incomplete")
        soak_minutes = _integer(
            parameters.get("soak_minutes"),
            "runtime soak metadata soak_minutes",
        )
        native_iterations = _integer(
            parameters.get("native_module_iterations"),
            "runtime soak metadata native_module_iterations",
        )
        if soak_minutes < 45:
            raise EvidenceError("runtime soak evidence is shorter than 45 minutes")
        if native_iterations < 1000:
            raise EvidenceError("native-module evidence has fewer than 1000 iterations")
        return {
            "soak_minutes": soak_minutes,
            "native_module_iterations": native_iterations,
        }
    if artifact_name == "long-fuzz-evidence":
        if set(parameters) != {"fuzz_seconds_per_target", "fuzz_targets"}:
            raise EvidenceError("long fuzz metadata parameter set is incomplete")
        seconds = _integer(
            parameters.get("fuzz_seconds_per_target"),
            "long fuzz metadata fuzz_seconds_per_target",
        )
        targets_value = _sequence(
            parameters.get("fuzz_targets"),
            "long fuzz metadata fuzz_targets",
        )
        targets = tuple(
            _string(target, f"long fuzz target {index}")
            for index, target in enumerate(targets_value)
        )
        if seconds < 600:
            raise EvidenceError("long fuzz evidence is shorter than 600 seconds per target")
        if targets != EXPECTED_FUZZ_TARGETS:
            raise EvidenceError("long fuzz evidence target set or order is incomplete")
        return {
            "fuzz_seconds_per_target": seconds,
            "fuzz_targets": list(targets),
        }
    raise EvidenceError(f"artifact {artifact_name} has no workflow evidence parameter policy")


def _validated_workflow_evidence_metadata(
    metadata: Mapping[str, Any],
    *,
    artifact_name: str,
    repository: str,
    candidate_sha: str,
    run: Mapping[str, object],
    artifact_created_at: datetime,
) -> dict[str, object]:
    actual_fields = set(metadata)
    if actual_fields != WORKFLOW_EVIDENCE_FIELDS:
        missing = sorted(WORKFLOW_EVIDENCE_FIELDS - actual_fields)
        extra = sorted(actual_fields - WORKFLOW_EVIDENCE_FIELDS)
        raise EvidenceError(
            f"artifact {artifact_name} metadata field set mismatch; "
            f"missing={missing}, extra={extra}"
        )
    if _string(metadata.get("schema"), "workflow evidence schema") != WORKFLOW_EVIDENCE_SCHEMA:
        raise EvidenceError(f"artifact {artifact_name} workflow evidence schema mismatch")
    if _string(metadata.get("kind"), "workflow evidence kind") != artifact_name:
        raise EvidenceError(f"artifact {artifact_name} workflow evidence kind mismatch")
    if _string(metadata.get("repository"), "workflow evidence repository") != repository:
        raise EvidenceError(f"artifact {artifact_name} workflow evidence repository mismatch")
    if _sha(metadata.get("candidate_sha"), "workflow evidence candidate_sha") != candidate_sha:
        raise EvidenceError(f"artifact {artifact_name} workflow evidence SHA mismatch")
    if _integer(metadata.get("run_id"), "workflow evidence run_id") != run["id"]:
        raise EvidenceError(f"artifact {artifact_name} workflow evidence run ID mismatch")
    if _integer(metadata.get("run_attempt"), "workflow evidence run_attempt") != run["attempt"]:
        raise EvidenceError(f"artifact {artifact_name} workflow evidence run attempt mismatch")
    event = _string(metadata.get("event"), "workflow evidence event")
    if event != run["event"]:
        raise EvidenceError(f"artifact {artifact_name} workflow evidence event mismatch")
    workflow_ref = _string(metadata.get("workflow_ref"), "workflow evidence workflow_ref")
    expected_workflow_ref = (
        f"{repository}/.github/workflows/{run['workflow']}@refs/heads/{MAIN_BRANCH}"
    )
    if workflow_ref != expected_workflow_ref:
        raise EvidenceError(f"artifact {artifact_name} workflow reference mismatch")
    job = _string(metadata.get("job"), "workflow evidence job")
    if job != EXPECTED_ARTIFACT_JOBS.get(artifact_name):
        raise EvidenceError(f"artifact {artifact_name} workflow evidence job mismatch")
    if _string(metadata.get("result"), "workflow evidence result") != "passed":
        raise EvidenceError(f"artifact {artifact_name} workflow evidence did not pass")
    created_at = _timestamp(metadata.get("created_at"), "workflow evidence created_at")
    run_created_at = _timestamp(run.get("created_at"), f"run {run['id']} created_at")
    if created_at < run_created_at or created_at > artifact_created_at:
        raise EvidenceError(f"artifact {artifact_name} workflow evidence timestamp is inconsistent")
    parameters = _validated_workflow_parameters(artifact_name, metadata.get("parameters"))
    return {
        "schema": WORKFLOW_EVIDENCE_SCHEMA,
        "kind": artifact_name,
        "repository": repository,
        "candidate_sha": candidate_sha,
        "run_id": run["id"],
        "run_attempt": run["attempt"],
        "event": event,
        "workflow_ref": workflow_ref,
        "job": job,
        "result": "passed",
        "created_at": _format_timestamp(created_at),
        "parameters": parameters,
    }


def _collection(
    api: RestApi,
    path: str,
    key: str,
    params: Mapping[str, object] | None = None,
) -> list[Mapping[str, Any]]:
    results: list[Mapping[str, Any]] = []
    expected_total: int | None = None
    page = 1
    while True:
        query = dict(params or {})
        query.update({"per_page": 100, "page": page})
        payload = _mapping(api.get(path, query), f"GitHub response for {path}")
        total = _nonnegative_integer(payload.get("total_count"), f"{path}.total_count")
        if expected_total is None:
            expected_total = total
        elif total != expected_total:
            raise EvidenceError(f"{path}.total_count changed during pagination")
        batch = _sequence(payload.get(key), f"{path}.{key}")
        first_index = len(results)
        for index, item in enumerate(batch):
            results.append(_mapping(item, f"{path}.{key}[{first_index + index}]"))
        if len(batch) < 100:
            break
        page += 1
        if page > 1000:
            raise EvidenceError(f"{path} pagination exceeded the safety limit")
    if expected_total != len(results):
        raise EvidenceError(
            f"{path} returned incomplete pagination: expected {expected_total}, received {len(results)}"
        )
    return results


def validate_main_ancestry(api: RestApi, repository: str, candidate_sha: str) -> dict[str, object]:
    """Require candidate_sha to be the merge base of candidate_sha...main."""

    path = f"repos/{repository}/compare/{candidate_sha}...{MAIN_BRANCH}"
    payload = _mapping(api.get(path), "compare response")
    status = _string(payload.get("status"), "compare.status")
    base = _mapping(payload.get("base_commit"), "compare.base_commit")
    merge_base = _mapping(payload.get("merge_base_commit"), "compare.merge_base_commit")
    base_sha = _sha(base.get("sha"), "compare.base_commit.sha")
    merge_base_sha = _sha(merge_base.get("sha"), "compare.merge_base_commit.sha")
    ahead_by = _nonnegative_integer(payload.get("ahead_by"), "compare.ahead_by")
    behind_by = _nonnegative_integer(payload.get("behind_by"), "compare.behind_by")
    if base_sha != candidate_sha or merge_base_sha != candidate_sha:
        raise EvidenceError("candidate SHA is not an ancestor of main")
    if status not in ("ahead", "identical") or behind_by != 0:
        raise EvidenceError(f"candidate SHA is not in main history (compare status: {status})")
    if status == "identical" and ahead_by != 0:
        raise EvidenceError("compare response is internally inconsistent for identical commits")
    if status == "ahead" and ahead_by == 0:
        raise EvidenceError("compare response is internally inconsistent for an ahead main branch")
    return {
        "branch": MAIN_BRANCH,
        "compare_status": status,
        "commits_after_candidate": ahead_by,
    }


def _validated_run(
    raw: Mapping[str, Any],
    candidate_sha: str,
    event: str,
    workflow: str,
) -> dict[str, object]:
    run_id = _integer(raw.get("id"), "workflow run id")
    attempt = _integer(raw.get("run_attempt"), f"workflow run {run_id} attempt")
    head_sha = _sha(raw.get("head_sha"), f"workflow run {run_id} head_sha")
    head_branch = _string(raw.get("head_branch"), f"workflow run {run_id} head_branch")
    actual_event = _string(raw.get("event"), f"workflow run {run_id} event")
    status = _string(raw.get("status"), f"workflow run {run_id} status")
    conclusion = _string(raw.get("conclusion"), f"workflow run {run_id} conclusion")
    path = _string(raw.get("path"), f"workflow run {run_id} path")
    html_url = _url(raw.get("html_url"), f"workflow run {run_id} html_url")
    created_at = _timestamp(raw.get("created_at"), f"workflow run {run_id} created_at")
    updated_at = _timestamp(raw.get("updated_at"), f"workflow run {run_id} updated_at")
    if updated_at < created_at:
        raise EvidenceError(f"workflow run {run_id} timestamps are inconsistent")
    if head_sha != candidate_sha or head_branch != MAIN_BRANCH or actual_event != event:
        raise EvidenceError(f"workflow run {run_id} does not match the requested exact-SHA event")
    if not path.endswith(f"/{workflow}"):
        raise EvidenceError(f"workflow run {run_id} does not identify {workflow}")
    return {
        "workflow": workflow,
        "event": actual_event,
        "id": run_id,
        "attempt": attempt,
        "url": html_url,
        "head_sha": head_sha,
        "head_branch": head_branch,
        "status": status,
        "conclusion": conclusion,
        "created_at": _format_timestamp(created_at),
        "updated_at": _format_timestamp(updated_at),
        "_created": created_at,
    }


def find_latest_successful_run(
    api: RestApi,
    repository: str,
    workflow: str,
    candidate_sha: str,
    event: str,
) -> dict[str, object]:
    path = f"repos/{repository}/actions/workflows/{urllib.parse.quote(workflow, safe='')}/runs"
    raw_runs = _collection(
        api,
        path,
        "workflow_runs",
        {
            "branch": MAIN_BRANCH,
            "event": event,
            "head_sha": candidate_sha,
            "status": "completed",
        },
    )
    matching: list[dict[str, object]] = []
    for raw in raw_runs:
        if (
            str(raw.get("head_sha", "")).lower() == candidate_sha
            and raw.get("head_branch") == MAIN_BRANCH
            and raw.get("event") == event
        ):
            matching.append(_validated_run(raw, candidate_sha, event, workflow))
    if not matching:
        raise EvidenceError(f"no completed {workflow} {event} run exists for candidate SHA")
    latest = max(matching, key=lambda item: (item["_created"], item["id"]))
    if latest["status"] != "completed" or latest["conclusion"] != "success":
        raise EvidenceError(
            f"latest {workflow} {event} run {latest['id']} is not successful "
            f"({latest['status']}/{latest['conclusion']})"
        )
    del latest["_created"]
    return latest


def _timed_job_step_evidence(
    raw: Mapping[str, Any],
    job_name: str,
    job_started_at: datetime,
    job_completed_at: datetime,
) -> list[dict[str, object]]:
    policy = TIMED_JOB_STEPS.get(job_name)
    if policy is None:
        return []

    required_name, minimum_seconds = policy
    raw_steps = _sequence(raw.get("steps"), f"job {job_name} steps")
    matching_steps: list[Mapping[str, Any]] = []
    for index, raw_step in enumerate(raw_steps):
        step = _mapping(raw_step, f"job {job_name} step {index}")
        if _string(step.get("name"), f"job {job_name} step {index} name") == required_name:
            matching_steps.append(step)
    if len(matching_steps) != 1:
        raise EvidenceError(
            f"job {job_name} must contain exactly one timed step named {required_name}"
        )

    step = matching_steps[0]
    status = _string(step.get("status"), f"job {job_name} step {required_name} status")
    conclusion = _string(
        step.get("conclusion"),
        f"job {job_name} step {required_name} conclusion",
    )
    if status != "completed" or conclusion != "success":
        raise EvidenceError(
            f"timed step {required_name} was not executed successfully "
            f"({status}/{conclusion})"
        )
    started_at = _timestamp(
        step.get("started_at"),
        f"job {job_name} step {required_name} started_at",
    )
    completed_at = _timestamp(
        step.get("completed_at"),
        f"job {job_name} step {required_name} completed_at",
    )
    if (
        started_at < job_started_at
        or completed_at > job_completed_at
        or completed_at < started_at
    ):
        raise EvidenceError(f"timed step {required_name} timestamps are inconsistent")
    duration_seconds = (completed_at - started_at).total_seconds()
    if duration_seconds < minimum_seconds:
        raise EvidenceError(
            f"timed step {required_name} measured duration is shorter than "
            f"{minimum_seconds} seconds"
        )
    return [
        {
            "name": required_name,
            "started_at": _format_timestamp(started_at),
            "completed_at": _format_timestamp(completed_at),
            "duration_seconds": duration_seconds,
        }
    ]


def _job_evidence(
    raw: Mapping[str, Any],
    run: Mapping[str, object],
    candidate_sha: str,
) -> dict[str, object]:
    name = _string(raw.get("name"), "job name")
    job_id = _integer(raw.get("id"), f"job {name} id")
    run_id = _integer(raw.get("run_id"), f"job {name} run_id")
    attempt = _integer(raw.get("run_attempt"), f"job {name} run_attempt")
    head_sha = _sha(raw.get("head_sha"), f"job {name} head_sha")
    status = _string(raw.get("status"), f"job {name} status")
    conclusion = _string(raw.get("conclusion"), f"job {name} conclusion")
    html_url = _url(raw.get("html_url"), f"job {name} html_url")
    started_at = _timestamp(raw.get("started_at"), f"job {name} started_at")
    completed_at = _timestamp(raw.get("completed_at"), f"job {name} completed_at")
    if (
        run_id != run["id"]
        or attempt != run["attempt"]
        or head_sha != candidate_sha
    ):
        raise EvidenceError(f"job {name} is not bound to the selected run attempt and SHA")
    if completed_at < started_at:
        raise EvidenceError(f"job {name} timestamps are inconsistent")
    if status != "completed" or conclusion != "success":
        raise EvidenceError(f"required job {name} is not successful ({status}/{conclusion})")
    evidence: dict[str, object] = {
        "id": job_id,
        "name": name,
        "url": html_url,
        "started_at": _format_timestamp(started_at),
        "completed_at": _format_timestamp(completed_at),
    }
    timed_steps = _timed_job_step_evidence(
        raw,
        name,
        started_at,
        completed_at,
    )
    if timed_steps:
        evidence["required_timed_steps"] = timed_steps
    return evidence


def validate_jobs(
    api: RestApi,
    repository: str,
    run: Mapping[str, object],
    candidate_sha: str,
    expected_names: Iterable[str],
    require_lint_steps: bool = False,
) -> list[dict[str, object]]:
    run_id = _integer(run.get("id"), "selected run id")
    attempt = _integer(run.get("attempt"), "selected run attempt")
    path = f"repos/{repository}/actions/runs/{run_id}/attempts/{attempt}/jobs"
    raw_jobs = _collection(api, path, "jobs", {"filter": "all"})
    expected_sequence = tuple(expected_names)
    expected = set(expected_sequence)
    if len(expected) != len(expected_sequence):
        raise EvidenceError("internal expected job policy contains duplicate names")
    jobs_by_name: dict[str, Mapping[str, Any]] = {}
    for raw in raw_jobs:
        name = _string(raw.get("name"), "job name")
        if name in jobs_by_name:
            raise EvidenceError(f"run {run_id} contains duplicate job name: {name}")
        jobs_by_name[name] = raw
    actual = set(jobs_by_name)
    if actual != expected:
        missing = sorted(expected - actual)
        extra = sorted(actual - expected)
        raise EvidenceError(
            f"run {run_id} required job set mismatch; missing={missing}, extra={extra}"
        )

    if require_lint_steps:
        lint = jobs_by_name.get(LINT_JOB)
        if lint is None:
            raise EvidenceError("lint job is missing")
        steps = _sequence(lint.get("steps"), "lint job steps")
        steps_by_name: dict[str, Mapping[str, Any]] = {}
        for index, step_value in enumerate(steps):
            step = _mapping(step_value, f"lint job step {index}")
            name = _string(step.get("name"), f"lint job step {index} name")
            if name in steps_by_name:
                raise EvidenceError(f"lint job contains duplicate step name: {name}")
            steps_by_name[name] = step
        for step_name in LINT_STEPS:
            step = steps_by_name.get(step_name)
            if step is None:
                raise EvidenceError(f"lint step {step_name} is missing")
            status = _string(step.get("status"), f"lint step {step_name} status")
            conclusion = _string(step.get("conclusion"), f"lint step {step_name} conclusion")
            if status != "completed" or conclusion != "success":
                raise EvidenceError(
                    f"lint step {step_name} was not executed successfully "
                    f"({status}/{conclusion})"
                )

    evidence = [
        _job_evidence(jobs_by_name[name], run, candidate_sha)
        for name in sorted(expected)
    ]
    job_ids = [item["id"] for item in evidence]
    if len(job_ids) != len(set(job_ids)):
        raise EvidenceError(f"run {run_id} contains duplicate job IDs")
    return evidence


def _artifact_timed_step_duration(
    run: Mapping[str, object],
    artifact_name: str,
) -> float:
    job_name = ARTIFACT_TIMED_JOBS.get(artifact_name)
    if job_name is None:
        raise EvidenceError(f"artifact {artifact_name} has no timed job policy")
    jobs = _sequence(run.get("jobs"), f"run {run['id']} jobs")
    matching_jobs = [
        _mapping(job, f"run {run['id']} job")
        for job in jobs
        if isinstance(job, Mapping) and job.get("name") == job_name
    ]
    if len(matching_jobs) != 1:
        raise EvidenceError(f"artifact {artifact_name} has no unique producing job timing")
    steps = _sequence(
        matching_jobs[0].get("required_timed_steps"),
        f"job {job_name} required timed steps",
    )
    required_step_name = TIMED_JOB_STEPS[job_name][0]
    matching_steps = [
        _mapping(step, f"job {job_name} required timed step")
        for step in steps
        if isinstance(step, Mapping) and step.get("name") == required_step_name
    ]
    if len(matching_steps) != 1:
        raise EvidenceError(f"artifact {artifact_name} has no unique authoritative step timing")
    return _number(
        matching_steps[0].get("duration_seconds"),
        f"job {job_name} timed step duration_seconds",
    )


def _validate_payload_workflow_binding(
    artifact_name: str,
    payload: Mapping[str, object],
    workflow_evidence: Mapping[str, object],
    run: Mapping[str, object],
) -> None:
    parameters = _mapping(
        workflow_evidence.get("parameters"),
        f"artifact {artifact_name} validated parameters",
    )
    if artifact_name == "runtime-soak-evidence":
        soak_minutes = _integer(
            parameters.get("soak_minutes"),
            "runtime soak validated soak_minutes",
        )
        native_iterations = _integer(
            parameters.get("native_module_iterations"),
            "runtime soak validated native_module_iterations",
        )
        duration_ms = _integer(
            payload.get("duration_ms"),
            "runtime soak payload duration_ms",
        )
        payload_native_iterations = _integer(
            payload.get("native_module_iterations"),
            "runtime soak payload native_module_iterations",
        )
        authoritative_seconds = _artifact_timed_step_duration(run, artifact_name)
        declared_seconds = soak_minutes * 60
        if authoritative_seconds < declared_seconds:
            raise EvidenceError(
                "runtime soak authoritative job step is shorter than its declared duration"
            )
        if duration_ms < declared_seconds * 1000:
            raise EvidenceError(
                "runtime soak machine result is shorter than its declared duration"
            )
        if duration_ms > (authoritative_seconds + 30) * 1000:
            raise EvidenceError(
                "runtime soak machine result duration exceeds its authoritative job step"
            )
        if payload_native_iterations != native_iterations:
            raise EvidenceError(
                "native-module machine result does not match its declared iteration count"
            )
    elif artifact_name == "long-fuzz-evidence":
        seconds_per_target = _integer(
            parameters.get("fuzz_seconds_per_target"),
            "long fuzz validated fuzz_seconds_per_target",
        )
        authoritative_seconds = _artifact_timed_step_duration(run, artifact_name)
        target_count = len(EXPECTED_FUZZ_TARGETS)
        if authoritative_seconds < seconds_per_target * target_count:
            raise EvidenceError(
                "long fuzz authoritative job step is shorter than its declared campaign"
            )
        targets = _mapping(payload.get("targets"), "long fuzz validated target results")
        for target in EXPECTED_FUZZ_TARGETS:
            result = _mapping(targets.get(target), f"long fuzz validated result {target}")
            reported_seconds = _integer(
                result.get("reported_seconds"),
                f"long fuzz validated result {target} reported_seconds",
            )
            if reported_seconds < seconds_per_target:
                raise EvidenceError(
                    f"long fuzz machine result is shorter than declared for {target}"
                )


def _git_runtime_input_tree(
    api: RestApi,
    repository: str,
    commit_sha: str,
) -> dict[str, object]:
    commit_path = f"repos/{repository}/git/commits/{commit_sha}"
    commit = _mapping(api.get(commit_path), f"Git commit object {commit_sha}")
    if _sha(commit.get("sha"), f"Git commit object {commit_sha} sha") != commit_sha:
        raise EvidenceError(f"Git commit object does not match requested SHA: {commit_sha}")
    tree_reference = _mapping(
        commit.get("tree"),
        f"Git commit object {commit_sha} tree",
    )
    root_tree_sha = _sha(
        tree_reference.get("sha"),
        f"Git commit object {commit_sha} root tree sha",
    )

    tree_path = f"repos/{repository}/git/trees/{root_tree_sha}"
    root_tree = _mapping(api.get(tree_path), f"Git root tree object {root_tree_sha}")
    if _sha(
        root_tree.get("sha"),
        f"Git root tree object {root_tree_sha} sha",
    ) != root_tree_sha:
        raise EvidenceError(f"Git root tree object does not match requested SHA: {root_tree_sha}")
    if root_tree.get("truncated") is not False:
        raise EvidenceError(f"Git root tree object is truncated: {root_tree_sha}")

    raw_entries = _sequence(
        root_tree.get("tree"),
        f"Git root tree object {root_tree_sha} entries",
    )
    entries: dict[str, Mapping[str, Any]] = {}
    for index, raw_entry in enumerate(raw_entries):
        entry = _mapping(
            raw_entry,
            f"Git root tree object {root_tree_sha} entry {index}",
        )
        path = _string(
            entry.get("path"),
            f"Git root tree object {root_tree_sha} entry {index} path",
        )
        if path in entries:
            raise EvidenceError(f"Git root tree object contains duplicate path: {path}")
        entries[path] = entry

    runtime_inputs: dict[str, dict[str, str]] = {}
    expected_types = {
        "CMakeLists.txt": "blob",
        "cmake": "tree",
        "src": "tree",
    }
    for path, expected_type in expected_types.items():
        entry = entries.get(path)
        if entry is None:
            raise EvidenceError(
                f"Git root tree {root_tree_sha} is missing runtime input path: {path}"
            )
        object_type = _string(
            entry.get("type"),
            f"Git root tree {root_tree_sha} path {path} type",
        )
        if object_type != expected_type:
            raise EvidenceError(
                f"Git root tree {root_tree_sha} path {path} is not a {expected_type}"
            )
        object_sha = _sha(
            entry.get("sha"),
            f"Git root tree {root_tree_sha} path {path} sha",
        )
        runtime_inputs[path] = {
            "type": object_type,
            "sha": object_sha,
        }
    return {
        "commit_sha": commit_sha,
        "root_tree_sha": root_tree_sha,
        "runtime_inputs": runtime_inputs,
    }


def _validate_benchmark_base_ancestry(
    api: RestApi,
    repository: str,
    base_sha: str,
    head_sha: str,
) -> dict[str, object]:
    path = f"repos/{repository}/compare/{base_sha}...{head_sha}"
    comparison = _mapping(
        api.get(path),
        "runtime benchmark Git ancestry comparison",
    )
    status = _string(
        comparison.get("status"),
        "runtime benchmark Git ancestry status",
    )
    ahead_by = _integer(
        comparison.get("ahead_by"),
        "runtime benchmark Git ancestry ahead_by",
    )
    behind_by = _nonnegative_integer(
        comparison.get("behind_by"),
        "runtime benchmark Git ancestry behind_by",
    )
    base_commit = _mapping(
        comparison.get("base_commit"),
        "runtime benchmark Git ancestry base_commit",
    )
    merge_base = _mapping(
        comparison.get("merge_base_commit"),
        "runtime benchmark Git ancestry merge_base_commit",
    )
    returned_base_sha = _sha(
        base_commit.get("sha"),
        "runtime benchmark Git ancestry base_commit SHA",
    )
    merge_base_sha = _sha(
        merge_base.get("sha"),
        "runtime benchmark Git ancestry merge_base_commit SHA",
    )
    if (
        status != "ahead"
        or ahead_by <= 0
        or behind_by != 0
        or returned_base_sha != base_sha
        or merge_base_sha != base_sha
    ):
        raise EvidenceError(
            "runtime benchmark base SHA is not a strict ancestor of candidate head"
        )
    return {
        "status": status,
        "ahead_by": ahead_by,
        "behind_by": behind_by,
        "base_sha": base_sha,
        "head_sha": head_sha,
        "merge_base_sha": merge_base_sha,
    }


def _validate_benchmark_git_binding(
    api: RestApi,
    repository: str,
    payload: Mapping[str, object],
) -> dict[str, object]:
    base_sha = _sha(payload.get("base_sha"), "runtime benchmark payload base_sha")
    head_sha = _sha(payload.get("head_sha"), "runtime benchmark payload head_sha")
    ancestry = _validate_benchmark_base_ancestry(
        api,
        repository,
        base_sha,
        head_sha,
    )
    base = _git_runtime_input_tree(api, repository, base_sha)
    head = _git_runtime_input_tree(api, repository, head_sha)
    base_inputs = _mapping(base.get("runtime_inputs"), "base Git runtime inputs")
    head_inputs = _mapping(head.get("runtime_inputs"), "head Git runtime inputs")

    path_evidence: dict[str, dict[str, object]] = {}
    authoritative_equivalent = True
    for path in EXPECTED_BENCHMARK_RUNTIME_INPUTS:
        base_entry = _mapping(base_inputs.get(path), f"base Git runtime input {path}")
        head_entry = _mapping(head_inputs.get(path), f"head Git runtime input {path}")
        base_type = _string(base_entry.get("type"), f"base Git runtime input {path} type")
        head_type = _string(head_entry.get("type"), f"head Git runtime input {path} type")
        base_object_sha = _sha(
            base_entry.get("sha"),
            f"base Git runtime input {path} sha",
        )
        head_object_sha = _sha(
            head_entry.get("sha"),
            f"head Git runtime input {path} sha",
        )
        equivalent = base_type == head_type and base_object_sha == head_object_sha
        authoritative_equivalent = authoritative_equivalent and equivalent
        path_evidence[path] = {
            "type": base_type,
            "base_sha": base_object_sha,
            "head_sha": head_object_sha,
            "equivalent": equivalent,
        }

    reported_equivalent = _boolean(
        payload.get("runtime_inputs_equivalent"),
        "runtime benchmark payload runtime_inputs_equivalent",
    )
    if reported_equivalent != authoritative_equivalent:
        raise EvidenceError(
            "runtime benchmark input equivalence contradicts authoritative Git trees"
        )
    return {
        "source": "github-git-root-tree",
        "equivalent": authoritative_equivalent,
        "base_commit_sha": base_sha,
        "head_commit_sha": head_sha,
        "base_root_tree_sha": base["root_tree_sha"],
        "head_root_tree_sha": head["root_tree_sha"],
        "base_ancestry": ancestry,
        "paths": path_evidence,
    }


def _artifact_evidence(
    api: RestApi,
    raw: Mapping[str, Any],
    repository: str,
    run: Mapping[str, object],
    candidate_sha: str,
    now: datetime,
) -> dict[str, object]:
    name = _string(raw.get("name"), "artifact name")
    artifact_id = _integer(raw.get("id"), f"artifact {name} id")
    size = _integer(raw.get("size_in_bytes"), f"artifact {name} size_in_bytes")
    if raw.get("expired") is not False:
        raise EvidenceError(f"artifact {name} is expired or has no authoritative expiration state")
    created_at = _timestamp(raw.get("created_at"), f"artifact {name} created_at")
    updated_at = _timestamp(raw.get("updated_at"), f"artifact {name} updated_at")
    expires_at = _timestamp(raw.get("expires_at"), f"artifact {name} expires_at")
    run_created_at = _timestamp(run.get("created_at"), f"run {run['id']} created_at")
    if created_at < run_created_at or updated_at < created_at:
        raise EvidenceError(f"artifact {name} timestamps are inconsistent with its run")
    if created_at > now or updated_at > now:
        raise EvidenceError(f"artifact {name} timestamps are in the future")
    if expires_at <= now or expires_at <= created_at:
        raise EvidenceError(f"artifact {name} is expired")
    digest = _string(raw.get("digest"), f"artifact {name} digest").lower()
    digest_match = _DIGEST_RE.fullmatch(digest)
    if digest_match is None:
        raise EvidenceError(f"artifact {name} has no valid SHA-256 digest")
    workflow_run = _mapping(raw.get("workflow_run"), f"artifact {name} workflow_run")
    workflow_run_id = _integer(workflow_run.get("id"), f"artifact {name} workflow_run.id")
    workflow_head_sha = _sha(
        workflow_run.get("head_sha"),
        f"artifact {name} workflow_run.head_sha",
    )
    workflow_branch = _string(
        workflow_run.get("head_branch"),
        f"artifact {name} workflow_run.head_branch",
    )
    if (
        workflow_run_id != run["id"]
        or workflow_head_sha != candidate_sha
        or workflow_branch != MAIN_BRANCH
    ):
        raise EvidenceError(f"artifact {name} is not bound to the selected exact-SHA run")
    archive_download_url = _url(
        raw.get("archive_download_url"),
        f"artifact {name} archive_download_url",
    )
    try:
        archive = api.download(archive_download_url)
    except EvidenceError:
        raise
    except Exception as error:
        raise EvidenceError(f"artifact {name} download failed: {error}") from error
    if not isinstance(archive, bytes):
        raise EvidenceError(f"artifact {name} download did not return bytes")
    if len(archive) != size:
        raise EvidenceError(
            f"artifact {name} download size mismatch: API={size}, downloaded={len(archive)}"
        )
    downloaded_digest = f"sha256:{hashlib.sha256(archive).hexdigest()}"
    if downloaded_digest != digest:
        raise EvidenceError(f"artifact {name} downloaded ZIP digest does not match the GitHub API")
    metadata, payload_evidence = _read_artifact_archive(
        archive,
        name,
        candidate_sha,
    )
    workflow_evidence = _validated_workflow_evidence_metadata(
        metadata,
        artifact_name=name,
        repository=repository,
        candidate_sha=candidate_sha,
        run=run,
        artifact_created_at=created_at,
    )
    if name == "runtime-benchmark-evidence":
        payload_evidence["authoritative_runtime_inputs"] = (
            _validate_benchmark_git_binding(
                api,
                repository,
                payload_evidence,
            )
        )
    _validate_payload_workflow_binding(
        name,
        payload_evidence,
        workflow_evidence,
        run,
    )
    return {
        "id": artifact_id,
        "name": name,
        "digest": digest,
        "size_in_bytes": size,
        "archive_download_url": archive_download_url,
        "created_at": _format_timestamp(created_at),
        "updated_at": _format_timestamp(updated_at),
        "expires_at": _format_timestamp(expires_at),
        "workflow_evidence": workflow_evidence,
        "payload_evidence": payload_evidence,
    }


def validate_artifacts(
    api: RestApi,
    repository: str,
    run: Mapping[str, object],
    candidate_sha: str,
    required_names: Iterable[str],
    now: datetime,
) -> list[dict[str, object]]:
    run_id = _integer(run.get("id"), "selected run id")
    path = f"repos/{repository}/actions/runs/{run_id}/artifacts"
    raw_artifacts = _collection(api, path, "artifacts")
    required = set(required_names)
    matching: dict[str, list[Mapping[str, Any]]] = {name: [] for name in required}
    for raw in raw_artifacts:
        name = raw.get("name")
        if isinstance(name, str) and name in matching:
            matching[name].append(raw)
    for name, candidates in matching.items():
        if not candidates:
            raise EvidenceError(f"required artifact {name} is missing from run {run_id}")
        if len(candidates) != 1:
            raise EvidenceError(f"required artifact {name} is ambiguous in run {run_id}")
    evidence = [
        _artifact_evidence(
            api,
            matching[name][0],
            repository,
            run,
            candidate_sha,
            now,
        )
        for name in sorted(required)
    ]
    artifact_ids = [item["id"] for item in evidence]
    if len(artifact_ids) != len(set(artifact_ids)):
        raise EvidenceError(f"run {run_id} contains duplicate required artifact IDs")
    return evidence


def validate_release_notes_narrative(
    text: str,
    *,
    expected_project_version: str,
    expected_abi_version: int,
) -> None:
    """Reject placeholders while accepting a commit-independent narrative.

    Tracked release notes must not contain the candidate SHA or run URLs:
    writing either value back into the notes would change the commit being
    described. Exact identifiers live in the runtime evidence manifest and are
    appended to the final GitHub release body. Platform package names, SBOM
    filenames, and package SHA-256 values are likewise appended only after the
    downstream package jobs complete.
    """

    if not isinstance(text, str) or not text.strip():
        raise EvidenceError("release notes narrative is missing")
    if re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+", expected_project_version) is None:
        raise EvidenceError("expected release-notes project version is invalid")
    if (
        isinstance(expected_abi_version, bool)
        or not isinstance(expected_abi_version, int)
        or expected_abi_version < 0
    ):
        raise EvidenceError("expected release-notes ABI version is invalid")
    for pattern in _PLACEHOLDER_PATTERNS:
        match = pattern.search(text)
        if match is not None:
            raise EvidenceError(f"release notes narrative contains placeholder text: {match.group(0)!r}")
    sha_match = _EMBEDDED_SHA_RE.search(text)
    if sha_match is not None:
        raise EvidenceError(
            "release notes narrative must not contain a full commit SHA: "
            f"{sha_match.group(0)!r}"
        )
    run_url_match = _ACTIONS_RUN_URL_RE.search(text)
    if run_url_match is not None:
        raise EvidenceError(
            "release notes narrative must not contain a GitHub Actions run URL: "
            f"{run_url_match.group(0)!r}"
        )
    if len(text.strip()) < 500:
        raise EvidenceError("release notes narrative is too short to be final")
    if len(re.findall(r"^##\s+\S", text, re.MULTILINE)) < 4:
        raise EvidenceError("release notes narrative must contain at least four substantive sections")
    lowered = text.lower()
    for term in REQUIRED_NOTES_EVIDENCE_TERMS:
        if term.lower() not in lowered:
            raise EvidenceError(f"release notes narrative does not describe evidence type: {term}")
    for artifact_name in REQUIRED_NOTES_ARTIFACTS:
        if artifact_name not in text:
            raise EvidenceError(f"release notes narrative does not name artifact: {artifact_name}")
    title_versions = re.findall(
        r"^#\s+Lua C\+\+\s+([0-9]+\.[0-9]+\.[0-9]+)\b",
        text,
        re.MULTILINE,
    )
    if title_versions != [expected_project_version]:
        raise EvidenceError(
            "release notes title version mismatch; "
            f"expected {expected_project_version}, found {title_versions}"
        )
    sdk_versions = re.findall(
        r"\bSDK\s+(?:version(?:\s+is)?|版本)\s*[:：]?\s*"
        r"([0-9]+\.[0-9]+\.[0-9]+)",
        text,
        re.IGNORECASE,
    )
    if not sdk_versions or set(sdk_versions) != {expected_project_version}:
        raise EvidenceError(
            "release notes SDK version mismatch; "
            f"expected {expected_project_version}, found {sdk_versions}"
        )
    abi_versions = re.findall(
        r"\bshared-library\s+ABI(?:\s+is)?\s*[:：]?\s*([0-9]+)",
        text,
        re.IGNORECASE,
    )
    if not abi_versions or set(abi_versions) != {str(expected_abi_version)}:
        raise EvidenceError(
            "release notes ABI version mismatch; "
            f"expected {expected_abi_version}, found {abi_versions}"
        )


def _run_evidence(
    api: RestApi,
    repository: str,
    workflow: str,
    candidate_sha: str,
    event: str,
    expected_jobs: Iterable[str],
    required_artifacts: Iterable[str],
    now: datetime,
    require_lint_steps: bool = False,
) -> dict[str, object]:
    run = find_latest_successful_run(api, repository, workflow, candidate_sha, event)
    run["jobs"] = validate_jobs(
        api,
        repository,
        run,
        candidate_sha,
        expected_jobs,
        require_lint_steps=require_lint_steps,
    )
    run["artifacts"] = validate_artifacts(
        api,
        repository,
        run,
        candidate_sha,
        required_artifacts,
        now,
    )
    return run


def verify_release_evidence(
    api: RestApi,
    repository: str,
    candidate_sha: str,
    release_notes_text: str,
    governance_evidence: Mapping[str, Any],
    source_readiness_evidence: Mapping[str, Any],
    *,
    expected_tag: str | None,
    expected_version: str,
    now: datetime | None = None,
    ci_workflow: str = CI_WORKFLOW,
    nightly_workflow: str = NIGHTLY_WORKFLOW,
) -> dict[str, object]:
    """Validate all exact-SHA release evidence and return its manifest."""

    if _REPOSITORY_RE.fullmatch(repository) is None:
        raise EvidenceError("repository must use the owner/name form")
    candidate_sha = _sha(candidate_sha, "candidate SHA")
    now = now or datetime.now(timezone.utc)
    if now.tzinfo is None:
        raise EvidenceError("verification time must include a timezone")
    now = now.astimezone(timezone.utc)

    try:
        governance = validate_governance_evidence(
            governance_evidence,
            expected_repository=repository,
            expected_sha=candidate_sha,
            expected_tag=expected_tag,
            expected_version=expected_version,
            now=now,
            require_approved=expected_tag is not None,
        )
    except GovernanceError as error:
        raise EvidenceError(f"governance evidence is invalid: {error}") from error
    try:
        source_readiness = validate_source_readiness_evidence(
            source_readiness_evidence,
            expected_repository=repository,
            expected_sha=candidate_sha,
            expected_version=expected_version,
            now=now,
        )
    except SourceReadinessError as error:
        raise EvidenceError(f"source-readiness evidence is invalid: {error}") from error

    ancestry = validate_main_ancestry(api, repository, candidate_sha)
    ci = _run_evidence(
        api,
        repository,
        ci_workflow,
        candidate_sha,
        "push",
        EXPECTED_CI_JOBS,
        CI_ARTIFACTS,
        now,
        require_lint_steps=True,
    )
    nightly_dispatch = _run_evidence(
        api,
        repository,
        nightly_workflow,
        candidate_sha,
        "workflow_dispatch",
        EXPECTED_NIGHTLY_JOBS,
        NIGHTLY_ARTIFACTS,
        now,
    )
    nightly_schedule = _run_evidence(
        api,
        repository,
        nightly_workflow,
        candidate_sha,
        "schedule",
        EXPECTED_NIGHTLY_JOBS,
        NIGHTLY_ARTIFACTS,
        now,
    )
    validate_release_notes_narrative(
        release_notes_text,
        expected_project_version=str(source_readiness["project_version"]),
        expected_abi_version=int(source_readiness["abi_version"]),
    )
    return {
        "schema": MANIFEST_SCHEMA,
        "generated_at": _format_timestamp(now),
        "repository": repository,
        "candidate_sha": candidate_sha,
        "version": source_readiness["version"],
        "abi_version": source_readiness["abi_version"],
        "governance": governance,
        "source_readiness": source_readiness,
        "main_history": ancestry,
        "tool": {
            "name": "tools/verify_release_evidence.py",
            "version": TOOL_VERSION,
        },
        "runs": {
            "ci_push": ci,
            "nightly_workflow_dispatch": nightly_dispatch,
            "nightly_schedule": nightly_schedule,
        },
        "release_notes": {
            "narrative_placeholders_checked": True,
            "package_checksums_checked": False,
            "package_checksums_policy": "deferred until packages exist",
        },
    }


def _write_manifest(path: Path, manifest: Mapping[str, object]) -> None:
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
        json.dump(manifest, temporary, indent=2, sort_keys=True)
        temporary.write("\n")
        temporary_path = Path(temporary.name)
    os.replace(temporary_path, path)


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sha", default=os.environ.get("GITHUB_SHA"))
    parser.add_argument("--repository", default=os.environ.get("GITHUB_REPOSITORY"))
    parser.add_argument("--token", default=os.environ.get("GITHUB_TOKEN"))
    parser.add_argument("--api-url", default=os.environ.get("GITHUB_API_URL", "https://api.github.com"))
    parser.add_argument("--ci-workflow", default=CI_WORKFLOW)
    parser.add_argument("--nightly-workflow", default=NIGHTLY_WORKFLOW)
    parser.add_argument("--expected-tag")
    parser.add_argument("--expected-version", required=True)
    parser.add_argument(
        "--governance-evidence",
        type=Path,
        required=True,
    )
    parser.add_argument(
        "--source-readiness-evidence",
        type=Path,
        required=True,
    )
    parser.add_argument(
        "--release-notes",
        type=Path,
        default=Path("docs/release/rc-notes-0.1.0.md"),
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("build/release-evidence.json"),
    )
    return parser


def main(
    argv: Sequence[str] | None = None,
    *,
    client: RestApi | None = None,
    now: datetime | None = None,
) -> int:
    args = _parser().parse_args(argv)
    try:
        # A failed verification must not leave an older manifest that a later
        # step could mistake for the result of this invocation.
        args.output.unlink(missing_ok=True)
        if not args.sha:
            raise EvidenceError("candidate SHA is required via --sha or GITHUB_SHA")
        if not args.repository:
            raise EvidenceError("repository is required via --repository or GITHUB_REPOSITORY")
        if not args.token:
            raise EvidenceError("GitHub token is required via --token or GITHUB_TOKEN")
        if not args.release_notes.is_file():
            raise EvidenceError(f"release notes file is missing: {args.release_notes}")
        if not args.governance_evidence.is_file():
            raise EvidenceError(
                f"governance evidence file is missing: {args.governance_evidence}"
            )
        try:
            governance_evidence = parse_evidence_json(
                args.governance_evidence.read_text(encoding="utf-8-sig")
            )
        except (OSError, UnicodeError, GovernanceError) as error:
            raise EvidenceError(f"governance evidence is unreadable: {error}") from error
        if not args.source_readiness_evidence.is_file():
            raise EvidenceError(
                "source-readiness evidence file is missing: "
                f"{args.source_readiness_evidence}"
            )
        try:
            source_readiness_evidence = parse_source_readiness_json(
                args.source_readiness_evidence.read_text(encoding="utf-8-sig")
            )
        except (OSError, UnicodeError, SourceReadinessError) as error:
            raise EvidenceError(
                f"source-readiness evidence is unreadable: {error}"
            ) from error
        api = client or GitHubRestClient(args.token, args.api_url)
        manifest = verify_release_evidence(
            api,
            args.repository,
            args.sha,
            args.release_notes.read_text(encoding="utf-8-sig"),
            governance_evidence,
            source_readiness_evidence,
            expected_tag=args.expected_tag or None,
            expected_version=args.expected_version,
            now=now,
            ci_workflow=args.ci_workflow,
            nightly_workflow=args.nightly_workflow,
        )
        _write_manifest(args.output, manifest)
    except Exception as error:  # Fail closed for API, parsing, filesystem, and policy errors.
        print(f"release evidence verification failed: {error}", file=sys.stderr)
        return 1
    print(
        f"Release evidence verified for {manifest['candidate_sha']}; "
        f"manifest written to {args.output}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
