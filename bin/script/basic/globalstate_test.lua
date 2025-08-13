-- Test GlobalState integration
print("=== GlobalState Integration Test ===")

-- Test 1: Basic global variable operations
print("Test 1: Basic global variables")
global_var1 = "Hello GlobalState"
global_var2 = 42
global_var3 = true

print("global_var1 =", global_var1)
print("global_var2 =", global_var2)
print("global_var3 =", global_var3)

-- Test 2: Global variable persistence
print("\nTest 2: Global variable persistence")
counter = 0
counter = counter + 1
counter = counter + 1
print("counter =", counter)

-- Test 3: Global variable overwriting
print("\nTest 3: Global variable overwriting")
test_var = "initial"
print("Before:", test_var)
test_var = "modified"
print("After:", test_var)

-- Test 4: Mixed operations
print("\nTest 4: Mixed operations")
x = 10
y = 20
result = x + y
print("x + y =", result)

-- Test 5: Global variables in expressions
print("\nTest 5: Global variables in expressions")
base = 5
multiplier = 3
final_result = base * multiplier + 2
print("base * multiplier + 2 =", final_result)

print("\n=== GlobalState Integration Test Completed ===")
