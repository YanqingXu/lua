-- Debug multiple concatenation operators
print("=== Testing multiple .. operators ===")

print("Two operators:")
print("a" .. "b" .. "c")

print("")
print("Three operators:")
print("a" .. "b" .. "c" .. "d")

print("")
print("With variables:")
local x = "x"
local y = "y"
print("a" .. x .. "b" .. y .. "c")

print("")
print("The exact problem:")
local i = 1
print("  " .. i .. "^2 = " .. (i * i))
