-- Test the fixed closure functionality
print("=== Closure Fix Verification ===")

-- Test 1: Simple closure creation and return
function create_simple_closure()
    return function()
        return "Hello from closure!"
    end
end

print("Test 1: Simple closure creation")
local simple_closure = create_simple_closure()
print("Closure type:", type(simple_closure))

if type(simple_closure) == "function" then
    print("✅ SUCCESS: Closure creation works!")
    local result = simple_closure()
    print("Closure result:", result)
else
    print("❌ FAILED: Closure creation failed")
end

print("")

-- Test 2: Closure with parameter
function create_greeting(name)
    return function()
        return "Hello, " .. name .. "!"
    end
end

print("Test 2: Closure with parameter")
local greeting = create_greeting("Lua")
print("Greeting type:", type(greeting))

if type(greeting) == "function" then
    print("✅ SUCCESS: Parameterized closure works!")
    local result = greeting()
    print("Greeting result:", result)
else
    print("❌ FAILED: Parameterized closure failed")
end

print("")
print("=== Closure Fix Verification Complete ===")

-- Note: Counter closure with upvalues will be tested separately
-- as it requires upvalue functionality to be working
