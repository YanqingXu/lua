-- Modern C++ Lua Interpreter Test File
-- ============================================
-- Complete Lua Program Example
-- Based on currently supported syntax features
-- ============================================

-- Program Configuration
local PROGRAM_NAME = "Lua Calculator & Demo"
local VERSION = "1.0"

-- ============================================
-- Utility Function Definitions
-- ============================================

-- Mathematical calculation functions
function add(a, b)
    return a + b
end

function multiply(a, b)
    return a * b
end

function power(base, exp)
    return base ^ exp
end

-- Check if a number is even
function isEven(n)
    if n % 2 == 0 then
        return true
    else
        return false
    end
end

-- Calculate factorial
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

-- Calculate Fibonacci sequence
function fibonacci(n)
    if n <= 1 then
        return n
    else
        local a = 0
        local b = 1
        for i = 2, n do
            local temp = a + b
            a = b
            b = temp
        end
        return b
    end
end

-- Array processing functions
function findMax(numbers)
    local max = numbers[1]
    for i = 2, #numbers do
        if numbers[i] > max then
            max = numbers[i]
        end
    end
    return max
end

function sumArray(numbers)
    local sum = 0
    for i = 1, #numbers do
        sum = sum + numbers[i]
    end
    return sum
end

-- String processing functions
function greet(name)
    return "Hello, " .. name .. "!"
end

function repeatString(str, count)
    local result = ""
    for i = 1, count do
        result = result .. str
    end
    return result
end

-- ============================================
-- Data Definitions
-- ============================================

-- Test data
local testNumbers = {1, 5, 3, 9, 2, 8, 4}
local userName = "Lua User"
local calculations = {}

-- ============================================
-- Main Program Logic
-- ============================================

-- Program initialization
function initProgram()
    print("=" .. repeatString("=", 40) .. "=")
    print(" " .. PROGRAM_NAME .. " v" .. VERSION)
    print("=" .. repeatString("=", 40) .. "=")
    print("")
end

-- Math demonstration
function mathDemo()
    print("Math Calculation Demo:")
    print("")

    -- Basic operations
    local a = 10
    local b = 3
    print("Basic Operations (a=" .. a .. ", b=" .. b .. "):")
    print("  Addition: " .. a .. " + " .. b .. " = " .. add(a, b))
    print("  Multiplication: " .. a .. " * " .. b .. " = " .. multiply(a, b))
    print("  Power: " .. a .. " ^ " .. b .. " = " .. power(a, b))
    print("")

    -- Sequence calculations
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

-- Data processing demonstration
function dataDemo()
    print("Data Processing Demo:")
    print("")

    -- Array operations
    print("Test Array: {1, 5, 3, 9, 2, 8, 4}")
    print("  Array Length: " .. #testNumbers)
    print("  Maximum Value: " .. findMax(testNumbers))
    print("  Sum: " .. sumArray(testNumbers))
    print("")

    -- String operations
    print("String Operations:")
    print("  " .. greet(userName))
    print("  Repeated String: " .. repeatString("*", 5))
    print("")

    -- Conditional demonstration
    print("Conditional Demo:")
    for i = 1, 10 do
        if isEven(i) then
            print("  " .. i .. " is even")
        else
            print("  " .. i .. " is odd")
        end
    end
    print("")
end

-- Loop demonstration
function loopDemo()
    print("Loop Control Demo:")
    print("")

    -- while loop
    print("While Loop (Countdown):")
    local count = 5
    while count > 0 do
        print("  Countdown: " .. count)
        count = count - 1
    end
    print("  Launch!")
    print("")

    -- for loop
    print("For Loop (Square Numbers):")
    for i = 1, 5 do
        print("  " .. i .. "^2 = " .. (i * i))
    end
    print("")

    -- repeat-until loop
    print("Repeat-until Loop:")
    local x = 1
    repeat
        print("  x = " .. x)
        x = x * 2
    until x > 10
    print("")
end

-- ============================================
-- Main Program Entry
-- ============================================

-- Run complete program
function main()
    -- Initialize program
    initProgram()

    -- Run demonstration modules
    mathDemo()
    dataDemo()
    loopDemo()

    -- Program completion
    print("Program demonstration completed!")
    print("This program demonstrates the syntax features supported by the current Lua interpreter project.")
    print("")
end

-- Start program
main()