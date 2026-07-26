#!/usr/bin/env python3
"""Contract tests for strict structured release-governance evidence."""

from __future__ import annotations

import copy
import json
import tempfile
import unittest
from datetime import datetime, timezone
from pathlib import Path

import verify_release_governance as governance


REPOSITORY = "example/lua"
CANDIDATE_SHA = "a" * 40
VERSION = "0.1.0-rc.1"
TAG = f"v{VERSION}"
NOW = datetime(2026, 7, 26, 12, 0, 0, tzinfo=timezone.utc)


def controls(state: str = "enforced") -> dict[str, str]:
    return {name: state for name in governance.CONTROL_NAMES}


def protected_attestation() -> dict[str, object]:
    return {
        "schema": governance.ATTESTATION_SCHEMA,
        "repository": REPOSITORY,
        "candidate_sha": CANDIDATE_SHA,
        "tag": TAG,
        "version": VERSION,
        "decision": "protected-ruleset",
        "approved_by": "release-owner",
        "independent_reviewer": "independent-reviewer",
        "approved_at": "2026-07-25T12:00:00Z",
        "expires_at": "2026-08-25T12:00:00Z",
        "record_url": "https://github.com/example/lua/issues/6#issuecomment-1",
        "controls": controls(),
        "risk_acceptance": None,
        "compensating_controls": [],
    }


def waiver_attestation() -> dict[str, object]:
    value = protected_attestation()
    value["decision"] = "time-limited-waiver"
    value["expires_at"] = "2026-08-20T12:00:00Z"
    value["risk_acceptance"] = "Accept direct-push risk for this exact RC only."
    value["compensating_controls"] = [
        "Two-person annotated-tag review.",
        "Exact-SHA CI and two nightly runs are mandatory.",
    ]
    value["controls"]["branch_must_be_up_to_date"] = "waived"
    return value


def encoded(value: object) -> str:
    return json.dumps(value, sort_keys=True, separators=(",", ":"))


def approved_evidence(
    attestation: dict[str, object] | None = None,
) -> dict[str, object]:
    return governance.build_governance_evidence(
        event="push",
        repository=REPOSITORY,
        candidate_sha=CANDIDATE_SHA,
        tag=TAG,
        version=VERSION,
        attestation_json=encoded(attestation or protected_attestation()),
        now=NOW,
    )


class GovernanceAttestationTests(unittest.TestCase):
    def validate(self, value: dict[str, object]) -> dict[str, object]:
        return governance.validate_attestation(
            value,
            expected_repository=REPOSITORY,
            expected_sha=CANDIDATE_SHA,
            expected_tag=TAG,
            expected_version=VERSION,
            now=NOW,
        )

    def test_protected_ruleset_requires_all_six_controls(self) -> None:
        result = self.validate(protected_attestation())
        self.assertEqual("protected-ruleset", result["decision"])
        self.assertEqual(set(governance.CONTROL_NAMES), set(result["controls"]))
        self.assertEqual({"enforced"}, set(result["controls"].values()))

    def test_valid_time_limited_waiver_is_normalized(self) -> None:
        result = self.validate(waiver_attestation())
        self.assertEqual("time-limited-waiver", result["decision"])
        self.assertIn("waived", result["controls"].values())
        self.assertTrue(result["risk_acceptance"])
        self.assertEqual(2, len(result["compensating_controls"]))

    def test_missing_and_extra_attestation_fields_fail_closed(self) -> None:
        for mutate in (
            lambda value: value.pop("approved_by"),
            lambda value: value.__setitem__("approved", True),
        ):
            with self.subTest(mutate=mutate):
                value = protected_attestation()
                mutate(value)
                with self.assertRaises(governance.GovernanceError):
                    self.validate(value)

    def test_duplicate_keys_and_non_object_old_boolean_are_rejected(self) -> None:
        raw = encoded(protected_attestation())
        duplicate = raw[:-1] + ',"repository":"example/lua"}'
        for candidate in (duplicate, "true"):
            with self.subTest(candidate=candidate[-20:]):
                with self.assertRaises(governance.GovernanceError):
                    governance.parse_attestation_json(candidate)

    def test_multiline_or_surrounding_whitespace_is_rejected(self) -> None:
        raw = encoded(protected_attestation())
        for candidate in (raw + "\n", f" {raw}", raw.replace(",", ",\n", 1)):
            with self.subTest():
                with self.assertRaises(governance.GovernanceError):
                    governance.parse_attestation_json(candidate)

    def test_exact_repository_sha_tag_and_version_are_required(self) -> None:
        cases = (
            ("repository", "other/lua"),
            ("candidate_sha", "b" * 40),
            ("tag", "v0.1.0"),
            ("version", "0.1.0"),
        )
        for field, replacement in cases:
            with self.subTest(field=field):
                value = protected_attestation()
                value[field] = replacement
                with self.assertRaises(governance.GovernanceError):
                    self.validate(value)

    def test_approval_must_be_current_and_canonical_utc(self) -> None:
        cases = (
            ("approved_at", "2026-07-26T12:00:01Z"),
            ("approved_at", "2026-07-25T12:00:00+00:00"),
            ("expires_at", "2026-07-26T12:00:00Z"),
            ("expires_at", "2026-08-25T12:00:00+00:00"),
        )
        for field, replacement in cases:
            with self.subTest(field=field, replacement=replacement):
                value = protected_attestation()
                value[field] = replacement
                with self.assertRaises(governance.GovernanceError):
                    self.validate(value)

    def test_record_url_must_be_github_and_bound_to_repository(self) -> None:
        for replacement in (
            "https://example.com/example/lua/issues/6",
            "https://github.com/other/lua/issues/6",
            "http://github.com/example/lua/issues/6",
            "https://github.com.evil.test/example/lua/issues/6",
        ):
            with self.subTest(url=replacement):
                value = protected_attestation()
                value["record_url"] = replacement
                with self.assertRaises(governance.GovernanceError):
                    self.validate(value)

    def test_approver_and_reviewer_must_be_independent(self) -> None:
        value = protected_attestation()
        value["independent_reviewer"] = "Release-Owner"
        with self.assertRaises(governance.GovernanceError):
            self.validate(value)

    def test_ruleset_rejects_waived_or_waiver_only_fields(self) -> None:
        for mutate in (
            lambda value: value["controls"].__setitem__(
                "tag_creation_restricted", "waived"
            ),
            lambda value: value.__setitem__("risk_acceptance", "not applicable"),
            lambda value: value["compensating_controls"].append("not applicable"),
        ):
            with self.subTest(mutate=mutate):
                value = protected_attestation()
                mutate(value)
                with self.assertRaises(governance.GovernanceError):
                    self.validate(value)

    def test_waiver_requires_waived_control_risk_and_compensation(self) -> None:
        cases = (
            lambda value: value.__setitem__("controls", controls()),
            lambda value: value.__setitem__("risk_acceptance", ""),
            lambda value: value.__setitem__("compensating_controls", []),
        )
        for mutate in cases:
            with self.subTest(mutate=mutate):
                value = waiver_attestation()
                mutate(value)
                with self.assertRaises(governance.GovernanceError):
                    self.validate(value)

    def test_waiver_cannot_exceed_thirty_days(self) -> None:
        value = waiver_attestation()
        value["expires_at"] = "2026-08-25T12:00:01Z"
        with self.assertRaises(governance.GovernanceError):
            self.validate(value)


class GovernanceEvidenceTests(unittest.TestCase):
    def test_tag_push_creates_approved_deeply_validated_evidence(self) -> None:
        evidence = approved_evidence()
        result = governance.validate_governance_evidence(
            evidence,
            expected_repository=REPOSITORY,
            expected_sha=CANDIDATE_SHA,
            expected_tag=TAG,
            expected_version=VERSION,
            now=NOW,
            require_approved=True,
        )
        self.assertEqual("approved", result["publication"])
        self.assertEqual("protected-ruleset", result["attestation"]["decision"])

    def test_manual_dispatch_is_candidate_only_even_if_variable_exists(self) -> None:
        evidence = governance.build_governance_evidence(
            event="workflow_dispatch",
            repository=REPOSITORY,
            candidate_sha=CANDIDATE_SHA,
            tag=None,
            version=VERSION,
            attestation_json=encoded(protected_attestation()),
            now=NOW,
        )
        self.assertEqual("candidate-only", evidence["publication"])
        self.assertIsNone(evidence["tag"])
        self.assertIsNone(evidence["attestation"])
        with self.assertRaises(governance.GovernanceError):
            governance.validate_governance_evidence(
                evidence,
                expected_repository=REPOSITORY,
                expected_sha=CANDIDATE_SHA,
                expected_tag=None,
                expected_version=VERSION,
                now=NOW,
                require_approved=True,
            )

    def test_evidence_rejects_extra_fields_and_mutated_nested_attestation(self) -> None:
        for mutate in (
            lambda value: value.__setitem__("approved", True),
            lambda value: value["attestation"].__setitem__("candidate_sha", "b" * 40),
        ):
            with self.subTest(mutate=mutate):
                evidence = approved_evidence()
                mutate(evidence)
                with self.assertRaises(governance.GovernanceError):
                    governance.validate_governance_evidence(
                        evidence,
                        expected_repository=REPOSITORY,
                        expected_sha=CANDIDATE_SHA,
                        expected_tag=TAG,
                        expected_version=VERSION,
                        now=NOW,
                    )

    def test_approved_evidence_cannot_predate_its_attestation(self) -> None:
        evidence = approved_evidence()
        evidence["generated_at"] = "2026-07-24T12:00:00Z"
        with self.assertRaisesRegex(
            governance.GovernanceError,
            "predates its approval",
        ):
            governance.validate_governance_evidence(
                evidence,
                expected_repository=REPOSITORY,
                expected_sha=CANDIDATE_SHA,
                expected_tag=TAG,
                expected_version=VERSION,
                now=NOW,
            )

    def test_cli_removes_stale_output_when_boolean_cannot_authorize_tag(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "governance-evidence.json"
            output.write_text("stale", encoding="utf-8")
            exit_code = governance.main(
                [
                    "--event",
                    "push",
                    "--repository",
                    REPOSITORY,
                    "--sha",
                    CANDIDATE_SHA,
                    "--tag",
                    TAG,
                    "--version",
                    VERSION,
                    "--attestation-json",
                    "true",
                    "--output",
                    str(output),
                ],
                now=NOW,
            )
            self.assertEqual(1, exit_code)
            self.assertFalse(output.exists())

    def test_cli_writes_normalized_json_atomically(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "governance-evidence.json"
            exit_code = governance.main(
                [
                    "--event",
                    "push",
                    "--repository",
                    REPOSITORY,
                    "--sha",
                    CANDIDATE_SHA,
                    "--tag",
                    TAG,
                    "--version",
                    VERSION,
                    "--attestation-json",
                    encoded(waiver_attestation()),
                    "--output",
                    str(output),
                ],
                now=NOW,
            )
            self.assertEqual(0, exit_code)
            parsed = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual("approved", parsed["publication"])
            self.assertEqual("time-limited-waiver", parsed["attestation"]["decision"])


if __name__ == "__main__":
    unittest.main()
