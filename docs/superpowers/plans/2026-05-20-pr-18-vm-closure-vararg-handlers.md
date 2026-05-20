# PR-18 VM Closure and Vararg Handlers Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend the VM command-handler table to cover `CLOSURE` and `VARARG`.

**Architecture:** Keep `SwitchDispatch` as the default strategy. Add handlers that delegate to the existing `VM::detail::closure(...)` and `VM::detail::vararg(...)` helpers, preserving current frame-relative upvalue capture and vararg stack-top behavior. Leave `CALL`, `TAILCALL`, and `RETURN` switch-only until a later PR extends handler control-flow statuses.

**Tech Stack:** C++23, existing VM handler table, existing closure/vararg helpers, existing lightweight unit test framework.

---

### Task 1: Add Failing Closure and Vararg Handler Tests

**Files:**
- Modify: `tests/unit/vm/test_vm_dispatch.cpp`

- [ ] **Step 1: Extend migrated opcode coverage**

Add `VM::hasHandler()` assertions for `CLOSURE` and `VARARG` in `testHandlersCoverMigratedOpcodes`.

- [ ] **Step 2: Add direct closure and vararg execution coverage**

Add `testClosureAndVarargHandlersExecuteDirectly`. Create a real `LuaState` frame and run:
- `CLOSURE`, with a child proto that has one `MOVE`-captured upvalue and one parent `GETUPVAL`-captured upvalue. Assert the produced `Function`, captured upvalues, and pseudo-instruction `pc` advancement.
- `VARARG` with fixed arity (`B > 0`), asserting varargs copy into target registers.
- `VARARG` with open arity (`B == 0`), asserting varargs copy and `LuaState::absoluteTop` expands to the open result range.

- [ ] **Step 3: Run focused tests and verify red**

Run: `.\build\cmake\Debug\lua_test.exe --filter "VM Dispatch"`

Expected: failures because these handlers are not registered yet.

---

### Task 2: Implement Closure and Vararg Handlers

**Files:**
- Modify: `src/vm/vm_handlers.cpp`
- Modify: `src/vm/vm.cpp`

- [ ] **Step 1: Add handler functions**

Implement:
- `handleClosure()`: require `LuaState`, `Function`, and `Proto`; call `VM::detail::closure(state, context.base, proto, function, context.pc, A, Bx)`.
- `handleVararg()`: require `LuaState` and `Proto`; call `VM::detail::vararg(state, context.base, proto, A, B)`.

- [ ] **Step 2: Register handlers**

Register `CLOSURE` and `VARARG` in `makeHandlerTable()`.

- [ ] **Step 3: Delegate switch cases**

Replace the two inline switch bodies in `src/vm/vm.cpp` with a grouped `OpExecutionContext` plus `VM::runHandler(opContext, inst)`.

---

### Task 3: Verify PR-18

**Files:**
- Modify: `docs/walkthroughs/hello-world.md` only if source line references drift.

- [ ] **Step 1: Run focused checks**

Run:

```powershell
.\build\cmake\Debug\lua_test.exe --filter "VM Dispatch"
.\build\cmake\Debug\lua_test.exe --filter "Function Codegen"
.\build\cmake\Debug\lua_test.exe --filter "Function Call"
.\build\cmake\Debug\lua_test.exe --filter "Call Pipeline"
.\build\cmake\Debug\lua_test.exe --filter "Symbol Binding"
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
