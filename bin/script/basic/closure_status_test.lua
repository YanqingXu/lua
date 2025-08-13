print("=== Closure Status Test ===")

-- Test 1: Basic closure creation and calling
print("\nTest 1: Basic closure creation")
local function make_func()
    return function()
        return 42
    end
end

local func = make_func()
print("func =", func)
local result = func()
print("func() =", result)

-- Test 2: Closure with upvalue (read-only)
print("\nTest 2: Upvalue read")
local function make_reader()
    local value = 100
    return function()
        return value
    end
end

local reader = make_reader()
local read_result = reader()
print("reader() =", read_result)

-- Test 3: Closure with parameters
print("\nTest 3: Closure with parameters")
local function make_adder(x)
    return function(y)
        return x + y
    end
end

local add5 = make_adder(5)
local add_result = add5(3)
print("add5(3) =", add_result)

-- Test 4: Multiple independent closures
print("\nTest 4: Independent closures")
local func1 = make_adder(10)
local func2 = make_adder(20)
print("func1(1) =", func1(1))
print("func2(1) =", func2(1))

print("\n=== Closure Status Test Complete ===")
