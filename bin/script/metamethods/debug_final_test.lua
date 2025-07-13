-- Final test for metamethod visibility
print("=== Final Metamethod Visibility Test ===")

-- Create object with __call metamethod
local obj = {}
setmetatable(obj, {
    __call = function(self, x)
        return "Called with: " .. tostring(x)
    end
})

print("\n--- Test 1: Function call works ---")
local result = obj(42)
print("obj(42):", result)

print("\n--- Test 2: Metamethod access comparison ---")
local mt = getmetatable(obj)

-- String access
local string_access = mt["__call"]
print("mt['__call'] ~= nil:", string_access ~= nil)

-- Dot access  
local dot_access = mt.__call
print("mt.__call ~= nil:", dot_access ~= nil)

-- Direct comparison
print("string_access == dot_access:", string_access == dot_access)
print("rawequal(string_access, dot_access):", rawequal(string_access, dot_access))

print("\n--- Test 3: Nil comparison details ---")
print("string_access == nil:", string_access == nil)
print("dot_access == nil:", dot_access == nil)

print("\n=== Test completed ===")
