#!/usr/bin/env python3
"""Run high-signal clang-tidy checks over every project translation unit."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import subprocess
import sys


PROJECT_ROOTS = {"src", "tests", "benchmarks", "examples"}
HIGH_SIGNAL_CHECKS = (
    "-*",
    "bugprone-implicit-widening-of-multiplication-result",
    "bugprone-suspicious-stringview-data-usage",
    "bugprone-unused-return-value",
    "portability-*",
    "-portability-template-virtual-member-function",
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", required=True, help="directory containing compile_commands.json")
    parser.add_argument("--clang-tidy", default="clang-tidy", help="clang-tidy executable")
    return parser.parse_args()


def project_translation_units(root: Path, database_path: Path) -> list[str]:
    with database_path.open(encoding="utf-8") as database_file:
        database = json.load(database_file)

    files: set[str] = set()
    for entry in database:
        source = Path(entry["file"])
        if not source.is_absolute():
            source = Path(entry["directory"]) / source
        source = source.resolve()
        try:
            relative = source.relative_to(root)
        except ValueError:
            continue
        if relative.parts and relative.parts[0] in PROJECT_ROOTS and source.suffix in {".cc", ".cpp", ".cxx"}:
            files.add(str(source))
    return sorted(files)


def main() -> int:
    args = parse_args()
    root = Path(__file__).resolve().parent.parent
    build_dir = Path(args.build_dir)
    if not build_dir.is_absolute():
        build_dir = root / build_dir
    database_path = build_dir / "compile_commands.json"
    if not database_path.is_file():
        print(f"missing compile database: {database_path}", file=sys.stderr)
        return 2

    files = project_translation_units(root, database_path)
    if not files:
        print("compile database contains no project translation units", file=sys.stderr)
        return 2

    checks = ",".join(HIGH_SIGNAL_CHECKS)
    print(f"clang-tidy: checking {len(files)} project translation units")
    completed = subprocess.run(
        [
            args.clang_tidy,
            "-p",
            str(build_dir),
            "--quiet",
            f"--checks={checks}",
            "--warnings-as-errors=*",
            *files,
        ],
        cwd=root,
        check=False,
    )
    return completed.returncode


if __name__ == "__main__":
    raise SystemExit(main())
