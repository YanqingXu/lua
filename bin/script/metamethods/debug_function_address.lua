-- Debug function object addresses using tostring
print("=== Debug Function Object Addresses ===")

-- Test 1: Function creation and assignment
print("\n--- Test 1: Function creation ---")
local func = function() return "test" end
print("func address:", tostring(func))

local func_copy = func
print("func_copy address:", tostring(func_copy))

-- Test 2: Table storage
print("\n--- Test 2: Table storage ---")
local t = {}
t.f = func
print("t.f address:", tostring(t.f))

-- Test 3: Multiple accesses
print("\n--- Test 3: Multiple accesses ---")
local access1 = t.f
local access2 = t.f
print("access1 address:", tostring(access1))
print("access2 address:", tostring(access2))

-- Test 4: String vs dot access
print("\n--- Test 4: String vs dot access ---")
local dot_access = t.f
local string_access = t["f"]
print("dot_access address:", tostring(dot_access))
print("string_access address:", tostring(string_access))

print("\n=== Debug completed ===")
