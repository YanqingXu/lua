#!/usr/bin/env python3
"""Failure-closed contract tests for downloaded package consumption."""

from __future__ import annotations

import json
import os
import stat
import subprocess
import tempfile
import zipfile
from pathlib import Path

from verify_release_package_consumer import (
    EXPECTED_TESTS,
    ReleasePackageConsumerError,
    verify_release_package_consumer,
)


VERSION = "0.1.0-rc.1"
RID = "windows-x64"
ROOT = f"lua-cpp-{VERSION}-{RID}"


def create_archive(directory: Path) -> Path:
    archive = directory / f"{ROOT}.zip"
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as zipped:
        zipped.writestr(
            f"{ROOT}/lib/cmake/LuaCpp/LuaCppConfig.cmake",
            "# exact package config fixture\n",
        )
        zipped.writestr(
            f"{ROOT}/lib/cmake/LuaCpp/LuaCppTargets.cmake",
            "# exact package targets fixture\n",
        )
    return archive


class FakeRunner:
    def __init__(
        self,
        *,
        discovered_tests: set[str] = EXPECTED_TESTS,
        failure_label: str | None = None,
        escape_cache: bool = False,
    ) -> None:
        self.discovered_tests = discovered_tests
        self.failure_label = failure_label
        self.escape_cache = escape_cache
        self.commands: list[list[str]] = []
        self.environments: list[dict[str, str]] = []

    def __call__(
        self,
        command: list[str],
        *,
        env: dict[str, str],
        capture_output: bool,
        text: bool,
        encoding: str,
        errors: str,
        check: bool,
    ) -> subprocess.CompletedProcess[str]:
        del capture_output, text, encoding, errors, check
        self.commands.append(command)
        self.environments.append(env)
        if "-S" in command:
            label = "configure"
        elif "--build" in command:
            label = "build"
        elif "--show-only=json-v1" in command:
            label = "discovery"
        else:
            label = "test"
        if self.failure_label == label:
            return subprocess.CompletedProcess(command, 19, "synthetic stdout", "synthetic stderr")

        if label == "configure":
            build = Path(command[command.index("-B") + 1])
            definitions = {
                argument.split("=", 1)[0].split(":", 1)[0][2:]: argument.split("=", 1)[1]
                for argument in command
                if argument.startswith("-D") and "=" in argument
            }
            package_root = Path(definitions["LUA_CPP_PACKAGE_ROOT"])
            config = Path(definitions["LuaCpp_DIR"])
            if self.escape_cache:
                package_root = build.parent / "outside-package"
                config = package_root / "lib/cmake/LuaCpp"
                config.mkdir(parents=True)
            build.mkdir(parents=True)
            (build / "CMakeCache.txt").write_text(
                "LUA_CPP_PACKAGE_ROOT:PATH="
                f"{package_root}\n"
                "LuaCpp_DIR:PATH="
                f"{config}\n",
                encoding="utf-8",
            )
        if label == "discovery":
            output = json.dumps(
                {"tests": [{"name": name} for name in sorted(self.discovered_tests)]}
            )
        else:
            output = "synthetic success"
        return subprocess.CompletedProcess(command, 0, output, "")


def expect_rejected(label: str, action: object, fragment: str) -> None:
    try:
        assert callable(action)
        action()
    except ReleasePackageConsumerError as error:
        if fragment not in str(error):
            raise AssertionError(
                f"{label} failed for the wrong reason: {error}"
            ) from error
    else:
        raise AssertionError(f"{label} was accepted")


def main() -> int:
    repository = Path(__file__).resolve().parent.parent
    consumer_source = repository / "tests/packaging/consumer"
    with tempfile.TemporaryDirectory() as temporary:
        directory = Path(temporary)
        archive = create_archive(directory)
        workspace = directory / "positive-work"
        runner = FakeRunner()
        old_prefix = os.environ.get("CMAKE_PREFIX_PATH")
        old_project_include = os.environ.get("CMAKE_PROJECT_INCLUDE")
        os.environ["CMAKE_PREFIX_PATH"] = str(repository / "forbidden-prefix")
        os.environ["CMAKE_PROJECT_INCLUDE"] = str(repository / "forbidden-hook.cmake")
        try:
            result = verify_release_package_consumer(
                archive=archive,
                expected_version=VERSION,
                expected_rid=RID,
                consumer_source=consumer_source,
                runner=runner,
                workspace=workspace,
            )
        finally:
            if old_prefix is None:
                os.environ.pop("CMAKE_PREFIX_PATH", None)
            else:
                os.environ["CMAKE_PREFIX_PATH"] = old_prefix
            if old_project_include is None:
                os.environ.pop("CMAKE_PROJECT_INCLUDE", None)
            else:
                os.environ["CMAKE_PROJECT_INCLUDE"] = old_project_include

        if set(result["tests"]) != EXPECTED_TESTS:
            raise AssertionError("positive consumer run did not execute both consumers")
        configure = runner.commands[0]
        configured_source = Path(configure[configure.index("-S") + 1])
        if configured_source == consumer_source or repository in configured_source.parents:
            raise AssertionError("consumer configured directly from the repository source tree")
        if not all(
            blocked.casefold() not in {key.casefold() for key in environment}
            for environment in runner.environments
            for blocked in ("CMAKE_PREFIX_PATH", "CMAKE_PROJECT_INCLUDE")
        ):
            raise AssertionError("consumer inherited a CMake package or project hook")
        if "-DCMAKE_FIND_USE_PACKAGE_REGISTRY:BOOL=FALSE" not in configure:
            raise AssertionError("consumer did not disable the CMake package registry")
        if "-DCMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY:BOOL=TRUE" not in configure:
            raise AssertionError("consumer did not disable package-registry lookup")

        traversal = directory / f"{ROOT}-traversal.zip"
        with zipfile.ZipFile(traversal, "w") as zipped:
            zipped.writestr(f"{ROOT}/../escaped", "escape")
        canonical_traversal = directory / f"{ROOT}.zip.bad"
        traversal.replace(canonical_traversal)
        expect_rejected(
            "noncanonical archive filename",
            lambda: verify_release_package_consumer(
                archive=canonical_traversal,
                expected_version=VERSION,
                expected_rid=RID,
                consumer_source=consumer_source,
                runner=FakeRunner(),
                workspace=directory / "bad-name-work",
            ),
            "filename mismatch",
        )

        unsafe_directory = directory / "unsafe"
        unsafe_directory.mkdir()
        unsafe_archive = unsafe_directory / f"{ROOT}.zip"
        with zipfile.ZipFile(unsafe_archive, "w") as zipped:
            zipped.writestr(f"{ROOT}/../escaped", "escape")
        expect_rejected(
            "path traversal archive",
            lambda: verify_release_package_consumer(
                archive=unsafe_archive,
                expected_version=VERSION,
                expected_rid=RID,
                consumer_source=consumer_source,
                runner=FakeRunner(),
                workspace=directory / "traversal-work",
            ),
            "unsafe path",
        )

        symlink_directory = directory / "symlink"
        symlink_directory.mkdir()
        symlink_archive = symlink_directory / f"{ROOT}.zip"
        with zipfile.ZipFile(symlink_archive, "w") as zipped:
            link = zipfile.ZipInfo(f"{ROOT}/lib/cmake/LuaCpp/LuaCppConfig.cmake")
            link.create_system = 3
            link.external_attr = (stat.S_IFLNK | 0o777) << 16
            zipped.writestr(link, "/tmp/forbidden")
        expect_rejected(
            "symlink archive",
            lambda: verify_release_package_consumer(
                archive=symlink_archive,
                expected_version=VERSION,
                expected_rid=RID,
                consumer_source=consumer_source,
                runner=FakeRunner(),
                workspace=directory / "symlink-work",
            ),
            "link or special file",
        )

        missing_directory = directory / "missing"
        missing_directory.mkdir()
        missing_archive = missing_directory / f"{ROOT}.zip"
        with zipfile.ZipFile(missing_archive, "w") as zipped:
            zipped.writestr(f"{ROOT}/README.txt", "no package config")
        expect_rejected(
            "archive without package config",
            lambda: verify_release_package_consumer(
                archive=missing_archive,
                expected_version=VERSION,
                expected_rid=RID,
                consumer_source=consumer_source,
                runner=FakeRunner(),
                workspace=directory / "missing-work",
            ),
            "missing LuaCppConfig.cmake",
        )

        expect_rejected(
            "escaped CMake cache binding",
            lambda: verify_release_package_consumer(
                archive=archive,
                expected_version=VERSION,
                expected_rid=RID,
                consumer_source=consumer_source,
                runner=FakeRunner(escape_cache=True),
                workspace=directory / "escaped-cache-work",
            ),
            "escaped the extracted package root",
        )
        expect_rejected(
            "missing shared consumer test",
            lambda: verify_release_package_consumer(
                archive=archive,
                expected_version=VERSION,
                expected_rid=RID,
                consumer_source=consumer_source,
                runner=FakeRunner(
                    discovered_tests={"installed_lua_cpp_consumer"}
                ),
                workspace=directory / "missing-test-work",
            ),
            "test set mismatch",
        )
        expect_rejected(
            "consumer build failure",
            lambda: verify_release_package_consumer(
                archive=archive,
                expected_version=VERSION,
                expected_rid=RID,
                consumer_source=consumer_source,
                runner=FakeRunner(failure_label="build"),
                workspace=directory / "build-failure-work",
            ),
            "build failed with exit code 19",
        )
        expect_rejected(
            "consumer runtime test failure",
            lambda: verify_release_package_consumer(
                archive=archive,
                expected_version=VERSION,
                expected_rid=RID,
                consumer_source=consumer_source,
                runner=FakeRunner(failure_label="test"),
                workspace=directory / "test-failure-work",
            ),
            "tests failed with exit code 19",
        )

    print("Downloaded release package consumer contract passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
