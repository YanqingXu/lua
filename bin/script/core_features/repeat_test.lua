-- 测试repeatString函数
print("=== RepeatString Test ===")

function repeatString(str, count)
    print("repeatString called with str='" .. str .. "', count=" .. count)
    local result = ""
    print("Initial result: '" .. result .. "'")
    
    for i = 1, count do
        print("Loop iteration " .. i)
        result = result .. str
        print("Result after iteration " .. i .. ": '" .. result .. "'")
    end
    
    print("Final result: '" .. result .. "'")
    return result
end

-- 测试1：简单调用
print("Test 1: repeatString('=', 3)")
local test1 = repeatString("=", 3)
print("Returned: '" .. test1 .. "'")

-- 测试2：模拟initProgram的调用
print("\nTest 2: repeatString('=', 40)")
local test2 = repeatString("=", 40)
print("Returned length:", #test2)

-- 测试3：字符串连接
print("\nTest 3: String concatenation")
local line = "=" .. test2 .. "="
print("Final line: '" .. line .. "'")

print("=== Test completed ===")
