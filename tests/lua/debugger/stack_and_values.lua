local total = 10

local function accumulate(value)
    local doubled = value * 2
    local data = {
        value = value,
        doubled = doubled,
        nested = {enabled = true},
    }
    data.self = data
    total = total + data.doubled
    return total
end

local function recurse(depth)
    if depth == 0 then
        return accumulate(5)
    end
    return recurse(depth - 1)
end

return recurse(2)

