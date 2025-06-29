-- Type debugging test

print("=== Testing Value Types ===")
local x = 5
local y = 9

print("x = " .. x)
print("y = " .. y)

local result1 = x > y
local result2 = y > x

print("Type of result1 (x > y):")
print(type(result1))
print("Value of result1:")
print(result1)

print("Type of result2 (y > x):")
print(type(result2))
print("Value of result2:")
print(result2)

print("")
print("=== Testing Boolean Values ===")
local t = true
local f = false

print("Type of true:")
print(type(t))
print("Value of true:")
print(t)

print("Type of false:")
print(type(f))
print("Value of false:")
print(f)

print("")
print("=== Testing tostring Function ===")
print("tostring(true):")
print(tostring(true))
print("tostring(false):")
print(tostring(false))
print("tostring(result1):")
print(tostring(result1))
print("tostring(result2):")
print(tostring(result2))
