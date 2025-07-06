-- Test Fibonacci in the same context as test.lua

-- Same global variables as test.lua
local a = 10
local b = 3

print("Global a = " .. a .. ", b = " .. b)

-- Same fibonacci function as test.lua (with renamed variables)
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

print("Testing Fibonacci:")
for i = 1, 10 do
    print("  Fibonacci F(" .. i .. ") = " .. fibonacci(i))
end

print("After test, global a = " .. a .. ", b = " .. b)
