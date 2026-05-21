# PR-28 Trace JSON Format Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:test-driven-development and superpowers:verification-before-completion while executing this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 使用 `std::format` 整理 Trace JSON 输出和值序列化格式化代码，保持 JSONL 输出契约兼容。

**Architecture:** `JsonTraceSink` 继续负责将 trace 事件写入 JSONL 文件，`Trace::value_serializer` 继续负责 JSON value/registers 片段。重构只替换局部格式化机制，不改变 trace 事件结构、文件路径行为、事件上限或 public API。

**Tech Stack:** C++23, `std::format`, existing VM trace tests, MSBuild, existing lightweight unit test framework.

---

### Task 1: 补 Trace JSONL 输出契约测试

**Files:**
- Modify: `tests/unit/vm/test_vm_trace_debug.cpp`

- [x] **Step 1: 添加 JsonTraceSink golden 测试**

新增 `testJsonTraceSinkWritesStableJsonLines`，写入 instruction / call / return / error 四类事件，断言输出 JSONL 行包含稳定字段、JSON 转义和事件计数。

- [x] **Step 2: 构建 lua_test 并运行 VM Trace Debug 专项**

Run:

```powershell
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
$msbuild = Join-Path $installPath 'MSBuild\Current\Bin\MSBuild.exe'
& $msbuild lua_test.vcxproj /m /p:Configuration=Debug /p:Platform=x64
bin\lua_test.exe --filter "VM Trace Debug"
```

Expected: VM Trace Debug tests pass, proving the golden output describes current behavior.

- [x] **Step 3: 记录结构红灯**

Run:

```powershell
rg -n "std::snprintf|#include <sstream>|#include <cstdio>" src\debug
```

Expected: matches remain before implementation.

### Task 2: 用 std::format 整理 src/debug 格式化

**Files:**
- Modify: `src/debug/value_serializer.cpp`
- Modify: `src/debug/json_trace_sink.cpp`

- [x] **Step 1: 替换 ValueSerializer 低层格式化**

将 `ptrToHex(...)`、控制字符 `\uXXXX` 转义、数字格式化中的 `std::snprintf` 改为 `std::format`，并移除不再需要的 `<sstream>` / `<iomanip>` / `<cstdio>`。

- [x] **Step 2: 替换 JsonTraceSink 事件行拼接**

将 instruction / call / return / error JSON 行改为 `std::format(...)` 生成完整行，寄存器快照保持可选字段。

### Task 3: 验证

**Files:**
- Modify: `docs/superpowers/plans/2026-05-21-pr-28-trace-json-format.md`

- [x] **Step 1: 构建 lua_test**

Run:

```powershell
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
$msbuild = Join-Path $installPath 'MSBuild\Current\Bin\MSBuild.exe'
& $msbuild lua_test.vcxproj /m /p:Configuration=Debug /p:Platform=x64
```

Expected: `lua_test.vcxproj` builds successfully.

- [x] **Step 2: 运行 VM Trace Debug 专项**

Run:

```powershell
bin\lua_test.exe --filter "VM Trace Debug"
```

Expected: all selected tests pass.

- [x] **Step 3: 确认 src/debug 旧格式化点清零**

Run:

```powershell
rg -n "std::snprintf|#include <sstream>|#include <cstdio>" src\debug
```

Expected: no matches.

- [x] **Step 4: 运行完整质量门**

Run:

```powershell
tools\run_quality_gate.ps1
```

Expected: all configured unit tests pass; clang tools may be skipped when unavailable.
