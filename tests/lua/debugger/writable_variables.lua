local shared = {name = "old", [1] = 1}

local function makeWorker()
    local captured = "before"
    return function()
        local localValue = "local-old"
        local marker = 1
        debug_write_result = localValue .. ":" .. captured .. ":" .. shared.name .. ":" .. shared[1]
        return marker
    end
end

local worker = makeWorker()
worker()
return debug_write_result
