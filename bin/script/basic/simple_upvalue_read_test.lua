print("=== Simple Upvalue Read Test ===")

-- Test: Simple upvalue read (no modification)
local function make_reader()
    local value = 42
    return function()
        return value  -- This should read the upvalue
    end
end

local reader = make_reader()
print("reader() =", reader())

print("=== Simple Upvalue Read Test Complete ===")
