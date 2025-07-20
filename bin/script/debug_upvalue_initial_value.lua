-- Debug upvalue initial value preservation
print("=== Upvalue Initial Value Debug ===")

-- Test 1: Simple upvalue with initial value
function create_counter(start)
    print("create_counter called with start =", start)
    local count = start or 0
    print("Local count initialized to:", count)
    print("About to return closure...")
    return function()
        print("Closure called, current count:", count)
        count = count + 1
        print("After increment, count:", count)
        return count
    end
end

print("Creating counter with start = 10...")
local counter = create_counter(10)
print("Counter created")

print("Calling counter first time...")
local result1 = counter()
print("First result:", result1, "(expected: 11)")

print("Calling counter second time...")
local result2 = counter()
print("Second result:", result2, "(expected: 12)")

-- Test 2: Multiple counters with different initial values
print("\nTesting multiple counters...")

local counter_a = create_counter(5)
local counter_b = create_counter(20)

print("Counter A first call:", counter_a(), "(expected: 6)")
print("Counter B first call:", counter_b(), "(expected: 21)")
print("Counter A second call:", counter_a(), "(expected: 7)")
print("Counter B second call:", counter_b(), "(expected: 22)")

print("=== Upvalue Initial Value Debug Complete ===")
