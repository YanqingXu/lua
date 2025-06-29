-- 循环中的CONCAT测试

print("=== Loop CONCAT Test ===")

function testLoopConcat()
    print("Before local declaration")
    local result = ""
    print("After local declaration, result =", result)
    
    print("Before loop")
    for i = 1, 3 do
        print("Loop iteration i=" .. i)
        result = result .. "x"
        print("result after concat =", result)
    end
    print("After loop, result =", result)
end

testLoopConcat()

print("=== Test End ===")
