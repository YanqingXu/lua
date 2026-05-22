# PR-37 Codegen Jump Backpatching Comments Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 补强 `src/compiler/codegen_jump.cpp` 中跳转回填算法注释，解释旧式 `i32` jump list、`PatchList`、`jpc_` 延迟回填和 `fixjump()` 偏移计算之间的关系，并加入 ASCII 图示。

**Architecture:** 不修改 CodeGenerator 行为，只在跳转 helper 附近补“为什么”注释。重点说明未解析 `JMP` 如何临时串成链表，为什么 `patchList()` 必须先读 next 再覆盖 `sBx`，以及为什么 `patchtohere(i32)` 需要通过 `jpc_` 延迟到下一条真实指令。

**Tech Stack:** C++23 preview, existing CodeGenerator condition pipeline, existing lightweight unit test framework.

---

### Task 1: 确认回填算法边界

**Files:**
- Read: `src/compiler/codegen_jump.cpp`
- Read: `src/compiler/codegen_expr.cpp`
- Read: `src/compiler/codegen_stmt.cpp`
- Read: `src/compiler/codegen_types.hpp`
- Read: `tests/unit/compiler/test_codegen_conditions.cpp`
- Read: `tests/unit/compiler/test_binary_unary_expr.cpp`

- [x] **Step 1: 确认旧式 jump list 表示**

Verify unresolved `JMP` instructions use `sBx = NO_JUMP` as the tail sentinel and temporarily store next jump links through `fixjump()`.

- [x] **Step 2: 确认 `PatchList` 条件分支路径**

Verify `CondResult::trueList` and `falseList` carry explicit PC vectors for `and` / `or` / `not` and comparison branches.

- [x] **Step 3: 确认现有测试覆盖未解析跳转**

Verify compiler tests check that condition bytecode has no pending `JMP sBx == -1` and runtime short-circuit semantics remain intact.

### Task 2: 补充回填算法注释

**Files:**
- Modify: `src/compiler/codegen_jump.cpp`

- [x] **Step 1: 添加文件级回填模型说明**

Add an ASCII diagram showing unresolved jump list chaining and final backpatching to the real target.

- [x] **Step 2: 解释 `jpc_` 延迟回填**

Explain why `jump()` chains pending `jpc_` entries behind a newly emitted `JMP` instead of making earlier jumps land on the intermediate jump.

- [x] **Step 3: 解释链表遍历与偏移写回**

Explain why `patchList()` reads `next` before overwriting `sBx`, and how `getjump()` / `fixjump()` share the same `pc + 1 + sBx` encoding as the VM.

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

- [x] **Step 2: 跑 compiler / control-flow 聚焦测试**

Run:

```powershell
.\bin\lua_test.exe --filter "Codegen Conditions"
.\bin\lua_test.exe --filter "Binary"
.\bin\lua_test.exe --filter "VM Core"
```

Expected: selected tests pass.

- [x] **Step 3: 跑完整质量门**

Run:

```powershell
.\tools\run_quality_gate.ps1
```

Expected: all registered tests pass.
