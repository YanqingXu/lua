-- Test extreme constant limits
print("=== Testing Extreme Constant Limits ===")

-- Generate many constants to test the limit
local function generateLargeTable()
    local t = {}
    
    -- Create table with many string keys to exhaust constant pool
    for i = 1, 150 do
        local key = "key" .. string.format("%03d", i)
        t[key] = "value" .. string.format("%03d", i)
    end
    
    return t
end

local t = generateLargeTable()
print("t.key001 =", t.key001)
print("t.key050 =", t.key050)
print("t.key100 =", t.key100)
print("t.key150 =", t.key150)

print("=== Test completed ===")
