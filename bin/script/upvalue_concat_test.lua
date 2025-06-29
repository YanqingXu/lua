-- Upvalue字符串连接测试

print("Testing upvalue concatenation...")

local PROGRAM_NAME = "Lua Calculator & Demo"
local VERSION = "1.0"

function testUpvalue()
    print("Inside function:")
    print("PROGRAM_NAME =", PROGRAM_NAME)
    print("VERSION =", VERSION)
    
    -- 测试问题行
    local result = " " .. PROGRAM_NAME .. " v" .. VERSION
    print("result =", result)
end

testUpvalue()

print("Done.")
