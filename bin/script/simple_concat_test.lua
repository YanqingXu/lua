-- 简单的CONCAT测试

print("=== Simple CONCAT Test ===")

function testSimpleConcat()
    print("Before local declaration")
    local result = ""
    print("After local declaration")
    print("result =", result)
    print("type(result) =", type(result))
    
    print("Before concatenation")
    result = result .. "hello"
    print("After concatenation")
    print("result =", result)
end

testSimpleConcat()

print("=== Test End ===")
