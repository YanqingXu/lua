-- Test constant folding
print("=== All constants ===")
print("a" .. "b" .. "c")

print("")
print("=== Mixed constants and variables ===")
local x = "x"
print("a" .. x .. "c")

print("")
print("=== All variables ===")
local a = "a"
local b = "b"
local c = "c"
print(a .. b .. c)

print("")
print("=== The problem case ===")
local i = 2
print("  " .. i .. "^2 = " .. 4)
