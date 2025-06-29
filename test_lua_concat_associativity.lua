-- Test Lua concatenation associativity
-- This should be run with standard Lua to verify behavior

print("=== Testing concatenation associativity ===")

-- Test: "a" .. "b" .. "c"
-- If right-associative: "a" .. ("b" .. "c") = "a" .. "bc" = "abc"
-- If left-associative: ("a" .. "b") .. "c" = "ab" .. "c" = "abc"
-- Both give same result, so let's test with numbers

print("String test:")
print("a" .. "b" .. "c")

print("")
print("Number test:")
print(1 .. 2 .. 3)

print("")
print("Mixed test:")
local x = 2
print("Result: " .. x .. " + " .. x .. " = " .. (x + x))

print("")
print("Precedence test:")
print("Value: " .. 2 + 3)  -- Should be "Value: 5" if .. has lower precedence than +
print("Value: " .. (2 + 3))  -- Should be "Value: 5"
