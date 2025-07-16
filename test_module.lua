-- Simple test module
local M = {}
M.name = "test_module"
M.version = "1.0"
M.greet = function(name)
    return "Hello, " .. (name or "World")
end
return M
