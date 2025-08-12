-- Test extended instruction set support
print("=== Extended Instructions Test ===")

-- Test 1: Table operations
print("Test 1: Table operations")
local t = {}
t["key1"] = "value1"
t["key2"] = 42
t[1] = "first"
t[2] = "second"

print("t['key1'] =", t["key1"])
print("t['key2'] =", t["key2"])
print("t[1] =", t[1])
print("t[2] =", t[2])

-- Test 2: Table length
print("\nTest 2: Table length")
local arr = {1, 2, 3, 4, 5}
print("Length of arr:", #arr)

-- Test 3: String length
print("\nTest 3: String length")
local str = "Hello World"
print("Length of str:", #str)

-- Test 4: Modulo operation
print("\nTest 4: Modulo operation")
local a = 17
local b = 5
print("17 % 5 =", a % b)

-- Test 5: Unary minus
print("\nTest 5: Unary minus")
local x = 42
print("-x =", -x)

-- Test 6: Logical NOT
print("\nTest 6: Logical NOT")
local flag = true
print("not true =", not flag)
print("not false =", not false)
print("not nil =", not nil)

-- Test 7: Comparison operations
print("\nTest 7: Comparison operations")
local num1 = 10
local num2 = 20
print("10 == 10:", num1 == 10)
print("10 == 20:", num1 == num2)
print("10 < 20:", num1 < num2)
print("20 < 10:", num2 < num1)
print("10 <= 10:", num1 <= 10)
print("10 <= 20:", num1 <= num2)

-- Test 8: Mixed table and arithmetic
print("\nTest 8: Mixed operations")
local data = {x = 5, y = 10}
local result = data.x + data.y
print("data.x + data.y =", result)

-- Test 9: Nested table access
print("\nTest 9: Nested table access")
local nested = {inner = {value = 99}}
print("nested.inner.value =", nested.inner.value)

print("\n=== Extended Instructions Test Completed ===")
