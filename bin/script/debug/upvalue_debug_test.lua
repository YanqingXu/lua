print("=== Upvalue Debug Test ===")

-- Test: Debug upvalue access step by step
print("Step 1: Creating outer function")
local function make_reader()
    print("  Inside make_reader")
    local value = 42
    print("  value =", value)
    
    print("  Creating inner function")
    local function inner()
        print("    Inside inner function")
        print("    About to access value")
        local result = value  -- This should trigger GETUPVAL
        print("    value from upvalue =", result)
        return result
    end
    
    print("  Returning inner function")
    return inner
end

print("Step 2: Calling make_reader")
local reader = make_reader()
print("reader =", reader)

print("Step 3: Calling reader")
local result = reader()
print("Final result =", result)

print("=== Upvalue Debug Test Complete ===")
