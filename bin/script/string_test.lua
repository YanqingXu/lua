-- Test the exact case from test.lua
print("=== Test.lua Case Debug ===")

function repeatString(str, count)
    local result = ""
    for i = 1, count do
        result = result .. str
    end
    return result
end

print("Testing repeatString('=', 40)")
local repeated = repeatString("=", 40)
print("Result:", repeated)
print("Type check:", repeated)

print("Testing concatenation")
local final = "=" .. repeated .. "="
print("Final:", final)

print("=== Test completed ===")
