-- Debug the specific issues from test.lua

print("=== Testing String Concatenation Issue ===")
for i = 1, 3 do
    print("Simple: " .. i)
    print("With calculation: " .. (i * i))
    print("Full format: " .. i .. "^2 = " .. (i * i))
    print("Alternative: " .. i .. "^2 = " .. i * i)
end

print("")
print("=== Testing Fibonacci in Context ===")

-- Same fibonacci function as in test.lua
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

-- Test in a loop like test.lua does
for i = 1, 5 do
    print("Fibonacci F(" .. i .. ") = " .. fibonacci(i))
end

print("")
print("=== Testing Variable Scoping ===")
local a = 100
local b = 200
print("Global a = " .. a .. ", b = " .. b)

function testScope()
    local a = 0
    local b = 1
    print("In function: a = " .. a .. ", b = " .. b)
    for i = 2, 3 do
        local temp = a + b
        print("Loop " .. i .. ": temp = " .. temp)
        a = b
        b = temp
        print("Loop " .. i .. ": a = " .. a .. ", b = " .. b)
    end
    return b
end

local result = testScope()
print("Function result: " .. result)
print("After function: a = " .. a .. ", b = " .. b)
