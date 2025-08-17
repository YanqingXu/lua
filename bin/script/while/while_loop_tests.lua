-- while loop tests
-- Tests for while and repeat-until loops

print("=== While loop tests ===")

-- Test 1: Basic while loop
print("Test 1: Basic while loop (count to 5)")
local i = 1
while i <= 5 do
    print("  i =", i)
    i = i + 1
end

-- Test 2: while condition is false (does not execute)
print("\nTest 2: while condition is false")
local executed = false
while false do
    executed = true
    print("  This should not be printed")
end
if not executed then
    print("✓ while with false condition correctly did not execute")
else
    print("✗ while with false condition incorrectly executed")
end

-- Test 3: break in while loop
print("\nTest 3: break in while loop")
local j = 1
while true do
    if j > 3 then
        print("  break at j=" .. j)
        break
    end
    print("  j =", j)
    j = j + 1
end

-- Test 4: nested while loops
print("\nTest 4: nested while loops")
local outer = 1
while outer <= 2 do
    print("  Outer loop: outer =", outer)
    local inner = 1
    while inner <= 3 do
        print("    Inner loop: inner =", inner)
        inner = inner + 1
    end
    outer = outer + 1
end

-- Test 5: factorial with while loop
print("\nTest 5: factorial with while loop")
local n = 5
local factorial = 1
local temp = n
while temp > 0 do
    factorial = factorial * temp
    temp = temp - 1
end
print("  " .. n .. "! =", factorial)
if factorial == 120 then
    print("✓ Factorial computed correctly")
else
    print("✗ Factorial computed incorrectly")
end

print("\n=== repeat-until loop tests ===")

-- Test 6: basic repeat-until
print("Test 6: basic repeat-until")
local k = 1
repeat
    print("  k =", k)
    k = k + 1
until k > 3

-- Test 7: repeat-until executes at least once
print("\nTest 7: repeat-until executes at least once")
local executed2 = false
repeat
    executed2 = true
    print("  repeat-until executes at least once")
until true
if executed2 then
    print("✓ repeat-until correctly executed at least once")
else
    print("✗ repeat-until did not execute")
end

-- Test 8: break in repeat-until
print("\nTest 8: break in repeat-until")
local m = 1
repeat
    if m == 3 then
        print("  break at m=" .. m)
        break
    end
    print("  m =", m)
    m = m + 1
until false

-- Test 9: repeat-until with complex condition
print("\nTest 9: repeat-until with complex condition")
local sum = 0
local num = 1
repeat
    sum = sum + num
    print("  num=" .. num .. ", sum=" .. sum)
    num = num + 1
until sum > 10 or num > 5

-- Test 10: nested repeat-until
print("\nTest 10: nested repeat-until")
local x = 1
repeat
    print("  Outer: x =", x)
    local y = 1
    repeat
        print("    Inner: y =", y)
        y = y + 1
    until y > 2
    x = x + 1
until x > 2

-- Test 11: differences between while and repeat-until
print("\nTest 11: differences between while and repeat-until")
print("while loop (condition is false):")
local count1 = 0
while false do
    count1 = count1 + 1
end
print("  Executions:", count1)

print("repeat-until loop (condition is true):")
local count2 = 0
repeat
    count2 = count2 + 1
until true
print("  Executions:", count2)

print("\n=== While loop tests completed ===")
