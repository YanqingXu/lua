# Return Values — 返回值处理

## 1. 这个模块解决什么问题？

RETURN 指令如何工作，多返回值如何传递和截断。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/vm/vm_handlers/vm_handlers_call.cpp` | RETURN handler |
| `src/vm/vm_call.cpp` | 返回值调整 |

## 3. RETURN 指令

```
RETURN A B

返回 B-1 个值，从 R(A) 开始。

特殊情况:
  B == 0: 返回从 R(A) 到栈顶的所有值
  B == 1: 返回 0 个值
  B >= 2: 返回 B-1 个值
```

## 4. 返回值放置

```
调用者的栈 (返回前):
  [func] [arg1] [arg2] ... [caller locals]

返回后:
  [ret1] [ret2] ... [retN] [nil pad] ... [caller locals]
  ↑                      ↑
  ci.func 开始            多余的补 nil
```

## 5. 返回值数量控制

```cpp
i32 actualResults = (B == 0) ? (top - A) : (B - 1);
i32 expectedResults = ci.nresults;

if (expectedResults == LUA_MULTRET) {
    // 接受所有返回值
    nresults = actualResults;
} else {
    // 截断或补 nil
    nresults = expectedResults;
}

// 放置返回值
for (i32 i = 0; i < nresults; i++) {
    stack[ci.func + i] = (i < actualResults) ? R(A + i) : Value(); // nil
}
```

## 6. 多返回值截断示例

```lua
function f() return 1, 2, 3 end

-- 单值上下文: 只取第一个
local x = f()      -- x = 1 (3个返回值截断为1)

-- 多值上下文: 全部取
local a, b, c = f() -- a=1, b=2, c=3

-- 表达式: 只取第一个
local y = f() + 1  -- y = 2 (f() 在表达式中被截断为 1)

-- 函数实参: 展开
print(f())         -- print(1, 2, 3)

-- 表构造器末尾: 展开
local t = {10, f()}  -- {10, 1, 2, 3}

-- 表构造器非末尾: 截断
local t = {f(), 10}  -- {1, 10}

-- return: 全部传递
return f()         -- return 1, 2, 3
```

## 7. TAILCALL 的返回值

```
TAILCALL 的返回值直接传递给最外层调用者，
不需要经过中间函数截断。

function wrapper()
    return inner()  -- TAILCALL: 返回值直接传递
end

wrapper()  -- 得到 inner() 的所有返回值
```
