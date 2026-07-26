#!/usr/bin/env python3
"""Contract tests for normalized source-readiness evidence."""

from __future__ import annotations

import json
import unittest
from datetime import datetime, timezone

import verify_source_readiness_evidence as readiness


REPOSITORY = "example/lua"
CANDIDATE_SHA = "a" * 40
VERSION = "0.1.0-rc.1"
NOW = datetime(2026, 7, 26, 12, 0, 0, tzinfo=timezone.utc)


def valid_evidence() -> dict[str, object]:
    return {
        "schema": readiness.EVIDENCE_SCHEMA,
        "generated_at": "2026-07-26T11:59:00Z",
        "repository": REPOSITORY,
        "candidate_sha": CANDIDATE_SHA,
        "version": VERSION,
        "project_version": "0.1.0",
        "abi_version": 0,
        "checks": {name: "passed" for name in readiness.CHECK_NAMES},
    }


class SourceReadinessEvidenceTests(unittest.TestCase):
    def validate(self, value: dict[str, object]) -> dict[str, object]:
        return readiness.validate_source_readiness_evidence(
            value,
            expected_repository=REPOSITORY,
            expected_sha=CANDIDATE_SHA,
            expected_version=VERSION,
            now=NOW,
        )

    def test_valid_evidence_binds_version_abi_and_exact_source(self) -> None:
        result = self.validate(valid_evidence())
        self.assertEqual(VERSION, result["version"])
        self.assertEqual(0, result["abi_version"])
        self.assertEqual(CANDIDATE_SHA, result["candidate_sha"])
        self.assertEqual({"passed"}, set(result["checks"].values()))

    def test_missing_extra_or_failed_check_is_rejected(self) -> None:
        cases = (
            lambda value: value.pop("abi_version"),
            lambda value: value.__setitem__("approved", True),
            lambda value: value["checks"].__setitem__(
                "public_api_contract",
                "failed",
            ),
            lambda value: value["checks"].pop("worktree_clean"),
        )
        for mutate in cases:
            with self.subTest(mutate=mutate):
                value = valid_evidence()
                mutate(value)
                with self.assertRaises(readiness.SourceReadinessError):
                    self.validate(value)

    def test_wrong_repository_sha_version_or_project_version_is_rejected(self) -> None:
        cases = (
            ("repository", "other/lua"),
            ("candidate_sha", "b" * 40),
            ("version", "0.1.0"),
            ("project_version", "0.2.0"),
        )
        for field, replacement in cases:
            with self.subTest(field=field):
                value = valid_evidence()
                value[field] = replacement
                with self.assertRaises(readiness.SourceReadinessError):
                    self.validate(value)

    def test_abi_must_be_non_negative_integer(self) -> None:
        for replacement in (True, -1, "0", 1.5):
            with self.subTest(value=replacement):
                value = valid_evidence()
                value["abi_version"] = replacement
                with self.assertRaises(readiness.SourceReadinessError):
                    self.validate(value)

    def test_timestamp_must_be_canonical_and_not_future(self) -> None:
        for replacement in (
            "2026-07-26T12:00:01Z",
            "2026-07-26T11:59:00+00:00",
        ):
            with self.subTest(value=replacement):
                value = valid_evidence()
                value["generated_at"] = replacement
                with self.assertRaises(readiness.SourceReadinessError):
                    self.validate(value)

    def test_parser_rejects_duplicate_keys(self) -> None:
        raw = json.dumps(valid_evidence(), separators=(",", ":"))
        duplicate = raw[:-1] + ',"abi_version":0}'
        with self.assertRaises(readiness.SourceReadinessError):
            readiness.parse_source_readiness_json(duplicate)


if __name__ == "__main__":
    unittest.main()
