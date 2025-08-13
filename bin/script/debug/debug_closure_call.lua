print("=== Debug Closure Call ===")

local function make_func()
    local function inner()
        return 42
    end
    return inner
end

local func = make_func()
print("func type:", type(func))

-- Try to call it
print("About to call func...")
-- local result = func()
-- print("Result:", result)

print("=== Debug Complete ===")
