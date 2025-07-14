-- Circular dependency test module A
-- This module requires module B, which in turn requires module A
-- This should trigger circular dependency detection

local M = {}

M.name = "circular_a"
M.value = "Module A"

-- This will cause a circular dependency
local module_b = require("test_modules.circular_b")

M.get_b_value = function()
    return module_b.value
end

M.greet = function()
    return "Hello from " .. M.name
end

return M
