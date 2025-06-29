-- 空字符串测试

print("=== Empty String Test ===")

function testEmptyString()
    print("Testing empty string")
    local result = ""
    print("result =", result)
    print("result type =", type(result))
    
    local result2 = result .. "hello"
    print("result2 =", result2)
end

testEmptyString()

print("=== Test End ===")
