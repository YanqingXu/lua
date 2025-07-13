-- Debug Value type checking
print("=== Debug Value Type ===")

-- Test 1: Function type consistency
print("\n--- Test 1: Function type consistency ---")
local func = function() return "test" end
local func_copy = func

print("func type:", type(func))
print("func_copy type:", type(func_copy))
print("Types equal:", type(func) == type(func_copy))

-- Test 2: Table storage type consistency
print("\n--- Test 2: Table storage type consistency ---")
local t = {}
t.f = func

print("t.f type:", type(t.f))
print("func type == t.f type:", type(func) == type(t.f))

-- Test 3: Nil comparison behavior
print("\n--- Test 3: Nil comparison behavior ---")
print("func == nil:", func == nil)
print("func ~= nil:", func ~= nil)
print("t.f == nil:", t.f == nil)
print("t.f ~= nil:", t.f ~= nil)

-- Test 4: Direct nil value
print("\n--- Test 4: Direct nil value ---")
local nilval = nil
print("nilval == nil:", nilval == nil)
print("nilval ~= nil:", nilval ~= nil)

print("\n=== Debug completed ===")
