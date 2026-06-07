# Statement Emission — 语句字节码发射

## 1. 这个模块解决什么问题？

各种 Lua 语句如何被翻译为字节码指令序列。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/compiler/codegen/statement_emitter.cpp` | 语句 → 指令 |
| `src/compiler/codegen/codegen_stmt.cpp` | 高层语句编译 |

## 3. 各语句类型的发射策略

### 赋值语句
```lua
a, b = 1, 2
  → 先求值 RHS (所有值):
    LOADK R(T0) K(1)
    LOADK R(T1) K(2)
  → 再写入 LHS:
    SETGLOBAL K("a") R(T0)
    SETGLOBAL K("b") R(T1)
```

### 局部声明
```lua
local a, b = 1, 2
  → 分配寄存器: a=R(0), b=R(1)
  → LOADK R(0) K(1)
  → LOADK R(1) K(2)
```

### If 语句
```lua
if cond then
    body1
elseif cond2 then
    body2
else
    body3
end

→ EQ/TEST 检测 cond
  JMP (如果 cond 为 false 跳到 elseif)
  ... body1 ...
  JMP (跳到 end)
  ... elseif 检测 ...
  ... body2 ...
  JMP (跳到 end)
  ... body3 ...
  (end: 继续执行)
```

### While 语句
```lua
while cond do
    body
end

→ (loop_start:) EQ/TEST 检测 cond
  JMP (如果 false 跳到 end)
  ... body ...
  JMP (跳回 loop_start)
  (end:)
```

### Repeat 语句
```lua
repeat
    body
until cond

→ ... body ...
  EQ/TEST 检测 cond
  JMP (如果 false 跳回 body 开始)
```

### 数值 For
```lua
for i = 1, 10, 2 do
    body
end

→ LOADK R(i) K(1)      -- init
  LOADK R(limit) K(10) -- limit
  LOADK R(step) K(2)   -- step
  FORPREP R(i) → (loop_start)
  ... body ...
  FORLOOP R(i) → (loop_start)
```

### 泛型 For
```lua
for k, v in pairs(t) do
    body
end

→ 编译 iterator 表达式 (pairs(t))
  TFORLOOP: call iterator, check result
```

### Return 语句
```lua
return a, b
  → MOVE R(A) R(a)
  → MOVE R(A+1) R(b)
  → RETURN R(A) 3  (返回 2 个值)
```
