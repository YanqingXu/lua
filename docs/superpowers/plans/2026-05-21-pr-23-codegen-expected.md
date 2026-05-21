# PR-23 CodeGenerator Expected Entry Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 推进 `std::expected` 第二段迁移，为 `CodeGenerator` 增加显式的 expected 入口，同时保持旧 `generate()` 调用兼容。

**Architecture:** `tryGenerate()` 是新的非抛出式编译入口，返回 `std::expected<Proto*, CodegenError>`。旧 `generate()` 保留 `Proto*` 签名，内部调用 `tryGenerate()`，失败时继续抛出 `CodegenError`，让现有调用点和测试保持兼容。

**Tech Stack:** C++23 `<expected>`, existing `LuaError` hierarchy, current `CodeGenerator` split implementation.

---

### Task 1: Add The Failing Boundary Tests

**Files:**
- Modify: `tests/unit/compiler/test_codegen_state.cpp`

- [x] **Step 1: Assert the new expected return type**

Add a compile-time signature check:

```cpp
using GenerateResult = decltype(std::declval<CodeGenerator&>().tryGenerate(std::declval<const Chunk&>()));
bool hasExpectedSignature = std::is_same_v<GenerateResult, std::expected<Proto*, CodegenError>>;
ASSERT_TRUE(suite, hasExpectedSignature, "tryGenerate returns expected proto or codegen error");
```

- [x] **Step 2: Assert success returns a `Proto*`**

Parse `return 42`, call `tryGenerate()`, and verify the returned proto is non-null and keeps the source name.

- [x] **Step 3: Assert codegen failure returns `CodegenError`**

Parse `break`, call `tryGenerate()`, and verify the result has no value and preserves the `no loop to break` message.

- [x] **Step 4: Assert legacy `generate()` still throws**

Parse `break`, call `generate()`, and verify callers can still catch `CodegenError`.

- [x] **Step 5: Verify red**

Run:

```powershell
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
$msbuild = Join-Path $installPath 'MSBuild\Current\Bin\MSBuild.exe'
& $msbuild lua_test.vcxproj /m /p:Configuration=Debug /p:Platform=x64
```

Expected before implementation: compile failure because `CodeGenerator::tryGenerate` and `CodegenError` do not exist.

### Task 2: Implement The Expected Entry

**Files:**
- Modify: `src/common/lua_error.hpp`
- Modify: `src/compiler/codegen.hpp`
- Modify: `src/compiler/codegen.cpp`

- [x] **Step 1: Add `CodegenError`**

Add a `LuaError`-derived error type:

```cpp
class CodegenError : public LuaError {
public:
    using LuaError::LuaError;
};
```

- [x] **Step 2: Add `CodeGenerator::tryGenerate()`**

Expose:

```cpp
[[nodiscard]] std::expected<Proto*, CodegenError> tryGenerate(const Chunk& chunk, StrView sourceName = {});
```

- [x] **Step 3: Move the current implementation behind `generateUnchecked()`**

Keep the old generation body unchanged except for moving it to a private helper.

- [x] **Step 4: Keep `generate()` compatible**

Make `generate()` call `tryGenerate()` and throw the returned `CodegenError` when no value exists.

- [x] **Step 5: Clean up a failed top-level proto**

When `tryGenerate()` catches a generation exception, unregister and delete the current top-level `Proto` before returning `std::unexpected`.

### Task 3: Verify

**Files:**
- Read: `tools/run_quality_gate.ps1`

- [x] **Step 1: Build `lua_test`**

Run the same MSBuild command from Task 1.

Expected after implementation: build succeeds with 0 errors.

- [x] **Step 2: Run the focused test suite**

Run:

```powershell
bin\lua_test.exe --filter "Codegen State"
```

Expected: the focused suite passes.

- [x] **Step 3: Run the full quality gate**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
```

Expected: documentation drift, build, and full test suite pass. `clang-format` / `clang-tidy` may be skipped if not installed.
