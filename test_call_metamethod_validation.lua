-- Validation test for existing __call metamethod functionality
print("=== __call Metamethod Validation Test ===")

-- Test 1: Basic __call metamethod
print("\n--- Test 1: Basic __call metamethod ---")
local obj1 = {value = 10}
setmetatable(obj1, {
    __call = function(self, x)
        print("__call called with x =", x)
        return self.value + x
    end
})

print("Testing: obj1(5)")
local result1 = obj1(5)
print("Result:", result1)
print("Expected: 15 (10 + 5)")

-- Test 2: __call with multiple arguments
print("\n--- Test 2: __call with multiple arguments ---")
local obj2 = {name = "Calculator"}
setmetatable(obj2, {
    __call = function(self, a, b, op)
        print("Calculator called with", a, op, b)
        if op == "+" then
            return a + b
        elseif op == "*" then
            return a * b
        else
            return 0
        end
    end
})

print("Testing: obj2(3, 4, '+')")
local result2 = obj2(3, 4, "+")
print("Result:", result2)
print("Expected: 7")

print("Testing: obj2(3, 4, '*')")
local result3 = obj2(3, 4, "*")
print("Result:", result3)
print("Expected: 12")

-- Test 3: __call with no arguments
print("\n--- Test 3: __call with no arguments ---")
local obj3 = {counter = 0}
setmetatable(obj3, {
    __call = function(self)
        self.counter = self.counter + 1
        print("Counter incremented to", self.counter)
        return self.counter
    end
})

print("Testing: obj3()")
local result4 = obj3()
print("Result:", result4)
print("Expected: 1")

print("Testing: obj3() again")
local result5 = obj3()
print("Result:", result5)
print("Expected: 2")

-- Test 4: __call returning multiple values (single assignment)
print("\n--- Test 4: __call returning multiple values (single assignment) ---")
local obj4 = {}
setmetatable(obj4, {
    __call = function(self, x)
        print("__call returning multiple values")
        return x, x * 2, x * 3
    end
})

print("Testing: local single = obj4(7)")
local single = obj4(7)
print("Result:", single)
print("Expected: 7 (first return value)")

print("\n=== Validation completed ===")
