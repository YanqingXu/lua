# Closure Test Cases

## 1. 基本闭包

```lua
local function make_adder(x)
    return function(y) return x + y end
end
local add5 = make_adder(5)
assert(add5(3) == 8)
assert(add5(10) == 15)
```

## 2. Upvalue 共享

```lua
local function make_pair()
    local x = 0
    return function() x = x + 1; return x end,
           function() return x end
end
local inc, get = make_pair()
inc(); inc()
assert(get() == 2)
```

## 3. Upvalue 生命周期

```lua
local function make_counter()
    local count = 0
    return function()
        count = count + 1
        return count
    end
end
local c1 = make_counter()
assert(c1() == 1)
assert(c1() == 2)
local c2 = make_counter()
assert(c2() == 1)  -- 独立计数
assert(c1() == 3)
```

## 4. 嵌套闭包

```lua
local function outer()
    local a = 1
    local function middle()
        local b = 2
        return function()
            return a + b
        end
    end
    return middle()
end
local f = outer()
assert(f() == 3)
```

## 5. 相关测试文件

- `tests/unit/core/test_function.cpp`
- `tests/unit/vm/test_function_call.cpp`
- `tests/lua/official/closure.lua`
