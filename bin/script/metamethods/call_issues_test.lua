-- Test specific __call metamethod issues
print("=== __call Metamethod Issues Test ===")

-- Issue 1: Multiple return values
print("\n--- Issue 1: Multiple return values ---")
local multi_return = {}
setmetatable(multi_return, {
    __call = function(self, x)
        print("Returning multiple values:", x, x*2, x*3)
        return x, x*2, x*3
    end
})

print("Calling multi_return(5)...")
local a, b, c = multi_return(5)
print("Received values:")
print("  a =", a)
print("  b =", b) 
print("  c =", c)
print("Expected: a=5, b=10, c=15")

-- Issue 2: Error handling for non-callable objects
print("\n--- Issue 2: Error handling ---")
local non_callable = {value = 42}

print("Attempting to call non-callable object...")
local success, error_msg = pcall(function()
    return non_callable()
end)

print("Call success:", success)
print("Error message:", error_msg)

-- Issue 3: Self parameter access
print("\n--- Issue 3: Self parameter access ---")
local self_test = {name = "TestObject", value = 100}
setmetatable(self_test, {
    __call = function(self, operation)
        print("Self name:", self.name)
        print("Self value:", self.value)
        if operation == "double" then
            return self.value * 2
        else
            return self.value
        end
    end
})

print("Calling self_test('double')...")
local result = self_test("double")
print("Result:", result)
print("Expected: 200")

-- Issue 4: Zero arguments
print("\n--- Issue 4: Zero arguments ---")
local no_args = {}
setmetatable(no_args, {
    __call = function(self)
        print("Called with no arguments")
        return "no_args_result"
    end
})

print("Calling no_args()...")
local no_args_result = no_args()
print("Result:", no_args_result)

print("\n=== Test completed ===")
