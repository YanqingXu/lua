local function leaf(value)
    return value + 1
end

local function tail(value)
    return leaf(value)
end

local total = 0
for index = 1, 3 do
    total = total + tail(index)
end

return total

