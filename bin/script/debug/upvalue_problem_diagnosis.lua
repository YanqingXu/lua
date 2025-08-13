print("=== Upvalue Problem Diagnosis ===")

-- Test: Multiple counter instances should be independent
print("Creating two independent counters...")

local function make_counter()
    local count = 0
    return function()
        count = count + 1
        print("  count =", count)
        return count
    end
end

local counter1 = make_counter()
local counter2 = make_counter()

print("\nTesting counter1:")
print("counter1() should return 1:", counter1())
print("counter1() should return 2:", counter1())

print("\nTesting counter2 (should be independent):")
print("counter2() should return 1:", counter2())
print("counter2() should return 2:", counter2())

print("\nTesting counter1 again (should continue from 3):")
print("counter1() should return 3:", counter1())

print("\n=== Expected vs Actual ===")
print("Expected: counter1 and counter2 have independent state")
print("Actual: If they share state, all calls will increment the same counter")

print("\n=== Upvalue Problem Diagnosis Complete ===")
