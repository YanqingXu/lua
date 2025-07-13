-- Complete __call Metamethod Test
-- Tests all aspects of __call metamethod implementation including edge cases

print("=== Complete __call Metamethod Test ===")

-- Test 1: Basic __call functionality
print("\n--- Test 1: Basic __call functionality ---")
local counter = {count = 0}
setmetatable(counter, {
    __call = function(self, increment)
        increment = increment or 1
        self.count = self.count + increment
        return "Counter called, count is now: " .. self.count
    end
})

print("Initial count:", counter.count)
local result1 = counter(5)
print("Result 1:", result1)
print("Count after first call:", counter.count)

local result2 = counter(3)
print("Result 2:", result2)
print("Count after second call:", counter.count)

-- Test 2: __call metamethod visibility
print("\n--- Test 2: __call metamethod visibility ---")
local mt = getmetatable(counter)
print("Metatable exists:", mt ~= nil)
print("__call exists in metatable:", mt.__call ~= nil)
print("__call is function:", type(mt.__call) == "function")

-- Test 3: Multiple arguments
print("\n--- Test 3: Multiple arguments ---")
local calculator = {}
setmetatable(calculator, {
    __call = function(self, operation, a, b)
        if operation == "add" then
            return a + b
        elseif operation == "multiply" then
            return a * b
        elseif operation == "concat" then
            return tostring(a) .. " " .. tostring(b)
        else
            return "Unknown operation: " .. tostring(operation)
        end
    end
})

print("Add 10 + 5:", calculator("add", 10, 5))
print("Multiply 7 * 3:", calculator("multiply", 7, 3))
print("Concat 'Hello' 'World':", calculator("concat", "Hello", "World"))
print("Unknown operation:", calculator("unknown", 1, 2))

-- Test 4: No arguments
print("\n--- Test 4: No arguments ---")
local greeter = {}
setmetatable(greeter, {
    __call = function(self)
        return "Hello from callable object!"
    end
})

print("Greeter result:", greeter())

-- Test 5: Nested calls
print("\n--- Test 5: Nested calls ---")
local nested = {}
setmetatable(nested, {
    __call = function(self, func, ...)
        if type(func) == "function" then
            return func(...)
        else
            return "Not a function: " .. tostring(func)
        end
    end
})

local function double(x)
    return x * 2
end

print("Nested call result:", nested(double, 21))
print("Nested call with non-function:", nested("not a function", 42))

-- Test 6: State preservation
print("\n--- Test 6: State preservation ---")
local stateful = {
    data = {},
    index = 0
}
setmetatable(stateful, {
    __call = function(self, value)
        self.index = self.index + 1
        self.data[self.index] = value
        return "Stored '" .. tostring(value) .. "' at index " .. self.index
    end
})

print("Store 'first':", stateful("first"))
print("Store 'second':", stateful("second"))
print("Store 42:", stateful(42))
print("Current index:", stateful.index)
print("Data at index 1:", stateful.data[1])
print("Data at index 2:", stateful.data[2])
print("Data at index 3:", stateful.data[3])

-- Test 7: Error handling
print("\n--- Test 7: Error handling ---")
local function test_call_error()
    local non_callable = {value = 42}
    -- This should fail because there's no __call metamethod
    local success, error_msg = pcall(function()
        return non_callable()
    end)
    
    if success then
        print("ERROR: Should have failed!")
    else
        print("Correctly failed with error:", error_msg)
    end
end

test_call_error()

-- Test 8: Return value handling
print("\n--- Test 8: Return value handling ---")
local multi_return = {}
setmetatable(multi_return, {
    __call = function(self, count)
        if count == 1 then
            return "single"
        elseif count == 2 then
            return "first", "second"  -- Note: Lua functions can return multiple values
        else
            return nil
        end
    end
})

local single = multi_return(1)
print("Single return:", single)

local first, second = multi_return(2)
print("Multiple return - first:", first, "second:", second)

local nil_result = multi_return(0)
print("Nil return:", nil_result)

-- Test 9: Recursive __call
print("\n--- Test 9: Recursive __call ---")
local recursive = {depth = 0}
setmetatable(recursive, {
    __call = function(self, max_depth)
        self.depth = self.depth + 1
        if self.depth >= max_depth then
            local result = "Reached depth " .. self.depth
            self.depth = 0  -- Reset for next test
            return result
        else
            return self(max_depth)  -- Recursive call
        end
    end
})

print("Recursive result:", recursive(3))

print("\n=== All __call metamethod tests completed ===")
