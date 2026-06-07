# Type Conversion — 类型转换

## 1. 隐式转换

### string → number
```lua
"42" + 1    -- 43 (算术运算中自动转换)
"3.14" * 2  -- 6.28
"-10" - 5   -- -15
```

VM 在算术指令 (ADD/SUB/MUL/DIV/MOD/POW) 中检查 string 类型并尝试转换。

### number → string
```lua
10 .. "20"  -- "1020" (拼接运算中自动转换)
42 .. ""    -- "42"
```

VM 的 CONCAT 指令将 number 转为字符串表示。

## 2. 显式转换

### tostring(v)
```cpp
// 将任何值转换为字符串表示
Value result = tostring(value);
// nil→"nil", true→"true", 42→"42", "hello"→"hello"
```

### tonumber(e [, base])
```cpp
// 将字符串转为数字
Value result = tonumber("42");     // 42.0
Value result = tonumber("FF", 16); // 255.0
Value result = tonumber("abc");    // nil
```

## 3. VM 内的类型检查

```cpp
// 算术指令中的转换:
Value& lhs = RK(B);
Value& rhs = RK(C);

if (lhs.isString()) {
    f64 n = tonumber(lhs.asString()->c_str());
    if (success) lhsNum = n;
    else error("attempt to perform arithmetic on a string value");
}

if (rhs.isString()) {
    // 同样尝试转换
}
```

## 4. 转换失败的处理

```
"abc" + 1 → 运行时错误 "attempt to perform arithmetic on a string value"

nil + 1   → 运行时错误 "attempt to perform arithmetic on a nil value"
```

## 5. 常见问题

| 操作 | Lua 5.1 行为 | 注意事项 |
|------|-------------|---------|
| `" 42" + 1` | 43 (容空格) | tonumber 需要 trim |
| `"0xFF" + 1` | 256 (十六进制) | tonumber 支持 base |
| `"42xyz" + 1` | error | 不是合法数字 |
| `tostring(nil)` | "nil" | 不是空字符串 |
