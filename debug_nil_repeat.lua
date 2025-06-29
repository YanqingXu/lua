-- Debug nil error in repeat-until
print("=== Testing for loop first ===")
for i = 1, 3 do
    local square = i * i
    print("  " .. i .. "^2 = " .. square)
end
print("For loop completed")

print("")
print("=== Testing repeat-until loop ===")
local x = 1
print("Initial x = " .. x)

repeat
    print("  x = " .. x)
    print("  Before multiplication: x = " .. x)
    x = x * 2
    print("  After multiplication: x = " .. x)
    print("  Condition x > 10: " .. tostring(x > 10))
until x > 10

print("Repeat-until completed")
print("Final x = " .. x)
