# Tail Call — 尾调用优化

## 1. 什么是尾调用？

```
尾调用: return f(args)

特点:
  - f 的返回值直接作为当前函数的返回值
  - 不需要保存当前函数的上下文
  - 可以复用当前栈帧
```

## 2. TAILCALL 指令

```
TAILCALL A B C

语义: return R(A)(R(A+1), ..., R(A+B-1))

与 CALL 的区别:
  - 不创建新 CallInfo
  - 复用当前 CallInfo (减少栈深度)
  - 直接 goto reentry (不回到调用者)
```

## 3. 栈帧复用

```
正常调用:
  CALL f:  创建新帧 → 执行 f → RETURN → 恢复旧帧
  
尾调用:
  TAILCALL f: 将参数移到 func 位置 → 复用当前帧 → 直接执行 f
```

## 4. 尾调用示例

```lua
-- 尾递归: 不会栈溢出
function factorial(n, acc)
    acc = acc or 1
    if n <= 1 then return acc end
    return factorial(n - 1, acc * n)  -- TAILCALL
end

print(factorial(100000))  -- OK (栈深度始终为 1)

-- 非尾调用: 会栈溢出
function bad_factorial(n)
    if n <= 1 then return 1 end
    return n * bad_factorial(n - 1)  -- 不是尾调用 (需要 n * result)
end
```

## 5. TAILCALL 的条件

```
必须是:
  return f(args)

不能是:
  return f(args) + 1    -- 需要 f 的返回值做运算
  return 1, f(args)     -- 不是最后一个表达式
  return (f(args))      -- 括号包裹 → 多返回值截断为 1

TAILCALL 传递所有返回值 (类似 return f())
```

## 6. 尾调用计数

```
CallInfo::tailcalls 记录尾调用次数
用于调试信息的调用栈展示
```
