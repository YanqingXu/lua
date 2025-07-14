-- Example module for testing the require() function
-- This demonstrates a typical Lua module structure

local M = {}

-- Module metadata
M.name = "test_module_example"
M.version = "1.0"

-- Module data
M.data = {
    counter = 0,
    messages = {}
}

-- Module functions
function M.increment()
    M.data.counter = M.data.counter + 1
    return M.data.counter
end

function M.get_counter()
    return M.data.counter
end

function M.add_message(msg)
    table.insert(M.data.messages, msg)
end

function M.get_messages()
    return M.data.messages
end

function M.greet(name)
    name = name or "World"
    return "Hello, " .. name .. "!"
end

-- Return the module table
return M
