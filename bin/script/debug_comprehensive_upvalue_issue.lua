-- Test the specific upvalue issue from comprehensive test
print("=== Upvalue Issue Debug ===")

-- Test the exact code from comprehensive test that might be causing the hang
print("Step 1: Creating counter function...")

function create_counter(start)
    print("Inside create_counter, start =", start)
    local count = start or 0
    print("Local count initialized to:", count)
    return function()
        print("Inside inner function, accessing count...")
        count = count + 1
        print("After increment, count =", count)
        return count
    end
end

print("Step 2: Creating counter instance...")
local counter = create_counter(10)
print("Counter created, type:", type(counter))

if type(counter) == "function" then
    print("Step 3: Calling counter first time...")
    local count1 = counter()
    print("First call result:", count1)
    
    print("Step 4: Calling counter second time...")
    local count2 = counter()
    print("Second call result:", count2)
    
    print("Expected: count1=11, count2=12")
    print("Actual: count1=" .. tostring(count1) .. ", count2=" .. tostring(count2))
else
    print("ERROR: Counter is not a function!")
end

print("=== Upvalue Issue Debug Complete ===")

-- Test nested closure as well
print("Testing nested closure...")

function outer_func(outer_x)
    print("outer_func called with", outer_x)
    return function(outer_y)
        print("middle function called with", outer_y)
        return function(outer_z)
            print("inner function called with", outer_z)
            local result = outer_x + outer_y + outer_z
            print("Computed result:", result)
            return result
        end
    end
end

print("Creating nested closure...")
local nested_result = outer_func(1)(2)(3)
print("Nested result:", nested_result)
print("Expected: 6, Actual:", nested_result)
