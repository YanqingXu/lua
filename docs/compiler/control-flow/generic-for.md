# Generic For Loop — 泛型循环

## 1. TFORLOOP

```
for var1, var2, ... in iterator1, iterator2, ... do
    body
end

等价于:
do
    local f, s, var = iterator1, iterator2, nil
    while true do
        local var1, var2 = f(s, var)
        var = var1
        if var1 == nil then break end
        ... body ...
    end
end
```

## 2. TFORLOOP 指令

```
TFORLOOP A C

A: 迭代器寄存器组的起始位置
   R(A):   iterator function (f)
   R(A+1): state (s)
   R(A+2): current var
   R(A+3)+: 循环变量

C: 期望的循环变量数量

执行:
  1. 调用 f(s, var)
  2. 如果第一个返回值是 nil → 退出循环 (pc++)
  3. 否则 → 将返回值放入 R(A+3), R(A+4), ...
  4. R(A+2) = R(A+3) (更新 current var)
```

## 3. 常用迭代器

```lua
-- pairs
for k, v in pairs(t) do ... end  -- f = next, s = t, var = nil

-- ipairs
for i, v in ipairs(t) do ... end

-- 自定义迭代器
function myIterator(state)
    -- 返回下一个值，或 nil 表示结束
end
for val in myIterator, initialState do ... end
```

## 4. TFORLOOP 支持 C/Lua 函数

```
VM 侧:
  TFLOOP 的迭代器可以是:
    - Lua 函数 (走 VM::call)
    - C 函数 (直接调用)
```
