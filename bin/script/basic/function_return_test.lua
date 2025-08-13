print("=== Function Return Test ===")

-- Test: Function returning a function
local function make_func()
    local function inner()
        return 42
    end
    return inner
end

print("About to call make_func()")
local func = make_func()
print("func =", func)
print("type(func) =", type(func))

print("=== Function Return Test Complete ===")
