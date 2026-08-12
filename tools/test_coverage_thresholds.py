#!/usr/bin/env python3
"""Contract tests for component coverage thresholds."""

from __future__ import annotations

import json
import tempfile
from pathlib import Path

from summarize_coverage import COMPONENTS, apply_thresholds, load_thresholds, summarize


def coverage_file(filename: str, covered: int, count: int) -> dict[str, object]:
    return {
        "filename": filename,
        "summary": {"lines": {"covered": covered, "count": count}},
    }


def main() -> int:
    files = [
        coverage_file("/repo/src/compiler/parser/parser.cpp", 90, 100),
        coverage_file("/repo/src/vm/vm_handlers.cpp", 90, 100),
        coverage_file("/repo/src/gc/gc_mark.cpp", 90, 100),
        coverage_file("/repo/src/api/lapi.cpp", 90, 100),
        coverage_file("/repo/src/runtime/bytecode_verifier.cpp", 90, 100),
        coverage_file("/repo/src/debugger/debug_runtime.cpp", 90, 100),
        coverage_file("/repo/src/runtime/sandbox_policy.hpp", 90, 100),
    ]
    report = summarize(files)
    assert report["schemaVersion"] == 2
    assert set(report["components"]) == set(COMPONENTS)

    with tempfile.TemporaryDirectory() as temporary:
        path = Path(temporary) / "thresholds.json"
        path.write_text(
            json.dumps(
                {
                    "schemaVersion": 1,
                    "components": {name: 89.0 for name in COMPONENTS},
                }
            ),
            encoding="utf-8",
        )
        thresholds = load_thresholds(path)
        assert apply_thresholds(report, thresholds) == []
        assert report["thresholdsPassed"] is True

        report = summarize(files)
        thresholds["c_api"] = 91.0
        violations = apply_thresholds(report, thresholds)
        assert violations == ["c_api: 90.00% < 91.00%"]
        assert report["thresholdsPassed"] is False

        path.write_text(
            json.dumps(
                {
                    "schemaVersion": 1,
                    "components": {"unknown": 1.0},
                }
            ),
            encoding="utf-8",
        )
        try:
            load_thresholds(path)
        except RuntimeError as error:
            assert "component mismatch" in str(error)
        else:
            raise AssertionError("mismatched component policy was accepted")

    print("Coverage threshold policy contract passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
