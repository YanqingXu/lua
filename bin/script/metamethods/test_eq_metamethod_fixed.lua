-- 修复的__eq元方法测试
print("=== Fixed __eq Metamethod Test ===")

-- 测试1: 自定义__eq元方法（使用相同元表）
print("\n--- Test 1: Custom __eq metamethod (same metatable) ---")
local obj1 = {value = 10}
local obj2 = {value = 10}
local obj3 = {value = 20}

-- 创建共享的元表
local sharedMeta = {
    __eq = function(a, b)
        print("__eq called: comparing", a.value, "and", b.value)
        return a.value == b.value
    end
}

-- 设置相同的元表
setmetatable(obj1, sharedMeta)
setmetatable(obj2, sharedMeta)
setmetatable(obj3, sharedMeta)

print("obj1 == obj2 (same value):", obj1 == obj2)  -- 应该调用__eq，返回true
print("obj1 == obj3 (different value):", obj1 == obj3)  -- 应该调用__eq，返回false

-- 测试2: 不同元表的对象（不应该调用__eq）
print("\n--- Test 2: Different metatables (should not call __eq) ---")
local obj4 = {value = 10}
local obj5 = {value = 10}

setmetatable(obj4, {
    __eq = function(a, b)
        print("obj4's __eq called")
        return a.value == b.value
    end
})

setmetatable(obj5, {
    __eq = function(a, b)
        print("obj5's __eq called")
        return a.value == b.value
    end
})

print("obj4 == obj5 (different metatables):", obj4 == obj5)  -- 应该是false，不调用__eq

print("\n=== Test completed ===")
