-- Debug test for specific algorithm issues

print("=== Testing Fibonacci Function ===")
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

for i = 1, 10 do
    print("F(" .. i .. ") = " .. fibonacci(i))
end

print("")
print("=== Testing Array Max Function ===")
function findMax(numbers)
    local max = numbers[1]
    for i = 2, #numbers do
        if numbers[i] > max then
            max = numbers[i]
        end
    end
    return max
end

local testNumbers = {1, 5, 3, 9, 2, 8, 4}
print("Array: {1, 5, 3, 9, 2, 8, 4}")
print("Max: " .. findMax(testNumbers))

print("")
print("=== Testing While Loop ===")
local count = 5
while count > 0 do
    print("Countdown: " .. count)
    count = count - 1
end
print("Launch!")

print("")
print("=== Testing For Loop ===")
for i = 1, 5 do
    print(i .. "^2 = " .. (i * i))
end

print("")
print("=== Testing Repeat-Until Loop ===")
local x = 1
repeat
    print("x = " .. x)
    x = x * 2
until x > 10
