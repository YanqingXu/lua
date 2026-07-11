# Compile Errors — 编译错误

## 1. ParseError

```cpp
class ParseError : public std::runtime_error {
    i32 line_;
    i32 column_;
    Str message_;
};
```

## 2. 常见编译错误

| 错误 | 示例 | 原因 |
|------|------|------|
| `'end' expected` | `if x then body` | 缺 `end` |
| `unexpected symbol` | `local = 1` | 语法不匹配 |
| `'=' expected` | `local x` (缺初始值?) | 解析期望不同 |
| `malformed number` | `4.5.` | 数字格式错误 |
| `unfinished string` | `"hello` | 未闭合字符串 |

## 3. 错误定位

ParseError 包含行号和列号，可直接定位到源码位置。
