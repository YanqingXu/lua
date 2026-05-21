# PR-24 VM Expected Execution Entry Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为 VM 的 `executeProto` 执行入口增加 `std::expected` 形式，使调用方可以选择非异常错误返回，同时保持现有抛异常入口兼容。

**Architecture:** 新增 `VM::tryExecuteProto(...)` 两个重载，返回 `std::expected<ExecResult, RuntimeError>`。现有 `executeProto(...)` 继续走原有抛异常路径，避免改变栈溢出等异常类型的历史行为；expected 入口只在边界处捕获 VM 运行期错误并转成 `RuntimeError` 值。

**Tech Stack:** C++23, `std::expected`, MSBuild Visual Studio project files, existing unit test framework.

---

### Task 1: 先添加 VM expected 入口测试

**Files:**
- Modify: `tests/unit/vm/test_runtime_services.cpp`

- [x] **Step 1: 写入失败测试**

在 Runtime Services 测试中新增三类断言：

```cpp
using TryResult = decltype(VM::tryExecuteProto(
    std::declval<RuntimeServices&>(),
    std::declval<LuaState*>(),
    std::declval<Proto*>(),
    1));
bool hasExpectedSignature = std::is_same_v<TryResult, std::expected<ExecResult, RuntimeError>>;
ASSERT_TRUE(suite, hasExpectedSignature, "tryExecuteProto returns expected exec result or runtime error");
```

再覆盖 successful proto execution、null proto error return、legacy throwing compatibility。

- [x] **Step 2: 运行构建确认红灯**

Run:

```powershell
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
$msbuild = Join-Path $installPath 'MSBuild\Current\Bin\MSBuild.exe'
& $msbuild lua_test.vcxproj /m /p:Configuration=Debug /p:Platform=x64
```

Expected: compile failure mentioning `tryExecuteProto` is not a member of `Lua::VM`.

### Task 2: 实现 VM expected 执行入口

**Files:**
- Modify: `src/vm/vm.hpp`
- Modify: `src/vm/vm.cpp`

- [x] **Step 1: 声明 expected API**

在 `vm.hpp` 中包含 `<expected>` 和 `common/lua_error.hpp`，并声明：

```cpp
[[nodiscard]] std::expected<ExecResult, RuntimeError> tryExecuteProto(
    LuaState* L, Proto* proto, i32 nexeccalls = 1);

[[nodiscard]] std::expected<ExecResult, RuntimeError> tryExecuteProto(
    RuntimeServices& services, LuaState* L, Proto* proto, i32 nexeccalls = 1);
```

- [x] **Step 2: 复用原有执行主体**

在 `vm.cpp` 中把 `executeProto(RuntimeServices&, ...)` 的原有主体抽成匿名命名空间内的 `executeProtoUnchecked(...)`，让旧入口继续直接调用它。

- [x] **Step 3: 增加 expected 包装**

`tryExecuteProto(...)` 调用 `executeProtoUnchecked(...)`，捕获 `RuntimeError` / `LuaError` / `std::exception` 并返回 `std::unexpected(RuntimeError(...))`，保留 `std::bad_alloc` 抛出。

### Task 3: 验证

**Files:**
- Modify: `docs/superpowers/plans/2026-05-21-pr-24-vm-expected.md`

- [x] **Step 1: 运行目标构建**

Run:

```powershell
& $msbuild lua_test.vcxproj /m /p:Configuration=Debug /p:Platform=x64
```

Expected: build succeeds.

- [x] **Step 2: 运行 Runtime Services 测试**

Run:

```powershell
bin\lua_test.exe --filter "Runtime Services"
```

Expected: new VM expected tests pass.

- [x] **Step 3: 运行完整质量门**

Run:

```powershell
tools\run_quality_gate.ps1
```

Expected: all configured unit tests pass; clang tools may be skipped when unavailable.

- [x] **Step 4: 检查公开入口兼容**

Run:

```powershell
& $msbuild lua_app.vcxproj /m /p:Configuration=Debug /p:Platform=x64
& $msbuild lua_bytecode.vcxproj /m /p:Configuration=Debug /p:Platform=x64
```

Expected: both consumer targets compile successfully with the updated `vm.hpp`.
