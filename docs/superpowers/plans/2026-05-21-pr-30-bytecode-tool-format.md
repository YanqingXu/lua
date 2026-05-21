# PR-30 Bytecode Tool Format Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 推进 `std::format` 第五段迁移，整理 `lua_bytecode` 工具入口层的 Usage 和错误输出，同时保持可观察 CLI 行为不变。

**Architecture:** 只修改 `src/bytecode/bytecode_main.cpp`，把入口层的文本输出迁到 `std::format`。`printProtoBytecode(...)` 和 Bytecode Printer 文本契约不变，构建系统不需要新增源文件。

**Tech Stack:** C++23 preview, `std::format`, Visual Studio MSBuild, existing `lua_bytecode` executable, existing quality gate.

---

### Task 1: 锁定入口输出契约

**Files:**
- Read: `src/bytecode/bytecode_main.cpp`

- [x] **Step 1: 记录无参数 Usage 行为**

Run:

```powershell
$output = & .\bin\lua_bytecode.exe 2>&1
"exit=$LASTEXITCODE"
$output
```

Expected:

```text
exit=1
Usage: bytecode_main <script.lua> [full]
```

- [x] **Step 2: 记录缺失文件错误行为**

Run:

```powershell
$output = & .\bin\lua_bytecode.exe __pr30_missing_input__.lua 2>&1
"exit=$LASTEXITCODE"
$output
```

Expected:

```text
exit=3
[ERROR] Exception: cannot open file: __pr30_missing_input__.lua
```

- [x] **Step 3: 确认迁移前旧式输出写法存在**

Run:

```powershell
rg -n 'std::cerr << "Usage: bytecode_main|std::cerr << "\[ERROR\]|<< e\.what\(\)|std::endl' src\bytecode\bytecode_main.cpp
```

Expected: 命中 Usage、Proto 生成失败、Exception 和 Unknown exception 输出行。

### Task 2: 迁移入口输出到 std::format

**Files:**
- Modify: `src/bytecode/bytecode_main.cpp`

- [x] **Step 1: 添加格式化和显式移动依赖**

Change:

```cpp
#include <format>
#include <utility>
```

- [x] **Step 2: 提取工具名常量**

Change:

```cpp
namespace {
constexpr const char* kToolName = "bytecode_main";
} // namespace
```

- [x] **Step 3: 使用 std::format 输出入口文本**

Change:

```cpp
std::cerr << std::format("Usage: {} <script.lua> [full]\n", kToolName);
std::cerr << std::format("[ERROR] {}\n", "Failed to generate Proto");
std::cerr << std::format("[ERROR] Exception: {}\n", e.what());
std::cerr << std::format("[ERROR] {}\n", "Unknown exception");
```

### Task 3: 验证

**Files:**
- Verify: `lua_bytecode.vcxproj`
- Verify: `bin\lua_bytecode.exe`
- Verify: `tools\run_quality_gate.ps1`

- [x] **Step 1: 构建 bytecode 工具**

Run:

```powershell
& 'D:\VS2026\MSBuild\Current\Bin\MSBuild.exe' lua_bytecode.vcxproj /m /p:Configuration=Debug /p:Platform=x64
```

Expected: build succeeds with `0 个警告` and `0 个错误`.

- [x] **Step 2: 复测入口输出**

Run the two commands from Task 1 again.

Expected: exit code and visible output match Task 1.

- [x] **Step 3: 确认旧式入口输出写法已消失**

Run:

```powershell
rg -n 'std::cerr << "Usage: bytecode_main|std::cerr << "\[ERROR\]|<< e\.what\(\)|std::endl' src\bytecode\bytecode_main.cpp
```

Expected: no matches.

- [x] **Step 4: 跑完整质量门**

Run:

```powershell
.\tools\run_quality_gate.ps1
```

Expected: all registered tests pass.
