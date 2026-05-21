# PR-32 VM Instruction Span Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 使用 `std::span<const Instruction>` 收窄 VM 执行链路中的字节码只读传递边界。

**Architecture:** 在 `Proto` 上新增只读指令 span 访问器，保留 `getCode()` 兼容现有可写和非执行路径。VM 主循环、frame helper、loop helper、branch/call handler 只读取指令流的位置改为使用 span，避免把 `Vec<Instruction>` 容器类型继续泄漏到执行边界。

**Tech Stack:** C++23 preview, `std::span`, Visual Studio MSBuild, existing lightweight unit test framework.

---

### Task 1: 锁定只读指令视图边界

**Files:**
- Modify: `tests/unit/vm/test_vm_internal_boundaries.cpp`

- [x] **Step 1: 添加编译期边界测试**

Add:

```cpp
#include "core/function.hpp"
#include <span>

void testProtoInstructionSpanBoundary(TestSuite& suite) {
    static_assert(std::is_same_v<decltype(std::declval<const Proto&>().getInstructionSpan()),
                                 std::span<const Instruction>>);

    ASSERT_TRUE(suite, true, "proto instruction stream exposes a read-only span");
}
```

- [x] **Step 2: 验证红灯**

Run:

```powershell
& 'D:\VS2026\MSBuild\Current\Bin\MSBuild.exe' lua_test.vcxproj /m /nr:false /p:Configuration=Debug /p:Platform=x64
```

Expected before implementation: build fails with `getInstructionSpan` not being a member of `Lua::Proto`.

### Task 2: 添加 Proto 只读 span 访问器

**Files:**
- Modify: `src/core/function.hpp`

- [x] **Step 1: 引入 span 头文件**

Add:

```cpp
#include <span>
```

- [x] **Step 2: 添加只读指令视图访问器**

Add next to the const `getCode()` overload:

```cpp
std::span<const Instruction> getInstructionSpan() const noexcept {
    return std::span<const Instruction>(code_.data(), code_.size());
}
```

### Task 3: 迁移 VM 执行侧只读指令访问

**Files:**
- Modify: `src/vm/vm.cpp`
- Modify: `src/vm/vm_frame.cpp`
- Modify: `src/vm/vm_loop.cpp`
- Modify: `src/vm/vm_handlers/vm_handlers_branch.cpp`
- Modify: `src/vm/vm_handlers/vm_handlers_call.cpp`

- [x] **Step 1: 迁移 VM 主循环和 bytecode dump**

Use:

```cpp
const auto code = proto->getInstructionSpan();
```

Expected: `pc` restore, bytecode dump, instruction fetch, and `savedpc` updates use `span::data()`, `span::size()`, and `operator[]`.

- [x] **Step 2: 迁移 closure / generic-for / branch / call helpers**

Use:

```cpp
const auto code = proto->getInstructionSpan();
```

Expected: CLOSURE pseudo-instruction reads, TFORLOOP follow-up jump reads, TEST/TESTSET jump reads, CALL/TAILCALL saved pc updates all use span.

### Task 4: 验证

**Files:**
- Verify: `src/vm`
- Verify: `tests/unit/vm/test_vm_internal_boundaries.cpp`
- Verify: `tools/run_quality_gate.ps1`

- [x] **Step 1: 结构扫描 VM 执行目录**

Run:

```powershell
rg -n 'const Vec<Instruction>& (code|dcode)|proto->getCode\(\)|currentProto->getCode\(\)' src\vm
```

Expected: no matches.

- [x] **Step 2: 跑聚焦测试**

Run:

```powershell
.\bin\lua_test.exe --filter "VM Internal Boundaries"
.\bin\lua_test.exe --filter "VM Dispatch"
```

Expected: selected tests pass.

- [x] **Step 3: 跑完整质量门**

Run:

```powershell
.\tools\run_quality_gate.ps1
```

Expected: all registered tests pass.
