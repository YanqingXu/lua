print("=== Closure Call Test ===")

local function make_func()
    local function inner()
        print("Inside inner function!")
        return 42
    end
    return inner
end

local func = make_func()
print("func =", func)

print("About to call func()")
local result = func()
print("result =", result)

print("=== Closure Call Test Complete ===")
