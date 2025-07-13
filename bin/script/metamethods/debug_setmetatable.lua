-- Debug setmetatable behavior
print("=== Debug setmetatable ===")

-- Test 1: Direct metatable creation
print("\n--- Test 1: Direct metatable ---")
local mt1 = {
    __call = function() return "direct" end
}
print("mt1.__call ~= nil:", mt1.__call ~= nil)

-- Test 2: setmetatable
print("\n--- Test 2: setmetatable ---")
local obj = {}
local mt2 = {
    __call = function() return "setmeta" end
}
setmetatable(obj, mt2)

local retrieved_mt = getmetatable(obj)
print("retrieved_mt.__call ~= nil:", retrieved_mt.__call ~= nil)
print("mt2.__call ~= nil:", mt2.__call ~= nil)
print("mt2 == retrieved_mt:", mt2 == retrieved_mt)

-- Test 3: Inline setmetatable
print("\n--- Test 3: Inline setmetatable ---")
local counter = {count = 0}
setmetatable(counter, {
    __call = function(self, increment)
        return "called"
    end
})

local mt3 = getmetatable(counter)
print("mt3.__call ~= nil:", mt3.__call ~= nil)

print("\n=== Debug completed ===")
