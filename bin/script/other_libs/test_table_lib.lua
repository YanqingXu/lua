-- Table Library Function Tests
-- Testing table manipulation functions

print("=== Table Library Tests ===")

-- Test table.insert
local t = {1, 2, 3}
print("Original table length:", #t)

-- Try table.insert if available
if table.insert then
    table.insert(t, 4)
    print("After table.insert(t, 4), length:", #t)
    print("t[4] =", t[4])
else
    print("table.insert is nil")
end

-- Try table.remove if available
if table.remove then
    local removed = table.remove(t)
    print("table.remove(t) returned:", removed)
    print("After table.remove, length:", #t)
else
    print("table.remove is nil")
end

-- Try table.concat if available
if table.concat then
    local str = table.concat(t, ", ")
    print("table.concat(t, ', ') =", str)
else
    print("table.concat is nil")
end

-- Try table.sort if available
if table.sort then
    local unsorted = {3, 1, 4, 1, 5}
    table.sort(unsorted)
    print("After table.sort:", table.concat and table.concat(unsorted, ", ") or "concat not available")
else
    print("table.sort is nil")
end

print("=== Table Library Tests Complete ===")
