-- Numeric for-loop tests
-- Tests for basic numeric for loops

print("=== Numeric for-loop tests ===")

-- Test 1: Basic increasing loop
print("Test 1: Basic increasing loop (1 to 5)")
for i = 1, 5 do
    print("  i =", i)
end

-- Test 2: Basic decreasing loop
print("\nTest 2: Basic decreasing loop (5 to 1)")
for i = 5, 1, -1 do
    print("  i =", i)
end

-- Test 3: Step of 2
print("\nTest 3: Step of 2 (0 to 10)")
for i = 0, 10, 2 do
    print("  i =", i)
end

-- Test 4: Negative step
print("\nTest 4: Negative step (10 to 0, step -2)")
for i = 10, 0, -2 do
    print("  i =", i)
end

-- Test 5: Single-iteration loop
print("\nTest 5: Single-iteration loop (5 to 5)")
for i = 5, 5 do
    print("  i =", i)
end

-- Test 6: Empty loop (should not execute)
print("\nTest 6: Empty loop test (1 to 0)")
local executed = false
for i = 1, 0 do
    executed = true
    print("  This should not be printed")
end
if not executed then
    print("✓ Empty loop correctly did not execute")
else
    print("✗ Empty loop executed unexpectedly")
end

-- Test 7: Floating point loop
print("\nTest 7: Floating point loop (0.5 to 2.5, step 0.5)")
for i = 0.5, 2.5, 0.5 do
    print("  i =", i)
end

-- Test 8: Nested for loops
print("\nTest 8: Nested for loops (2x3 matrix)")
for i = 1, 2 do
    for j = 1, 3 do
        print("  (" .. i .. "," .. j .. ")")
    end
end

-- Test 9: Break in loop
print("\nTest 9: Break in loop (1 to 10, break at 5)")
for i = 1, 10 do
    if i == 5 then
        print("  Break at i=5")
        break
    end
    print("  i =", i)
end

-- Test 10: Loop variable scope
print("\nTest 10: Loop variable scope test")
local outerI = 100
for i = 1, 3 do
    print("  i inside loop =", i)
end
print("  outerI outside loop =", outerI)
-- Note: loop variable i is not accessible outside the loop

-- Test 11: Large range loop (performance)
print("\nTest 11: Large range loop test (sum 1 to 1000)")
local sum = 0
for i = 1, 1000 do
    sum = sum + i
end
print("  Sum from 1 to 1000 =", sum)
print("  Expected = 500500")
if sum == 500500 then
    print("✓ Large range loop computed correctly")
else
    print("✗ Large range loop computed incorrectly")
end

print("\n=== Numeric for-loop tests completed ===")
