-- 测试系统性修复
print("=== System Fix Verification ===")

-- 测试1: 普通函数调用
print("\n--- Test 1: Regular function call ---")
function regular_func(x)
    print("Regular function called with:", x)
    return x * 2
end

local result1 = regular_func(10)
print("Regular function result:", result1)

-- 测试2: __call元方法
print("\n--- Test 2: __call metamethod ---")
local callable_obj = {}
setmetatable(callable_obj, {
    __call = function(self, x)
        print("__call metamethod called with:", x)
        return x + 100
    end
})

local result2 = callable_obj(20)
print("__call result:", result2)

-- 测试3: 嵌套函数调用
print("\n--- Test 3: Nested function calls ---")
function outer_func(x)
    print("Outer function called with:", x)
    local inner_result = regular_func(x)
    return inner_result + 1
end

local result3 = outer_func(5)
print("Nested function result:", result3)

print("\n=== All tests completed ===")
