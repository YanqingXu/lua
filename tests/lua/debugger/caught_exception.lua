local marker = {kind = "caught", code = 23}

local ok, value = pcall(function()
    error(marker)
end)

assert(not ok)
assert(value == marker)
return true
