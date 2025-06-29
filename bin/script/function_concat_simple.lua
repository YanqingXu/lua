-- 函数中的字符串连接测试

print("Testing function concatenation...")

function test()
    local result = ""
    result = result .. "hello"
    return result
end

local output = test()
print("output =", output)

print("Done.")
