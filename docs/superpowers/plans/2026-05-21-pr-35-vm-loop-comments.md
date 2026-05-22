# PR-35 VM Loop Comments Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 补强 VM 主循环关键注释，解释 `pc++`、`savedpc` 和 debug hook 的时序约定。

**Architecture:** 不修改执行逻辑，只在 `src/vm/vm.cpp` 的共享 dispatch backend 中添加“为什么”注释。注释覆盖 reentry 时从 `CallInfo::savedpc` 恢复 `pc`、取指后立即预增 `pc`、hook 前写入 `savedpc`、count hook 与 line hook 顺序，以及 hook 后刷新 `base` 的原因。

**Tech Stack:** C++23 preview, existing VM dispatch strategies, existing lightweight unit test framework.

---

### Task 1: 确认 VM 主循环时序边界

**Files:**
- Read: `src/vm/vm.cpp`
- Read: `src/vm/vm_trace.cpp`
- Read: `src/vm/vm_handlers.hpp`
- Read: `src/vm/vm_handlers/vm_handlers_branch.cpp`
- Read: `src/vm/vm_handlers/vm_handlers_call.cpp`

- [x] **Step 1: 确认 shared dispatch backend**

Verify `SwitchDispatch` and `TableDispatch` both enter `runDispatchBackend`, so one comment location explains both dispatch modes.

- [x] **Step 2: 确认 pc 语义**

Verify the loop stores `instructionPc = pc`, fetches `code[pc]`, then increments `pc` before handler execution. Branch, TEST/TESTSET, CALL, and TAILCALL handlers adjust this next-pc value.

- [x] **Step 3: 确认 hook 时序**

Verify `savedpc` is written before `dispatchCountHook` and `dispatchLineHook`, and `dispatchLineHook` receives `instructionPc` for source-line reporting.

### Task 2: 补充主循环解释性注释

**Files:**
- Modify: `src/vm/vm.cpp`

- [x] **Step 1: 解释 reentry savedpc 恢复**

Add a comment above `proto = func->getProto()` explaining that `savedpc` points to the next instruction and new frames start at `pc = 0`.

- [x] **Step 2: 解释 `instructionPc` 与 `pc++`**

Add a comment before instruction fetch explaining that `instructionPc` is the executing instruction index, while `pc` is advanced immediately to represent the next instruction.

- [x] **Step 3: 解释 hook 前保存 `savedpc`**

Add a comment before `currentCI.savedpc = code.data() + pc` explaining why hooks and error/yield paths must observe a resumable next instruction.

- [x] **Step 4: 解释 hook 顺序和 base 刷新**

Add a comment before hook dispatch explaining count hook before line hook and why `base` is refreshed after each hook.

### Task 3: 验证

**Files:**
- Verify: `lua_test.vcxproj`
- Verify: `tools/run_quality_gate.ps1`

- [x] **Step 1: 构建 lua_test**

Run:

```powershell
& 'D:\VS2026\MSBuild\Current\Bin\MSBuild.exe' lua_test.vcxproj /m /nr:false /p:Configuration=Debug /p:Platform=x64
```

Expected: build succeeds with 0 warnings and 0 errors.

- [x] **Step 2: 跑 VM 聚焦测试**

Run:

```powershell
.\bin\lua_test.exe --filter "VM Dispatch"
.\bin\lua_test.exe --filter "VM Trace"
.\bin\lua_test.exe --filter "VM Internal Boundaries"
.\bin\lua_test.exe --filter "Function Call"
```

Expected: selected tests pass.

- [x] **Step 3: 跑完整质量门**

Run:

```powershell
.\tools\run_quality_gate.ps1
```

Expected: all registered tests pass.
