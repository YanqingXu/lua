# PR-04 AST Visitor Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 新增标准 AST Visitor 基础设施，并让 `CodeGenerator::emitValue` 的表达式右值分派通过 `ExprVisitor` 示范迁移。

**Architecture:** `src/compiler/ast_visitor.hpp` 提供 CRTP 风格的 `ExprVisitor` / `StmtVisitor`，统一封装 `std::visit` 分派。`CodeGenerator` 先只继承并使用 `ExprVisitor<CodeGenerator, ValueResult>`，语句生成暂不迁移，避免超出 PR-04 的示范范围。

**Tech Stack:** C++20/23、`std::variant`、MSBuild、现有轻量单元测试框架。

---

### Task 1: Visitor 编译期测试

**Files:**
- Create: `tests/unit/compiler/test_ast_visitor.cpp`
- Modify: `tests/unit/framework/test_runner.cpp`
- Modify: `tests/unit/framework/test_registry.hpp`
- Modify: `CMakeLists.txt`
- Modify: `lua_test.vcxproj`
- Modify: `lua_test.vcxproj.filters`

- [ ] **Step 1: 写入失败测试**

新增一个最小 `ExprVisitor` / `StmtVisitor` 使用样例，先包含尚不存在的 `compiler/ast_visitor.hpp`。测试验证：

```cpp
struct ExprNameVisitor : ExprVisitor<ExprNameVisitor, const char*> {
    const char* visitNode(const NilExpr&) { return "nil"; }
    const char* visitNode(const BoolExpr&) { return "bool"; }
    const char* visitNode(const NumberExpr&) { return "number"; }
    const char* visitNode(const StringExpr&) { return "string"; }
    const char* visitNode(const VarargExpr&) { return "vararg"; }
    const char* visitNode(const NameExpr&) { return "name"; }
    const char* visitNode(const BinaryExpr&) { return "binary"; }
    const char* visitNode(const UnaryExpr&) { return "unary"; }
    const char* visitNode(const TableExpr&) { return "table"; }
    const char* visitNode(const CallExpr&) { return "call"; }
    const char* visitNode(const IndexExpr&) { return "index"; }
    const char* visitNode(const MemberExpr&) { return "member"; }
    const char* visitNode(const FunctionExpr&) { return "function"; }
    const char* visitNode(const ParenExpr&) { return "paren"; }
};
```

- [ ] **Step 2: 运行测试确认失败**

Run: `& 'D:\VS2026\MSBuild\Current\Bin\MSBuild.exe' .\lua_test.vcxproj /p:Configuration=Debug /p:Platform=x64 /m`

Expected: 编译失败，提示找不到 `compiler/ast_visitor.hpp`。

### Task 2: 新增 Visitor 基础设施

**Files:**
- Create: `src/compiler/ast_visitor.hpp`
- Modify: `lua.vcxproj`
- Modify: `lua.vcxproj.filters`

- [ ] **Step 1: 新增 CRTP Visitor**

实现：

```cpp
template <typename Derived, typename R = void>
struct ExprVisitor {
    R visit(const Expr& expr) {
        return std::visit([this](const auto& node) -> R {
            return static_cast<Derived*>(this)->visitNode(node);
        }, expr.variant);
    }
};
```

`StmtVisitor` 对 `Stmt` 做同样处理。

- [ ] **Step 2: 运行新增测试确认通过**

Run: `.\bin\lua_test.exe --filter "AST Visitor"`

Expected: AST Visitor 测试通过。

### Task 3: `emitValue` 示例迁移

**Files:**
- Modify: `src/compiler/codegen.hpp`
- Modify: `src/compiler/codegen_expr.cpp`

- [ ] **Step 1: 让 `CodeGenerator` 继承表达式 Visitor**

在 `codegen.hpp` 引入 `compiler/ast_visitor.hpp`，并让类继承：

```cpp
class CodeGenerator : private ExprVisitor<CodeGenerator, ValueResult>
```

同时声明 `visitNode(...)` 重载，覆盖所有 `ExprVariant` 成员。

- [ ] **Step 2: 将 `emitValue` 分派替换为 Visitor 调用**

保留 `state_.currentLine` 设置/恢复逻辑，将长 `std::get_if` 链移动到对应 `visitNode` 重载中：

```cpp
ValueResult result = ExprVisitor<CodeGenerator, ValueResult>::visit(e);
```

- [ ] **Step 3: 运行表达式相关测试**

Run: `.\bin\lua_test.exe --filter "Binary/Unary Expressions"`

Expected: 失败数为 0。

### Task 4: 全量验证

**Files:**
- No additional code files.

- [ ] **Step 1: 运行质量门**

Run: `powershell -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1`

Expected: 文档漂移检查 OK，MSBuild 成功，单元测试失败数为 0；`clang-format` / `clang-tidy` 在当前环境可能因 PATH 缺失跳过。

- [ ] **Step 2: 运行显式完整测试**

Run: `.\bin\lua_test.exe`

Expected: 注册测试数比 PR-03 多 2 个以上，`Failed: 0`。

- [ ] **Step 3: 检查空白错误**

Run: `git diff --check`

Expected: exit code 0。

