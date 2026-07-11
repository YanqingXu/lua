# Lexer State Machine — 词法状态机

## 1. 这个模块解决什么问题？

Lexer 如何用状态机模式识别各种词法单元。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/compiler/lexer/lexer.cpp` | 状态机实现 |
| `src/compiler/lexer/lexer_cursor.cpp` | 字符级游标 |

## 3. scanToken() 主状态机

```
scanToken():
  skipWhitespace()         → 跳过空白和注释
  ↓
  beginToken()             → 记录 Token 起始位置
  ↓
  switch(currentChar):
    'a'-'z','A'-'Z','_' → identifier()
    '0'-'9'             → decimalNumber()
    '0x', '0X'          → hexadecimalNumber()
    '"'                 → shortString('"')
    '\''                → shortString('\'')
    '['                 → tryLongString() 或 '['
    '{' → LBRACE
    '}' → RBRACE
    '(' → LPAREN
    ')' → RPAREN
    '-' → 检查是否 '--' (注释) 否则 MINUS
    '+' → PLUS
    '*' → STAR
    '/' → SLASH
    '%' → PERCENT
    '^' → CARET
    '#' → HASH
    '=' → 检查是否 '==' 否则 EQUAL
    '<' → 检查是否 '<=' 否则 LT
    '>' → 检查是否 '>=' 否则 GT
    '~' → 检查是否 '~=' 否则 ERROR
    '.' → 检查是否 '..' 或 '...' 或 '.' 或小数
    ':' → 检查是否 '::' 否则 COLON
    ';' → SEMICOLON
    ',' → COMMA
    EOF → EOF
    其他 → ERROR
```

## 4. skipWhitespace() 状态

```
skipWhitespace():
  while (true):
    switch(currentChar):
      ' ', '\t', '\v', '\f' → advance()
      '\r', '\n'             → consumeNewlineSequence()
      '-'                    → 检查 '--' → skipComment()
      其他                   → return
```

## 5. number() 状态机

```
decimalNumber():
  consumeDecimalDigits()      → 整数部分
  if '.' and isDigit(peekNext()) → 小数部分
  if 'e' or 'E'              → consumeDecimalDigits() (指数部分)
  (如果遇到非数字字符 → consumeMalformedNumberSuffix())

hexadecimalNumber():
  consumeHexDigits()
  if '.' and isHexDigit(peekNext()) → 小数部分
  if 'p' or 'P'              → consumeDecimalDigits() (指数部分)
```

## 6. string() 状态机

```
shortString(quote):
  while (true):
    switch(currentChar):
      '\\'     → appendShortStringEscape()   (转义)
      quote    → advance(); return makeToken(STRING)
      '\n'     → error (字符串不能跨行)
      EOF      → error (未闭合字符串)
      default  → appendLongStringChar()      (普通字符)

longString(level):
  找到关闭分隔符 ']' + '='*level + ']'
  按 Lua 5.1 规则规范化换行
```

## 7. 游标状态保存/恢复

```
saveState() → LexerState {
  lexemeLength,    // 当前已累积的词素长度
  input,           // 输入游标位置
  tokenStartLine,  // Token 起始行
  tokenStartColumn // Token 起始列
}

restoreState(state) → 恢复到之前保存的状态

用途：长字符串检测失败时回退
  - 看到 '[' 尝试解析长字符串
  - 如果不是有效的长字符串分隔符 → 恢复状态 → 返回 '[' Token
```
