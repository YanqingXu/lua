# Tokenize — 词法分析

## 1. 这个模块解决什么问题？

将源代码文本转换为 Token（词法单元）流。

## 2. 在整体执行链路中的位置

```
Load Source → Tokenize → Parse → Compile → VM Execute
                 ↑
            (第二阶段)
```

## 3. 核心文件

| 文件 | 作用 |
|------|------|
| `src/compiler/lexer/lexer.hpp/cpp` | 词法分析器主类 |
| `src/compiler/lexer/lexer_cursor.hpp/cpp` | 字符游标（逐字符读取、位置跟踪） |
| `src/compiler/parser/token.hpp` | Token 类型和值定义 |

## 4. Token 类型

```
Token {
    TokenType type;     // Token 类别
    StrView lexeme;     // 词素（源文本）
    i32 line;           // 行号
    i32 column;         // 列号
    TokenValue value;   // 值（数字/字符串/布尔等）
}
```

### TokenType 枚举

| 类别 | 示例 |
|------|------|
| **关键字** | `local`, `function`, `if`, `then`, `else`, `return`, `nil`, `true`, `false`, `and`, `or`, `not`, `for`, `while`, `do`, `end`, `repeat`, `until`, `break`, `in`, `elseif` |
| **字面量** | `42`, `3.14`, `"hello"`, `'world'`, `[[long string]]` |
| **运算符** | `+`, `-`, `*`, `/`, `%`, `^`, `#`, `==`, `~=`, `<=`, `>=`, `<`, `>`, `=`, `..`, `...` |
| **分隔符** | `(`, `)`, `{`, `}`, `[`, `]`, `;`, `:`, `,`, `.` |
| **标识符** | `myVar`, `_G`, `table_sort` |
| **EOF** | 文件结束 |

## 5. Lexer 核心算法

```
nextToken():
  skipWhitespace()        → 跳过空白和注释
  ↓
  switch(currentChar):
    'a'-'z', 'A'-'Z', '_' → identifier() → 关键字或标识符
    '0'-'9'               → decimalNumber() 或 hexadecimalNumber()
    '"', '\''              → shortString(quote)
    '['                    → tryLongString() 或 '['
    '{', '}', ...          → 直接返回对应 Token
    '-', '+', '*', ...     → handleOperator()
    EOF                    → EOF Token
```

## 6. 关键字识别

使用哈希表 O(1) 识别：

```cpp
static const HashMap<StrView, TokenType> keywords = {
    {"and", AND}, {"break", BREAK}, {"do", DO},
    {"else", ELSE}, {"elseif", ELSEIF}, {"end", END},
    {"false", FALSE}, {"for", FOR}, {"function", FUNCTION},
    {"if", IF}, {"in", IN}, {"local", LOCAL},
    {"nil", NIL}, {"not", NOT}, {"or", OR},
    {"repeat", REPEAT}, {"return", RETURN}, {"then", THEN},
    {"true", TRUE}, {"until", UNTIL}, {"while", WHILE}
};
```

## 7. LL(1) 前瞻

```
peekToken(): 缓存下一个 Token，不消费
nextToken(): 返回 peekToken 缓存（如果有）或调用 scanToken()

用途：Parser 需要前瞻一个 Token 来决定走哪个语法分支
```

## 8. 字符串处理

| 类型 | 语法 | 处理函数 |
|------|------|---------|
| 短字符串（双引号） | `"hello\n"` | `shortString('"')` |
| 短字符串（单引号） | `'world'` | `shortString('\'')` |
| 长字符串 | `[[multi\nline]]` | `longString(0)` |
| 嵌套长字符串 | `[=[level 1]=]` | `longString(1)` |

## 9. 注释处理

```lua
-- 单行注释                    → skipLineComment()
--[[ 多行注释 ]]               → skipLongComment(0)
--[=[ 嵌套级别长注释 ]=]       → skipLongComment(1)
```

## 10. 数字字面量

```
整数:      42, 0, -1
浮点:      3.14, .5, 1., 1e10, 1.5e-3
十六进制:  0xFF, 0x1A, 0x1.2p3
```

## 11. 常见 Bug

| 问题 | 原因 |
|------|------|
| 关键字被识别为标识符 | 关键字表缺失 |
| 长字符串跨行报错 | 分隔符匹配逻辑错误 |
| 数字解析错误 | `1.` / `.5` / `1e2` 边界处理 |
| 转义字符错误 | `\n`, `\t`, `\\`, `\"` 处理 |
