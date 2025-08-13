print("=== For Loop Test ===")

-- Test 1: Basic for loop (1 to 10)
print("\nTest 1: Basic for loop (1 to 10)")
for i = 1, 10 do
    print("  i =", i)
end

-- Test 2: For loop with step
print("\nTest 2: For loop with step (1 to 10, step 2)")
for i = 1, 10, 2 do
    print("  i =", i)
end

-- Test 3: Negative step
print("\nTest 3: Negative step (10 to 1, step -1)")
for i = 10, 1, -1 do
    print("  i =", i)
end

-- Test 4: For loop with expressions
print("\nTest 4: For loop with expressions")
local start = 5
local finish = 8
for i = start, finish do
    print("  i =", i)
end

-- Test 5: Nested for loops
print("\nTest 5: Nested for loops")
for i = 1, 3 do
    print("  Outer i =", i)
    for j = 1, 2 do
        print("    Inner j =", j)
    end
end

print("\n=== For Loop Test Complete ===")
