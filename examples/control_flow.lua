local function fib(n)
    if n < 2 then
        return n
    end

    local a = 0
    local b = 1
    local i = 2

    while i <= n do
        local next_value = a + b
        a = b
        b = next_value
        i = i + 1
    end

    return b
end

print("fib(8)", fib(8))

