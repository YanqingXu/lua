-- Closures and Upvalues Test
-- Tests closure creation and upvalue handling

print("=== Closures and Upvalues Test ===")

local test_count = 0
local passed_count = 0

function test_assert(condition, test_name)
    test_count = test_count + 1
    if condition then
        passed_count = passed_count + 1
        print("[OK] " .. test_name)
    else
        print("[FAILED] " .. test_name)
    end
end

-- Simple Closure
print("Testing simple closures...")

function create_counter(start)
    local count = start or 0
    return function()
        count = count + 1
        return count
    end
end

local counter1 = create_counter(10)
local result1 = counter1()
local result2 = counter1()
test_assert(result1 == 11, "Closure counter (first call)")
test_assert(result2 == 12, "Closure counter (second call)")

-- Multiple Independent Closures
print("Testing multiple independent closures...")

local counter2 = create_counter(20)
local result3 = counter2()
local result4 = counter1()  -- Should continue from previous state
test_assert(result3 == 21, "Independent closure (first call)")
test_assert(result4 == 13, "Original closure (continued state)")

-- Closure with Multiple Upvalues
print("Testing closures with multiple upvalues...")

function create_calculator(initial_a, initial_b)
    local a = initial_a
    local b = initial_b
    return function(operation)
        if operation == "add" then
            return a + b
        elseif operation == "multiply" then
            return a * b
        else
            return 0
        end
    end
end

local calc = create_calculator(5, 3)
local add_result = calc("add")
local mul_result = calc("multiply")
test_assert(add_result == 8, "Closure with multiple upvalues (add)")
test_assert(mul_result == 15, "Closure with multiple upvalues (multiply)")

-- Nested Closures
print("Testing nested closures...")

function outer_function(x)
    return function(y)
        return function(z)
            return x + y + z
        end
    end
end

local nested_func = outer_function(1)
local inner_func = nested_func(2)
local final_result = inner_func(3)
test_assert(final_result == 6, "Nested closures (three levels)")

-- Direct nested call
local direct_result = outer_function(2)(3)(4)
test_assert(direct_result == 9, "Direct nested closure calls")

-- Closure Modifying Upvalue
print("Testing closure modifying upvalues...")

function create_accumulator(initial)
    local total = initial
    return function(value)
        total = total + value
        return total
    end
end

local accumulator = create_accumulator(100)
local acc_result1 = accumulator(5)
local acc_result2 = accumulator(10)
local acc_result3 = accumulator(3)
test_assert(acc_result1 == 105, "Accumulator (first addition)")
test_assert(acc_result2 == 115, "Accumulator (second addition)")
test_assert(acc_result3 == 118, "Accumulator (third addition)")

-- Closure Returning Multiple Functions
print("Testing closure returning multiple functions...")

function create_counter_pair(start)
    local count = start
    local increment = function()
        count = count + 1
        return count
    end
    local decrement = function()
        count = count - 1
        return count
    end
    return increment, decrement
end

local inc, dec = create_counter_pair(50)
local inc_result1 = inc()
local inc_result2 = inc()
local dec_result1 = dec()
test_assert(inc_result1 == 51, "Counter pair (increment 1)")
test_assert(inc_result2 == 52, "Counter pair (increment 2)")
test_assert(dec_result1 == 51, "Counter pair (decrement)")

-- Summary
print("")
print("=== Closures and Upvalues Test Summary ===")
print("Total tests: " .. tostring(test_count))
print("Passed tests: " .. tostring(passed_count))

local failed_count = test_count - passed_count
print("Failed tests: " .. tostring(failed_count))

if passed_count == test_count then
    print("All closure features working correctly!")
else
    print("Some closure features failed")
end

print("=== Closures and Upvalues Test Completed ===")
