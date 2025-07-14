-- 测试__eq元方法
print("=== __eq Metamethod Test ===")

-- 测试1: 基础等于比较
print("\n--- Test 1: Basic equality comparison ---")
print("42 == 42:", 42 == 42)
print("42 == 43:", 42 == 43)
print("'hello' == 'hello':", "hello" == "hello")
print("'hello' == 'world':", "hello" == "world")
print("true == true:", true == true)
print("true == false:", true == false)
print("nil == nil:", nil == nil)

-- 测试2: 表的默认等于比较（地址比较）
print("\n--- Test 2: Default table equality (address comparison) ---")
local table1 = {a = 1}
local table2 = {a = 1}
local table3 = table1
print("table1 == table1:", table1 == table1)  -- 应该是true（同一个对象）
print("table1 == table2:", table1 == table2)  -- 应该是false（不同对象，即使内容相同）
print("table1 == table3:", table1 == table3)  -- 应该是true（同一个对象）

-- 测试3: 自定义__eq元方法
print("\n--- Test 3: Custom __eq metamethod ---")
local obj1 = {value = 10}
local obj2 = {value = 10}
local obj3 = {value = 20}

-- 设置元表，定义__eq元方法
local meta = {
    __eq = function(a, b)
        print("__eq called: a.value =", a.value, "b.value =", b.value)
        return a.value == b.value
    end
}

setmetatable(obj1, meta)
setmetatable(obj2, meta)
setmetatable(obj3, meta)

print("obj1 == obj2 (same value):", obj1 == obj2)  -- 应该是true
print("obj1 == obj3 (different value):", obj1 == obj3)  -- 应该是false

-- 测试4: 复杂的__eq元方法
print("\n--- Test 4: Complex __eq metamethod ---")
local person1 = {name = "Alice", age = 30}
local person2 = {name = "Alice", age = 30}
local person3 = {name = "Bob", age = 30}

local personMeta = {
    __eq = function(a, b)
        print("Person __eq called")
        return a.name == b.name and a.age == b.age
    end
}

setmetatable(person1, personMeta)
setmetatable(person2, personMeta)
setmetatable(person3, personMeta)

print("person1 == person2 (same data):", person1 == person2)  -- 应该是true
print("person1 == person3 (different name):", person1 == person3)  -- 应该是false

-- 测试5: 不同类型的比较（应该是false，不调用元方法）
print("\n--- Test 5: Different type comparison ---")
local obj = {value = 42}
setmetatable(obj, {
    __eq = function(a, b)
        print("This should not be called for different types")
        return false
    end
})

print("obj == 42:", obj == 42)  -- 应该是false，不调用元方法
print("obj == 'hello':", obj == "hello")  -- 应该是false，不调用元方法

print("\n=== Test completed ===")
