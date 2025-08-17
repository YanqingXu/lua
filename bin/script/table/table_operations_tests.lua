-- Advanced table operation tests
-- Tests for advanced operations and edge cases of tables

print("=== Advanced table operation tests ===")

-- Test 1: Table copy (shallow copy)
print("Test 1: Shallow copy of table")
local original = {a = 1, b = 2, c = {nested = 3}}
local copy = {}
for k, v in pairs(original) do
    copy[k] = v
end
print("  original.a =", original.a)
print("  copy.a =", copy.a)
copy.a = 10
print("  After modifying copy:")
print("  original.a =", original.a)
print("  copy.a =", copy.a)

-- Test 2: Table comparison
print("\nTest 2: Table comparison")
local table1 = {a = 1, b = 2}
local table2 = {a = 1, b = 2}
local table3 = table1

print("  table1 == table2:", table1 == table2)  -- false, different table objects
print("  table1 == table3:", table1 == table3)  -- true, same table object

-- Test 3: Table as keys
print("\nTest 3: Table as keys")
local keyTable = {x = 1}
local valueTable = {y = 2}
local tableWithTableKey = {}
tableWithTableKey[keyTable] = valueTable
print("  Using table as key set successfully")
print("  tableWithTableKey[keyTable].y =", tableWithTableKey[keyTable].y)

-- Test 4: Functions as table values
print("\nTest 4: Functions as table values")
local functionTable = {
    add = function(a, b) return a + b end,
    multiply = function(a, b) return a * b end
}
print("  functionTable.add(3, 4) =", functionTable.add(3, 4))
print("  functionTable.multiply(3, 4) =", functionTable.multiply(3, 4))

-- Test 5: Dynamic expansion of table
print("\nTest 5: Dynamic expansion of table")
local expandable = {}
for i = 1, 5 do
    expandable[i] = i * i
end
print("  Table after dynamic addition:")
for i = 1, 5 do
    print("    expandable[" .. i .. "] =", expandable[i])
end

-- Test 6: Sparse array
print("\nTest 6: Sparse array")
local sparse = {}
sparse[1] = "First"
sparse[100] = "One Hundredth"
sparse[1000] = "One Thousandth"
print("  sparse[1] =", sparse[1])
print("  sparse[100] =", sparse[100])
print("  sparse[1000] =", sparse[1000])
print("  #sparse =", #sparse)

-- Test 7: Sequence part and hash part of table
print("\nTest 7: Sequence part and hash part of table")
local hybrid = {
    "Seq 1",  -- [1]
    "Seq 2",  -- [2]
    "Seq 3",  -- [3]
    name = "Hash key",
    [10] = "Jump index",
    age = 25
}
print("  Sequence part length: #hybrid =", #hybrid)
print("  hybrid[1] =", hybrid[1])
print("  hybrid.name =", hybrid.name)
print("  hybrid[10] =", hybrid[10])

-- Test 8: Iteration order of table
print("\nTest 8: Iteration order of table")
local orderTest = {
    c = 3,
    a = 1,
    b = 2,
    [1] = "Number 1",
    [2] = "Number 2"
}
print("  pairs iteration order:")
for k, v in pairs(orderTest) do
    print("    " .. tostring(k) .. " =", v)
end

-- Test 9: Estimating table size
print("\nTest 9: Estimating table size")
local sizeTest = {}
local count = 0
-- Add some elements
for i = 1, 10 do
    sizeTest[i] = i
    count = count + 1
end
sizeTest.name = "Test"
count = count + 1
sizeTest.value = 100
count = count + 1

print("  Manually counted elements:", count)
local pairsCount = 0
for k, v in pairs(sizeTest) do
    pairsCount = pairsCount + 1
end
print("  Elements counted by pairs:", pairsCount)

-- Test 10: Handling nil values in table
print("\nTest 10: Handling nil values in table")
local nilTest = {a = 1, b = 2, c = 3}
print("  Before: nilTest.b =", nilTest.b)
nilTest.b = nil
print("  After setting to nil: nilTest.b =", tostring(nilTest.b))

-- Verify that the nil value was indeed removed
local foundB = false
for k, v in pairs(nilTest) do
    if k == "b" then
        foundB = true
    end
end
if not foundB then
    print("✓ nil value correctly removed from table")
else
    print("✗ nil value not removed from table")
end

print("\n=== Advanced table operation tests completed ===")
