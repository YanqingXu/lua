-- Comprehensive Upvalue Test Suite
print("=== Comprehensive Upvalue Test Suite ===")

-- Test 1: Basic upvalue access
print("\n--- Test 1: Basic upvalue access ---")
local outer_var = "outer_value"

function test_basic_upvalue()
    print("Accessing outer_var:", outer_var)
    return outer_var
end

local result1 = test_basic_upvalue()
print("Result:", result1)

-- Test 2: Upvalue modification
print("\n--- Test 2: Upvalue modification ---")
local counter = 0

function increment()
    counter = counter + 1
    print("Counter is now:", counter)
    return counter
end

increment()
increment()
increment()
print("Final counter value:", counter)

-- Test 3: Multiple upvalues
print("\n--- Test 3: Multiple upvalues ---")
local name = "Alice"
local age = 25

function create_person_info()
    local info = name .. " is " .. age .. " years old"
    print("Person info:", info)
    return info
end

local person_info = create_person_info()

-- Test 4: Nested functions with upvalues
print("\n--- Test 4: Nested functions with upvalues ---")
local base_value = 10

function outer_function(multiplier)
    print("Outer function called with multiplier:", multiplier)
    
    function inner_function(x)
        local result = base_value * multiplier * x
        print("Inner calculation:", base_value, "*", multiplier, "*", x, "=", result)
        return result
    end
    
    return inner_function
end

local calculator = outer_function(5)
local calc_result = calculator(3)
print("Calculator result:", calc_result)

-- Test 5: Upvalue in loop
print("\n--- Test 5: Upvalue in loop ---")
local sum = 0

function add_to_sum(value)
    sum = sum + value
    return sum
end

for i = 1, 5 do
    local new_sum = add_to_sum(i)
    print("Added", i, "sum is now", new_sum)
end

print("Final sum:", sum)

-- Test 6: String concatenation with upvalues
print("\n--- Test 6: String concatenation with upvalues ---")
local prefix = "Hello"
local suffix = "World"

function create_greeting(middle)
    local greeting = prefix .. " " .. middle .. " " .. suffix
    print("Created greeting:", greeting)
    return greeting
end

local greeting1 = create_greeting("Beautiful")
local greeting2 = create_greeting("Amazing")

print("=== All upvalue tests completed ===")
