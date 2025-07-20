-- Debug table issues
print("=== Debug Table Issues ===")

-- Test 1: Table as function parameter
print("Testing table as function parameter...")

function get_table_sum(t)
    print("Inside function, t.a =", t.a)
    print("Inside function, t.b =", t.b)
    print("Inside function, t.c =", t.c)
    return t.a + t.b + t.c
end

local sum_table = {a = 1, b = 2, c = 3}
print("sum_table.a =", sum_table.a)
print("sum_table.b =", sum_table.b)
print("sum_table.c =", sum_table.c)

local sum_result = get_table_sum(sum_table)
print("sum_result =", sum_result)
print("Expected: 6")

-- Test 2: Table length
print("Testing table length...")

local length_test = {10, 20, 30, 40}
print("length_test[1] =", length_test[1])
print("length_test[2] =", length_test[2])
print("length_test[3] =", length_test[3])
print("length_test[4] =", length_test[4])

local length_result = #length_test
print("length_result =", length_result)
print("Expected: 4")

print("=== End ===")
