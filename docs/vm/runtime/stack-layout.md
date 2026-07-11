# Stack Layout — 栈布局

## 1. 这个模块解决什么问题？

详细说明函数调用前后栈的变化，以及多返回值如何传递。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/vm/state/stack.cpp` | Stack 实现 |
| `src/vm/vm_call.cpp` | 调用时栈操作 |

## 3. 调用前的栈

```
调用者栈 (调用 f(a, b) 之前):

┌─────────────┐
│ ...         │  ← 调用者的局部变量等
├─────────────┤
│ f           │  ← func (函数对象)
│ a 的值      │  ← arg1
│ b 的值      │  ← arg2
└─────────────┘ ← 栈顶
```

## 4. 调用后的栈

```
被调用者栈 (进入 f 之后):

┌─────────────┐ ← caller.top (调用者的栈顶 = f 调用前的位置)
├─────────────┤ ← ci.top (= ci.base + maxStackSize)
│ R(3)        │  ← 局部变量 / 临时值
│ R(2)        │  ← 局部变量 / 临时值
│ R(1)        │  ← 局部变量 / 临时值
├─────────────┤ ← ci.base + numParams
│ b 的值      │  ← R(1) = 参数2
│ a 的值      │  ← R(0) = 参数1
├─────────────┤ ← ci.base
│ f           │  ← ci.func (函数对象)
├─────────────┤
│ ... (caller)│  ← 调用者的数据被保护
└─────────────┘
```

## 5. 返回时的栈

```
从 f 返回后 (返回值 1 个):

┌─────────────┐
│ ...         │  ← 调用者的局部变量
├─────────────┤
│ 返回值      │  ← 覆盖了原来的 f 位置
│ a 的值      │  ← arg1 (可能被覆盖)
│ b 的值      │  ← arg2 (可能被覆盖)
└─────────────┘

返回值放置规则:
  ret[0] → stack[ci.func]        (覆盖函数对象)
  ret[1] → stack[ci.func + 1]    (覆盖第一个参数)
  ...
  ret[n] → stack[ci.func + n]
```

## 6. 多返回值示例

```lua
function f() return 1, 2, 3 end
local a, b = f()
```

```
调用 f 前:
  [f] [nil] [nil]  ← 两个 nil 是 local a, b 的位置

调用 f (进入):
  f 的栈: [f] [](无参数) [R0] [R1] [R2]
  RETURN R0 4  → 返回 3 个值: 1, 2, 3

返回后:
  [1] [2] [3]    ← 返回值覆盖了 f 和预留位置
  a = 1, b = 2   (只有 2 个目标，多余的 3 被丢弃)
```

## 7. nresults 控制

```
CALL A B C 中 C 的值:

C == 0:  期望 1 个返回值
C == n:  期望 n+1 个返回值 (多余的丢弃，不足的补 nil)
C == LUA_MULTRET (-1): 接受所有返回值

示例:
  f()           → CALL func 0 0 (期望 1 个返回值，多余丢弃)
  local a = f() → CALL func 0 0 (期望 1 个返回值)
  a, b = f()    → CALL func 0 1 (期望 2 个返回值)
  return f()    → CALL func 0 MULTRET (接受所有)
```

## 8. 栈扩展

```
当栈空间不足时:
  stack.checkSpace(needed);
  → 如果 capacity < size + needed:
      stack.resize(max(capacity * 2, size + needed));
      // 注意: base 指针可能失效，需要 refreshBase()

因此每次可能触发栈扩展的操作后，都需要:
  base = refreshBase(L);
```
