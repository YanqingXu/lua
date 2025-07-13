-- Debug metamethod visibility issue
print("=== Debug Metamethod Visibility Issue ===")

-- Create a simple object with __call metamethod
local obj = {}
local mt = {
    __call = function(self, x)
        return "Called with: " .. tostring(x)
    end
}

print("\n--- Step 1: Before setmetatable ---")
print("obj metatable:", getmetatable(obj))

setmetatable(obj, mt)

print("\n--- Step 2: After setmetatable ---")
local retrieved_mt = getmetatable(obj)
print("obj metatable:", retrieved_mt)
print("mt == retrieved_mt:", mt == retrieved_mt)

print("\n--- Step 3: Direct metamethod access ---")
print("mt.__call:", mt.__call)
print("mt.__call type:", type(mt.__call))
print("mt.__call ~= nil:", mt.__call ~= nil)

print("\n--- Step 4: Retrieved metamethod access ---")
print("retrieved_mt.__call:", retrieved_mt.__call)
print("retrieved_mt.__call type:", type(retrieved_mt.__call))
print("retrieved_mt.__call ~= nil:", retrieved_mt.__call ~= nil)

print("\n--- Step 5: String key access ---")
print("retrieved_mt['__call']:", retrieved_mt["__call"])
print("retrieved_mt['__call'] type:", type(retrieved_mt["__call"]))
print("retrieved_mt['__call'] ~= nil:", retrieved_mt["__call"] ~= nil)

print("\n--- Step 6: Function call test ---")
local result = obj(42)
print("obj(42) result:", result)

print("\n--- Step 7: Metamethod comparison ---")
print("mt.__call == retrieved_mt.__call:", mt.__call == retrieved_mt.__call)

print("\n=== Debug completed ===")
