-- Logical expressions tests
-- Test combinations of logical operators

print("=== Logical Expressions Tests ===")

-- Test 1: and operator
print("Test 1: and operator")
local a = true
local b = true
local c = false

if a and b then
    print("✓ true and true = true")
else
    print("✗ true and true = false")
end

if a and c then
    print("✗ true and false = true")
else
    print("✓ true and false = false")
end

-- Test 2: or operator
print("\nTest 2: or operator")
if a or c then
    print("✓ true or false = true")
else
    print("✗ true or false = false")
end

if c or false then
    print("✗ false or false = true")
else
    print("✓ false or false = false")
end

-- Test 3: not operator
print("\nTest 3: not operator")
if not c then
    print("✓ not false = true")
else
    print("✗ not false = false")
end

if not a then
    print("✗ not true = true")
else
    print("✓ not true = false")
end

-- Test 4: Compound logical expressions
print("\nTest 4: Compound logical expressions")
local x = 5
local y = 10
local z = 15

if x < y and y < z then
    print("✓ x < y < z")
else
    print("✗ Condition is false")
end

if x > y or y < z then
    print("✓ x > y or y < z")
else
    print("✗ Both conditions are false")
end

-- Test 5: Short-circuit evaluation
print("\nTest 5: Short-circuit evaluation")
local function returnTrue()
    print("  returnTrue called")
    return true
end

local function returnFalse()
    print("  returnFalse called")
    return false
end

print("Test false and returnTrue():")
if false and returnTrue() then
    print("✗ Result is true")
else
    print("✓ Result is false (returnTrue should not be called)")
end

print("Test true or returnFalse():")
if true or returnFalse() then
    print("✓ Result is true (returnFalse should not be called)")
else
    print("✗ Result is false")
end

-- Test 6: Comparison operators combination
print("\nTest 6: Comparison operators combination")
local num1 = 10
local num2 = 20
local num3 = 10

if num1 == num3 and num1 < num2 then
    print("✓ num1 == num3 and num1 < num2")
else
    print("✗ Condition is false")
end

if num1 ~= num2 and num2 > num3 then
    print("✓ num1 ~= num2 and num2 > num3")
else
    print("✗ Condition is false")
end

-- Test 7: Complex parentheses combinations
print("\nTest 7: Complex parentheses combinations")
local p = true
local q = false
local r = true

if (p and q) or (not q and r) then
    print("✓ (p and q) or (not q and r)")
else
    print("✗ Condition is false")
end

if not (p and q) and (p or r) then
    print("✓ not (p and q) and (p or r)")
else
    print("✗ Condition is false")
end

print("\n=== Logical Expressions Tests Completed ===")
