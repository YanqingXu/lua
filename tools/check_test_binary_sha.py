#!/usr/bin/env python3
"""Verify that lua_test reports the current source build identity."""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path
from typing import Callable, Sequence


SHA_PATTERN = re.compile(r"^[0-9a-fA-F]{40}$")
BUILD_SHA_LINE_PATTERN = re.compile(r"^Build Git SHA: ([0-9a-fA-F]{40})$")
CommandRunner = Callable[..., subprocess.CompletedProcess[str]]


class ContractError(RuntimeError):
    """Raised when build provenance does not satisfy the SHA contract."""


def normalize_sha(value: str, source: str) -> str:
    if SHA_PATTERN.fullmatch(value) is None:
        raise ContractError(
            f"{source} must be exactly 40 hexadecimal characters; got {value!r}"
        )
    return value.lower()


def _run_command(
    command: Sequence[str],
    description: str,
    runner: CommandRunner | None,
) -> subprocess.CompletedProcess[str]:
    command_runner = runner or subprocess.run
    try:
        return command_runner(
            list(command),
            capture_output=True,
            text=True,
            timeout=30,
            check=False,
        )
    except (OSError, subprocess.TimeoutExpired) as error:
        raise ContractError(f"Unable to run {description}: {error}") from error


def resolve_git_head(
    source_dir: Path,
    git_executable: str,
    runner: CommandRunner | None = None,
) -> str:
    result = _run_command(
        [
            git_executable,
            "-C",
            str(source_dir),
            "rev-parse",
            "--verify",
            "HEAD",
        ],
        "Git while resolving the current source HEAD",
        runner,
    )
    if result.returncode != 0:
        detail = (result.stderr or "").strip() or "<no stderr>"
        raise ContractError(
            "Unable to resolve the current source Git HEAD "
            f"(exit {result.returncode}): {detail}"
        )

    output_lines = (result.stdout or "").splitlines()
    if len(output_lines) != 1:
        raise ContractError(
            "Git HEAD output must contain exactly one 40-hex line; "
            f"got {len(output_lines)} lines"
        )
    return normalize_sha(output_lines[0], "current source Git HEAD")


def read_binary_sha(
    test_executable: Path,
    runner: CommandRunner | None = None,
) -> str:
    result = _run_command(
        [str(test_executable), "--build-info"],
        "lua_test --build-info",
        runner,
    )
    if result.returncode != 0:
        detail = (result.stderr or "").strip() or "<no stderr>"
        raise ContractError(
            f"lua_test --build-info failed with exit {result.returncode}: {detail}"
        )

    build_sha_lines = [
        line for line in (result.stdout or "").splitlines() if "Build Git SHA:" in line
    ]
    if len(build_sha_lines) != 1:
        raise ContractError(
            "lua_test --build-info must emit exactly one 'Build Git SHA:' line; "
            f"got {len(build_sha_lines)}"
        )

    match = BUILD_SHA_LINE_PATTERN.fullmatch(build_sha_lines[0])
    if match is None:
        raise ContractError(
            "lua_test --build-info emitted a malformed build SHA line: "
            f"{build_sha_lines[0]!r}"
        )
    return normalize_sha(match.group(1), "lua_test build SHA")


def verify_contract(
    test_executable: Path,
    *,
    expected_sha: str | None = None,
    source_dir: Path | None = None,
    git_executable: str = "git",
    runner: CommandRunner | None = None,
) -> str:
    if (expected_sha is None) == (source_dir is None):
        raise ContractError(
            "Specify exactly one source candidate: expected_sha or source_dir"
        )

    if expected_sha is not None:
        candidate_sha = normalize_sha(expected_sha, "configured SHA override")
    else:
        assert source_dir is not None
        candidate_sha = resolve_git_head(source_dir, git_executable, runner)

    binary_sha = read_binary_sha(test_executable, runner)
    if binary_sha != candidate_sha:
        raise ContractError(
            "lua_test build SHA does not match the current source candidate: "
            f"binary={binary_sha}, source={candidate_sha}"
        )
    return binary_sha


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Compare lua_test --build-info with either the live source Git HEAD "
            "or an explicit 40-hex source-archive override."
        )
    )
    parser.add_argument(
        "--test-executable",
        type=Path,
        required=True,
        help="Path to the lua_test executable",
    )
    candidate_group = parser.add_mutually_exclusive_group(required=True)
    candidate_group.add_argument(
        "--expected-sha",
        help="Explicit 40-hex source candidate used for a Git-less build",
    )
    candidate_group.add_argument(
        "--source-dir",
        type=Path,
        help="Git worktree whose HEAD is resolved at test runtime",
    )
    parser.add_argument(
        "--git-executable",
        default="git",
        help="Git executable used with --source-dir (default: git)",
    )
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    arguments = build_parser().parse_args(argv)
    try:
        verified_sha = verify_contract(
            arguments.test_executable,
            expected_sha=arguments.expected_sha,
            source_dir=arguments.source_dir,
            git_executable=arguments.git_executable,
        )
    except ContractError as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1

    print(f"PASS: lua_test build SHA matches source candidate {verified_sha}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
