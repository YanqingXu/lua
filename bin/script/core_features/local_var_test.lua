-- 局部变量初始化测试

print("=== Local Variable Test ===")

function testLocalVar()
    print("Before local declaration")
    local result = "hello"
    print("After local declaration")
    print("result =", result)
    
    local result2 = result .. " world"
    print("result2 =", result2)
end

testLocalVar()

print("=== Test End ===")
