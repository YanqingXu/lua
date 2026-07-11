# Expression Parser — 表达式解析

## 1. 这个模块解决什么问题？

如何解析 Lua 的表达式（运算符、优先级、结合性）。

## 2. 支持的表达式

- `nil` / `true` / `false` — 字面量
- number — 数字
- string — 字符串
- `...` — 变长参数
- name — 变量引用
- table constructor — 表构造器
- function expression — 函数表达式
- unary: `-x`, `not x`, `#x` — 一元运算
- binary: `a + b`, `x and y` — 二元运算
- function call: `f(1, 2)`, `obj:method()` — 函数调用
- index: `t[key]`, `t.member` — 索引访问

## 3. 运算符优先级

| 优先级 | 运算符 | 结合性 |
|--------|--------|--------|
| 1 (低) | `or` | 左结合 |
| 2 | `and` | 左结合 |
| 3 | `<` `>` `<=` `>=` `~=` `==` | 左结合 |
| 4 | `..` | 右结合 |
| 5 | `+` `-` | 左结合 |
| 6 | `*` `/` `%` | 左结合 |
| 7 | `not` `-` `#` (一元) | — |
| 8 (高) | `^` | 右结合 |

## 4. 关键函数调用链

```
parseExpression()
  ↓
parseOrExpr() → parseAndExpr() → parseRelationalExpr()
  ↓
parseConcatExpr() → parseAdditiveExpr() → parseMultiplicativeExpr()
  ↓
parseUnaryExpr() → parsePowerExpr() → parsePrimaryExpr()
  ↓
parsePostfixExpr()             → 调用、索引和成员访问
```

## 5. 显式优先级层

```cpp
ExprPtr Parser::parseAdditiveExpr() {
    auto left = parseMultiplicativeExpr();
    while (check(TokenType::Plus) || check(TokenType::Minus)) {
        Token op = advance();
        auto right = parseMultiplicativeExpr();
        left = makeBinaryExpr(op, std::move(left), std::move(right));
    }
    return left;
}
```

左结合层使用循环，`..` 和 `^` 的右结合层递归调用自身。实现没有使用 Pratt parser 或统一的 precedence climbing。

## 6. 前缀表达式

```
parsePrimaryExpression():
  NIL       → NilExpr
  TRUE      → BoolExpr(true)
  FALSE     → BoolExpr(false)
  NUMBER    → NumberExpr(value)
  STRING    → StringExpr(value)
  DOTDOTDOT → VarargExpr
  NAME      → NameExpr(name)
  FUNCTION  → parseFunctionExpr()
  LBRACE    → parseTableConstructor()
  LPAREN    → parseExpression(); expect(RPAREN); ParenExpr
```

## 7. 后缀表达式

```
parseSuffixExpression(left):
  DOT       → MemberExpr(left, name)
  LBRACK    → IndexExpr(left, parseExpression())
  COLON     → 方法调用 (isMethodCall = true)
  LPAREN    → CallExpr(left, parseArgs())
  LBRACE    → CallExpr(left, [tableArg])
  STRING    → CallExpr(left, [stringArg])
```

## 8. 关系表达式

`parseRelationalExpr()` 在 additive/concat 结果之上消费一个关系运算符并构造 `BinaryExpr`。关系表达式不会被改写为数学意义上的链式比较；需要多段条件时应使用显式的 `and`。

## 9. 常见 Bug

| 问题 | 原因 |
|------|------|
| `2^-2` 解析错误 | 一元 `-` 优先级 vs `^` 优先级 |
| `a and b or c` 优先级错 | and/or 优先级混淆 |
| `f()()` 解析失败 | 函数调用返回值的后缀表达式 |
