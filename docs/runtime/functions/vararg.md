# Vararg — 可变参数

## 1. VARARG 指令

```
VARARG A B

将可变参数 (...) 复制到寄存器:
  R(A), R(A+1), ..., R(A+B-1) = vararg

B == 0: 复制所有 vararg
B >= 1: 复制 B 个 (不足补 nil)
```

## 2. vararg 存储

```
在 CallInfo 中:
  可变参数存储在栈上，紧接固定参数之后

函数声明:
  function f(a, b, ...) end
  
栈布局:
  [func] [a] [b] [vararg1] [vararg2] [vararg3] ...
                  ↑
              固定参数后的位置
```

## 3. Lua 5.1 的旧式 arg

```lua
-- Lua 5.1 旧行为 (兼容)
function f(...)
    print(arg[1])  -- 第一个可变参数
    print(arg.n)   -- 可变参数数量
end

-- 变长函数但没有声明 ... → 自动提供 arg
function f()
    print(arg[1])  -- 有效 (旧式行为)
    print(arg.n)
end
```

## 4. 新式 ... (Lua 5.1+)

```lua
-- 显式声明 ...
function f(a, b, ...)
    local args = {...}  -- 打包成 table
    print(select("#", ...))  -- 数量
    print(select(1, ...))    -- 第一个
end
```

## 5. 常见 Bug

| 问题 | 原因 |
|------|------|
| vararg 在嵌套函数中不可见 | 每个函数有独立的 vararg |
| arg 全局变量被覆盖 | 多个变长函数同时使用 arg |
| vararg 数量统计错误 | 固定参数后的偏移计算错误 |
