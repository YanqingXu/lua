-- Debug table length operator
print("=== Debug Table Length ===")

-- Test 1: Simple array
local arr = {10, 20, 30, 40}
print("Array contents:")
print("arr[1] =", arr[1])
print("arr[2] =", arr[2])
print("arr[3] =", arr[3])
print("arr[4] =", arr[4])
print("arr[5] =", arr[5])

print("Length of arr:", #arr)
print("Expected: 4")

-- Test 2: Empty table
local empty = {}
print("Length of empty table:", #empty)
print("Expected: 0")

-- Test 3: Table with gaps
local gaps = {10, 20, nil, 40}
print("Length of table with gaps:", #gaps)
print("Expected: 2")

print("=== Debug End ===")
