# PR-17 VM Loop and Close Handlers Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend the VM command-handler table to cover `CLOSE` and loop opcodes.

**Architecture:** Keep `SwitchDispatch` as the default strategy. Add handlers for `CLOSE`, `FORLOOP`, `FORPREP`, and `TFORLOOP`, preserving the current post-fetch `pc` semantics from `vm.cpp`: `FORLOOP` and `FORPREP` update the already-incremented next-instruction index directly, while `TFORLOOP` continues to delegate to `VM::detail::tforLoop()`.

**Tech Stack:** C++23, existing VM handler table, existing loop helper, existing lightweight unit test framework.

---

### Task 1: Add Failing Loop and Close Handler Tests

**Files:**
- Modify: `tests/unit/vm/test_vm_dispatch.cpp`

- [ ] **Step 1: Extend migrated opcode coverage**

Add `VM::hasHandler()` assertions for `CLOSE`, `FORLOOP`, `FORPREP`, and `TFORLOOP` in `testHandlersCoverMigratedOpcodes`.

- [ ] **Step 2: Add direct close and loop execution coverage**

Add `testLoopAndCloseHandlersExecuteDirectly`. Create a real `LuaState` frame and run:
- `CLOSE`, with two open upvalues, asserting only upvalues at or above `base + A` close.
- `FORPREP`, asserting the internal index is initialized to `init - step` and `pc += sBx`.
- `FORLOOP`, asserting both continuing and terminating numeric-loop paths.
- `TFORLOOP`, using a small C iterator function, asserting non-nil iterator results update the control variable and apply the following `JMP`, while nil results only advance once.

- [ ] **Step 3: Run focused tests and verify red**

Run: `.\build\cmake\Debug\lua_test.exe --filter "VM Dispatch"`

Expected: failures because these handlers are not registered yet.

---

### Task 2: Implement Loop and Close Handlers

**Files:**
- Modify: `src/vm/vm_handlers.cpp`
- Modify: `src/vm/vm.cpp`

- [ ] **Step 1: Add handler functions**

Implement:
- `handleClose()`: require `LuaState`, read current `CallInfo`, and call `closeUpvalues(ci.base + A)`.
- `handleForLoop()`: preserve numeric validation, step/index/limit calculation, `pc += sBx`, internal index writeback, and user-visible loop variable writeback.
- `handleForPrep()`: preserve numeric validation, write `init - step`, and `pc += sBx`.
- `handleTForLoop()`: require `LuaState` and `Proto`, then call `VM::detail::tforLoop(state, context.base, proto, context.pc, A, C)`.

- [ ] **Step 2: Register handlers**

Register `CLOSE`, `FORLOOP`, `FORPREP`, and `TFORLOOP` in `makeHandlerTable()`.

- [ ] **Step 3: Delegate switch cases**

Replace the four inline switch bodies in `src/vm/vm.cpp` with a grouped `OpExecutionContext` plus `VM::runHandler(opContext, inst)`.

---

### Task 3: Verify PR-17

**Files:**
- Modify: `docs/walkthroughs/hello-world.md` only if source line references drift.

- [ ] **Step 1: Run focused checks**

Run:

```powershell
.\build\cmake\Debug\lua_test.exe --filter "VM Dispatch"
.\build\cmake\Debug\lua_test.exe --filter "For"
.\build\cmake\Debug\lua_test.exe --filter "Symbol Binding"
.\build\cmake\Debug\lua_test.exe --filter "Call Pipeline"
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
