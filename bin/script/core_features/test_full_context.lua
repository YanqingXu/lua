-- Test in full context like test.lua

-- Same global variables
local PROGRAM_NAME = "Lua Calculator & Demo"
local VERSION = "1.0"
local testNumbers = {1, 5, 3, 9, 2, 8, 4}
local userName = "Lua User"
local calculations = {}

-- Same functions as test.lua
function add(x, y)
    return x + y
end

function multiply(x, y)
    return x * y
end

function power(x, y)
    return x ^ y
end

function factorial(n)
    if n <= 1 then
        return 1
    else
        local result = 1
        for i = 2, n do
            result = result * i
        end
        return result
    end
end

function fibonacci(n)
    if n <= 1 then
        return n
    else
        local fib_a = 0
        local fib_b = 1
        for i = 2, n do
            local temp = fib_a + fib_b
            fib_a = fib_b
            fib_b = temp
        end
        return fib_b
    end
end

function findMax(numbers)
    local max = numbers[1]
    for i = 2, #numbers do
        if numbers[i] > max then
            max = numbers[i]
        end
    end
    return max
end

-- Test in the same way as test.lua
function mathDemo()
    print("Math Calculation Demo:")
    print("")

    local a = 10
    local b = 3
    print("Basic Operations (a=" .. a .. ", b=" .. b .. "):")
    print("  Addition: " .. a .. " + " .. b .. " = " .. add(a, b))
    print("  Multiplication: " .. a .. " * " .. b .. " = " .. multiply(a, b))
    print("  Power: " .. a .. " ^ " .. b .. " = " .. power(a, b))
    print("")

    print("Sequence Calculations:")
    for i = 1, 8 do
        print("  Factorial " .. i .. "! = " .. factorial(i))
    end
    print("")

    for i = 1, 10 do
        print("  Fibonacci F(" .. i .. ") = " .. fibonacci(i))
    end
    print("")
end

-- Run the test
mathDemo()
