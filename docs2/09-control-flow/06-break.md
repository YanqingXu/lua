# Break — 跳出循环

## 1. Break 的字节码

```
break 编译为:
  1. 对每个可能捕获了当前作用域变量的闭包 → OP_CLOSE
  2. JMP → 循环体之后 (end 的位置)
```

## 2. Break 的作用域关闭

```lua
while true do
    local x = 1
    local f = function() return x end
    break  -- 需要关闭 x 的 upvalue
end
```

```
字节码:
  CLOSE  R(x)       ; 关闭 x 的 upvalue
  JMP    → end      ; 跳出循环
```

## 3. Break 的限制

```lua
-- break 只能出现在循环体内 (while, repeat, for)
-- 不能出现在 if 分支中 (除非 if 在循环内)

-- ✓ 合法
while true do
    if cond then break end
end

-- ✗ 非法
if cond then break end  -- 语法错误
```
