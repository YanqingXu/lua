# PR-19 VM Call and Return Handlers Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend the VM command-handler table to cover `CALL`, `TAILCALL`, and `RETURN`.

**Architecture:** Keep `SwitchDispatch` as the default strategy. Extend `HandlerStatus` so call-family handlers can ask the dispatch loop to continue, reenter the current Lua frame, return yielded execution, or return from the outermost frame. Move the existing switch bodies into `vm_handlers.cpp` without changing `VM::detail::precall(...)`, `postcall(...)`, tailcall frame reuse, yield detection, or return-value movement semantics.

**Tech Stack:** C++23, existing VM handler table, existing VM call helpers, existing lightweight unit test framework.

---

### Task 1: Add Failing Call-Family Handler Tests

**Files:**
- Modify: `tests/unit/vm/test_vm_dispatch.cpp`

- [ ] **Step 1: Extend migrated opcode coverage**

Add `VM::hasHandler()` assertions for `CALL`, `TAILCALL`, and `RETURN` in `testHandlersCoverMigratedOpcodes`.

- [ ] **Step 2: Add direct call-family execution coverage**

Add `testCallAndReturnHandlersExecuteDirectly` with real `LuaState` frames:
- `CALL` to a C function returns handler status `Continue`, writes the fixed result into `R(A)`, pops the C `CallInfo`, and refreshes the caller top.
- `CALL` to a Lua function returns handler status `Reenter`, pushes a Lua `CallInfo`, saves the caller pc, and increments the handler context call depth.
- `CALL` to a yielding C function returns handler status `Yielded` and saves the current call depth in `LuaState`.
- `TAILCALL` to a Lua function returns handler status `Reenter`, reuses the current frame, and increments the frame tailcall count.
- `RETURN` from the outermost Lua frame returns handler status `Returned`, moves return values to `ci.func`, closes the frame, and decrements the handler context call depth.

- [ ] **Step 3: Run focused tests and verify red**

Run:

```powershell
& 'D:\VS2026\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build\cmake --target lua_test --config Debug
.\build\cmake\Debug\lua_test.exe --filter "VM Dispatch"
```

Expected: failures because `CALL`, `TAILCALL`, and `RETURN` are not registered in the handler table yet.

---

### Task 2: Implement Call-Family Handlers

**Files:**
- Modify: `src/vm/vm_handlers.hpp`
- Modify: `src/vm/vm_handlers.cpp`
- Modify: `src/vm/vm.cpp`

- [ ] **Step 1: Extend handler status protocol**

Change `HandlerStatus` to include:
- `Continue`: stay in the current dispatch loop.
- `Reenter`: jump to `SwitchDispatch` reentry because the current frame changed.
- `Yielded`: return `ExecResult::Yielded`.
- `Returned`: return `ExecResult::Returned`.

- [ ] **Step 2: Add handler functions**

Implement:
- `handleCall()`: preserve C-call result cleanup, Lua-call `nexeccalls++`, yield detection, call tracing, and caller saved pc update.
- `handleTailCall()`: preserve upvalue closing, caller saved pc update, Lua tailcall frame reuse, and C tailcall fallthrough behavior.
- `handleReturn()`: preserve return tracing, return hook dispatch, upvalue closing, return-value movement, `nexeccalls--`, `postcall(...)`, and reentry signaling.

- [ ] **Step 3: Register handlers**

Register `CALL`, `TAILCALL`, and `RETURN` in `makeHandlerTable()`.

- [ ] **Step 4: Delegate switch cases**

Replace the three inline switch bodies in `src/vm/vm.cpp` with a grouped `OpExecutionContext` plus `VM::runHandler(opContext, inst)`. After the handler returns, copy back `base` and `nexeccalls`, then translate status to `break`, `goto reentry`, `return ExecResult::Yielded`, or `return ExecResult::Returned`.

---

### Task 3: Verify PR-19

**Files:**
- Modify: `docs/walkthroughs/hello-world.md` if source line references drift.

- [ ] **Step 1: Run focused checks**

Run:

```powershell
.\build\cmake\Debug\lua_test.exe --filter "VM Dispatch"
.\build\cmake\Debug\lua_test.exe --filter "Function Call"
.\build\cmake\Debug\lua_test.exe --filter "Call Pipeline"
.\build\cmake\Debug\lua_test.exe --filter "coroutine"
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
