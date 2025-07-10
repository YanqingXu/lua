-- CONCAT错误重现测试

print("=== CONCAT Error Test ===")

-- 全局变量
local PROGRAM_NAME = "Lua Calculator & Demo"
local VERSION = "1.0"

-- 字符串重复函数
function repeatString(str, count)
    print("  repeatString called with str='" .. str .. "', count=" .. count)
    local result = ""
    for i = 1, count do
        print("    Loop iteration i=" .. i)
        result = result .. str
        print("    result after concat =", result)
    end
    print("  repeatString returning:", result)
    return result
end

-- 测试函数
function testConcat()
    print("Before repeatString call")
    local repeated = repeatString("=", 5)
    print("After repeatString call, repeated =", repeated)
    
    print("Before complex concatenation")
    local header = "=" .. repeated .. "="
    print("header =", header)
end

-- 运行测试
testConcat()

print("=== Test End ===")
