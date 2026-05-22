# PR-36 GC Finalizer Comments Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 补强 `GarbageCollector::collect()` 中 finalizer 顺序注释，解释 `prepareFinalizers()`、`propagateMarks()`、weak cleanup、sweep 和 `runFinalizers()` 的时序关系。

**Architecture:** 不修改 GC 行为，只在 `src/gc/garbage_collector.cpp` 的五步 collect 流程中添加“为什么”注释。注释聚焦带 `__gc` 的不可达 userdata 为什么要先复活并重新传播标记，以及为什么真正运行 finalizer 要晚于本轮 sweep。

**Tech Stack:** C++23 preview, existing mark-sweep GC, existing lightweight unit test framework.

---

### Task 1: 确认 GC finalizer 时序边界

**Files:**
- Read: `src/gc/garbage_collector.cpp`
- Read: `src/gc/gc_finalize.cpp`
- Read: `src/gc/gc_mark.cpp`
- Read: `src/gc/gc_sweep.cpp`
- Read: `src/gc/gc_weak.cpp`
- Read: `tests/unit/gc/test_gc.cpp`

- [x] **Step 1: 确认 collect 五步顺序**

Verify `collect(LuaState*)` runs mark, finalizer preparation plus mark propagation, weak cleanup, sweep, then finalizer execution.

- [x] **Step 2: 确认 prepareFinalizers 的复活行为**

Verify `prepareFinalizers()` finds white userdata with `__gc`, marks `FINALIZED`, pushes it into `pendingFinalizers_`, and calls `markObject()` so `propagateMarks()` can retain its reachable graph.

- [x] **Step 3: 确认 sweep 和 finalizer 测试覆盖**

Verify tests cover weak cleanup before sweep and `__gc` finalizer execution once per userdata.

### Task 2: 补充 collect finalizer 顺序注释

**Files:**
- Modify: `src/gc/garbage_collector.cpp`

- [x] **Step 1: 解释 prepareFinalizers 必须早于 weak cleanup / sweep**

Add comments above the `prepareFinalizers()` block explaining that unreachable userdata with `__gc` must be scheduled for finalization rather than freed immediately.

- [x] **Step 2: 解释 second propagation protects resurrection graph**

Add comments explaining that `propagateMarks()` marks userdata, metatable, closure, and other objects reachable from the revived userdata so sweep does not delete objects the finalizer may still access.

- [x] **Step 3: 解释 runFinalizers 晚于 sweep**

Add comments above `runFinalizers()` explaining that other white garbage is already reclaimed, while finalized userdata remains alive for possible resurrection and will not be queued twice.

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

- [x] **Step 2: 跑 GC 相关聚焦测试**

Run:

```powershell
.\bin\lua_test.exe --filter "GC"
.\bin\lua_test.exe --filter "LuaState"
.\bin\lua_test.exe --filter "Metamethod"
```

Expected: selected tests pass.

- [x] **Step 3: 跑完整质量门**

Run:

```powershell
.\tools\run_quality_gate.ps1
```

Expected: all registered tests pass.
