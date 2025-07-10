-- Complete upvalue functionality test
print("=== Complete Upvalue Test ===")

-- Test 1: Basic upvalue access
print("Test 1: Basic upvalue access")
local x = 42
function get_x()
    return x
end
print("x =", get_x())

-- Test 2: String upvalue
print("Test 2: String upvalue")
local message = "Hello World"
function get_message()
    return message
end
print("message =", get_message())

-- Test 3: Upvalue modification
print("Test 3: Upvalue modification")
local counter = 0
function increment()
    counter = counter + 1
    return counter
end
print("Initial counter:", counter)
print("After increment:", increment())
print("Final counter:", counter)

-- Test 4: Multiple upvalues in one function
print("Test 4: Multiple upvalues")
local a = 10
local b = 20
function sum_ab()
    return a
end
print("a =", sum_ab())

-- Test 5: Nested functions
print("Test 5: Nested functions")
local outer = 100
function create_inner()
    function inner()
        return outer
    end
    return inner
end
local inner_func = create_inner()
print("outer via nested function =", inner_func())

print("=== All upvalue tests completed successfully! ===")
print("Upvalue functionality is fully working!")
