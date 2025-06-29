-- Debug Fibonacci function
function fibonacci(n)
    print("fibonacci(" .. n .. ") called")
    if n <= 1 then
        print("Base case: returning " .. n)
        return n
    else
        print("Recursive case for n=" .. n)
        local a = 0
        local b = 1
        print("Initial: a=" .. a .. ", b=" .. b)
        for i = 2, n do
            print("Loop iteration i=" .. i)
            print("Before: a=" .. a .. ", b=" .. b)
            local temp = a + b
            print("temp = a + b = " .. temp)
            a = b
            print("a = b, now a=" .. a)
            b = temp
            print("b = temp, now b=" .. b)
        end
        print("Final result: b=" .. b)
        return b
    end
end

print("Testing Fibonacci step by step:")
for i = 1, 5 do
    print("=== F(" .. i .. ") ===")
    local result = fibonacci(i)
    print("Result: " .. result)
    print("")
end
