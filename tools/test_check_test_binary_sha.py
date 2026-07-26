#!/usr/bin/env python3
"""Negative and positive probes for check_test_binary_sha.py."""

from __future__ import annotations

import subprocess
import unittest
from pathlib import Path

import check_test_binary_sha as checker


SHA_A = "a" * 40
SHA_B = "b" * 40


def completed(
    *,
    returncode: int = 0,
    stdout: str = "",
    stderr: str = "",
) -> subprocess.CompletedProcess[str]:
    return subprocess.CompletedProcess(
        args=[],
        returncode=returncode,
        stdout=stdout,
        stderr=stderr,
    )


class SequenceRunner:
    def __init__(self, *results: subprocess.CompletedProcess[str]) -> None:
        self.results = list(results)
        self.commands: list[list[str]] = []

    def __call__(self, command: list[str], **_: object) -> subprocess.CompletedProcess[str]:
        self.commands.append(command)
        if not self.results:
            raise AssertionError("Unexpected command execution")
        return self.results.pop(0)


class BinaryShaContractTests(unittest.TestCase):
    def test_explicit_override_match_succeeds(self) -> None:
        runner = SequenceRunner(completed(stdout=f"Build Git SHA: {SHA_A}\n"))

        actual = checker.verify_contract(
            Path("lua_test"),
            expected_sha=SHA_A.upper(),
            runner=runner,
        )

        self.assertEqual(actual, SHA_A)
        self.assertEqual(runner.commands, [["lua_test", "--build-info"]])

    def test_live_git_head_match_succeeds(self) -> None:
        runner = SequenceRunner(
            completed(stdout=f"{SHA_A}\n"),
            completed(stdout=f"Build Git SHA: {SHA_A}\n"),
        )

        actual = checker.verify_contract(
            Path("lua_test"),
            source_dir=Path("source"),
            git_executable="git-tool",
            runner=runner,
        )

        self.assertEqual(actual, SHA_A)
        self.assertEqual(
            runner.commands[0],
            ["git-tool", "-C", "source", "rev-parse", "--verify", "HEAD"],
        )

    def test_head_change_after_configuration_is_rejected(self) -> None:
        runner = SequenceRunner(
            completed(stdout=f"{SHA_B}\n"),
            completed(stdout=f"Build Git SHA: {SHA_A}\n"),
        )

        with self.assertRaisesRegex(
            checker.ContractError,
            f"binary={SHA_A}, source={SHA_B}",
        ):
            checker.verify_contract(
                Path("lua_test"),
                source_dir=Path("source"),
                runner=runner,
            )

    def test_unknown_override_is_rejected_before_binary_runs(self) -> None:
        runner = SequenceRunner()

        with self.assertRaisesRegex(checker.ContractError, "exactly 40 hexadecimal"):
            checker.verify_contract(
                Path("lua_test"),
                expected_sha="unknown",
                runner=runner,
            )

        self.assertEqual(runner.commands, [])

    def test_git_failure_is_not_downgraded(self) -> None:
        runner = SequenceRunner(completed(returncode=128, stderr="not a work tree"))

        with self.assertRaisesRegex(checker.ContractError, "exit 128"):
            checker.verify_contract(
                Path("lua_test"),
                source_dir=Path("source"),
                runner=runner,
            )

    def test_malformed_git_output_is_rejected(self) -> None:
        runner = SequenceRunner(completed(stdout="unknown\n"))

        with self.assertRaisesRegex(checker.ContractError, "exactly 40 hexadecimal"):
            checker.verify_contract(
                Path("lua_test"),
                source_dir=Path("source"),
                runner=runner,
            )

    def test_multiple_git_lines_are_rejected(self) -> None:
        runner = SequenceRunner(completed(stdout=f"{SHA_A}\n{SHA_B}\n"))

        with self.assertRaisesRegex(checker.ContractError, "exactly one 40-hex line"):
            checker.verify_contract(
                Path("lua_test"),
                source_dir=Path("source"),
                runner=runner,
            )

    def test_nonzero_binary_exit_is_rejected(self) -> None:
        runner = SequenceRunner(completed(returncode=7, stderr="fixture failure"))

        with self.assertRaisesRegex(checker.ContractError, "exit 7"):
            checker.verify_contract(
                Path("lua_test"),
                expected_sha=SHA_A,
                runner=runner,
            )

    def test_missing_build_sha_line_is_rejected(self) -> None:
        runner = SequenceRunner(completed(stdout="Build type: Debug\n"))

        with self.assertRaisesRegex(checker.ContractError, "exactly one"):
            checker.verify_contract(
                Path("lua_test"),
                expected_sha=SHA_A,
                runner=runner,
            )

    def test_duplicate_build_sha_lines_are_rejected(self) -> None:
        runner = SequenceRunner(
            completed(
                stdout=(
                    f"Build Git SHA: {SHA_A}\n"
                    f"Build Git SHA: {SHA_A}\n"
                )
            )
        )

        with self.assertRaisesRegex(checker.ContractError, "got 2"):
            checker.verify_contract(
                Path("lua_test"),
                expected_sha=SHA_A,
                runner=runner,
            )

    def test_malformed_build_sha_line_is_rejected(self) -> None:
        runner = SequenceRunner(completed(stdout="Build Git SHA: unknown\n"))

        with self.assertRaisesRegex(checker.ContractError, "malformed"):
            checker.verify_contract(
                Path("lua_test"),
                expected_sha=SHA_A,
                runner=runner,
            )

    def test_candidate_modes_are_mutually_exclusive(self) -> None:
        runner = SequenceRunner()

        with self.assertRaisesRegex(checker.ContractError, "exactly one"):
            checker.verify_contract(Path("lua_test"), runner=runner)
        with self.assertRaisesRegex(checker.ContractError, "exactly one"):
            checker.verify_contract(
                Path("lua_test"),
                expected_sha=SHA_A,
                source_dir=Path("source"),
                runner=runner,
            )


if __name__ == "__main__":
    unittest.main(verbosity=2)
