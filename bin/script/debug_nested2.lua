-- 调试嵌套函数调用
print("=== Debug Nested Call Test ===")

function test()
    print("Inside test function")
    return 42
end

print("Before calling test()")
local result = test()
print("After calling test(), result =", result)

print("Now testing direct nested call:")
print("Result:", test())

print("=== Test completed ===")
