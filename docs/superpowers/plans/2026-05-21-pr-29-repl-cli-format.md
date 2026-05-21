# PR-29 REPL CLI Format Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:test-driven-development and superpowers:verification-before-completion while executing this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 使用 `std::format` 整理 REPL / CLI 错误输出格式化，保持现有错误文本兼容。

**Architecture:** `REPL::reportError(...)` 继续作为脚本模式和 REPL 模式的错误输出边界，`main.cpp` 继续只通过 REPL 错误报告函数输出 CLI 错误。重构只替换局部字符串拼接机制，不改变 REPL public API、CLI 模式选择或错误文本契约。

**Tech Stack:** C++23, `std::format`, existing REPL command tests, MSBuild, existing lightweight unit test framework.

---

### Task 1: 补 REPL 错误输出契约测试

**Files:**
- Modify: `tests/unit/app/test_repl_commands.cpp`

- [x] **Step 1: 添加 reportError 输出测试**

新增测试，重定向 `std::cerr`，断言：

```text
lua_test.exe: chunk.lua:17: syntax boom
stdin:3: repl boom
```

- [x] **Step 2: 添加未知元命令输出测试**

新增测试，调用 `REPL::runMetaCommand(...)`，断言未知命令输出精确为：

```text
unknown REPL command: .wat
```

- [x] **Step 3: 构建并运行 REPL Commands 专项建立基线**

Run:

```powershell
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
$msbuild = Join-Path $installPath 'MSBuild\Current\Bin\MSBuild.exe'
& $msbuild lua_test.vcxproj /m /p:Configuration=Debug /p:Platform=x64
bin\lua_test.exe --filter "REPL Commands"
```

Expected: REPL Commands tests pass, proving the added assertions describe current behavior.

- [x] **Step 4: 记录结构红灯**

Run:

```powershell
rg -n "std::ostringstream|#include <sstream>" src\main.cpp src\repl.cpp
```

Expected: matches remain before implementation.

### Task 2: 用 std::format 整理错误输出

**Files:**
- Modify: `src/repl.cpp`
- Modify: `src/main.cpp`

- [x] **Step 1: 替换 REPL 错误格式化**

为 `src/repl.cpp` 添加 `<format>`，将 `reportError(...)`、`printParseError(...)`、未知元命令错误输出改为 `std::format(...)`。

- [x] **Step 2: 替换 CLI 测试脚本缺失错误格式化**

为 `src/main.cpp` 添加 `<format>`，删除 `<sstream>`，将 `std::ostringstream` 拼接改为 `std::format("test script not found: {}", kTestScriptPath)`。

### Task 3: 验证

**Files:**
- Modify: `docs/superpowers/plans/2026-05-21-pr-29-repl-cli-format.md`

- [x] **Step 1: 构建 lua_test**

Run:

```powershell
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
$msbuild = Join-Path $installPath 'MSBuild\Current\Bin\MSBuild.exe'
& $msbuild lua_test.vcxproj /m /p:Configuration=Debug /p:Platform=x64
```

Expected: `lua_test.vcxproj` builds successfully.

- [x] **Step 2: 运行 REPL Commands 专项**

Run:

```powershell
bin\lua_test.exe --filter "REPL Commands"
```

Expected: all selected tests pass.

- [x] **Step 3: 确认 REPL / CLI 旧拼接清零**

Run:

```powershell
rg -n "std::ostringstream|#include <sstream>" src\main.cpp src\repl.cpp
```

Expected: no matches.

- [x] **Step 4: 运行完整质量门**

Run:

```powershell
tools\run_quality_gate.ps1
```

Expected: all configured unit tests pass; clang tools may be skipped when unavailable.
