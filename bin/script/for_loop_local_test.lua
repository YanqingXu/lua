-- For循环中局部变量测试

print("=== For Loop Local Variable Test ===")

function testForLoop()
    print("Before local declaration")
    local result = "start"
    print("result before loop =", result)
    
    for i = 1, 2 do
        print("In loop, i =", i)
        print("In loop, result =", result)
        result = result .. i
        print("In loop, result after concat =", result)
    end
    
    print("result after loop =", result)
end

testForLoop()

print("=== Test End ===")
