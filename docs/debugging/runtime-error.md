# Runtime Errors — 运行时错误

## 1. RuntimeError

```cpp
class RuntimeError : public std::exception {
    Str message_;  // 错误描述
    // 可选: 调用栈信息
};
```

## 2. 常见运行时错误

| 错误 | 示例 | VM 位置 |
|------|------|---------|
| 算术类型错误 | `"abc" + 1` | ADD handler |
| nil 索引 | `nil[1]` | GETTABLE handler |
| 栈溢出 | 过深递归 | executeProto |
| 除零 | `1 / 0` | DIV handler (取决于策略) |
| 调用非函数 | `42()` | CALL handler |
| nil table index | `t[nil]` | GETTABLE/SETTABLE |

## 3. 错误传播

```
VM 指令抛出 RuntimeError
  ↓
tryExecuteProto 捕获
  ↓
返回 std::unexpected(RuntimeError)
  ↓
上层 pcall → 返回 false, error_msg
或 propagate → 程序终止并打印错误
```

## 4. 错误对象

Lua 5.1 支持任意类型的错误对象:
```lua
error("message")        -- 字符串错误
error({code = 404})     -- table 错误
error(42)               -- 数字错误
```
