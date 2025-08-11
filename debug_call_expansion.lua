-- Simple test to debug call expansion detection
function m()
    return 10, 20, 30
end

function pass(...)
    return ...
end

-- This should trigger hasLastCallExpansion=1
local x, y = pass(m())
print('Result:', x, y)
