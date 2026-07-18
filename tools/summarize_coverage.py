#!/usr/bin/env python3
"""Summarize llvm-cov export data by runtime security/compatibility component."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


COMPONENTS: dict[str, tuple[str, ...]] = {
    "parser_codegen": ("/src/compiler/parser/", "/src/compiler/lexer/", "/src/compiler/codegen/"),
    "opcode_handlers": ("/src/vm/vm_handlers", "/src/vm/vm_loop.cpp", "/src/vm/vm.cpp"),
    "gc_phases": ("/src/gc/",),
    "c_api": ("/src/api/",),
    "bytecode_verifier": ("/src/runtime/bytecode_verifier.cpp",),
    "sandbox_denied_paths": (
        "/src/runtime/sandbox_policy.hpp",
        "/src/vm/state/global_state.cpp",
        "/src/lib/baselib.cpp",
        "/src/lib/lib_manager.cpp",
    ),
}


def normalized(filename: str) -> str:
    return filename.replace("\\", "/")


def summarize(files: list[dict[str, Any]]) -> dict[str, Any]:
    report: dict[str, Any] = {"schemaVersion": 1, "components": {}}
    missing: list[str] = []
    for component, patterns in COMPONENTS.items():
        matched = [entry for entry in files if any(pattern in normalized(entry["filename"]) for pattern in patterns)]
        line_count = sum(int(entry["summary"]["lines"]["count"]) for entry in matched)
        covered = sum(int(entry["summary"]["lines"]["covered"]) for entry in matched)
        percent = 100.0 if line_count == 0 else covered * 100.0 / line_count
        report["components"][component] = {
            "files": len(matched),
            "coveredLines": covered,
            "totalLines": line_count,
            "linePercent": round(percent, 2),
        }
        if not matched or line_count == 0:
            missing.append(component)

    if missing:
        raise RuntimeError("coverage export has no executable lines for: " + ", ".join(missing))
    return report


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("export", type=Path, help="llvm-cov export JSON")
    parser.add_argument("output", type=Path, help="component summary JSON")
    args = parser.parse_args()

    payload = json.loads(args.export.read_text(encoding="utf-8"))
    data = payload.get("data", [])
    if not data:
        raise RuntimeError("llvm-cov export contains no data")
    files: list[dict[str, Any]] = []
    for record in data:
        files.extend(record.get("files", []))

    report = summarize(files)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    for name, metrics in report["components"].items():
        print(
            f"{name}: {metrics['coveredLines']}/{metrics['totalLines']} lines "
            f"({metrics['linePercent']:.2f}%) across {metrics['files']} files"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
