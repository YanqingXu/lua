-- Basic closure test
print("=== Closure Test ===")

-- Test 1: Simple function creation (should work)
print("Test 1: Simple function")
local function simple()
    return 42
end

print("simple() =", simple())

-- Test 2: Function with parameters (should work)
print("\nTest 2: Function with parameters")
local function add(a, b)
    return a + b
end

print("add(10, 20) =", add(10, 20))

-- Test 3: Nested function (basic closure - may not work yet)
print("\nTest 3: Nested function")
local function outer(x)
    local function inner(y)
        return x + y
    end
    return inner
end

local closure = outer(10)
print("closure created")

-- Try to call the closure (may not work yet)
-- local result = closure(5)
-- print("closure(5) =", result)

print("\n=== Test completed ===")
