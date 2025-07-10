-- 测试函数内部的复杂字符串连接

print("Testing function context...")

-- 定义函数
function add(x, y)
    return x + y
end

-- 在函数内部测试（模拟mathDemo的环境）
function mathDemo()
    print("Math Calculation Demo:")
    print("")

    -- Basic operations
    local a = 10
    local b = 3
    print("Basic Operations (a=" .. a .. ", b=" .. b .. "):")
    print("  Addition: " .. a .. " + " .. b .. " = " .. add(a, b))
    print("Test completed in function.")
end

-- 调用函数
mathDemo()

print("All tests completed.")
