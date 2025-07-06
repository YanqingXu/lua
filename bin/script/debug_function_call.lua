-- Debug function call
function add(a, b)
    return a + b
end

local a = 10
local b = 3

print("Testing function call:")
local result = add(a, b)
print("result = " .. result)

print("")
print("Testing in concatenation:")
print("  Addition: " .. a .. " + " .. b .. " = " .. add(a, b))
