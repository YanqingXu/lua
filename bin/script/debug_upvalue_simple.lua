-- Simple upvalue access test
print("=== Simple Upvalue Access Test ===")

-- Test 1: Simple upvalue read
function create_reader(value)
    print("Creating reader with value:", value)
    return function()
        print("Reading upvalue, type:", type(value))
        print("Reading upvalue, value:", value)
        return value
    end
end

print("Test 1: Simple upvalue read")
local reader = create_reader(42)
local read_result = reader()
print("Read result:", read_result)
print("")

-- Test 2: Simple upvalue write (without arithmetic)
function create_setter(initial)
    print("Creating setter with initial:", initial)
    local stored = initial
    return function(new_value)
        print("Setting upvalue to:", new_value)
        stored = new_value
        print("Upvalue set, returning:", stored)
        return stored
    end
end

print("Test 2: Simple upvalue write")
local setter = create_setter(10)
local set_result = setter(20)
print("Set result:", set_result)
print("")

-- Test 3: Upvalue arithmetic (the problematic case)
function create_adder(start)
    print("Creating adder with start:", start)
    local num = start
    return function()
        print("Before arithmetic - num type:", type(num))
        print("Before arithmetic - num value:", num)
        print("Attempting: num = num + 1")
        num = num + 1  -- This line should fail
        print("After arithmetic - num:", num)
        return num
    end
end

print("Test 3: Upvalue arithmetic (expected to fail)")
local adder = create_adder(5)
local add_result = adder()
print("Add result:", add_result)

print("=== Simple Upvalue Access Test Complete ===")
