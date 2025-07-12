-- Step by step table test
print("=== Step by Step Table Test ===")

-- Test 1: Simple table field access
print("Test 1: Simple table field access")
local t1 = {}
t1.x = 10
print("t1.x =", t1.x)

-- Test 2: Test table field in different context
print("Test 2: Table field in different context")
local t2 = {}
t2.name = "test"
local name_value = t2.name
print("name_value =", name_value)

-- Test 3: Test with local variable first
print("Test 3: Test with local variable first")
local test_string = "hello"
local t3 = {}
t3.str = test_string
print("t3.str =", t3.str)

-- Test 4: Test multiple tables
print("Test 4: Multiple tables")
local ta = {}
local tb = {}
ta.val = 100
tb.val = 200
print("ta.val =", ta.val)
print("tb.val =", tb.val)

-- Test 5: Test nested assignment
print("Test 5: Nested assignment")
local t4 = {}
t4.x = 5
t4.y = t4.x
print("t4.x =", t4.x)
print("t4.y =", t4.y)

print("=== Step by step test completed ===")
