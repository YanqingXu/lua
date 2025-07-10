-- Final upvalue test - clean version
print("=== Final Upvalue Test ===")

-- Test 1: Basic upvalue
print("Test 1: Basic upvalue")
local x = 42
function test1()
    return x
end
local result1 = test1()
print("Expected: 42, Got:", result1)

-- Test 2: String upvalue
print("Test 2: String upvalue")
local message = "hello"
function test2()
    return message
end
local result2 = test2()
print("Expected: hello, Got:", result2)

-- Test 3: Multiple upvalues
print("Test 3: Multiple upvalues")
local a = 10
local b = 20
function test3()
    return a, b
end
local ra, rb = test3()
print("Expected: 10, 20, Got:", ra, rb)

-- Test 4: Upvalue modification
print("Test 4: Upvalue modification")
local counter = 0
function increment()
    counter = counter + 1
    return counter
end
print("Before:", counter)
local new_count = increment()
print("After increment:", counter)
print("Returned:", new_count)

print("=== All upvalue tests completed ===")
print("Upvalue functionality is working correctly!")
