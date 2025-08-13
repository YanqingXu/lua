print("=== Closure Instance Test ===")

-- Test: Create two separate closure instances
print("Creating first counter...")
local function make_counter1()
    local count = 0
    return function()
        count = count + 1
        print("  counter1 count =", count)
        return count
    end
end

print("Creating second counter...")
local function make_counter2()
    local count = 0
    return function()
        count = count + 1
        print("  counter2 count =", count)
        return count
    end
end

local counter1 = make_counter1()
local counter2 = make_counter2()

print("Testing counter1:")
counter1()
counter1()

print("Testing counter2:")
counter2()
counter2()

print("Testing counter1 again:")
counter1()

print("=== Closure Instance Test Complete ===")
