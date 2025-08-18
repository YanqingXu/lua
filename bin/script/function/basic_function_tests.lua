-- Basic function tests
-- Test basic features like function definition, calling, and parameter passing

print("=== Basic function tests ===")

-- Test 1: Simple function definition and call
print("Test 1: Simple function definition and call")
function greet()
    return "Hello, World!"
end

local message = greet()
print("  greet() =", message)

-- Test 2: Function with parameters
print("\nTest 2: Function with parameters")
function add(a, b)
    return a + b
end

local sum = add(3, 5)
print("  add(3, 5) =", sum)

-- Test 3: Function with multiple parameters
print("\nTest 3: Function with multiple parameters")
function multiply(a, b, c)
    return a * b * c
end

local product = multiply(2, 3, 4)
print("  multiply(2, 3, 4) =", product)

-- Test 4: Variadic function
print("\nTest 4: Variadic function")
function sum(...)
    local args = {...}
    local total = 0
    for i = 1, #args do
        total = total + args[i]
    end
    return total
end

local result1 = sum(1, 2, 3)
local result2 = sum(1, 2, 3, 4, 5)
print("  sum(1, 2, 3) =", result1)
print("  sum(1, 2, 3, 4, 5) =", result2)

-- Test 5: Multiple return values
print("\nTest 5: Multiple return values")
function divmod(a, b)
    return a // b, a % b
end

local quotient, remainder = divmod(17, 5)
print("  divmod(17, 5) =", quotient, remainder)

-- Test 6: Local function
print("\nTest 6: Local function")
local function localFunction(x)
    return x * x
end

local square = localFunction(7)
print("  localFunction(7) =", square)

-- Test 7: Anonymous function
print("\nTest 7: Anonymous function")
local anonymous = function(x, y)
    return x + y
end

local result3 = anonymous(10, 20)
print("  anonymous(10, 20) =", result3)

-- Test 8: Function as parameter
print("\nTest 8: Function as parameter")
function applyFunction(func, x, y)
    return func(x, y)
end

local result4 = applyFunction(add, 15, 25)
print("  applyFunction(add, 15, 25) =", result4)

-- Test 9: Function as return value
print("\nTest 9: Function as return value")
function createMultiplier(factor)
    return function(x)
        return x * factor
    end
end

local double = createMultiplier(2)
local triple = createMultiplier(3)
print("  double(5) =", double(5))
print("  triple(5) =", triple(5))

-- Test 10: Recursive function
print("\nTest 10: Recursive function")
function factorial(n)
    if n <= 1 then
        return 1
    else
        return n * factorial(n - 1)
    end
end

local fact5 = factorial(5)
print("  factorial(5) =", fact5)

-- Test 11: Tail recursion optimization test
print("\nTest 11: Tail recursive function")
function tailFactorial(n, acc)
    acc = acc or 1
    if n <= 1 then
        return acc
    else
        return tailFactorial(n - 1, n * acc)
    end
end

local tailFact5 = tailFactorial(5)
print("  tailFactorial(5) =", tailFact5)

-- Test 12: Simulating default function parameters
print("\nTest 12: Simulating default function parameters")
function greetWithDefault(name, greeting)
    name = name or "World"
    greeting = greeting or "Hello"
    return greeting .. ", " .. name .. "!"
end

print("  greetWithDefault() =", greetWithDefault())
print("  greetWithDefault('Alice') =", greetWithDefault("Alice"))
print("  greetWithDefault('Bob', 'Hi') =", greetWithDefault("Bob", "Hi"))

print("\n=== Basic function tests completed ===")
