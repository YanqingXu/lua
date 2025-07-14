-- Nested module in subdirectory for testing package.path search
-- This module tests the ability to load modules from subdirectories

local M = {}

M.name = "nested_module"
M.location = "test_modules/subdir"
M.description = "A module located in a subdirectory for testing package.path functionality"

-- Simple functionality
M.data = {
    items = {},
    counter = 0
}

function M.add_item(item)
    table.insert(M.data.items, item)
    M.data.counter = M.data.counter + 1
    return M.data.counter
end

function M.get_items()
    return M.data.items
end

function M.get_count()
    return M.data.counter
end

function M.clear()
    M.data.items = {}
    M.data.counter = 0
end

function M.get_info()
    return {
        name = M.name,
        location = M.location,
        description = M.description,
        item_count = M.data.counter
    }
end

-- Test that this module can use standard libraries
function M.format_items()
    local result = {}
    for i, item in ipairs(M.data.items) do
        table.insert(result, string.format("%d: %s", i, tostring(item)))
    end
    return result
end

function M.get_timestamp()
    return os.date("%Y-%m-%d %H:%M:%S")
end

return M
