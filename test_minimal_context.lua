-- Minimal context test

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

print("=== Test 1: Direct call ===")
print("F(4) = " .. fibonacci(4))

print("")
print("=== Test 2: In a loop ===")
for i = 1, 5 do
    print("F(" .. i .. ") = " .. fibonacci(i))
end

print("")
print("=== Test 3: With local variables ===")
local a = 10
local b = 3
for i = 1, 5 do
    print("F(" .. i .. ") = " .. fibonacci(i))
end

print("")
print("=== Test 4: With other functions ===")
function add(x, y)
    return x + y
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

for i = 1, 5 do
    print("F(" .. i .. ") = " .. fibonacci(i))
end
