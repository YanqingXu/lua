-- Debug for loop bounds
print("=== Testing for loop bounds ===")

print("for i = 2, 4 do:")
for i = 2, 4 do
    print("  i = " .. i)
end

print("")
print("for i = 1, 5 do:")
for i = 1, 5 do
    print("  i = " .. i)
end

print("")
print("=== Testing Fibonacci step by step ===")
function fibonacci_debug(n)
    print("fibonacci_debug(" .. n .. ") called")
    if n <= 1 then
        print("  Base case: returning " .. n)
        return n
    else
        print("  Iterative case for n=" .. n)
        local fib_a = 0
        local fib_b = 1
        print("  Initial: fib_a=" .. fib_a .. ", fib_b=" .. fib_b)
        
        print("  Starting for loop from 2 to " .. n)
        for i = 2, n do
            print("    Loop iteration i=" .. i)
            print("    Before: fib_a=" .. fib_a .. ", fib_b=" .. fib_b)
            local temp = fib_a + fib_b
            print("    temp = fib_a + fib_b = " .. temp)
            fib_a = fib_b
            print("    fib_a = fib_b, now fib_a=" .. fib_a)
            fib_b = temp
            print("    fib_b = temp, now fib_b=" .. fib_b)
        end
        print("  Loop finished, returning fib_b=" .. fib_b)
        return fib_b
    end
end

local result = fibonacci_debug(4)
print("Final result: " .. result)
