# Runtime benchmark

`lua_runtime_bench` validates representative game-server runtime scenarios. It is a standalone Release benchmark
with correctness checks, raw samples, and versioned JSON output. A checksum, Lua status, stack invariant, GC
invariant, or allocator invariant failure makes the process return non-zero; performance numbers are never emitted
as successful evidence after a correctness failure.

## Covered workloads

| Metric | Timed boundary |
| --- | --- |
| `parse_compile_mib_per_second` | In-memory source parsing plus bytecode generation; file I/O and GC cleanup are excluded. |
| `vm_instructions_per_second` | A deterministic numeric Lua loop. Instruction count is calibrated with the trace sink, then trace is disabled for timing. |
| `cpp_to_lua_ns_per_call` | Registry lookup, argument push, protected Lua call, result read, and stack restoration. |
| `lua_to_cpp_ns_per_call` | A precompiled Lua loop calling a registered C++ function; exact host call count is checked. |
| `coroutine_resume_yield_ns` | Each `lua_resume` call that reaches a Lua `coroutine.yield`; setup and compilation are excluded. |
| `table_operations_per_second` | Preallocated array/hash tables, with one read and one write of each table per loop iteration. |
| `closure_upvalue_lifecycle_per_second` | Creation, validation, release, and collection of exactly 100,000 uniquely captured closures. |
| `allocation_mib_per_second` | Successful allocator bytes granted during the 100,000-closure allocation phase. |
| `gc_pause_p50_us` / `p95` / `p99` / `max` | One fixed-size `GarbageCollector::step` call per frame; allocation and Lua execution are outside the pause timer. |
| `heap_growth_bytes_per_million_frames` | Linear slope of counting-allocator live bytes sampled after completed GC cycles, with a fixed retained set and transient per-frame allocations. |

The GC `size` parameter is reported as `gc_step_size`. Its work accounting is the runtime's local approximation
scaled by `stepmul`; it must not be interpreted as a precise number of traced bytes. Heap evidence records both the
counting allocator's live bytes and the collector's estimated managed bytes/object count. Intermediate heap
checkpoints are captured only after an incremental collection cycle completes; the first and last checkpoints follow
full collections. `getTotalMemory()` is never called inside the GC pause timer.

P50/P95/P99 use the nearest-rank definition. The `ci` profile always creates 100,000 closures and keeps at least
10,000 individual pause samples; these two workloads are intentionally not reduced for smoke execution.

## Build and run

Single-config generators:

```powershell
cmake -S . -B build/bench `
  -DCMAKE_BUILD_TYPE=Release `
  -DLUA_CPP_BUILD_BENCHMARKS=ON `
  -DLUA_CPP_BUILD_TESTS=OFF `
  -DLUA_CPP_BUILD_TOOLS=OFF
cmake --build build/bench --parallel 2 --target lua_runtime_bench
build/bench/lua_runtime_bench --profile ci --json build/bench/runtime-bench.json
powershell -NoProfile -ExecutionPolicy Bypass `
  -File tools/check_runtime_bench.ps1 `
  -ResultPath build/bench/runtime-bench.json
```

For Visual Studio, add `--config Release` to the build and run
`build/bench/Release/lua_runtime_bench.exe`.

Profiles:

- `ci`: quick contract evidence, three default timing samples, 100,000 closures, 10,000 GC pause frames, and 20,000
  heap-stability frames.
- `full`: seven samples, larger throughput workloads, 30,000 GC pause frames, and 200,000 heap frames.
- `endurance`: full throughput workloads, 100,000 pause frames, and 1,000,000 heap frames.

Use `--samples N` to override ordinary timing samples. It does not reduce closure count, GC pause frames, or heap
frames. Use `--json PATH` to choose the evidence file. The output includes schema version, build/compiler/OS/git
metadata, the selected workload parameters, all raw timing samples, all GC pause samples, and the heap checkpoint
curve. The CI validator intentionally accepts only the unmodified `ci` workload; sample overrides are exploratory
evidence and are rejected by that gate.

## CI policy

The required CI contract builds only the benchmark and core in Release, runs the `ci` profile, validates the JSON,
and uploads it as an artifact. The validator binds the artifact to the exact `ci` workload and commit, verifies every
metric's unit, direction, sample count, and recomputed median, then recomputes the raw GC nearest-rank quantiles and
heap trend. Heap stability permits at most 64 KiB or 10% final growth (whichever is larger) and a positive trend of at
most 256 KiB or one warmed heap per million frames (whichever is larger). Absolute throughput should not be compared
to a committed threshold on a changing GitHub-hosted machine. Once the harness exists on the base branch, regression
policy should compare base and head interleaved on the same runner; that follow-up is tracked in
[#7](https://github.com/YanqingXu/lua/issues/7).
