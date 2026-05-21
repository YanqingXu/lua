# PR-25 AST Visitor Concepts Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为 AST Visitor 引入 C++20 concepts 约束，使 visitor 覆盖缺口和返回类型不匹配在编译期更早暴露。

**Architecture:** 在 `src/compiler/ast_visitor.hpp` 中新增公开 concepts：单节点 `VisitsNode` / `VisitsNodeAs`，以及全 AST variant 覆盖 `VisitsExprNodes` / `VisitsStmtNodes`。`ExprVisitor` / `StmtVisitor` 的 `visit()` 入口使用内部访问检查约束全部 variant alternative，保持对 `CodeGenerator` 私有 `visitNode(...)` 的 friend 访问兼容。

**Tech Stack:** C++23, C++20 concepts, `std::variant`, existing lightweight unit test framework, MSBuild.

---

### Task 1: 添加 concepts 编译期测试

**Files:**
- Modify: `tests/unit/compiler/test_ast_visitor.cpp`

- [x] **Step 1: 写入失败测试**

新增 `static_assert` 覆盖：

```cpp
static_assert(VisitsNode<ExprNameVisitor, NumberExpr>);
static_assert(!VisitsNode<PartialExprVisitor, NilExpr>);
static_assert(VisitsNodeAs<ExprNameVisitor, NumberExpr, const char*>);
static_assert(!VisitsNodeAs<ExprNameVisitor, NumberExpr, int>);
static_assert(VisitsExprNodes<ExprNameVisitor, const char*>);
static_assert(!VisitsExprNodes<PartialExprVisitor, const char*>);
static_assert(VisitsStmtNodes<StmtNameVisitor, const char*>);
```

- [x] **Step 2: 运行构建确认红灯**

Run:

```powershell
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
$msbuild = Join-Path $installPath 'MSBuild\Current\Bin\MSBuild.exe'
& $msbuild lua_test.vcxproj /m /p:Configuration=Debug /p:Platform=x64
```

Expected: compile failure mentioning missing `VisitsNode` / `VisitsNodeAs` / `VisitsExprNodes`.

### Task 2: 实现 AST Visitor concepts

**Files:**
- Modify: `src/compiler/ast_visitor.hpp`

- [x] **Step 1: 添加公开 concepts**

声明 `VisitsNode`、`VisitsNodeAs`、`VisitsExprNodes`、`VisitsStmtNodes`，用于测试和后续 AST 使用者复用。

- [x] **Step 2: 给 visitor 入口加静态约束**

在 `ExprVisitor::visit()` / `StmtVisitor::visit()` 入口对所有 variant alternative 做 `static_assert`，错误信息明确指向缺少 `visitNode(const Node&)` 或返回类型不兼容。

- [x] **Step 3: 保持私有 visitor 兼容**

内部检查放在 `ExprVisitor` / `StmtVisitor` 成员上下文中，确保 `CodeGenerator` 通过 `friend struct ExprVisitor<CodeGenerator, ValueResult>` 继续访问私有 `visitNode(...)`。

### Task 3: 验证

**Files:**
- Modify: `docs/superpowers/plans/2026-05-21-pr-25-ast-visitor-concepts.md`

- [x] **Step 1: 运行目标构建**

Run:

```powershell
& $msbuild lua_test.vcxproj /m /p:Configuration=Debug /p:Platform=x64
```

Expected: build succeeds.

- [x] **Step 2: 运行 AST Visitor 测试**

Run:

```powershell
bin\lua_test.exe --filter "AST Visitor"
```

Expected: AST Visitor tests pass.

- [x] **Step 3: 运行完整质量门**

Run:

```powershell
tools\run_quality_gate.ps1
```

Expected: all configured unit tests pass; clang tools may be skipped when unavailable.
