# PR-31 Nodiscard Boundaries Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为 `parse()` / `generate()` / GC `collect()` 这些必须检查返回值的入口补齐 `[[nodiscard]]` 覆盖。

**Architecture:** 保持函数签名、返回类型和运行时行为不变，只在 public declaration 层添加属性。对现有确实故意忽略 `collect()` 返回值的调用点使用 `(void)` 显式表达意图，避免新属性产生无意义警告。

**Tech Stack:** C++23 preview, `[[nodiscard]]`, Visual Studio MSBuild, existing quality gate.

---

### Task 1: 建立结构基线

**Files:**
- Read: `src/compiler/parser.hpp`
- Read: `src/compiler/codegen.hpp`
- Read: `src/gc/garbage_collector.hpp`
- Read: `src/lib/baselib.cpp`
- Read: `tests/unit/vm/test_lua_state_init.cpp`

- [x] **Step 1: 确认 parse 已覆盖 nodiscard**

Run:

```powershell
rg -n '\[\[nodiscard\]\] std::expected<Chunk' src\compiler\parser.hpp
```

Expected: `Parser::parse()` declaration is already marked.

- [x] **Step 2: 确认 generate 和 collect 尚未覆盖**

Run:

```powershell
rg -n 'usize collect\(|Proto\* generate\(|\[\[nodiscard\]\]' src\compiler\codegen.hpp src\gc\garbage_collector.hpp src\compiler\parser.hpp
```

Expected: `CodeGenerator::generate(...)` and both `GarbageCollector::collect(...)` declarations appear without `[[nodiscard]]`.

- [x] **Step 3: 找出会受 collect 属性影响的忽略返回值调用**

Run:

```powershell
rg --pcre2 -n '^\s*(?!\(void\))[^;]*\bcollect\([^;]*\);' src tests
```

Expected: production hit in `src/lib/baselib.cpp`, test hit in `tests/unit/vm/test_lua_state_init.cpp`, plus documented examples and collected-value assignments that do not require behavior changes.

### Task 2: 添加 nodiscard 属性和显式忽略标记

**Files:**
- Modify: `src/compiler/codegen.hpp`
- Modify: `src/gc/garbage_collector.hpp`
- Modify: `src/lib/baselib.cpp`
- Modify: `tests/unit/vm/test_lua_state_init.cpp`

- [x] **Step 1: 标记 legacy generate 返回值不可忽略**

Change:

```cpp
[[nodiscard]] Proto* generate(const Chunk& chunk, StrView sourceName = {});
```

- [x] **Step 2: 标记 GC collect 返回值不可忽略**

Change:

```cpp
[[nodiscard]] usize collect();
[[nodiscard]] usize collect(LuaState* currentState);
```

- [x] **Step 3: 显式保留 collectgarbage("collect") 的既有返回语义**

Change:

```cpp
(void)gc.collect(L);
L->pushNumber(0);
return 1;
```

- [x] **Step 4: 显式标记固定字符串测试只关心存活性**

Change:

```cpp
(void)gc.collect();
```

### Task 3: 验证

**Files:**
- Verify: `lua_test.vcxproj`
- Verify: `tools/run_quality_gate.ps1`

- [x] **Step 1: 结构扫描 nodiscard 声明**

Run:

```powershell
rg -n '\[\[nodiscard\]\].*(parse|generate|collect)' src\compiler\parser.hpp src\compiler\codegen.hpp src\gc\garbage_collector.hpp
```

Expected: `parse`, `generate`, and both `collect` declarations are listed.

- [x] **Step 2: 构建 lua_test**

Run:

```powershell
& 'D:\VS2026\MSBuild\Current\Bin\MSBuild.exe' lua_test.vcxproj /m /p:Configuration=Debug /p:Platform=x64
```

Expected: build succeeds without nodiscard warnings.

- [x] **Step 3: 跑完整质量门**

Run:

```powershell
.\tools\run_quality_gate.ps1
```

Expected: all registered tests pass.
