print("=== Closure Test (Task 2.4) ===")

-- Test 1: Basic closure
print("\nTest 1: Basic closure")
local function make_counter()
    local count = 0
    return function()
        count = count + 1
        return count
    end
end

local counter1 = make_counter()
print("  counter1() =", counter1())
print("  counter1() =", counter1())
print("  counter1() =", counter1())

-- Test 2: Multiple closures with separate state
print("\nTest 2: Multiple closures")
local counter2 = make_counter()
print("  counter2() =", counter2())
print("  counter1() =", counter1())
print("  counter2() =", counter2())

-- Test 3: Closure with parameters
print("\nTest 3: Closure with parameters")
local function make_adder(x)
    return function(y)
        return x + y
    end
end

local add5 = make_adder(5)
local add10 = make_adder(10)
print("  add5(3) =", add5(3))
print("  add10(3) =", add10(3))

-- Test 4: Nested closures
print("\nTest 4: Nested closures")
local function outer(a)
    return function(b)
        return function(c)
            return a + b + c
        end
    end
end

local f1 = outer(1)
local f2 = f1(2)
print("  outer(1)(2)(3) =", f2(3))

-- Test 5: Closure modifying upvalue
print("\nTest 5: Modifying upvalue")
local function make_accumulator(initial)
    local sum = initial or 0
    return function(value)
        sum = sum + value
        return sum
    end
end

local acc = make_accumulator(10)
print("  acc(5) =", acc(5))
print("  acc(3) =", acc(3))
print("  acc(2) =", acc(2))

-- Test 6: Closure accessing multiple upvalues
print("\nTest 6: Multiple upvalues")
local function make_calculator(x, y)
    return {
        add = function() return x + y end,
        sub = function() return x - y end,
        mul = function() return x * y end
    }
end

local calc = make_calculator(20, 5)
print("  calc.add() =", calc.add())
print("  calc.sub() =", calc.sub())
print("  calc.mul() =", calc.mul())

print("\n=== Closure Test Complete ===")
