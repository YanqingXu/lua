# Error Handling — 编译错误处理

## 1. 这个模块解决什么问题？

Lexer 和 Parser 遇到错误时如何处理和报告。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/common/lua_error.hpp` | 错误类型定义 |
| `src/compiler/lexer/lexer.cpp` | 词法错误 |
| `src/compiler/parser/parser.cpp` | 语法错误 |

## 3. 错误类型

```cpp
class ParseError : public std::runtime_error {
    i32 line_, column_;
    Str message_;
public:
    ParseError(const Str& msg, i32 line, i32 col);
    i32 getLine() const;
    i32 getColumn() const;
};
```

## 4. Lexer 错误

| 错误 | 触发条件 | 示例 |
|------|---------|------|
| 未闭合字符串 | 字符串到行末/文件末未找到闭合引号 | `"hello` |
| 非法字符 | 无法识别的字符 | `@` |
| 转义序列错误 | 无效的转义序列 | `"\x"` |
| 数字格式错误 | 畸形数字 | `4.5.` |
| 长字符串错误 | 分隔符不匹配 | `[=[text]]` (level 不一致) |
| 注释错误 | 未闭合的长注释 | `--[[` 无对应 `]]` |

## 5. Parser 错误

| 错误 | 触发条件 | 示例 |
|------|---------|------|
| 期望 Token | 期望的 Token 不出现 | `if x then` 缺 `end` |
| 语法不匹配 | Token 序列不符合任何语法规则 | `local = 1` |
| 作用域错误 | break 不在循环中 | 顶级 `break` |

## 6. 错误恢复策略

### FailFast（默认）
```cpp
// 遇到第一个错误立即停止解析
ParseError error = ...;
return std::unexpected(error);
```

### StatementBoundary
```cpp
// 尝试跳过错误语句继续解析
try {
    stmt = parseStatement();
} catch (...) {
    skipToNextStatement();  // 跳到下一个分号或关键字
    continue;
}
```

## 7. 错误信息格式

```
[string "source"]:3: unexpected symbol near 'end'
```

## 8. 常见 Bug

| 问题 | 原因 |
|------|------|
| 错误行号列号不准 | Token 位置跟踪有偏差 |
| 错误信息不清晰 | 期望 Token 的描述不够具体 |
| 错误恢复后状态混乱 | 跳过的 Token 导致后续解析出错 |
