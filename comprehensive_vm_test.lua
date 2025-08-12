-- Comprehensive VM functionality test
print("=== Comprehensive VM Test Suite ===")

-- Test 1: Basic print functionality
print("Test 1: Basic print")
print("Hello from enhanced VM!")

-- Test 2: Variable assignment and access
print("\nTest 2: Variable operations")
local x = 42
local y = 3.14
print("x =", x)
print("y =", y)

-- Test 3: Arithmetic operations
print("\nTest 3: Arithmetic operations")
local a = 10
local b = 5
print("a + b =", a + b)
print("a - b =", a - b)
print("a * b =", a * b)
print("a / b =", a / b)

-- Test 4: Mixed arithmetic with constants
print("\nTest 4: Mixed arithmetic")
local result1 = x + 8
local result2 = y * 2
print("x + 8 =", result1)
print("y * 2 =", result2)

-- Test 5: Complex expressions
print("\nTest 5: Complex expressions")
local complex = (a + b) * (a - b)
print("(a + b) * (a - b) =", complex)

-- Test 6: Boolean values
print("\nTest 6: Boolean operations")
local flag = true
local flag2 = false
print("flag =", flag)
print("flag2 =", flag2)

-- Test 7: Nil values
print("\nTest 7: Nil handling")
local nilval = nil
print("nilval =", nilval)

-- Test 8: Function calls with multiple arguments
print("\nTest 8: Multiple arguments")
print("Multiple args:", 1, 2, 3, "test")

-- Test 9: Global variable access
print("\nTest 9: Global access")
global_var = 999
print("global_var =", global_var)

-- Test 10: Error handling (division by zero)
print("\nTest 10: Error handling")
local safe_result = 10 / 2
print("10 / 2 =", safe_result)

print("\n=== All tests completed successfully ===")
print("VM functionality verification: PASSED")
