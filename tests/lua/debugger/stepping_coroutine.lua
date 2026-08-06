local worker = coroutine.create(function()
    local inside = 10
    coroutine.yield(inside)
    return inside + 1
end)

local ok, first = coroutine.resume(worker)
local ok2, second = coroutine.resume(worker)
return first + second
