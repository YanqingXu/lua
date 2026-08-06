local payload = {kind = "debugger-fixture", code = 17}

local function fail()
    error(payload)
end

fail()

