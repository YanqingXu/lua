# Token System — Token 系统

## 1. 这个模块解决什么问题？

Token 是词法分析和语法分析之间的桥梁，定义了源码的所有词法单元。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/compiler/parser/token.hpp` | Token 类型、值、枚举定义 |

## 3. Token 结构

```cpp
struct Token {
    TokenType type;       // Token 类型
    StrView lexeme;       // 词素（源文本片段）
    i32 line;             // 起始行号
    i32 column;           // 起始列号
    TokenValue value;     // 值（number/string/bool）
};
```

## 4. Token 类型

### 关键字（21个）
```
and, break, do, else, elseif, end, false, for,
function, if, in, local, nil, not, or, repeat,
return, then, true, until, while
```

### 字面量
```
NUMBER    — 数字 (42, 3.14, 0xFF, 1e10)
STRING    — 字符串 ("hello", 'world', [[multi]])
```

### 运算符（单字符）
```
PLUS (+), MINUS (-), STAR (*), SLASH (/), PERCENT (%),
CARET (^), HASH (#), EQUAL (=), 
LT (<), GT (>), LPAREN ((), RPAREN ()),
LBRACE ({), RBRACE (}), LBRACK ([), RBRACK (]),
SEMICOLON (;), COLON (:), COMMA (,), DOT (.)
```

### 运算符（多字符）
```
DOTDOT (..), DOTDOTDOT (...), 
EQEQ (==), NOTEQ (~=), 
LTEQ (<=), GTEQ (>=)
```

### 标识符
```
NAME — 变量名、函数名等 ([a-zA-Z_][a-zA-Z0-9_]*)
```

## 5. TokenValue

```cpp
using TokenValue = std::variant<
    std::monostate,   // 无值（关键字、运算符）
    f64,              // 数字值
    Str               // 字符串值
>;
```

## 6. 关键字识别（O(1)）

```cpp
static const HashMap<StrView, TokenType> keywords = {
    {"and", TokenType::AND},
    {"break", TokenType::BREAK},
    // ... 21 个关键字
};

// identifier() 中:
if (keywords.contains(lexeme)) {
    return makeToken(keywords[lexeme]);
}
return makeToken(TokenType::NAME);
```
