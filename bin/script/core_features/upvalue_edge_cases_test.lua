-- Upvalue Edge Cases and Error Conditions Test
print("=== Upvalue Edge Cases Test ===")

-- Test 1: Upvalue with nil values
print("\n--- Test 1: Upvalue with nil values ---")
local nil_var = nil

function test_nil_upvalue()
    print("Nil upvalue value:", nil_var)
    if nil_var == nil then
        print("Upvalue is correctly nil")
    else
        print("ERROR: Upvalue should be nil")
    end
    return nil_var
end

test_nil_upvalue()

-- Test 2: Upvalue shadowing
print("\n--- Test 2: Upvalue shadowing ---")
local shadowed_var = "outer"

function test_shadowing()
    local shadowed_var = "inner"
    print("Inner shadowed_var:", shadowed_var)
    
    function inner_func()
        print("From inner function, shadowed_var:", shadowed_var)
        return shadowed_var
    end
    
    return inner_func()
end

print("Outer shadowed_var:", shadowed_var)
local shadow_result = test_shadowing()
print("Shadow test result:", shadow_result)

-- Test 3: Upvalue with different types
print("\n--- Test 3: Upvalue with different types ---")
local number_var = 42
local string_var = "hello"
local boolean_var = true

function test_mixed_types()
    print("Number upvalue:", number_var)
    print("String upvalue:", string_var)
    print("Boolean upvalue:", boolean_var)
    
    local result = string_var .. " " .. number_var
    if boolean_var then
        result = result .. " (true)"
    end
    
    print("Mixed result:", result)
    return result
end

test_mixed_types()

-- Test 4: Upvalue modification in nested scopes
print("\n--- Test 4: Upvalue modification in nested scopes ---")
local mutable_var = 1

function outer_modifier()
    print("Before modification:", mutable_var)
    mutable_var = mutable_var * 2
    print("After outer modification:", mutable_var)
    
    function inner_modifier()
        print("Before inner modification:", mutable_var)
        mutable_var = mutable_var + 10
        print("After inner modification:", mutable_var)
        return mutable_var
    end
    
    return inner_modifier()
end

print("Initial mutable_var:", mutable_var)
local modified_result = outer_modifier()
print("Final mutable_var:", mutable_var)
print("Modification result:", modified_result)

-- Test 5: Multiple functions sharing same upvalue
print("\n--- Test 5: Multiple functions sharing same upvalue ---")
local shared_state = 0

function incrementer()
    shared_state = shared_state + 1
    print("Incrementer: shared_state =", shared_state)
    return shared_state
end

function doubler()
    shared_state = shared_state * 2
    print("Doubler: shared_state =", shared_state)
    return shared_state
end

function resetter()
    shared_state = 0
    print("Resetter: shared_state =", shared_state)
    return shared_state
end

print("Initial shared_state:", shared_state)
incrementer()
incrementer()
doubler()
incrementer()
resetter()

-- Test 6: Upvalue in conditional statements
print("\n--- Test 6: Upvalue in conditional statements ---")
local condition_var = true
local value_var = "conditional_value"

function conditional_upvalue_test()
    if condition_var then
        print("Condition is true, value_var:", value_var)
        return value_var
    else
        print("Condition is false")
        return "default"
    end
end

local cond_result1 = conditional_upvalue_test()
condition_var = false
local cond_result2 = conditional_upvalue_test()

print("First result:", cond_result1)
print("Second result:", cond_result2)

-- Test 7: Upvalue with arithmetic operations
print("\n--- Test 7: Upvalue with arithmetic operations ---")
local base_number = 5

function arithmetic_upvalue_test(x)
    local sum = base_number + x
    local product = base_number * x
    local difference = x - base_number
    
    print("Base:", base_number, "Input:", x)
    print("Sum:", sum, "Product:", product, "Difference:", difference)
    
    return sum, product, difference
end

local s, p, d = arithmetic_upvalue_test(10)
print("Arithmetic results - Sum:", s, "Product:", p, "Difference:", d)

print("=== Edge cases tests completed ===")
