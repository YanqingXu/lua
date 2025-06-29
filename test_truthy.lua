-- Test truthy values
print("Testing truthy values:")

local val1 = true
local val2 = false
local val3 = 1
local val4 = 0
local val5 = "hello"
local val6 = ""

print("true is truthy: " .. tostring(val1))
print("false is truthy: " .. tostring(val2))
print("1 is truthy: " .. tostring(val3))
print("0 is truthy: " .. tostring(val4))
print("'hello' is truthy: " .. tostring(val5))
print("'' is truthy: " .. tostring(val6))

print("")
print("Testing if statements:")
if val1 then print("true -> if executed") end
if val2 then print("false -> if executed") else print("false -> else executed") end
if val3 then print("1 -> if executed") end
if val4 then print("0 -> if executed") end
