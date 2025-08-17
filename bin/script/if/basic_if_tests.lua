-- Basic if statement tests
-- Test basic if-then-else structures

print("=== Basic If Statement Tests ===")

-- Test 1: Simple if statement
print("Test 1: Simple if statement")
local x = 5
if x > 3 then
    print("✓ x is greater than 3")
else
    print("✗ x is not greater than 3")
end

-- Test 2: if-else statement
print("\nTest 2: if-else statement")
local y = 2
if y > 5 then
    print("✗ y is greater than 5")
else
    print("✓ y is not greater than 5")
end

-- Test 3: if-elseif-else statement
print("\nTest 3: if-elseif-else statement")
local score = 85
if score >= 90 then
    print("✗ Grade is A")
elseif score >= 80 then
    print("✓ Grade is B")
elseif score >= 70 then
    print("✗ Grade is C")
else
    print("✗ Grade is D")
end

-- Test 4: Boolean value tests
print("\nTest 4: Boolean value tests")
local flag = true
if flag then
    print("✓ flag is true")
else
    print("✗ flag is false")
end

local flag2 = false
if flag2 then
    print("✗ flag2 is true")
else
    print("✓ flag2 is false")
end

-- Test 5: nil value tests
print("\nTest 5: nil value tests")
local nilValue = nil
if nilValue then
    print("✗ nil is considered true")
else
    print("✓ nil is considered false")
end

-- Test 6: Number 0 test (in Lua, 0 is truthy)
print("\nTest 6: Number 0 test")
local zero = 0
if zero then
    print("✓ Number 0 is considered true")
else
    print("✗ Number 0 is considered false")
end

-- Test 7: Empty string test (in Lua, empty string is truthy)
print("\nTest 7: Empty string test")
local emptyStr = ""
if emptyStr then
    print("✓ Empty string is considered true")
else
    print("✗ Empty string is considered false")
end

print("\n=== Basic If Statement Tests Completed ===")
