# PR-20 VM Table Dispatch Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a real `TableDispatch` strategy that executes VM bytecode through the command-handler table.

**Architecture:** Keep `SwitchDispatch` as the default strategy for debugging and compatibility. Expose a singleton `tableDispatchStrategy()` that can be assigned to `RuntimeServices::dispatchStrategy`. Share the same execution-frame setup, hooks, traces, and `HandlerStatus` control-flow translation between switch and table dispatch so both strategies preserve call, tailcall, return, and yield semantics.

**Tech Stack:** C++23, existing `DispatchStrategy` abstraction, existing VM command-handler table, existing lightweight unit test framework.

---

### Task 1: Add Failing TableDispatch Tests

**Files:**
- Modify: `tests/unit/vm/test_vm_dispatch.cpp`

- [x] **Step 1: Add strategy availability coverage**

Add a test that asserts:
- `VM::tableDispatchStrategy().name()` is `"table"`.
- `VM::tableDispatchStrategy()` is distinct from `VM::defaultDispatchStrategy()`.

- [x] **Step 2: Add compiled chunk execution coverage**

Add a test that:
- Compiles a Lua chunk through `Parser` and `CodeGenerator`.
- Sets `services.dispatchStrategy = &VM::tableDispatchStrategy()`.
- Executes a compiled chunk that uses functions, comparison, arithmetic, multiple returns, and tailcall forwarding.
- Asserts the returned values are `13` and `42`.

- [x] **Step 3: Run focused tests and verify red**

Run:

```powershell
& 'D:\VS2026\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build\cmake --target lua_test --config Debug
```

Expected: compile failure because `tableDispatchStrategy()` is not declared yet.

---

### Task 2: Implement TableDispatch

**Files:**
- Modify: `src/vm/vm_dispatch_strategy.hpp`
- Modify: `src/vm/vm_dispatch_strategy.cpp`
- Modify: `src/vm/vm.cpp`

- [x] **Step 1: Declare the table strategy**

Add `class TableDispatch final : public DispatchStrategy` and `DispatchStrategy& tableDispatchStrategy() noexcept`.

- [x] **Step 2: Expose the singleton and name**

Implement `TableDispatch::name()` as `"table"` and return a static `TableDispatch` from `tableDispatchStrategy()`.

- [x] **Step 3: Share execution-loop plumbing**

Move the common execution-loop body into an internal helper in `src/vm/vm.cpp` that takes a backend selector:
- `Switch`: dispatches through the existing grouped `switch`.
- `Table`: dispatches every opcode through `VM::runHandler(...)`.

Both paths must translate `HandlerStatus::Continue`, `Reenter`, `Yielded`, and `Returned` the same way.

- [x] **Step 4: Wire strategy run methods**

Make `SwitchDispatch::run()` call the shared helper with the switch backend and `TableDispatch::run()` call it with the table backend.

---

### Task 3: Verify PR-20

**Files:**
- Modify: `docs/walkthroughs/hello-world.md` if dispatch strategy wording or source line references drift.

- [x] **Step 1: Run focused checks**

Run:

```powershell
.\build\cmake\Debug\lua_test.exe --filter "VM Dispatch"
.\build\cmake\Debug\lua_test.exe --filter "Call Pipeline"
.\build\cmake\Debug\lua_test.exe --filter "coroutine"
.\build\cmake\Debug\lua_test.exe --filter "VM Internal Boundaries"
```

- [x] **Step 2: Run full checks**

Run:

```powershell
& 'D:\VS2026\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build\cmake --config Debug
.\build\cmake\Debug\lua_test.exe
& 'D:\VS2026\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir build\cmake -C Debug --output-on-failure
& 'D:\VS2026\MSBuild\Current\Bin\MSBuild.exe' lua_app.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
& 'D:\VS2026\MSBuild\Current\Bin\MSBuild.exe' lua_test.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
.\bin\lua_test.exe
.\tools\run_quality_gate.ps1
git diff --check
```
