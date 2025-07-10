-- Advanced Upvalue and Closure Test Suite
print("=== Advanced Upvalue and Closure Test Suite ===")

-- Test 1: Simple closure
print("\n--- Test 1: Simple closure ---")
function create_counter()
    local count = 0
    
    function increment()
        count = count + 1
        return count
    end
    
    return increment
end

local counter1 = create_counter()
local counter2 = create_counter()

print("Counter1 first call:", counter1())
print("Counter1 second call:", counter1())
print("Counter2 first call:", counter2())
print("Counter1 third call:", counter1())

-- Test 2: Closure with multiple upvalues
print("\n--- Test 2: Closure with multiple upvalues ---")
function create_calculator(initial_value, step)
    local current = initial_value
    
    function add()
        current = current + step
        return current
    end
    
    function multiply(factor)
        current = current * factor
        return current
    end
    
    function get_current()
        return current
    end
    
    -- Return multiple functions that share upvalues
    return add, multiply, get_current
end

local add_func, mult_func, get_func = create_calculator(10, 5)

print("Initial value:", get_func())
print("After add:", add_func())
print("After multiply by 2:", mult_func(2))
print("After another add:", add_func())

-- Test 3: Nested closures
print("\n--- Test 3: Nested closures ---")
function create_nested_closure(outer_val)
    print("Creating nested closure with outer_val:", outer_val)
    
    function level1(level1_val)
        print("Level 1 function called with:", level1_val)
        
        function level2(level2_val)
            print("Level 2 function called with:", level2_val)
            local result = outer_val + level1_val + level2_val
            print("Nested calculation:", outer_val, "+", level1_val, "+", level2_val, "=", result)
            return result
        end
        
        return level2
    end
    
    return level1
end

local nested_func = create_nested_closure(100)
local level1_func = nested_func(20)
local final_result = level1_func(3)
print("Final nested result:", final_result)

-- Test 4: Upvalue with string operations
print("\n--- Test 4: Upvalue with string operations ---")
local global_prefix = "LOG"

function create_logger(category)
    local log_category = category
    
    function log_message(message)
        local full_message = global_prefix .. "[" .. log_category .. "]: " .. message
        print(full_message)
        return full_message
    end
    
    return log_message
end

local info_logger = create_logger("INFO")
local error_logger = create_logger("ERROR")

info_logger("System started")
error_logger("Something went wrong")
info_logger("Process completed")

-- Test 5: Upvalue modification across calls
print("\n--- Test 5: Upvalue modification across calls ---")
function create_accumulator()
    local values = ""
    local count = 0
    
    function add_value(value)
        if count > 0 then
            values = values .. ", "
        end
        values = values .. value
        count = count + 1
        print("Added value:", value, "Current list:", values)
        return values
    end
    
    function get_summary()
        return "Total items: " .. count .. " - Values: " .. values
    end
    
    return add_value, get_summary
end

local add_val, get_summary = create_accumulator()

add_val("apple")
add_val("banana")
add_val("cherry")
print("Summary:", get_summary())

print("=== Advanced upvalue tests completed ===")
