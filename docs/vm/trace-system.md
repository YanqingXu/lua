---
status: current
verified_against: docs/status/project-status.md; src/debug/trace_types.hpp; src/debug/trace_sink.hpp; src/debug/json_trace_sink.cpp; src/debug/value_serializer.cpp; src/vm/vm_trace.cpp; src/main.cpp; src/app/app_options.cpp; tests/unit/vm/test_vm_trace_debug.cpp
last_checked: 2026-05-19
applies_to: current JSONL VM trace system
---

# VM Trace System

The VM trace system records execution events from the bytecode interpreter and writes them as JSONL. It is intentionally off by default and is enabled by installing an `ITraceSink`.

Current user-facing entry:

```powershell
bin\lua_app.exe --trace trace.jsonl examples\hello.lua
```

There is no checked-in HTML trace viewer yet. Earlier viewer ideas are historical; the current implemented surface is JSONL output.

## Components

| Component | File | Responsibility |
|---|---|---|
| `TraceEventKind` / `TraceEvent` | `src/debug/trace_types.hpp` | Shared event shape |
| `ITraceSink` | `src/debug/trace_sink.hpp` | Trace sink interface |
| `NullTraceSink` | `src/debug/trace_sink.hpp` | No-op sink |
| `JsonTraceSink` | `src/debug/json_trace_sink.*` | JSONL writer |
| Value serialization | `src/debug/value_serializer.*` | Convert `Value` and registers to JSON-compatible text |
| VM trace hooks | `src/vm/vm_trace.cpp` | Build and emit instruction/call/return events |
| CLI wiring | `src/app/app_options.cpp`, `src/main.cpp` | Parse `--trace <file>` and install sink |

## Event Types

`TraceEventKind` currently defines:

- `Instruction`
- `Call`
- `Return`
- `Error`

`JsonTraceSink` implements `onError`, but the current VM path does not yet emit runtime error trace events. Treat error events as reserved schema support.

## Instruction Event

Instruction events include decoded operands and source location:

```json
{"seq":0,"kind":"instruction","pc":0,"op":"LOADK","a":0,"b":0,"c":0,"bx":0,"sbx":0,"line":1,"source":"examples/hello.lua","callDepth":1,"registers":[]}
```

Fields:

| Field | Meaning |
|---|---|
| `seq` | Monotonic event sequence number |
| `kind` | `"instruction"` |
| `pc` | Program counter within the current `Proto` |
| `op` | Opcode name |
| `a`, `b`, `c`, `bx`, `sbx` | Decoded instruction operands |
| `line` | Source line from `Proto` line info |
| `source` | Source name from the active `Proto` |
| `callDepth` | Current logical VM call depth |
| `registers` | Serialized frame register snapshot when available |

## Call And Return Events

Call events are emitted around visible VM call points:

```json
{"seq":3,"kind":"call","funcName":"?","source":"examples/hello.lua","line":1,"callDepth":2}
```

Return events are smaller:

```json
{"seq":8,"kind":"return","callDepth":1}
```

## Register Snapshots

Instruction events may include `registers`. Each element includes:

| Field | Meaning |
|---|---|
| `slot` | Register index relative to the current frame |
| `name` | Local variable name when debug info can resolve it |
| `type` | Lua value type string |
| `value` | Serialized value |

The serializer is observational only: it reads VM values and does not mutate stack state.

## Control Flow

```text
main.cpp
  -> parse --trace <file>
  -> create JsonTraceSink
  -> VM::setTraceSink(...)
  -> VM::executeProto(...)
  -> vm_trace.cpp emits events
  -> JsonTraceSink writes JSONL
```

`VM::setTraceSink(nullptr)` disables trace output.

## Known Gaps

- No committed HTML or browser viewer.
- No trace-level filtering such as opcode-only vs full registers.
- Error events are represented in the schema but are not yet emitted by the VM error paths.
- Trace sink state is global, matching the current VM entry-point shape.

## Verification

Relevant checks:

```powershell
bin\lua_test.exe --filter "VM Trace Debug"
bin\lua_app.exe --trace bin\trace-example.jsonl examples\hello.lua
```
