# PR-15 VM Unary Handlers Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend the VM command-handler table to cover unary opcodes.

**Architecture:** Keep `SwitchDispatch` as the default strategy. Reuse existing VM helpers for `UNM`, `LEN`, and `CONCAT`, implement `NOT` inline in a handler, then delegate the unary switch cases through `VM::runHandler()`.

**Tech Stack:** C++23, existing VM handler table, existing lightweight unit test framework.

---

### Task 1: Add Failing Unary Handler Tests

**Files:**
- Modify: `tests/unit/vm/test_vm_dispatch.cpp`

- [ ] **Step 1: Extend migrated opcode coverage**

Add `VM::hasHandler()` assertions for `UNM`, `NOT`, `LEN`, and `CONCAT` in `testHandlersCoverMigratedOpcodes`.

- [ ] **Step 2: Add direct unary execution coverage**

Add a `testUnaryHandlersExecuteDirectly` test that creates a real `LuaState` frame, runs `UNM`, `NOT`, `LEN`, and `CONCAT` through `VM::runHandler()`, and asserts the resulting register values.

- [ ] **Step 3: Run focused tests and verify red**

Run: `.\build\cmake\Debug\lua_test.exe --filter "VM Dispatch"`

Expected: failures because unary handlers are not registered yet.

---

### Task 2: Implement Unary Handlers

**Files:**
- Modify: `src/vm/vm_handlers.cpp`
- Modify: `src/vm/vm.cpp`

- [ ] **Step 1: Add handler functions**

Add handlers for:
- `UNM`: copy source register, call `VM::detail::unaryMinus()`, refresh base, write result.
- `NOT`: write `Value(!context.base[b].isTrue())`.
- `LEN`: copy source register, call `VM::detail::length()`, refresh base, write result.
- `CONCAT`: call `VM::detail::concat(context.services, state, context.base, a, b, c)`, refresh base.

- [ ] **Step 2: Register handlers**

Register `UNM`, `NOT`, `LEN`, and `CONCAT` in `makeHandlerTable()`.

- [ ] **Step 3: Delegate switch cases**

Replace the four inline unary switch bodies in `src/vm/vm.cpp` with a grouped `OpExecutionContext` plus `VM::runHandler(opContext, inst)`.

---

### Task 3: Verify PR-15

**Files:**
- Modify: `docs/walkthroughs/hello-world.md` only if source line references drift.

- [ ] **Step 1: Run focused checks**

Run:

```powershell
.\build\cmake\Debug\lua_test.exe --filter "VM Dispatch"
.\build\cmake\Debug\lua_test.exe --filter "Unary"
.\build\cmake\Debug\lua_test.exe --filter "Metamethod"
.\build\cmake\Debug\lua_test.exe --filter "VM Internal Boundaries"
```

- [ ] **Step 2: Run full checks**

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
