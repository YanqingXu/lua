-- Debug __call argument passing
print("=== Debug __call Argument Passing ===")

local debugger = {}
setmetatable(debugger, {
    __call = function(self, ...)
        local args = {...}
        print("Number of arguments:", #args)
        for i, arg in ipairs(args) do
            print("  Arg " .. i .. ":", arg, "(type: " .. type(arg) .. ")")
        end
        return "Received " .. #args .. " arguments"
    end
})

print("\n--- Test 1: Single argument ---")
local result1 = debugger(42)
print("Result:", result1)

print("\n--- Test 2: Multiple arguments ---")
local result2 = debugger("hello", 123, true)
print("Result:", result2)

print("\n--- Test 3: No arguments ---")
local result3 = debugger()
print("Result:", result3)

print("\n=== Debug completed ===")
