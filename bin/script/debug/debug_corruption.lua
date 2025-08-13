function m()
    return 10, 20, 30
end

function pass(...)
    return ...
end

-- This is the exact scenario that causes the corruption
print("-- multi return through call chain")
local x, y = pass(m())
print("Result:", x, y)
