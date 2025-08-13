print("=== Table Debug Test (Task 2.1) ===")

-- Test 1: Basic table creation and access
print("\nTest 1: Basic table operations")
local t1 = {}
print("  Empty table created:", type(t1))

-- Test 2: Table with initial values
print("\nTest 2: Table initialization")
local t2 = {a = 1, b = 2, c = 3}
print("  t2.a =", t2.a)
print("  t2.b =", t2.b)
print("  t2.c =", t2.c)

-- Test 3: Array-style table
print("\nTest 3: Array-style table")
local arr = {10, 20, 30, 40}
print("  arr[1] =", arr[1])
print("  arr[2] =", arr[2])
print("  arr[3] =", arr[3])
print("  arr[4] =", arr[4])

-- Test 4: Mixed table (array + hash)
print("\nTest 4: Mixed table")
local mixed = {1, 2, 3, name = "test", value = 42}
print("  mixed[1] =", mixed[1])
print("  mixed[2] =", mixed[2])
print("  mixed[3] =", mixed[3])
print("  mixed.name =", mixed.name)
print("  mixed.value =", mixed.value)

-- Test 5: Dynamic table modification
print("\nTest 5: Dynamic modification")
local dynamic = {}
dynamic.x = 100
dynamic.y = 200
dynamic[1] = "first"
dynamic[2] = "second"
print("  dynamic.x =", dynamic.x)
print("  dynamic.y =", dynamic.y)
print("  dynamic[1] =", dynamic[1])
print("  dynamic[2] =", dynamic[2])

-- Test 6: Table length
print("\nTest 6: Table length")
local len_test = {1, 2, 3, 4, 5}
print("  #len_test =", #len_test)

-- Test 7: Nested tables
print("\nTest 7: Nested tables")
local nested = {
    inner = {
        value = 999,
        data = {1, 2, 3}
    }
}
print("  nested.inner.value =", nested.inner.value)
print("  nested.inner.data[1] =", nested.inner.data[1])

-- Test 8: Table as function parameter
print("\nTest 8: Table as parameter")
local function print_table(t)
    print("  Table received, t.test =", t.test)
end
print_table({test = "parameter_value"})

print("\n=== Table Debug Test Complete ===")
