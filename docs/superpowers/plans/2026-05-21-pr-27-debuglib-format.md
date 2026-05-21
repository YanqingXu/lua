# PR-27 Debug Library Format Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:test-driven-development and superpowers:verification-before-completion while executing this plan.

**Goal:** 使用 `std::format` 替换 Debug Library 中的 `std::ostringstream` 拼接，保持 `debug.traceback(...)` 和函数描述文本契约不变。

**Architecture:** 只修改 `src/lib/debuglib.cpp` 的局部字符串构造方式。`debug` 标准库注册、Lua API 行为、Visual Studio / CMake 源文件清单都保持不变。

**Tech Stack:** C++23, `std::format`, existing Debug Library tests, MSBuild, existing lightweight unit test framework.

---

### Task 1: 补 traceback 输出契约测试

**Files:**
- Modify: `tests/unit/stdlib/test_debuglib.cpp`

- [x] **Step 1: 锁定 traceback 函数帧文本**

在 `testTracebackFromLua` 中断言 traceback 包含 Lua 函数帧描述：

```cpp
": in function <test_debuglib_trace.lua:1>"
```

- [x] **Step 2: 运行 Debug Library 测试建立基线**

Run:

```powershell
bin\lua_test.exe --filter "Debug Library"
```

Expected: Debug Library tests pass before implementation, proving the added assertion describes current behavior.

### Task 2: 用 std::format 替换 Debug Library ostringstream

**Files:**
- Modify: `src/lib/debuglib.cpp`

- [x] **Step 1: 替换函数描述拼接**

将 `describeFunction(...)` 中的 `std::ostringstream` 改为 `std::format("function <{}:{}>", source, line)`。

- [x] **Step 2: 替换 traceback 单帧拼接**

将 `formatFrameLine(...)` 中的 `std::ostringstream` 改为 `std::format(...)`，保留有行号和无行号两种输出分支。

- [x] **Step 3: 替换完整 traceback 拼接**

将 `luaDebug_traceback(...)` 中的 `std::ostringstream` 改为 `Str` 累积，并用 `std::format(...)` 拼接消息头和帧行。

- [x] **Step 4: 更新头文件依赖**

删除 `<sstream>`，新增 `<format>`。

### Task 3: 验证

**Files:**
- Modify: `docs/superpowers/plans/2026-05-21-pr-27-debuglib-format.md`

- [x] **Step 1: 运行 Debug Library 测试**

Run:

```powershell
bin\lua_test.exe --filter "Debug Library"
```

Expected: Debug Library tests pass.

- [x] **Step 2: 构建 lua_test 目标**

Run:

```powershell
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
$msbuild = Join-Path $installPath 'MSBuild\Current\Bin\MSBuild.exe'
& $msbuild lua_test.vcxproj /m /p:Configuration=Debug /p:Platform=x64
```

Expected: `lua_test.vcxproj` builds successfully.

- [x] **Step 3: 确认 Debug Library 不再使用 ostringstream**

Run:

```powershell
rg -n "std::ostringstream|#include <sstream>" src\lib\debuglib.cpp
```

Expected: no matches.

- [x] **Step 4: 运行完整质量门**

Run:

```powershell
tools\run_quality_gate.ps1
```

Expected: all configured unit tests pass; clang tools may be skipped when unavailable.
