-- 全局作用域测试

print("=== Global Scope Test ===")

-- 定义全局变量
PROGRAM_NAME = "Lua Calculator & Demo"
VERSION = "1.0"

print("Outside function:")
print("PROGRAM_NAME =", PROGRAM_NAME)
print("VERSION =", VERSION)

-- 在函数中访问全局变量
function testGlobals()
    print("Inside function:")
    print("PROGRAM_NAME =", PROGRAM_NAME)
    print("VERSION =", VERSION)
    
    -- 测试字符串连接
    local result = " " .. PROGRAM_NAME .. " v" .. VERSION
    print("Concatenation result:", result)
end

testGlobals()

print("=== Test End ===")
