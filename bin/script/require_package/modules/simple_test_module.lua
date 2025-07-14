-- Simple test module for require() testing
local M = {}

M.name = "simple_test_module"
M.version = "1.0"

function M.hello()
    return "Hello from simple test module!"
end

function M.add(a, b)
    return a + b
end

function M.get_info()
    return {
        name = M.name,
        version = M.version,
        functions = {"hello", "add", "get_info"}
    }
end

return M
