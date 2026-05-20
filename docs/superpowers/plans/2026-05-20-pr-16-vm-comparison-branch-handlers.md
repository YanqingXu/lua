# PR-16 VM Comparison and Branch Handlers Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend the VM command-handler table to cover comparison and branch opcodes.

**Architecture:** Keep `SwitchDispatch` as the default strategy. Add handlers for `JMP`, `EQ`, `LT`, `LE`, `TEST`, and `TESTSET`, preserving the current post-fetch `pc` semantics from `vm.cpp`: the loop has already incremented `pc`, so branch handlers update the next-instruction index directly.

**Tech Stack:** C++23, existing VM handler table, existing comparison helpers, existing lightweight unit test framework.

---

### Task 1: Add Failing Branch and Comparison Handler Tests

**Files:**
- Modify: `tests/unit/vm/test_vm_dispatch.cpp`

- [ ] **Step 1: Extend migrated opcode coverage**

Add `VM::hasHandler()` assertions for `JMP`, `EQ`, `LT`, `LE`, `TEST`, and `TESTSET` in `testHandlersCoverMigratedOpcodes`.

- [ ] **Step 2: Add direct branch and comparison execution coverage**

Add `testBranchAndComparisonHandlersExecuteDirectly`. Create a real `LuaState` frame and a `Proto` containing a placeholder instruction plus a `JMP` instruction at index 1. Run:
- `JMP` and assert `pc += sBx`.
- `EQ`, `LT`, and `LE` and assert skip/no-skip `pc` changes.
- `TEST` and assert the next `JMP` offset is applied before the final `pc++`.
- `TESTSET` and assert both the copy side effect and the next-`JMP` offset.

- [ ] **Step 3: Run focused tests and verify red**

Run: `.\build\cmake\Debug\lua_test.exe --filter "VM Dispatch"`

Expected: failures because these handlers are not registered yet.

---

### Task 2: Implement Branch and Comparison Handlers

**Files:**
- Modify: `src/vm/vm_handlers.cpp`
- Modify: `src/vm/vm.cpp`

- [ ] **Step 1: Add helper guard for `Proto`**

Add a local `requireProto()` helper in `vm_handlers.cpp`, parallel to `requireState()` and `requireFunction()`, because `TEST` and `TESTSET` must inspect `context.proto->getCode()[context.pc]`.

- [ ] **Step 2: Add handler functions**

Implement:
- `handleJump()`: `context.pc += GETARG_sBx(inst)`.
- `handleComparison()`: read RK operands, call `VM::detail::equal/lessThan/lessEqual`, refresh `context.base`, and increment `context.pc` when `result != (A != 0)`.
- `handleTest()`: preserve the current `TEST` semantics, including reading the next instruction's `sBx` only when `context.pc < proto->getCode().size()`, then `context.pc++`.
- `handleTestSet()`: preserve copy plus jump semantics, then `context.pc++`.

- [ ] **Step 3: Register handlers**

Register `JMP`, `EQ`, `LT`, `LE`, `TEST`, and `TESTSET` in `makeHandlerTable()`.

- [ ] **Step 4: Delegate switch cases**

Replace the six inline switch bodies in `src/vm/vm.cpp` with a grouped `OpExecutionContext` plus `VM::runHandler(opContext, inst)`.

---

### Task 3: Verify PR-16

**Files:**
- Modify: `docs/walkthroughs/hello-world.md` only if source line references drift.

- [ ] **Step 1: Run focused checks**

Run:

```powershell
.\build\cmake\Debug\lua_test.exe --filter "VM Dispatch"
.\build\cmake\Debug\lua_test.exe --filter "Comparison"
.\build\cmake\Debug\lua_test.exe --filter "Condition"
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
