print("=== Same Function Closure Test ===")

-- Test: Multiple calls to the same function should create independent closures
local function make_counter()
    local count = 0
    return function()
        count = count + 1
        print("  count =", count)
        return count
    end
end

print("Creating counter1 from make_counter()...")
local counter1 = make_counter()

print("Creating counter2 from make_counter()...")
local counter2 = make_counter()

print("Testing counter1:")
counter1()
counter1()

print("Testing counter2 (should be independent):")
counter2()
counter2()

print("Testing counter1 again (should continue from 3):")
counter1()

print("=== Same Function Closure Test Complete ===")
