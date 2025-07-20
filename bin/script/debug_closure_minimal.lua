-- Minimal closure test case for debugging
print("=== Minimal Closure Debug Test ===")

-- Test 1: Simple closure creation
print("Step 1: Creating closure function...")

function create_counter(start)
    print("Inside create_counter, start =", start)
    local count = start or 0
    print("Local count initialized to:", count)
    
    print("About to return inner function...")
    return function()
        print("Inside inner function, count =", count)
        count = count + 1
        print("After increment, count =", count)
        return count
    end
end

print("Step 2: Calling create_counter(10)...")
local counter = create_counter(10)
print("create_counter returned:", counter)
print("Type of counter:", type(counter))

if type(counter) == "function" then
    print("SUCCESS: Counter is a function!")
    print("Step 3: Calling counter()...")
    local result = counter()
    print("counter() returned:", result)
else
    print("ERROR: Counter is not a function, it's a", type(counter))
    print("Value:", counter)
end

print("=== End Minimal Closure Test ===")
