#!/usr/bin/env python3
"""Prove that filtered lua_test invocations cannot succeed without running tests."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path
from typing import Sequence


EMPTY_SELECTION_DIAGNOSTIC = "Test selection matched zero registered tests"
KNOWN_FILTER = "Test Framework Contract"
MISSING_FILTER = "__lua_cpp_contract_missing_test__"


class SelectionContractError(RuntimeError):
    """Raised when lua_test does not fail closed for an empty selection."""


def run_test_binary(executable: Path, *arguments: str) -> subprocess.CompletedProcess[str]:
    try:
        return subprocess.run(
            [str(executable), *arguments],
            capture_output=True,
            text=True,
            timeout=60,
            check=False,
        )
    except (OSError, subprocess.TimeoutExpired) as error:
        raise SelectionContractError(f"Unable to run lua_test: {error}") from error


def require_empty_selection_failure(
    executable: Path,
    *arguments: str,
) -> None:
    result = run_test_binary(executable, *arguments)
    combined = f"{result.stdout}\n{result.stderr}"
    if result.returncode != 2:
        raise SelectionContractError(
            f"empty selection returned {result.returncode}, expected 2"
        )
    if EMPTY_SELECTION_DIAGNOSTIC not in combined:
        raise SelectionContractError("empty selection diagnostic is missing")
    if "[OK] ALL TESTS PASSED!" in combined:
        raise SelectionContractError("empty selection was reported as a passing test run")


def verify_selection_contract(executable: Path) -> None:
    if not executable.is_file():
        raise SelectionContractError(f"lua_test executable is missing: {executable}")

    positive = run_test_binary(executable, "--filter", KNOWN_FILTER)
    if positive.returncode != 0:
        raise SelectionContractError(
            f"known filter failed with exit {positive.returncode}: {positive.stderr.strip()}"
        )
    if "Selected Tests:" not in positive.stdout or "[OK] ALL TESTS PASSED!" not in positive.stdout:
        raise SelectionContractError("known filter did not execute and report a passing selection")

    require_empty_selection_failure(executable, "--filter", MISSING_FILTER)
    require_empty_selection_failure(
        executable,
        "--filter",
        KNOWN_FILTER,
        "--exclude-filter",
        KNOWN_FILTER,
    )


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--test-executable", type=Path, required=True)
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = _parser().parse_args(argv)
    try:
        verify_selection_contract(args.test_executable)
    except SelectionContractError as error:
        print(f"test selection contract failed: {error}", file=sys.stderr)
        return 1
    print("Test selection contract passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
