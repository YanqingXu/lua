print("=== Simple Closure Test ===")

-- Test 1: Simple function return (no closure)
print("Test 1: Simple function")
local function simple()
    return 42
end
print("simple() =", simple())

-- Test 2: Function with local variable (no upvalue)
print("\nTest 2: Function with local")
local function with_local()
    local x = 10
    return x
end
print("with_local() =", with_local())

-- Test 3: Basic closure (should fail due to upvalue)
print("\nTest 3: Basic closure")
local function make_counter()
    local count = 0
    return function()
        return count
    end
end
local counter = make_counter()
print("counter() =", counter())

print("\n=== Simple Closure Test Complete ===")
