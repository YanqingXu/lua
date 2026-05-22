# PR-38 Stage 4 Comment Placement Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 调整阶段 4 教育价值增强中的注释规范：新增教育性注释不插入函数体中间，并把 PR-35/36/37 已加入的函数内部长注释上移为文件级、函数组或函数前说明。

**Architecture:** 不修改运行逻辑。路线图明确“文件级 / 函数组注释、函数前契约注释、类型旁表格注释、深度教学文档”的分层策略；源码中保留解释性内容，但从执行语句之间移到 `runDispatchBackend()`、`collect(LuaState*)` 或 jump helper 文件级模型之前。

**Tech Stack:** C++23 preview, Markdown documentation, existing lightweight unit test framework.

---

### Task 1: 审查现有阶段 4 注释位置

**Files:**
- Read: `docs/roadmap/optimization_and_refactoring.md`
- Read: `src/vm/vm.cpp`
- Read: `src/gc/garbage_collector.cpp`
- Read: `src/compiler/codegen_jump.cpp`

- [x] **Step 1: 定位阶段 4 原注释规范**

Verify 4.1 currently encourages why-oriented comments but does not explicitly prevent long educational comments inside function bodies.

- [x] **Step 2: 定位 PR-35/36/37 新增长注释**

Verify VM dispatch timing, GC finalizer ordering, and jump backpatching explanations are the affected comment blocks.

### Task 2: 调整注释规范与源码落点

**Files:**
- Modify: `docs/roadmap/optimization_and_refactoring.md`
- Modify: `src/vm/vm.cpp`
- Modify: `src/gc/garbage_collector.cpp`
- Modify: `src/compiler/codegen_jump.cpp`

- [x] **Step 1: 更新阶段 4.1 注释分层**

Document file-level/function-group notes, function pre-contract notes, type-adjacent tables, and deep walkthrough docs as the preferred locations.

- [x] **Step 2: 上移 VM 主循环时序说明**

Move the `pc` / `savedpc` / hook timing explanation to a `Dispatch timing note` before `runDispatchBackend()`.

- [x] **Step 3: 上移 GC finalizer 顺序说明**

Move `prepareFinalizers()` and `runFinalizers()` ordering rationale to a `collect(LuaState*) phase contract` before the function body.

- [x] **Step 4: 上移 jump backpatching helper 说明**

Keep the ASCII model at file level and fold helper-specific rationale into `Helper contracts`, removing the PR-37 multi-line helper comments from function bodies.

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

- [x] **Step 2: 跑注释触达区域的聚焦测试**

Run:

```powershell
.\bin\lua_test.exe --filter "Codegen Conditions"
.\bin\lua_test.exe --filter "GC"
.\bin\lua_test.exe --filter "VM Core"
.\bin\lua_test.exe --filter "VM Dispatch"
```

Expected: selected tests pass.

- [x] **Step 3: 跑完整质量门**

Run:

```powershell
.\tools\run_quality_gate.ps1
```

Expected: all registered tests pass.
