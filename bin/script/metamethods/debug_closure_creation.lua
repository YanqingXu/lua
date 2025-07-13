-- Debug closure creation and identity
print("=== Debug Closure Creation ===")

-- Test 1: Function identity in table constructor
print("\n--- Test 1: Function identity in table constructor ---")
local func = function() return "test" end
local t1 = {
    __call = func
}
local t2 = {
    __call = func  -- Same function reference
}

print("func:", func)
print("t1.__call:", t1.__call)
print("t1['__call']:", t1["__call"])
print("t2.__call:", t2.__call)
print("func == t1.__call:", func == t1.__call)
print("func == t1['__call']:", func == t1["__call"])
print("t1.__call == t1['__call']:", t1.__call == t1["__call"])
print("t1.__call == t2.__call:", t1.__call == t2.__call)

-- Test 2: Inline function in table constructor
print("\n--- Test 2: Inline function in table constructor ---")
local t3 = {
    __call = function() return "inline" end
}

print("t3.__call:", t3.__call)
print("t3['__call']:", t3["__call"])
print("t3.__call == t3['__call']:", t3.__call == t3["__call"])

-- Test 3: Multiple inline functions (should be different)
print("\n--- Test 3: Multiple inline functions ---")
local f1 = function() return "f1" end
local f2 = function() return "f2" end
print("f1:", f1)
print("f2:", f2)
print("f1 == f2:", f1 == f2)  -- Should be false

print("\n=== Debug completed ===")
