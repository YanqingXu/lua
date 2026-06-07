# __call / __len / __concat — 其他元方法

## 1. __call

把 table 当函数调用:

```lua
local t = setmetatable({}, {
    __call = function(self, a, b)
        return a + b
    end
})

print(t(1, 2))  -- 3 (t 被当作函数调用)
```

### VM 侧
```
CALL 指令:
  1. 检查 func 是否为 Function
  2. 如果不是 → 检查 metatable 的 __call
  3. 调用 __call(table, args...)
```

## 2. __len

`#t` 运算符:

```lua
local t = setmetatable({1, 2, 3}, {
    __len = function(self)
        return 999
    end
})

print(#t)  -- 999 (覆盖默认长度计算)
```

### Lua 5.1 对 table 的限制

```
Lua 5.1: table 的 __len 被忽略 (#t 总是用内置算法)
本项目: 可以选择支持 table 的 __len (注意兼容性)
```

## 3. __concat

`..` 运算符:

```lua
local t = setmetatable({}, {
    __concat = function(a, b)
        return "[" .. tostring(a) .. ":" .. tostring(b) .. "]"
    end
})

print("hello" .. t)  -- 触发 __concat
```

## 4. __tostring

`tostring()` 调用:

```lua
local t = setmetatable({x = 1, y = 2}, {
    __tostring = function(self)
        return string.format("Point(%d, %d)", self.x, self.y)
    end
})

print(t)  -- Point(1, 2)
```

## 5. __gc (GC Finalizer)

```lua
local t = setmetatable({}, {
    __gc = function(self)
        print("Table is being collected!")
    end
})
t = nil
collectgarbage()  -- 触发 __gc
```

GC 在回收对象前调用 `__gc` 元方法。
