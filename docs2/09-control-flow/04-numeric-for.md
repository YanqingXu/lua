# Numeric For Loop — 数值循环

## 1. FORPREP / FORLOOP

```
for var = init, limit, step do
    body
end

字节码:
  编译 init, limit, step 表达式 → 放入寄存器
  R(var) = init
  R(var+1) = limit
  R(var+2) = step
  
  FORPREP R(var) → pc += sBx (初始化: R(var) -= R(var+2))
  (loop_start:)  ← FORLOOP 跳回这里
  ... body ...
  FORLOOP R(var) → 如果继续: R(var) += R(var+2); 检查 R(var) <= R(var+1); pc += sBx
```

## 2. FORPREP 的作用

```
FORPREP 做初始减法:
  R(var) -= R(var+2)

原因:
  进入循环后第一次执行 FORLOOP 会做 R(var) += step
  为了第一次迭代能得到 init 的值 → 先减去 step
```

## 3. 循环变量不重新求值

```lua
-- init, limit, step 只求值一次
local function getStep() print("evaluated"); return 1 end
for i = 1, 10, getStep() do
    print(i)
end
-- "evaluated" 只打印一次 (不是 10 次)
```

## 4. 循环变量的作用域

```lua
-- 循环变量是每次迭代新绑定的 (但 Lua 5.1 中行为特殊)
-- 闭包捕获时需要注意:
local funcs = {}
for i = 1, 3 do
    funcs[i] = function() return i end
end
-- Lua 5.1: 所有闭包都返回 4! (共享同一个 i)
-- Lua 5.2+: 每个闭包返回不同的值 (每次迭代新 i)
```

## 5. 常见 Bug

| 问题 | 原因 |
|------|------|
| FORPREP/FORLOOP 配合错误 | sBx 偏移计算错误 |
| step 为负时的终止条件 | 需要检查 `>=` 而不是 `<=` |
| 循环变量被闭包捕获 | Lua 5.1 共享行为 vs 5.2 独立行为 |
