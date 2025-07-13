-- Debug Value identity and copying
print("=== Debug Value Identity ===")

-- Test 1: Direct assignment
print("\n--- Test 1: Direct assignment ---")
local func = function() return "test" end
local func_copy = func

print("func:", func)
print("func_copy:", func_copy)
print("func == func_copy:", func == func_copy)

-- Test 2: Table storage and retrieval
print("\n--- Test 2: Table storage and retrieval ---")
local t = {}
t.f = func

print("Original func:", func)
print("t.f:", t.f)
print("func == t.f:", func == t.f)

-- Test 3: Multiple retrievals from same table
print("\n--- Test 3: Multiple retrievals ---")
local val1 = t.f
local val2 = t.f

print("val1:", val1)
print("val2:", val2)
print("val1 == val2:", val1 == val2)
print("val1 == func:", val1 == func)

print("\n=== Debug completed ===")
