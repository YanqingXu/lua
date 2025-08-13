print("=== Upvalue Test ===")

-- Test 1: Basic upvalue capture
print("Test 1: Basic upvalue capture")
local function make_counter()
    local count = 0
    return function()
        print("count =", count)
        return count
    end
end

local counter = make_counter()
print("counter() =", counter())

-- Test 2: Upvalue modification
print("\nTest 2: Upvalue modification")
local function make_incrementer()
    local count = 0
    return function()
        count = count + 1
        print("count after increment =", count)
        return count
    end
end

local inc = make_incrementer()
print("inc() =", inc())
print("inc() =", inc())

print("\n=== Upvalue Test Complete ===")
