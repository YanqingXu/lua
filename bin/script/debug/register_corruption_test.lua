function m()
    return 10, 20, 30
end

function pass(...)
    return ...
end

-- This specific scenario causes register corruption
print("Testing register corruption scenario...")
local x, y = pass(m())
print("Result:", x, y)
