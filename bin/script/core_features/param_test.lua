-- 参数传递测试
print("=== Parameter Test ===")

-- 测试1：检查参数类型
function checkType(x)
    print("Parameter type:", type(x))
    print("Parameter value:", x)
    return x
end

print("Testing checkType(5):")
local result1 = checkType(5)
print("Result:", result1)

print("Testing checkType('hello'):")
local result2 = checkType("hello")
print("Result:", result2)

-- 测试2：简单的数字操作
function double(x)
    print("double() received:", x, "type:", type(x))
    return x * 2
end

print("Testing double(5):")
local result3 = double(5)
print("Result:", result3)

print("=== Parameter test completed ===")
