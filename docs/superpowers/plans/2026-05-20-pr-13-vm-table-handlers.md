# PR-13 VM Table Handlers Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend the VM command-handler table to cover the basic table opcode group.

**Architecture:** Keep `SwitchDispatch` as the default strategy. Add handlers for `GETTABLE`, `SETTABLE`, `NEWTABLE`, `SELF`, and `SETLIST` to `vm_handlers.cpp`, then delegate those switch cases through `runHandler()` while preserving stack-base refresh after metamethod-capable operations.

**Tech Stack:** C++23, existing VM handler table, existing lightweight unit test framework.

---

### Task 1: Red Tests

**Files:**
- Modify: `tests/unit/vm/test_vm_dispatch.cpp`

- [ ] **Step 1: Extend handler registration coverage**

Assert `VM::hasHandler()` is true for `GETTABLE`, `SETTABLE`, `NEWTABLE`, `SELF`, and `SETLIST`, while `CALL` remains switch-only.

- [ ] **Step 2: Add direct table handler execution coverage**

Create a local `Proto`, `LuaState`, and GC-registered `Table`; run the five table handlers through `VM::runHandler()` and assert field reads, field writes, receiver copy, new table creation, and array writes.

- [ ] **Step 3: Verify red**

Run:

```powershell
& 'D:\VS2026\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build\cmake --target lua_test --config Debug
.\build\cmake\Debug\lua_test.exe --filter "VM Dispatch"
```

Expected: failures because these table handlers are not registered yet.

### Task 2: Handler Implementation

**Files:**
- Modify: `src/vm/vm_handlers.cpp`
- Modify: `src/vm/vm.cpp`

- [ ] **Step 1: Add table RK helper**

Add a small internal helper matching the existing `vm.cpp` RK behavior: return `proto->getConstant(INDEXK(rk))` for constants and `base[rk]` for registers.

- [ ] **Step 2: Implement table handlers**

Move the exact logic for `GETTABLE`, `SETTABLE`, `NEWTABLE`, `SELF`, and `SETLIST` into handler functions. Preserve copies before metamethod calls and refresh `context.base` after `gettable()` / `settable()`.

- [ ] **Step 3: Register handlers and delegate switch cases**

Register the five handlers in the table and replace the five inline switch bodies with a grouped `runHandler()` call.

### Task 3: Verification

**Files:**
- Verify: all modified files.

- [ ] **Step 1: Run targeted tests**

Run `.\build\cmake\Debug\lua_test.exe --filter "VM Dispatch"` plus table/metamethod-related slices.

- [ ] **Step 2: Run full gate**

Run full CMake tests, CTest, VS project builds, `tools\run_quality_gate.ps1`, and `git diff --check`.
