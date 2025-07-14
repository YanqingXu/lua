-- Circular dependency test module B
-- This module requires module A, which in turn requires module B
-- This should trigger circular dependency detection

local M = {}

M.name = "circular_b"
M.value = "Module B"

-- This will cause a circular dependency
local module_a = require("test_modules.circular_a")

M.get_a_value = function()
    return module_a.value
end

M.greet = function()
    return "Hello from " .. M.name
end

return M
