function m()
    return 10, 20, 30
end

function pass(...)
    return ...
end

-- Focus on the problematic case
print("Testing pass(m())...")
local x, y = pass(m())
print("Result:", x, y)
