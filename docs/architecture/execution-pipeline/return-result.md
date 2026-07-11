# Return Result — 返回值

## 1. 这个模块解决什么问题？

函数执行完毕后，如何将返回值传递给调用者。

## 2. 在整体执行链路中的位置

```
Load Source → Tokenize → Parse → Compile → Create Function → VM Execute → Return Result
                                                                                   ↑
                                                                              (第七阶段)
```

## 3. 核心文件

| 文件 | 作用 |
|------|------|
| `src/vm/vm.cpp` | RETURN 指令实现 |
| `src/vm/vm_handlers/vm_handlers_call.cpp` | CALL/RETURN handler |
| `src/vm/vm_call.cpp` | 函数调用辅助 |
| `src/vm/state/lua_state.cpp` | LuaState 栈操作 |
| `src/vm/state/call_info.hpp` | CallInfo 定义 |

## 4. RETURN 指令语义

```
RETURN R(A), ..., R(A+B-2)

含义: 返回 B-1 个值，从 R(A) 开始

特殊情况:
  B == 0: 返回 R(A) 到栈顶的所有值
  B == 1: 不返回值（返回 0 个值）
  B >= 2: 返回 B-1 个值

返回值放置:
  调用者的栈上，从 func 位置开始替换
```

## 5. 多返回值规则

```lua
-- 函数定义
function f() return 1, 2, 3 end

-- 单返回值上下文 → 只取第一个
local x = f()          -- x = 1

-- 多返回值上下文 → 取全部
local a, b, c = f()    -- a=1, b=2, c=3

-- 作为函数实参 → 展开
print(f())             -- print(1, 2, 3)

-- 在表构造器中 → 展开（非末尾位置展开为 1）
local t = {f()}        -- {1, 2, 3}
local t = {f(), 4}     -- {1, 4}  ← 非末尾只取第一个
```

## 6. 栈上的返回值布局

```
调用前（调用者栈）:
  [func] [arg1] [arg2] ... ← 函数和参数

调用中（被调用者栈）:
  [func] [arg1] [arg2] [R0] [R1] [R2] ...

返回后（调用者栈）:
  [ret1] [ret2] ... ← 返回值覆盖了 func 位置
```

## 7. CALL 指令的 nresults 控制

```
CALL A B C

  A: 函数所在的寄存器
  B: 参数个数（不含函数本身）- 1
  C: 期望的返回值数量 - 1

  C == 0: 期望 1 个返回值
  C == LUA_MULTRET: 接受任意数量的返回值
  C == n: 期望 n+1 个返回值（多余的丢弃，不足的补 nil）
```

## 8. 尾调用优化（TAILCALL）

```
return f(args)

TAILCALL: 复用当前 CallInfo，不创建新帧
  - 将 args 移到当前帧的 func 位置
  - 直接跳转到 f 的字节码
  - 不增加调用深度
```

## 9. 常见 Bug

| 问题 | 原因 |
|------|------|
| 多返回值丢失 | CALL 的 nresults 参数错误 |
| 多返回值在非末尾展开 | 表构造器/表达式中的展开规则 |
| 尾调用递归爆栈 | TAILCALL 未正确复用栈帧 |
| 返回值数量不对 | RETURN 的 B 参数计算错误 |
| nil 填充不正确 | nresults > 实际返回值时的补 nil 逻辑 |
