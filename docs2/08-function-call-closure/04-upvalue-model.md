# Upvalue Model — Upvalue 模型

## 1. 什么是 Upvalue？

Upvalue = 被闭包捕获的外部局部变量。

```lua
local function make_counter()
    local x = 0       -- x 会被内部函数捕获
    return function()
        x = x + 1     -- x 是 upvalue
        return x
    end
end

local counter = make_counter()
print(counter())  -- 1
print(counter())  -- 2
-- x 在 make_counter 返回后仍然存活!
```

## 2. 捕获的是值还是变量？

**变量（引用）**，不是值。

```lua
local function make_pair()
    local x = 1
    return function() x = x + 1; return x end,
           function() x = x * 2; return x end
end

local inc, double = make_pair()
print(inc())     -- 2
print(double())  -- 4  (共享同一个 x!)
print(inc())     -- 5
```

## 3. Open Upvalue

```
Open Upvalue: 外部变量还在栈上

状态:
  v_ → 栈上的变量位置 (直接指针)
  stackIndex: 变量在栈中的索引
  isOpen(): true

示例:
  local x = 1
  local f = function() return x end
  -- x 还在栈上 → f 的 upvalue 是 Open 状态
```

## 4. Closed Upvalue

```
Closed Upvalue: 外部变量已从栈上移除

状态:
  v_ → &closedValue_ (指向自己的存储)
  closedValue_: 变量的复制品
  isClosed(): true

何时关闭:
  - break 跳出作用域
  - return 退出函数
  - 最后一次引用被移除
```

## 5. 对象关系图

```
make_counter() 调用期间:

Stack:
  [x = 0]  ← make_counter 的局部变量
  [f = Closure] ← 内部函数
  
Upvalue:
  v_ ──→ Stack[x]  (Open 状态)

make_counter() 返回后:

Stack (已清除):
  [counter = Closure]

Upvalue (已关闭):
  v_ ──→ closedValue_ = 0  (Closed 状态)

后续 counter() 调用:
  x = x + 1 → 修改 closedValue_
```

## 6. 多个闭包共享 Upvalue

```lua
local function make_shared()
    local x = 0
    return function() x = x + 1; return x end,
           function() x = x - 1; return x end
end

-- 两个闭包的 upvalue[0] 指向同一个 Upvalue
-- findOrCreateUpvalue 保证: 同一个栈变量只创建一次 Upvalue
```
