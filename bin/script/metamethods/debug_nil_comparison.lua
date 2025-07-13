-- Debug nil comparison issue
print("=== Debug Nil Comparison Issue ===")

-- Create a simple table
local t = {
    __call = function() return "test" end
}

print("\n--- Step 1: Direct value inspection ---")
local val1 = t["__call"]
local val2 = t.__call

print("val1 (t['__call']):", val1)
print("val1 type:", type(val1))
print("val1 == nil:", val1 == nil)
print("val1 ~= nil:", val1 ~= nil)

print("val2 (t.__call):", val2)
print("val2 type:", type(val2))
print("val2 == nil:", val2 == nil)
print("val2 ~= nil:", val2 ~= nil)

print("\n--- Step 2: Value comparison ---")
print("val1 == val2:", val1 == val2)
print("val1 ~= val2:", val1 ~= val2)

print("\n--- Step 3: Nil value creation ---")
local nilval = nil
print("nilval:", nilval)
print("nilval type:", type(nilval))
print("nilval == nil:", nilval == nil)
print("nilval ~= nil:", nilval ~= nil)

print("\n--- Step 4: Function comparison with nil ---")
local func = function() return "test" end
print("func:", func)
print("func type:", type(func))
print("func == nil:", func == nil)
print("func ~= nil:", func ~= nil)

print("\n=== Debug completed ===")
