-- Very simple nested test
function double(x)
    return x + x
end

function test()
    return double(5)
end

local result = test()
print("result =", result)
