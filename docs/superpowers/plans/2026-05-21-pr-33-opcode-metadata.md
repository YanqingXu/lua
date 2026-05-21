# PR-33 Opcode Metadata Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** constexpr 化 opcode 元数据表，并合并 `OpcodeGroup` / metamethod 元数据，消除 compiler opcode 属性表和 VM dispatch 分组表之间的重复维护。

**Architecture:** 在 `src/compiler/opcode.hpp` 中引入统一的 `OpcodeMetadata` constexpr 表，覆盖 opcode 名称、指令格式、B/C 参数模式、A/T 标记、VM dispatch 分组和 metamethod 标记。`opcode.cpp` 保留现有 public accessor 作为兼容入口，`vm_dispatch.hpp` 保留现有 helper 名称，但二者都改为读取同一张表。

**Tech Stack:** C++23 preview, constexpr metadata, Visual Studio MSBuild, existing lightweight unit test framework.

---

### Task 1: 建立 handler 表一致性保护

**Files:**
- Modify: `tests/unit/vm/test_vm_dispatch.cpp`

- [x] **Step 1: 添加 handler group 一致性断言**

In `testHandlerTableCoversOpcodeSpace`, assert every handler entry group matches `opcodeMetadata(expected).group`.

- [x] **Step 2: 验证红灯**

Run:

```powershell
& 'D:\VS2026\MSBuild\Current\Bin\MSBuild.exe' lua_test.vcxproj /m /nr:false /p:Configuration=Debug /p:Platform=x64
```

Expected before implementation: build fails with `opcodeMetadata` not being found.

### Task 2: 引入统一 constexpr opcode 元数据

**Files:**
- Modify: `src/compiler/opcode.hpp`

- [x] **Step 1: 添加 `OpcodeGroup` 和 `OpcodeMetadata`**

Move the dispatch group enum into the compiler opcode metadata boundary under `Lua::VM`, then define a metadata record containing the previous opcode mode/name fields plus dispatch group and metamethod flags.

- [x] **Step 2: 添加 `kOpcodeMetadata`**

Create an inline constexpr array with one entry per opcode, preserving the existing opcode name, op mode, argument modes, A/T flags, dispatch group, and metamethod behavior.

- [x] **Step 3: 添加 constexpr accessors**

Add:

```cpp
constexpr bool isValidOpcode(OpCode op) noexcept;
constexpr const OpcodeMetadata& opcodeMetadata(OpCode op) noexcept;
```

Expected: invalid opcode values return an `UNKNOWN` metadata record, matching the existing `getOpName` fallback.

### Task 3: 迁移现有读取入口

**Files:**
- Modify: `src/compiler/opcode.cpp`
- Modify: `src/vm/vm_dispatch.hpp`

- [x] **Step 1: 迁移 opcode 属性访问函数**

Change `getOpMode`, `getBMode`, `getCMode`, `testAMode`, `testTMode`, and `getOpName` to read `opcodeMetadata(op)`.

- [x] **Step 2: 迁移 VM dispatch helper**

Change `opcodeGroup` and `mayInvokeMetamethod` to read `opcodeMetadata(op)`, keeping existing helper APIs intact.

### Task 4: 验证

**Files:**
- Verify: `lua_test.vcxproj`
- Verify: `src/compiler/opcode.cpp`
- Verify: `src/vm/vm_dispatch.hpp`
- Verify: `tools/run_quality_gate.ps1`

- [x] **Step 1: 构建 lua_test**

Run:

```powershell
& 'D:\VS2026\MSBuild\Current\Bin\MSBuild.exe' lua_test.vcxproj /m /nr:false /p:Configuration=Debug /p:Platform=x64
```

Expected: build succeeds with 0 warnings and 0 errors.

- [x] **Step 2: 跑聚焦测试**

Run:

```powershell
.\bin\lua_test.exe --filter "VM Dispatch"
.\bin\lua_test.exe --filter "Bytecode Printer"
```

Expected: selected tests pass.

- [x] **Step 3: 结构扫描旧重复表**

Run:

```powershell
rg -n "switch \(op\)|opModes|opNames|case OpCode::" src\compiler\opcode.cpp src\vm\vm_dispatch.hpp
```

Expected: no matches.

- [x] **Step 4: 跑完整质量门**

Run:

```powershell
.\tools\run_quality_gate.ps1
```

Expected: all registered tests pass.
