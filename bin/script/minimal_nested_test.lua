-- Minimal test to isolate the nested call issue
function square(n)
    return n * n
end

function test(x)
    return square(x)
end

local result = test(5)
print("test(5) =", result)
