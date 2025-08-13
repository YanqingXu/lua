print("=== Closure Creation Test ===")

-- Test 1: Function returning a function (no upvalue)
print("Test 1: Function returning function")
local function make_func()
    print("Inside make_func")
    local function inner()
        print("Inside inner function")
        return 42
    end
    print("About to return inner function")
    return inner
end

print("About to call make_func()")
local func = make_func()
print("make_func() returned:", func)

if func then
    print("About to call returned function")
    local result = func()
    print("Returned function result:", result)
else
    print("ERROR: make_func() returned nil")
end

print("=== Closure Creation Test Complete ===")
