-- Debug stack positions
print("=== Stack Position Debug ===")

local first = 100
print("first =", first)

local second = 200  
print("second =", second)

function test_first()
    return first
end

function test_second()
    return second
end

print("test_first() =", test_first())
print("test_second() =", test_second())

print("=== Stack debug completed ===")
