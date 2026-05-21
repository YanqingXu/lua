# PR-34 Parser Token String View Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 收窄 `Parser::tokenString` 的返回边界，从拥有型 `Str` 改为借用型 `StrView`，只在 AST 和语义结构需要保存字符串时显式落成 `Str`。

**Architecture:** `Token` 已经拥有 `lexeme`，字符串字面量的解码结果也存放在 `TokenValue` 的 `Str` 中，因此 `Parser::tokenString` 可以安全返回指向当前 token 内部存储的 `StrView`。Parser 分片中的 AST 字段、函数名、参数名、循环变量等仍保持拥有型 `Str`，避免 `advance()` 替换 `current_` 后留下悬垂 view。

**Tech Stack:** C++23 preview, `std::string_view`, Visual Studio MSBuild, existing lightweight unit test framework.

---

### Task 1: 建立 tokenString 返回边界红灯

**Files:**
- Modify: `tests/unit/compiler/test_parser_boundaries.cpp`

- [x] **Step 1: 添加编译期返回类型断言**

Expose the private parser helper only in this boundary sentinel test and assert:

```cpp
static_assert(std::is_same_v<decltype(Parser::tokenString(std::declval<const Token&>())), StrView>);
```

- [x] **Step 2: 添加借用存储行为断言**

Create a name token and a string token, then verify the returned view points at `token.lexeme` or the decoded `TokenValue` string storage.

- [x] **Step 3: 验证红灯**

Run:

```powershell
& 'D:\VS2026\MSBuild\Current\Bin\MSBuild.exe' lua_test.vcxproj /m /nr:false /p:Configuration=Debug /p:Platform=x64
```

Expected before implementation: build fails at the new static assertion because `Parser::tokenString` still returns `Str`.

### Task 2: 迁移 tokenString 为借用型视图

**Files:**
- Modify: `src/compiler/parser.hpp`
- Modify: `src/compiler/parser.cpp`

- [x] **Step 1: 改返回类型**

Change:

```cpp
static StrView tokenString(const Token& token) noexcept;
```

- [x] **Step 2: 内联私有 helper**

Implement the helper in `parser.hpp` so the boundary sentinel can inspect it without changing Parser's production public interface:

```cpp
static StrView tokenString(const Token& token) noexcept {
    if (std::holds_alternative<Str>(token.value)) {
        return std::get<Str>(token.value);
    }
    return token.lexeme;
}
```

- [x] **Step 3: 删除旧 out-of-line `Str` 实现**

Remove the `Parser::tokenString` definition from `parser.cpp`.

### Task 3: 显式保留 AST 字符串所有权

**Files:**
- Modify: `src/compiler/parser_func.cpp`
- Modify: `src/compiler/parser_primary.cpp`
- Modify: `src/compiler/parser_stmt.cpp`
- Modify: `src/compiler/parser_table.cpp`

- [x] **Step 1: 跨 `advance()` 的名字先落成 `Str`**

Use direct construction before consuming the current token:

```cpp
Str varName(tokenString(current_));
Str methodName(tokenString(current_));
```

- [x] **Step 2: AST 字段赋值显式构造 owned string**

Use:

```cpp
nameExpr.name = Str(tokenString(current_));
strExpr.value = Str(tokenString(current_));
memberExpr.member = Str(tokenString(current_));
```

- [x] **Step 3: 字符串列表直接 emplace owned string**

Use:

```cpp
params.emplace_back(tokenString(current_));
localStmt.names.emplace_back(tokenString(current_));
forStmt.vars.emplace_back(tokenString(current_));
```

### Task 4: 验证

**Files:**
- Verify: `lua_test.vcxproj`
- Verify: `tools/run_quality_gate.ps1`

- [x] **Step 1: 构建 lua_test**

Run:

```powershell
& 'D:\VS2026\MSBuild\Current\Bin\MSBuild.exe' lua_test.vcxproj /m /nr:false /p:Configuration=Debug /p:Platform=x64
```

Expected: build succeeds with 0 warnings and 0 errors.

- [x] **Step 2: 跑聚焦测试**

Run:

```powershell
.\bin\lua_test.exe --filter "Parser Boundary"
.\bin\lua_test.exe --filter "Syntax Sugar"
.\bin\lua_test.exe --filter "Method Call"
.\bin\lua_test.exe --filter "Function Codegen"
```

Expected: selected tests pass.

- [x] **Step 3: 结构扫描 view 暂存风险**

Run:

```powershell
rg -n "static Str tokenString|StrView\s+\w+\s*=\s*tokenString|auto\s+\w+\s*=\s*tokenString|=\s*tokenString\(|push_back\(tokenString\(" src\compiler
```

Expected: no matches.

- [x] **Step 4: 跑完整质量门**

Run:

```powershell
.\tools\run_quality_gate.ps1
```

Expected: all registered tests pass.
