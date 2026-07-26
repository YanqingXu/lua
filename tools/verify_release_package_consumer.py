#!/usr/bin/env python3
"""Consume one downloaded Lua C++ SDK ZIP in a fresh, source-independent build."""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import stat
import subprocess
import sys
import tempfile
import zipfile
from collections.abc import Callable, Mapping, Sequence
from pathlib import Path

from validate_release_artifacts import (
    ReleaseArtifactError,
    SUPPORTED_RELEASE_RIDS,
    validate_member_name,
)


EXPECTED_CONSUMER_FILES = ("CMakeLists.txt", "main.c")
EXPECTED_TESTS = {
    "installed_lua_cpp_consumer",
    "installed_lua_cpp_shared_consumer",
}
MAX_ARCHIVE_ENTRIES = 20_000
MAX_UNCOMPRESSED_BYTES = 2 * 1024 * 1024 * 1024
_VERSION_RE = re.compile(r"[0-9]+\.[0-9]+\.[0-9]+(?:-rc\.[0-9]+)?")


class ReleasePackageConsumerError(RuntimeError):
    """Raised when an extracted release package cannot be consumed exactly."""


CommandRunner = Callable[..., subprocess.CompletedProcess[str]]


def fail(message: str) -> None:
    raise ReleasePackageConsumerError(message)


def _clean_environment() -> dict[str, str]:
    blocked = {
        "cmake_prefix_path",
        "cmake_project_include",
        "cmake_project_include_before",
        "cmake_toolchain_file",
        "luacpp_dir",
    }
    return {
        key: value
        for key, value in os.environ.items()
        if key.casefold() not in blocked
    }


def _run_checked(
    command: Sequence[str],
    *,
    label: str,
    runner: CommandRunner,
    environment: Mapping[str, str],
) -> subprocess.CompletedProcess[str]:
    try:
        result = runner(
            list(command),
            env=dict(environment),
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            check=False,
        )
    except OSError as error:
        fail(f"{label} could not start: {error}")
    if result.returncode != 0:
        fail(
            f"{label} failed with exit code {result.returncode}\n"
            f"stdout:\n{result.stdout}\n"
            f"stderr:\n{result.stderr}"
        )
    return result


def _extract_archive(
    archive: Path,
    destination: Path,
    expected_root: str,
) -> Path:
    if not archive.is_file() or archive.is_symlink():
        fail(f"release archive is missing or is not a regular file: {archive}")

    seen_names: set[str] = set()
    seen_casefolded: set[str] = set()
    total_size = 0
    try:
        with zipfile.ZipFile(archive) as zipped:
            entries = zipped.infolist()
            if not entries:
                fail("release archive is empty")
            if len(entries) > MAX_ARCHIVE_ENTRIES:
                fail("release archive contains too many entries")

            for info in entries:
                member = validate_member_name(info.filename, expected_root)
                normalized = member.as_posix().rstrip("/")
                casefolded = normalized.casefold()
                if normalized in seen_names:
                    fail(f"release archive contains a duplicate entry: {normalized}")
                if casefolded in seen_casefolded:
                    fail(
                        "release archive contains a case-colliding entry: "
                        f"{normalized}"
                    )
                seen_names.add(normalized)
                seen_casefolded.add(casefolded)

                unix_mode = info.external_attr >> 16
                file_type = stat.S_IFMT(unix_mode)
                if file_type not in (0, stat.S_IFDIR, stat.S_IFREG):
                    fail(
                        "release archive contains a link or special file: "
                        f"{info.filename}"
                    )
                if info.flag_bits & 0x1:
                    fail(f"release archive contains an encrypted entry: {info.filename}")
                total_size += info.file_size
                if total_size > MAX_UNCOMPRESSED_BYTES:
                    fail("release archive exceeds the uncompressed size limit")

                target = destination.joinpath(*member.parts)
                for parent in target.parents:
                    if parent == destination.parent:
                        break
                    if parent.exists() and not parent.is_dir():
                        fail(
                            "release archive has a file/directory collision: "
                            f"{info.filename}"
                        )
                if info.is_dir():
                    if target.exists() and not target.is_dir():
                        fail(
                            "release archive has a file/directory collision: "
                            f"{info.filename}"
                        )
                    target.mkdir(parents=True, exist_ok=True)
                    continue

                if target.exists():
                    fail(f"release archive would overwrite an entry: {info.filename}")
                target.parent.mkdir(parents=True, exist_ok=True)
                with zipped.open(info) as source, target.open("xb") as output:
                    shutil.copyfileobj(source, output)
    except ReleaseArtifactError as error:
        fail(str(error))
    except (OSError, zipfile.BadZipFile) as error:
        fail(f"release archive cannot be safely extracted: {archive}: {error}")

    package_root = destination / expected_root
    if not package_root.is_dir() or package_root.is_symlink():
        fail(f"release archive has no canonical package root: {expected_root}")
    return package_root


def _copy_consumer_source(source: Path, destination: Path) -> None:
    if not source.is_dir() or source.is_symlink():
        fail(f"consumer source directory is missing or unsafe: {source}")
    destination.mkdir(parents=True)
    for name in EXPECTED_CONSUMER_FILES:
        source_file = source / name
        if not source_file.is_file() or source_file.is_symlink():
            fail(f"consumer source file is missing or unsafe: {source_file}")
        shutil.copyfile(source_file, destination / name)


def _read_cache_path(cache: Path, key: str) -> Path:
    if not cache.is_file():
        fail(f"consumer configure did not create {cache}")
    prefix = f"{key}:"
    for line in cache.read_text(encoding="utf-8", errors="strict").splitlines():
        if line.startswith(prefix) and "=" in line:
            return Path(line.split("=", 1)[1])
    fail(f"consumer CMake cache is missing {key}")


def _assert_exact_package_binding(build: Path, package_root: Path) -> None:
    cache = build / "CMakeCache.txt"
    expected_root = package_root.resolve(strict=True)
    expected_config = (package_root / "lib/cmake/LuaCpp").resolve(strict=True)
    actual_root = _read_cache_path(cache, "LUA_CPP_PACKAGE_ROOT").resolve(strict=True)
    actual_config = _read_cache_path(cache, "LuaCpp_DIR").resolve(strict=True)
    if actual_root != expected_root:
        fail(
            "consumer CMake cache escaped the extracted package root: "
            f"expected {expected_root}, found {actual_root}"
        )
    if actual_config != expected_config:
        fail(
            "consumer resolved LuaCpp outside the extracted package: "
            f"expected {expected_config}, found {actual_config}"
        )


def _parse_discovered_tests(output: str) -> set[str]:
    try:
        payload = json.loads(output)
    except json.JSONDecodeError as error:
        fail(f"CTest discovery did not return JSON: {error}")
    tests = payload.get("tests") if isinstance(payload, dict) else None
    if not isinstance(tests, list):
        fail("CTest discovery JSON has no tests array")
    names = {
        entry.get("name")
        for entry in tests
        if isinstance(entry, dict) and isinstance(entry.get("name"), str)
    }
    if names != EXPECTED_TESTS:
        fail(
            "downloaded package consumer test set mismatch; "
            f"expected={sorted(EXPECTED_TESTS)}, found={sorted(names)}"
        )
    return names


def verify_release_package_consumer(
    *,
    archive: Path,
    expected_version: str,
    expected_rid: str,
    consumer_source: Path,
    configuration: str = "Release",
    cmake_command: str = "cmake",
    ctest_command: str = "ctest",
    runner: CommandRunner = subprocess.run,
    workspace: Path | None = None,
) -> Mapping[str, object]:
    """Extract, configure, build, and run both consumers against one exact ZIP."""

    if _VERSION_RE.fullmatch(expected_version) is None:
        fail("expected release version is invalid")
    if expected_rid not in SUPPORTED_RELEASE_RIDS:
        fail(f"unsupported release runtime identifier: {expected_rid}")
    if not configuration or any(character.isspace() for character in configuration):
        fail("consumer configuration must be one non-whitespace token")

    expected_root = f"lua-cpp-{expected_version}-{expected_rid}"
    expected_archive_name = f"{expected_root}.zip"
    if archive.name != expected_archive_name:
        fail(
            "release archive filename mismatch: "
            f"expected {expected_archive_name}, found {archive.name}"
        )

    temporary: tempfile.TemporaryDirectory[str] | None = None
    if workspace is None:
        temporary = tempfile.TemporaryDirectory(prefix="lua-cpp-package-consumer-")
        work = Path(temporary.name)
    else:
        if workspace.exists():
            fail(f"consumer workspace must not already exist: {workspace}")
        workspace.mkdir(parents=True)
        work = workspace

    try:
        extract_root = work / "extracted"
        extract_root.mkdir()
        package_root = _extract_archive(archive, extract_root, expected_root)
        config_directory = package_root / "lib/cmake/LuaCpp"
        if not (config_directory / "LuaCppConfig.cmake").is_file():
            fail("downloaded package is missing LuaCppConfig.cmake")

        isolated_source = work / "consumer-source"
        _copy_consumer_source(consumer_source, isolated_source)
        build = work / "consumer-build"
        environment = _clean_environment()

        configure = [
            cmake_command,
            "-S",
            str(isolated_source),
            "-B",
            str(build),
            f"-DLUA_CPP_PACKAGE_ROOT:PATH={package_root}",
            f"-DLuaCpp_DIR:PATH={config_directory}",
            "-DCMAKE_PREFIX_PATH:STRING=",
            "-DCMAKE_FIND_USE_PACKAGE_REGISTRY:BOOL=FALSE",
            "-DCMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY:BOOL=TRUE",
            "-DCMAKE_EXPORT_PACKAGE_REGISTRY:BOOL=FALSE",
            f"-DCMAKE_BUILD_TYPE:STRING={configuration}",
        ]
        _run_checked(
            configure,
            label="downloaded package consumer configure",
            runner=runner,
            environment=environment,
        )
        _assert_exact_package_binding(build, package_root)

        _run_checked(
            [
                cmake_command,
                "--build",
                str(build),
                "--config",
                configuration,
                "--clean-first",
            ],
            label="downloaded package consumer build",
            runner=runner,
            environment=environment,
        )
        discovery = _run_checked(
            [
                ctest_command,
                "--test-dir",
                str(build),
                "-C",
                configuration,
                "--show-only=json-v1",
            ],
            label="downloaded package consumer test discovery",
            runner=runner,
            environment=environment,
        )
        discovered_tests = _parse_discovered_tests(discovery.stdout)
        test_result = _run_checked(
            [
                ctest_command,
                "--test-dir",
                str(build),
                "-C",
                configuration,
                "--output-on-failure",
            ],
            label="downloaded package consumer tests",
            runner=runner,
            environment=environment,
        )
        return {
            "root": expected_root,
            "tests": sorted(discovered_tests),
            "test_output": test_result.stdout,
        }
    finally:
        if temporary is not None:
            temporary.cleanup()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--archive", type=Path, required=True)
    parser.add_argument("--expected-version", required=True)
    parser.add_argument("--expected-rid", required=True)
    parser.add_argument("--consumer-source", type=Path, required=True)
    parser.add_argument("--configuration", default="Release")
    args = parser.parse_args()

    try:
        result = verify_release_package_consumer(
            archive=args.archive,
            expected_version=args.expected_version,
            expected_rid=args.expected_rid,
            consumer_source=args.consumer_source,
            configuration=args.configuration,
        )
    except ReleasePackageConsumerError as error:
        print(f"Downloaded release package consumer verification failed: {error}", file=sys.stderr)
        return 1

    print(
        "Downloaded release package consumers passed: "
        f"{result['root']}; tests={','.join(result['tests'])}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
