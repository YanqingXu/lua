-- 循环和字符串重复测试

print("=== Loop and String Repeat Test ===")

-- 测试1：简单for循环
print("Test 1: Simple for loop")
for i = 1, 3 do
    print("  i =", i)
end

-- 测试2：字符串重复函数
print("Test 2: String repeat function")
function repeatString(str, count)
    print("  repeatString called with str='" .. str .. "', count=" .. count)
    local result = ""
    for i = 1, count do
        print("    Loop iteration i=" .. i)
        result = result .. str
        print("    Current result='" .. result .. "'")
    end
    print("  Final result='" .. result .. "'")
    return result
end

local repeated = repeatString("=", 5)
print("Repeated result:", repeated)

-- 测试3：使用重复字符串
print("Test 3: Using repeated string")
local header = "=" .. repeated .. "="
print("Header:", header)

print("=== Test End ===")
