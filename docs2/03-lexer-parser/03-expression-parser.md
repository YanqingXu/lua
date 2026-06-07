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
parseSubExpression(minPrecedence)
  ↓
parsePrimaryExpression()       → 基本表达式
  ↓
parseSuffixExpression()        → 后缀（调用/索引）
```

## 5. Pratt Parser 伪代码

```cpp
ExprPtr parseSubExpression(i32 minPrecedence) {
    // 解析前缀表达式
    ExprPtr left = parsePrimaryExpression();
    left = parseSuffixExpression(left);  // 方法调用、索引
    
    // 只要当前运算符优先级 >= minPrecedence，继续解析
    while (getPrecedence(currentToken) >= minPrecedence) {
        Token op = advance();
        i32 nextPrec = getPrecedence(op) + (isRightAssoc(op) ? 0 : 1);
        ExprPtr right = parseSubExpression(nextPrec);
        left = makeBinaryExpr(op, left, right);
    }
    
    return left;
}
```

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

## 8. 关系链表达式

Lua 5.1 支持关系链：`a < b == c > d`
等价于 `(a < b) and (b == c) and (c > d)`

## 9. 常见 Bug

| 问题 | 原因 |
|------|------|
| `2^-2` 解析错误 | 一元 `-` 优先级 vs `^` 优先级 |
| `a and b or c` 优先级错 | and/or 优先级混淆 |
| `f()()` 解析失败 | 函数调用返回值的后缀表达式 |
