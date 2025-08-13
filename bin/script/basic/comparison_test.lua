print("=== Comparison Test ===")

-- Test basic arithmetic first
print("Arithmetic test:")
print("  10 + 5 =", 10 + 5)
print("  10 ^ 2 =", 10 ^ 2)
print("  'Hello' .. 'World' =", "Hello" .. "World")

-- Test comparison in conditional context
print("\nComparison in if statements:")
local x = 10
local y = 20

if x == y then
    print("  x == y: true")
else
    print("  x == y: false")
end

if x < y then
    print("  x < y: true")
else
    print("  x < y: false")
end

if x > y then
    print("  x > y: true")
else
    print("  x > y: false")
end

-- Test logical operations
print("\nLogical operations:")
local a = true
local b = false
print("  true and false:", a and b)
print("  true or false:", a or b)
print("  not true:", not a)

print("\n=== Comparison Test Complete ===")
