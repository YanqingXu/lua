-- Generic for-loop tests
-- Tests for iterators like pairs and ipairs

print("=== Generic for-loop tests ===")

-- Test 1: Iterate array with ipairs
print("Test 1: Iterate array with ipairs")
local array = {10, 20, 30, 40, 50}
for i, v in ipairs(array) do
    print("  array[" .. i .. "] =", v)
end

-- Test 2: Iterate table with pairs
print("\nTest 2: Iterate table with pairs")
local table1 = {
    name = "Zhang San",
    age = 25,
    city = "Beijing"
}
for k, v in pairs(table1) do
    print("  " .. k .. " =", v)
end

-- Test 3: Iterate mixed-index table
print("\nTest 3: Iterate mixed-index table")
local mixedTable = {
    "First",
    "Second",
    name = "Test",
    [5] = "Fifth"
}

print("Using ipairs:")
for i, v in ipairs(mixedTable) do
    print("  [" .. i .. "] =", v)
end

print("Using pairs:")
for k, v in pairs(mixedTable) do
    print("  [" .. tostring(k) .. "] =", v)
end

-- Test 4: Iterate empty table
print("\nTest 4: Iterate empty table")
local emptyTable = {}
local count = 0
for k, v in pairs(emptyTable) do
    count = count + 1
    print("  This should not be printed")
end
if count == 0 then
    print("✓ Empty table correctly did not iterate")
else
    print("✗ Empty table iterated unexpectedly")
end

-- Test 5: Nested table iteration
print("\nTest 5: Nested table iteration")
local nestedTable = {
    group1 = {a = 1, b = 2},
    group2 = {c = 3, d = 4}
}
for groupName, group in pairs(nestedTable) do
    print("  Group:", groupName)
    for key, value in pairs(group) do
        print("    " .. key .. " =", value)
    end
end

-- Test 6: Nil values in arrays
print("\nTest 6: Nil values in arrays")
local arrayWithNil = {1, 2, nil, 4, 5}
print("Using ipairs (stops at nil):")
for i, v in ipairs(arrayWithNil) do
    print("  [" .. i .. "] =", v)
end

print("Using pairs (skips nil):")
for k, v in pairs(arrayWithNil) do
    print("  [" .. k .. "] =", v)
end

-- Test 7: Table with string keys
print("\nTest 7: Table with string keys")
local stringKeyTable = {
    ["first name"] = "Zhang",
    ["last name"] = "San",
    ["full name"] = "Zhang San"
}
for k, v in pairs(stringKeyTable) do
    print("  '" .. k .. "' =", v)
end

-- Test 8: Table with numeric keys (non-contiguous)
print("\nTest 8: Table with numeric keys (non-contiguous)")
local sparseArray = {
    [1] = "One",
    [3] = "Three",
    [5] = "Five",
    [10] = "Ten"
}

print("Using ipairs:")
for i, v in ipairs(sparseArray) do
    print("  [" .. i .. "] =", v)
end

print("Using pairs:")
for k, v in pairs(sparseArray) do
    print("  [" .. k .. "] =", v)
end

print("\n=== Generic for-loop tests completed ===")
