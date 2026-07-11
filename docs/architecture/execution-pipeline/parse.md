# Parse — 语法分析

## 1. 这个模块解决什么问题？

将 Token 流转换为抽象语法树（AST）。

## 2. 在整体执行链路中的位置

```
Load Source → Tokenize → Parse → Compile → VM Execute
                            ↑
                       (第三阶段)
```

## 3. 核心文件

| 文件 | 作用 |
|------|------|
| `src/compiler/parser/parser.hpp/cpp` | 语法分析器入口和总体调度 |
| `src/compiler/parser/parser_expr.cpp` | 表达式解析 |
| `src/compiler/parser/parser_stmt.cpp` | 语句解析 |
| `src/compiler/parser/parser_func.cpp` | 函数定义解析 |
| `src/compiler/parser/parser_primary.cpp` | 基本表达式解析 |
| `src/compiler/parser/parser_table.cpp` | 表构造器解析 |
| `src/compiler/ast.hpp` | AST 节点定义 |

## 4. AST 节点体系

### 表达式节点（14 种）

| 节点 | Lua 示例 |
|------|---------|
| `NilExpr` | `nil` |
| `BoolExpr` | `true`, `false` |
| `NumberExpr` | `42`, `3.14` |
| `StringExpr` | `"hello"` |
| `VarargExpr` | `...` |
| `NameExpr` | `x`, `myVar` |
| `BinaryExpr` | `a + b`, `x > y` |
| `UnaryExpr` | `-x`, `not a`, `#t` |
| `TableExpr` | `{1, 2, key=val}` |
| `CallExpr` | `f(1, 2)`, `obj:m()` |
| `IndexExpr` | `t[key]` |
| `MemberExpr` | `t.member` |
| `FunctionExpr` | `function(x) return x end` |
| `ParenExpr` | `(expr)` |

### 语句节点（13 种）

| 节点 | Lua 示例 |
|------|---------|
| `EmptyStmt` | `;` |
| `AssignStmt` | `a, b = 1, 2` |
| `LocalStmt` | `local x = 1` |
| `CallStmt` | `print("hello")` |
| `IfStmt` | `if x then ... end` |
| `WhileStmt` | `while x do ... end` |
| `RepeatStmt` | `repeat ... until x` |
| `ForNumStmt` | `for i=1,10 do ... end` |
| `ForInStmt` | `for k,v in pairs(t) do ... end` |
| `FunctionStmt` | `function f() ... end` |
| `ReturnStmt` | `return 1, 2` |
| `BreakStmt` | `break` |
| `DoStmt` | `do ... end` |

## 5. 递归下降解析

每个语法规则对应一个解析函数：

```
parse()              → parseStatementList() → Vec<StmtPtr>
  ↓
parseStatement()     → 根据 lookahead Token 分发：
  ├── if     → parseIfStmt()
  ├── while  → parseWhileStmt()
  ├── for    → parseForStmt()
  ├── function → parseFunctionStmt()
  ├── local  → parseLocalStmt()
  ├── return → parseReturnStmt()
  ├── break  → parseBreakStmt()
  ├── do     → parseDoStmt()
  ├── repeat → parseRepeatStmt()
  └── 否则   → parseExprStmt()
```

## 6. 运算符优先级解析

```
Precedence  Operator              Associativity
─────────────────────────────────────────────
  1         or                    left
  2         and                   left
  3         < > <= >= ~= ==       left
  4         ..                    right
  5         + -                   left
  6         * / %                 left
  7         not - #               unary (right)
  8         ^                     right
```

### 显式优先级调用链

```
parseExpression
  → parseOrExpr
  → parseAndExpr
  → parseRelationalExpr
  → parseConcatExpr
  → parseAdditiveExpr
  → parseMultiplicativeExpr
  → parseUnaryExpr
  → parsePowerExpr
  → parsePrimaryExpr
  → parsePostfixExpr
```

每个优先级层由独立递归下降函数表达；左结合层使用循环，`..` 和 `^` 使用右递归。

## 7. 错误恢复模式

| 模式 | 行为 | 用途 |
|------|------|------|
| `FailFast` | 遇到第一个错误就停止 | 生产环境 |
| `StatementBoundary` | 尝试跳过错误语句继续解析 | REPL/IDE |

## 8. 常见 Bug

| 问题 | 原因 |
|------|------|
| 运算符优先级错误 | 优先级表定义不正确 |
| `function f` vs `local function f` | 绑定规则不同 |
| 表构造器嵌套解析失败 | 递归调用栈问题 |
| 方法调用 `obj:method()` | self 参数自动插入逻辑 |
