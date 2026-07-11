# Scope Resolution — 作用域解析

## 1. 这个模块解决什么问题？

Lua 解释器里局部变量和 upvalue 很容易出 bug，这个文档专门说明作用域机制。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/compiler/codegen/scope_manager.cpp` | 作用域管理 |
| `src/compiler/codegen/name_binder.cpp` | 名称绑定 |
| `src/compiler/codegen/codegen_binding.cpp` | 符号绑定到寄存器 |

## 3. Local 变量如何登记？

```cpp
// 解析 local 声明时
scopeManager.enterBlock();          // 进入新作用域
for (each var in local decl) {
    i32 reg = allocReg();           // 分配寄存器
    scopeManager.addLocal(name, reg); // 登记
}
// ... 编译初始化表达式
// 编译块体
scopeManager.exitBlock();           // 退出作用域 → 释放寄存器
```

## 4. 块作用域如何进入和退出？

```
do
    -- 进入新作用域 (enterBlock)
    local x = 1
    -- ...
end  -- 退出作用域 (exitBlock)
     -- x 的寄存器被释放
     -- 如果有 upvalue 指向 x，需要 close

类似地：
  while ... do ... end   ← 每次迭代进入新作用域
  for ... do ... end     ← 每次迭代进入新作用域
  repeat ... until ...   ← body 作用域
  if ... then ... end    ← 每个分支是独立作用域
  function f() ... end   ← 函数体是独立作用域
```

## 5. 内层函数如何捕获外层变量？

```lua
local x = 1
local function f()
    return x  -- x 被捕获为 upvalue
end
```

```
编译时:
  1. 解析 NameExpr("x")
  2. 在当前作用域查找 "x"
  3. 没找到 → 在上一层作用域查找
  4. 找到 → 标记为 upvalue
  5. 生成 UpvalueDesc{ 栈层级: 1, 变量索引: 0 }
  6. CodeGen 记录: 函数 f 需要 upvalue[0] = 外层 x

运行时:
  1. CLOSURE 指令
  2. 根据 UpvalueDesc 查找或创建 Upvalue
  3. 绑定到 Closure
```

## 6. `local function f()` 的特殊处理

```lua
local function f(n)
    if n > 0 then return f(n - 1) end  -- f 可以自递归调用
    return 0
end
```

`local function f()` 在声明 body 之前就将 f 登记到当前作用域，使 f 可以自递归。

## 7. break / return 时如何关闭 upvalue？

```
当 break 或 return 离开作用域时，需要关闭所有在该作用域内创建的 upvalue：

function outer()
    local x = 1
    if true then
        return function() return x end  -- 需要 close x 的 upvalue
    end
end

OP_CLOSE 指令: close all upvalues in stack up to (>=) R(A)
```

## 8. 常见 Bug

| 问题 | 原因 |
|------|------|
| 闭包捕获的变量值不对 | upvalue 绑定到错误的栈位置 |
| `break` 后 upvalue 未关闭 | 漏掉 OP_CLOSE |
| `local function` 递归失败 | 声明顺序：先登记名，后编译体 |
| for 循环变量在闭包中共享 | 每次迭代应创建新的 upvalue |
