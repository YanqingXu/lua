-- 简单的__eq元方法测试
print("=== Simple __eq Metamethod Test ===")

-- 测试1: 基础比较
print("\n--- Test 1: Basic comparison ---")
print("42 == 42:", 42 == 42)
print("42 == 43:", 42 == 43)

-- 测试2: 表的默认比较
print("\n--- Test 2: Table default comparison ---")
local t1 = {a = 1}
local t2 = {a = 1}
local t3 = t1
print("t1 == t1:", t1 == t1)  -- 同一对象，应该是true
print("t1 == t2:", t1 == t2)  -- 不同对象，应该是false
print("t1 == t3:", t1 == t3)  -- 同一对象，应该是true

-- 测试3: 自定义__eq元方法
print("\n--- Test 3: Custom __eq metamethod ---")
local obj1 = {value = 10}
local obj2 = {value = 10}

setmetatable(obj1, {
    __eq = function(a, b)
        print("__eq called: comparing", a.value, "and", b.value)
        return a.value == b.value
    end
})

setmetatable(obj2, {
    __eq = function(a, b)
        print("__eq called: comparing", a.value, "and", b.value)
        return a.value == b.value
    end
})

print("obj1 == obj2:", obj1 == obj2)

print("\n=== Test completed ===")
