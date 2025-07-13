-- Simple __call Metamethod Validation Test
print("=== Simple __call Metamethod Validation ===")

-- Test 1: Basic __call functionality
print("\n--- Test 1: Basic __call functionality ---")
local counter = {count = 0}
setmetatable(counter, {
    __call = function(self, increment)
        increment = increment or 1
        self.count = self.count + increment
        return "Count: " .. self.count
    end
})

print("Initial count:", counter.count)
local result1 = counter(5)
print("After counter(5):", result1)
print("Counter.count:", counter.count)

local result2 = counter(3)
print("After counter(3):", result2)
print("Counter.count:", counter.count)

-- Test 2: __call metamethod visibility
print("\n--- Test 2: __call metamethod visibility ---")
local mt = getmetatable(counter)
print("Metatable exists:", mt ~= nil)
if mt then
    print("__call exists in metatable:", mt.__call ~= nil)
    print("__call is function:", type(mt.__call) == "function")
end

-- Test 3: Error handling
print("\n--- Test 3: Error handling ---")
local non_callable = {value = 42}
local success, error_msg = pcall(function()
    return non_callable()
end)

if success then
    print("ERROR: Should have failed!")
else
    print("Correctly failed with error:", error_msg)
end

-- Test 4: Multiple arguments
print("\n--- Test 4: Multiple arguments ---")
local calculator = {}
setmetatable(calculator, {
    __call = function(self, a, b)
        return a + b
    end
})

local sum = calculator(10, 5)
print("Calculator(10, 5):", sum)

print("\n=== Simple validation completed ===")
