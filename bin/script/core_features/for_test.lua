-- Complete for loop test
print("=== For Loop Test Suite ===")

print("Test 1: Basic for loop")
for i = 1, 3 do
    print("Basic loop, i =", i)
end

print("Test 2: For loop with positive step")
for i = 1, 7, 2 do
    print("Positive step, i =", i)
end

print("Test 3: Countdown for loop")
for i = 5, 1, -1 do
    print("Countdown, i =", i)
end

print("Test 4: For loop with negative step")
for i = 10, 2, -3 do
    print("Negative step, i =", i)
end

print("Test 5: Single iteration loop")
for i = 5, 5 do
    print("Single iteration, i =", i)
end

print("Test 6: Zero iterations (should not print)")
for i = 5, 1 do
    print("This should not print, i =", i)
end

print("=== All for loop tests completed ===")
