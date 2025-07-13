-- Debug using rawequal to check object identity
print("=== Debug Using rawequal ===")

-- Test 1: Basic function identity
print("\n--- Test 1: Basic function identity ---")
local func = function() return "test" end
local func_copy = func

print("func == func_copy:", func == func_copy)
print("rawequal(func, func_copy):", rawequal(func, func_copy))

-- Test 2: Table storage and retrieval
print("\n--- Test 2: Table storage and retrieval ---")
local t = {}
t.f = func

print("func == t.f:", func == t.f)
print("rawequal(func, t.f):", rawequal(func, t.f))

-- Test 3: Multiple table accesses
print("\n--- Test 3: Multiple table accesses ---")
local access1 = t.f
local access2 = t.f

print("access1 == access2:", access1 == access2)
print("rawequal(access1, access2):", rawequal(access1, access2))

-- Test 4: String vs dot access
print("\n--- Test 4: String vs dot access ---")
local dot_access = t.f
local string_access = t["f"]

print("dot_access == string_access:", dot_access == string_access)
print("rawequal(dot_access, string_access):", rawequal(dot_access, string_access))

print("\n=== Debug completed ===")
