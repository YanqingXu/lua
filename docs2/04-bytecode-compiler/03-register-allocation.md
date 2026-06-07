# Register Allocation — 寄存器分配

## 1. 这个模块解决什么问题？

编译时如何为局部变量和临时值分配寄存器。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/compiler/codegen/scope_manager.cpp` | 作用域 + 寄存器管理 |
| `src/compiler/codegen/codegen_binding.cpp` | 符号绑定到寄存器 |

## 3. 寄存器模型

```
Lua 使用基于栈的寄存器：

Stack:
  R(0) — 可以在 base 偏移 0 处访问
  R(1) — base 偏移 1
  R(2) — base 偏移 2
  ...

每个 local 变量占用一个寄存器。
临时表达式结果也占用寄存器。
```

## 4. 分配策略

```cpp
// 简单栈式分配

allocReg():  分配下一个空闲寄存器
  return nextReg++;

freeReg(reg): 释放寄存器
  // 释放最上面的连续区域
  while (nextReg > 0 && isFree(nextReg - 1))
    nextReg--;
```

## 5. 局部变量分配示例

```lua
local a = 1        -- R(0) = 1
local b = 2        -- R(1) = 2
local c = a + b    -- R(2) = ADD R(0) R(1)
```

```
寄存器分配:
  R(0): a
  R(1): b
  R(2): c (临时的加法结果)
```

## 6. 作用域与寄存器释放

```lua
do
    local x = 1    -- 分配 R(0)
    -- x 存活
end                 -- 退出作用域，释放 R(0)
local y = 2         -- 可以复用 R(0) = 2
```

## 7. 临时寄存器回收

```
语句级临时寄存器会回收到活动 locals 边界：

local a = f() + g()
  — 调用 f() 结果在临时寄存器
  — 调用 g() 结果在临时寄存器
  — 加法结果在临时寄存器
  — 赋值给 a 后临时寄存器可回收
```

## 8. 函数调用时的寄存器保护

```
调用函数时，参数和函数本身占用连续寄存器：

print(add(2, 3))
  → R(0) = print     (函数)
  → R(1) = add       (内层函数)
  → R(2) = 2         (参数1)
  → R(3) = 3         (参数2)
  → CALL R(1) 2 1    (调用 add(2,3))
  → R(1) = 6         (add 返回值覆盖 add 位置)
  → CALL R(0) 1 1    (调用 print(6))
```

## 9. maxStackSize

```
Proto::maxStackSize 记录函数需要的最大寄存器数。
VM 按此值确保栈空间足够。

计算公式: max(allocated registers at any point) + 预留
```
