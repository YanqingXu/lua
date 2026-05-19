local vector_mt = {}

function vector_mt.__add(left, right)
    return setmetatable({
        x = left.x + right.x,
        y = left.y + right.y
    }, vector_mt)
end

local a = setmetatable({ x = 2, y = 3 }, vector_mt)
local b = setmetatable({ x = 5, y = 7 }, vector_mt)
local sum = a + b

print("sum", sum.x, sum.y)
