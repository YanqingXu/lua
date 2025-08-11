function m()
    return 10, 20, 30
end

function pass(...)
    return ...
end

-- Test just the function call without assignment
pass(m())
