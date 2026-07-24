#!/usr/bin/env python3
"""Verify that arbitrary Lua error bytes still produce one valid JSON result."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 3:
        raise RuntimeError("usage: verify_worker_json.py <worker> <script>")

    worker = Path(sys.argv[1]).resolve()
    script = Path(sys.argv[2]).resolve()
    result = subprocess.run(
        [
            str(worker),
            "--instruction-budget",
            "100000",
            "--timeout-ms",
            "1000",
            str(script),
        ],
        check=False,
        capture_output=True,
    )
    if result.returncode != 20:
        raise RuntimeError(
            f"worker returned {result.returncode}, stderr={result.stderr.decode('utf-8', errors='replace')!r}"
        )
    if b"\xff" in result.stdout:
        raise RuntimeError("worker emitted a raw invalid UTF-8 byte")

    text = result.stdout.decode("ascii")
    lines = text.splitlines()
    if len(lines) != 1 or not text.endswith("\n"):
        raise RuntimeError(f"worker did not emit exactly one newline-terminated result: {text!r}")
    payload = json.loads(lines[0])
    if payload.get("schema") != 1 or payload.get("outcome") != "runtime_error":
        raise RuntimeError(f"unexpected worker result: {payload!r}")
    if payload.get("allocator_live_bytes") != 0:
        raise RuntimeError("worker allocator did not return to zero")
    if "\u00ff" not in payload.get("message", ""):
        raise RuntimeError("escaped error byte was not preserved in the parsed message")

    print("Production worker arbitrary-byte JSON contract passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
