# While Loop — While 循环

## 1. 字节码模式

```
while cond do
    body
end

字节码:
  (loop_start:)  ← 跳回这里
  EQ/LT/LE/TEST  cond  false → JMP to end
  ... body ...
  JMP → loop_start
  (end:)
```

## 2. 示例

```lua
local i = 1
while i <= 10 do
    print(i)
    i = i + 1
end
```

## 3. While 中的 break

```
while cond do
    ...
    break
    ...
end

break 编译为:
  JMP → end  (跳出循环)
  + 可能需要在跳出前关闭 upvalue (OP_CLOSE)
```

## 4. 常见 Bug

| 问题 | 原因 |
|------|------|
| 无限循环 | 条件永远为真或跳转回填目标错误 |
| break 后 upvalue 未关闭 | 漏掉 OP_CLOSE |
