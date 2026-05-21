# PR-26 Bytecode Printer Format Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 使用 `std::format` 重写 Bytecode Printer 的局部格式化输出，同时保持 `printProtoBytecode(...)` 的文本契约不变。

**Architecture:** 只修改 `src/bytecode/bytecode_printer.cpp` 的字符串组装方式，把 `std::ostringstream` helper 拼接替换为 `std::format`。输出流仍由调用方传入，`printProtoBytecode(...)` public API 和测试中的输出片段保持兼容。

**Tech Stack:** C++23, `std::format`, existing `Proto` / opcode helpers, MSBuild, existing lightweight unit test framework.

---

### Task 1: 补输出兼容 characterization 测试

**Files:**
- Modify: `tests/unit/bytecode/test_bytecode_printer.cpp`

- [x] **Step 1: 添加转义和越界常量测试**

新增一个测试，构造包含引号、反斜杠、换行、制表符的字符串常量，并构造一个 `LOADK` 指向越界常量索引，断言输出包含：

```cpp
"K[0] = string \"quote\\\" slash\\\\ line\\n tab\\t\""
"0000 | line 20 | LOADK | A=0 Bx=3 ; K[3] = <out of range>"
```

- [x] **Step 2: 运行 Bytecode Printer 测试建立基线**

Run:

```powershell
bin\lua_test.exe --filter "Bytecode Printer"
```

Expected: Bytecode Printer tests pass before the refactor, proving the new checks describe current output.

### Task 2: 用 std::format 重写局部格式化

**Files:**
- Modify: `src/bytecode/bytecode_printer.cpp`

- [x] **Step 1: 替换 helper 中的 ostringstream**

将 `formatValue(...)`、`formatConstant(...)`、`addRKComment(...)`、`iAsBx target` 的字符串构造改为 `std::format(...)`。

- [x] **Step 2: 替换指令行前缀格式化**

将 `std::setw(4) << std::setfill('0') << pc` 等输出格式换为：

```cpp
out << std::format("{:04} | line {} | {} | ", pc, f->getLine(pc), getOpName(op));
```

- [x] **Step 3: 移除不再使用的头文件**

删除 `<iomanip>` / `<sstream>`，新增 `<format>`。

### Task 3: 验证

**Files:**
- Modify: `docs/superpowers/plans/2026-05-21-pr-26-bytecode-printer-format.md`

- [x] **Step 1: 运行 Bytecode Printer 测试**

Run:

```powershell
bin\lua_test.exe --filter "Bytecode Printer"
```

Expected: Bytecode Printer tests pass.

- [x] **Step 2: 构建 lua_bytecode 目标**

Run:

```powershell
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
$msbuild = Join-Path $installPath 'MSBuild\Current\Bin\MSBuild.exe'
& $msbuild lua_bytecode.vcxproj /m /p:Configuration=Debug /p:Platform=x64
```

Expected: `lua_bytecode.vcxproj` builds successfully.

- [x] **Step 3: 运行完整质量门**

Run:

```powershell
tools\run_quality_gate.ps1
```

Expected: all configured unit tests pass; clang tools may be skipped when unavailable.
