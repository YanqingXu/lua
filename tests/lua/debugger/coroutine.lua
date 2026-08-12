local worker = coroutine.create(function(seed)
    local current = seed
    coroutine.yield(current)
    current = current + 1
    return current
end)

local first_ok, first = coroutine.resume(worker, 40)
local second_ok, second = coroutine.resume(worker)
return first_ok, first, second_ok, second
