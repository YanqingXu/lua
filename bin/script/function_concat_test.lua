-- 函数中的字符串连接测试

print("=== Function Concatenation Test ===")

-- 全局变量
local PROGRAM_NAME = "Lua Calculator & Demo"
local VERSION = "1.0"

-- 字符串重复函数
function repeatString(str, count)
    local result = ""
    for i = 1, count do
        result = result .. str
    end
    return result
end

-- 初始化函数（模拟原始问题）
function initProgram()
    print("=" .. repeatString("=", 40) .. "=")
    print(" " .. PROGRAM_NAME .. " v" .. VERSION)
    print("=" .. repeatString("=", 40) .. "=")
    print("")
end

-- 运行测试
initProgram()

print("=== Test End ===")
