-- Test string concatenation precedence
print("=== Testing String Concatenation Precedence ===")

for i = 1, 3 do
    print("Test " .. i .. ": " .. i .. "^2 = " .. (i * i))
end

print("")
print("=== Testing Individual Parts ===")
local i = 2
print("i = " .. i)
print("i * i = " .. (i * i))
print("Combined: " .. i .. "^2 = " .. (i * i))

print("")
print("=== Testing Precedence ===")
print("2 + 3 .. 4 should be '54' (not '7'): " .. 2 + 3 .. 4)
print("2 .. 3 + 4 should be '27' (not '234'): " .. 2 .. 3 + 4)
