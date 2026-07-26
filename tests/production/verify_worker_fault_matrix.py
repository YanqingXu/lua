#!/usr/bin/env python3
"""Exercise deterministic worker failure paths and validate their JSON contract."""

from __future__ import annotations

import argparse
import json
import platform
import re
import subprocess
from pathlib import Path
from typing import Any


REQUIRED_FIELDS: dict[str, type] = {
    "schema": int,
    "outcome": str,
    "lua_status": int,
    "runtime_status": int,
    "duration_us": int,
    "allocator_live_bytes": int,
    "allocator_peak_bytes": int,
    "allocator_limit_bytes": int,
    "process_memory_limit_bytes": int,
    "process_limits_enabled": bool,
    "instruction_budget": int,
    "native_work_budget": int,
    "timeout_ms": int,
    "consumed_instructions": int,
    "remaining_instruction_budget": int,
    "consumed_native_work": int,
    "remaining_native_work_budget": int,
    "last_stop_reason": int,
    "cancellation_requested": int,
    "message": str,
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--worker", required=True, type=Path)
    parser.add_argument("--scripts", required=True, type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--candidate-sha")
    parser.add_argument(
        "--enforce-process-limits",
        action="store_true",
        help="Install the worker's OS process limits instead of the sanitizer-safe test bypass.",
    )
    return parser.parse_args()


def validate_payload(
    *,
    scenario: str,
    payload: Any,
    expected_outcome: str,
    expected_stop_reason: int,
    expected_exit_code: int,
    expected_runtime_status: int,
    actual_exit_code: int,
    process_limits_enabled: bool,
) -> dict[str, Any]:
    if not isinstance(payload, dict):
        raise RuntimeError(f"{scenario}: result must be a JSON object")
    for field, expected_type in REQUIRED_FIELDS.items():
        if field not in payload:
            raise RuntimeError(f"{scenario}: missing JSON field {field!r}")
        if type(payload[field]) is not expected_type:
            raise RuntimeError(
                f"{scenario}: field {field!r} must be {expected_type.__name__}, "
                f"got {type(payload[field]).__name__}"
            )

    if payload["schema"] != 1:
        raise RuntimeError(f"{scenario}: unsupported worker schema {payload['schema']!r}")
    if actual_exit_code != expected_exit_code:
        raise RuntimeError(
            f"{scenario}: exit code {actual_exit_code}, expected {expected_exit_code}"
        )
    if payload["outcome"] != expected_outcome:
        raise RuntimeError(
            f"{scenario}: outcome {payload['outcome']!r}, expected {expected_outcome!r}"
        )
    if payload["last_stop_reason"] != expected_stop_reason:
        raise RuntimeError(
            f"{scenario}: stop reason {payload['last_stop_reason']}, "
            f"expected {expected_stop_reason}"
        )
    if payload["process_limits_enabled"] is not process_limits_enabled:
        raise RuntimeError(f"{scenario}: process limit mode does not match the invocation")
    if payload["allocator_live_bytes"] != 0:
        raise RuntimeError(f"{scenario}: allocator did not return to zero")
    if payload["allocator_peak_bytes"] > payload["allocator_limit_bytes"]:
        raise RuntimeError(f"{scenario}: allocator peak exceeded its hard limit")
    if payload["duration_us"] < 0:
        raise RuntimeError(f"{scenario}: duration must be non-negative")
    if payload["runtime_status"] != expected_runtime_status:
        raise RuntimeError(
            f"{scenario}: runtime status {payload['runtime_status']}, "
            f"expected {expected_runtime_status}"
        )

    if expected_outcome == "success":
        if payload["lua_status"] != 0 or actual_exit_code != 0:
            raise RuntimeError(f"{scenario}: success must agree across status and exit code")
    elif payload["lua_status"] == 0 or actual_exit_code == 0:
        raise RuntimeError(f"{scenario}: failure must agree across status and exit code")

    if expected_stop_reason == 4:
        if payload["cancellation_requested"] != 1:
            raise RuntimeError(f"{scenario}: cancellation flag was not published")
    elif payload["cancellation_requested"] != 0:
        raise RuntimeError(f"{scenario}: unexpected cancellation flag")

    if expected_stop_reason == 2:
        if payload["remaining_native_work_budget"] != 0:
            raise RuntimeError(f"{scenario}: native-work rejection did not exhaust its budget")
        if payload["consumed_native_work"] != payload["native_work_budget"]:
            raise RuntimeError(f"{scenario}: native-work consumption is inconsistent")

    return payload


def run_scenario(
    *,
    worker: Path,
    scripts: Path,
    process_limits_enabled: bool,
    name: str,
    script: str,
    arguments: list[str],
    expected_outcome: str,
    expected_stop_reason: int,
    expected_exit_code: int,
    expected_runtime_status: int = 0,
) -> dict[str, Any]:
    command = [str(worker)]
    if not process_limits_enabled:
        command.append("--skip-process-limits-for-tests")
    command.extend(arguments)
    command.append(str(scripts / script))

    result = subprocess.run(command, check=False, capture_output=True, timeout=15)
    if result.stderr:
        raise RuntimeError(
            f"{name}: unexpected stderr: {result.stderr.decode('utf-8', errors='replace')!r}"
        )
    if b"\xff" in result.stdout:
        raise RuntimeError(f"{name}: worker emitted a raw invalid UTF-8 byte")

    try:
        text = result.stdout.decode("ascii")
    except UnicodeDecodeError as error:
        raise RuntimeError(f"{name}: worker result is not ASCII-safe JSON") from error
    lines = text.splitlines()
    if len(lines) != 1 or not text.endswith("\n"):
        raise RuntimeError(
            f"{name}: expected exactly one newline-terminated JSON object, got {text!r}"
        )
    try:
        raw_payload = json.loads(lines[0])
    except json.JSONDecodeError as error:
        raise RuntimeError(f"{name}: invalid JSON result: {text!r}") from error

    payload = validate_payload(
        scenario=name,
        payload=raw_payload,
        expected_outcome=expected_outcome,
        expected_stop_reason=expected_stop_reason,
        expected_exit_code=expected_exit_code,
        expected_runtime_status=expected_runtime_status,
        actual_exit_code=result.returncode,
        process_limits_enabled=process_limits_enabled,
    )
    if name == "invalid_error" and "\u00ff" not in payload["message"]:
        raise RuntimeError("invalid_error: escaped error byte was not preserved")

    return {
        "name": name,
        "exit_code": result.returncode,
        "result": payload,
    }


def main() -> int:
    args = parse_args()
    worker = args.worker.resolve()
    scripts = args.scripts.resolve()
    if not worker.is_file():
        raise RuntimeError(f"worker does not exist: {worker}")
    if not scripts.is_dir():
        raise RuntimeError(f"script directory does not exist: {scripts}")
    if args.candidate_sha is not None and re.fullmatch(r"[0-9a-fA-F]{40}", args.candidate_sha) is None:
        raise RuntimeError("--candidate-sha must be exactly 40 hexadecimal characters")

    scenarios = [
        {
            "name": "success",
            "script": "worker_success.lua",
            "arguments": ["--instruction-budget", "100000", "--timeout-ms", "1000"],
            "expected_outcome": "success",
            "expected_stop_reason": 0,
            "expected_exit_code": 0,
        },
        {
            "name": "completion_disarms_cancellation",
            "script": "worker_success.lua",
            "arguments": [
                "--instruction-budget",
                "100000",
                "--timeout-ms",
                "1000",
                "--cancel-after-ms",
                "1000",
            ],
            "expected_outcome": "success",
            "expected_stop_reason": 0,
            "expected_exit_code": 0,
        },
        {
            "name": "instruction_budget",
            "script": "worker_instruction_limit.lua",
            "arguments": ["--instruction-budget", "1000", "--timeout-ms", "1000"],
            "expected_outcome": "instruction_budget",
            "expected_stop_reason": 1,
            "expected_exit_code": 20,
        },
        {
            "name": "native_work_budget",
            "script": "worker_native_work_limit.lua",
            "arguments": [
                "--instruction-budget",
                "100000",
                "--native-work-budget",
                "64",
                "--timeout-ms",
                "1000",
            ],
            "expected_outcome": "native_work_budget",
            "expected_stop_reason": 2,
            "expected_exit_code": 20,
        },
        {
            "name": "deadline",
            "script": "worker_instruction_limit.lua",
            "arguments": [
                "--instruction-budget",
                "1000000000",
                "--timeout-ms",
                "1",
            ],
            "expected_outcome": "deadline",
            "expected_stop_reason": 3,
            "expected_exit_code": 20,
        },
        {
            "name": "cross_thread_cancellation",
            "script": "worker_instruction_limit.lua",
            "arguments": [
                "--instruction-budget",
                "1000000000",
                "--timeout-ms",
                "1000",
                "--cancel-after-ms",
                "5",
            ],
            "expected_outcome": "cancelled",
            "expected_stop_reason": 4,
            "expected_exit_code": 20,
        },
        {
            "name": "resource_limit",
            "script": "worker_resource_limit.lua",
            "arguments": [
                "--instruction-budget",
                "100000",
                "--timeout-ms",
                "1000",
                "--max-output-bytes",
                "16384",
            ],
            "expected_outcome": "resource_limit",
            "expected_stop_reason": 0,
            "expected_exit_code": 20,
        },
        {
            "name": "allocator_limit",
            "script": "worker_allocator_limit.lua",
            "arguments": [
                "--allocator-memory-mb",
                "1",
                "--instruction-budget",
                "100000",
                "--native-work-budget",
                "4194304",
                "--timeout-ms",
                "1000",
                "--max-output-bytes",
                "4194304",
            ],
            "expected_outcome": "allocator_limit",
            "expected_stop_reason": 0,
            "expected_exit_code": 20,
        },
        {
            "name": "compile_error",
            "script": "worker_compile_error.lua",
            "arguments": ["--instruction-budget", "100000", "--timeout-ms", "1000"],
            "expected_outcome": "compile_error",
            "expected_stop_reason": 0,
            "expected_exit_code": 20,
        },
        {
            "name": "runtime_error",
            "script": "worker_runtime_error.lua",
            "arguments": ["--instruction-budget", "100000", "--timeout-ms", "1000"],
            "expected_outcome": "runtime_error",
            "expected_stop_reason": 0,
            "expected_exit_code": 20,
        },
        {
            "name": "invalid_error",
            "script": "worker_invalid_error.lua",
            "arguments": ["--instruction-budget", "100000", "--timeout-ms", "1000"],
            "expected_outcome": "runtime_error",
            "expected_stop_reason": 0,
            "expected_exit_code": 20,
        },
        {
            "name": "host_config_error",
            "script": "worker_success.lua",
            "arguments": ["--invalid-worker-option"],
            "expected_outcome": "host_config_error",
            "expected_stop_reason": 0,
            "expected_exit_code": 64,
            "expected_runtime_status": 1,
        },
    ]

    results = [
        run_scenario(
            worker=worker,
            scripts=scripts,
            process_limits_enabled=args.enforce_process_limits,
            **scenario,
        )
        for scenario in scenarios
    ]
    evidence = {
        "schema": 1,
        "status": "passed",
        "candidate_sha": args.candidate_sha.lower() if args.candidate_sha is not None else None,
        "process_limits_enabled": args.enforce_process_limits,
        "platform": {
            "system": platform.system(),
            "release": platform.release(),
            "machine": platform.machine(),
        },
        "scenario_count": len(results),
        "results": results,
    }

    if args.output is not None:
        output = args.output.resolve()
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(
            json.dumps(evidence, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )

    print(
        f"Production worker fault matrix passed: {len(results)} scenarios, "
        f"process_limits_enabled={str(args.enforce_process_limits).lower()}."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
