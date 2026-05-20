# PR-12 VM Global and Upvalue Handlers Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend the VM command-handler table to cover global and upvalue access opcodes.

**Architecture:** Keep `SwitchDispatch` as the default strategy and keep call/return/table opcodes in `vm.cpp`. Add handlers for `GETGLOBAL`, `SETGLOBAL`, `GETUPVAL`, and `SETUPVAL` to `vm_handlers.cpp`, then delegate those four switch cases through `runHandler()`.

**Tech Stack:** C++23, existing VM handler table, existing lightweight unit test framework.

---

### Task 1: Red Tests

**Files:**
- Modify: `tests/unit/vm/test_vm_dispatch.cpp`

- [ ] **Step 1: Extend handler registration test**

Assert `VM::hasHandler()` is true for `GETGLOBAL`, `SETGLOBAL`, `GETUPVAL`, and `SETUPVAL`.

- [ ] **Step 2: Add direct execution test**

Create a local `Proto`, `Function`, `LuaState`, and closed `Upvalue`; run the four handlers through `VM::runHandler()` and assert register/global/upvalue side effects.

- [ ] **Step 3: Verify red**

Run:

```powershell
& 'D:\VS2026\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build\cmake --target lua_test --config Debug
.\build\cmake\Debug\lua_test.exe --filter "VM Dispatch"
```

Expected: failures because these handlers are not registered yet.

### Task 2: Handler Implementation

**Files:**
- Modify: `src/vm/vm_handlers.cpp`
- Modify: `src/vm/vm.cpp`

- [ ] **Step 1: Add helper validation and base refresh**

Add small internal helpers for requiring `LuaState`, `Function`, and refreshing `base` after possible metamethod paths.

- [ ] **Step 2: Implement global handlers**

Move `GETGLOBAL` and `SETGLOBAL` logic into handler functions, preserving environment fallback to `LuaState::getGlobalTable()`.

- [ ] **Step 3: Implement upvalue handlers**

Move `GETUPVAL` and `SETUPVAL` logic into handler functions, preserving invalid-index runtime errors.

- [ ] **Step 4: Register handlers and delegate switch cases**

Register the four handlers in the table and replace the four inline switch bodies with `runHandler()`.

### Task 3: Verification

**Files:**
- Verify: all modified files.

- [ ] **Step 1: Run targeted tests**

Run `.\build\cmake\Debug\lua_test.exe --filter "VM Dispatch"`.

- [ ] **Step 2: Run full gate**

Run full CMake tests, CTest, VS project builds, `tools\run_quality_gate.ps1`, and `git diff --check`.
