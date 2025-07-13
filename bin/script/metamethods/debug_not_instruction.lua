-- Debug NOT instruction behavior
print("=== Debug NOT Instruction ===")

-- Test 1: Basic NOT operation
print("\n--- Test 1: Basic NOT operation ---")
local func = function() return "test" end

print("func:", func)
print("func == nil:", func == nil)
print("not (func == nil):", not (func == nil))

-- Test 2: Direct boolean values
print("\n--- Test 2: Direct boolean values ---")
print("not true:", not true)
print("not false:", not false)

-- Test 3: NOT with different types
print("\n--- Test 3: NOT with different types ---")
print("not nil:", not nil)
print("not 42:", not 42)
print("not 'hello':", not "hello")

-- Test 4: Comparison chain
print("\n--- Test 4: Comparison chain ---")
local t = { f = func }
print("t.f:", t.f)
print("t.f == nil:", t.f == nil)
print("not (t.f == nil):", not (t.f == nil))

print("\n=== Debug completed ===")
