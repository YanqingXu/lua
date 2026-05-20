# PR-14 VM Arithmetic Handlers Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend the VM command-handler table to cover arithmetic opcodes.

**Architecture:** Keep `SwitchDispatch` as the default strategy. Reuse the existing `VM::detail::execArithmetic(...)` helper from PR-02 and add a single arithmetic handler for `ADD`, `SUB`, `MUL`, `DIV`, `MOD`, and `POW`, then delegate those switch cases through `runHandler()`.

**Tech Stack:** C++23, existing VM handler table, existing arithmetic helper, existing lightweight unit test framework.

---

### Task 1: Red Tests

**Files:**
- Modify: `tests/unit/vm/test_vm_dispatch.cpp`

- [ ] **Step 1: Extend handler registration coverage**

Assert `VM::hasHandler()` is true for `ADD`, `SUB`, `MUL`, `DIV`, `MOD`, and `POW`, while `CALL` remains switch-only.

- [ ] **Step 2: Add direct arithmetic handler execution coverage**

Create a local `Proto` and `LuaState`; run all six arithmetic handlers through `VM::runHandler()` and assert numeric results for register operands and RK constant operands.

- [ ] **Step 3: Verify red**

Run:

```powershell
& 'D:\VS2026\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build\cmake --target lua_test --config Debug
.\build\cmake\Debug\lua_test.exe --filter "VM Dispatch"
```

Expected: failures because these arithmetic handlers are not registered yet.

### Task 2: Handler Implementation

**Files:**
- Modify: `src/vm/vm_handlers.cpp`
- Modify: `src/vm/vm.cpp`

- [ ] **Step 1: Add arithmetic handler**

Add `handleArithmetic()` that reads `A/B/C` and dispatches to `VM::detail::execArithmetic(context.state, context.proto, context.base, a, b, c, GET_OPCODE(inst))`.

- [ ] **Step 2: Register arithmetic opcodes**

Register `ADD`, `SUB`, `MUL`, `DIV`, `MOD`, and `POW` to the arithmetic handler.

- [ ] **Step 3: Delegate switch cases**

Replace the six inline arithmetic switch cases in `SwitchDispatch::run()` with an `OpExecutionContext` and `VM::runHandler()`.

### Task 3: Verification

**Files:**
- Verify: all modified files.

- [ ] **Step 1: Run targeted tests**

Run `.\build\cmake\Debug\lua_test.exe --filter "VM Dispatch"` plus arithmetic and metamethod-related slices.

- [ ] **Step 2: Run full gate**

Run full CMake tests, CTest, VS project builds, `tools\run_quality_gate.ps1`, and `git diff --check`.
