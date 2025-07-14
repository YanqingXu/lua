-- Simple test module for basic require() functionality testing
-- This module demonstrates a basic Lua module pattern

local M = {}

-- Module metadata
M.name = "simple_module"
M.version = "1.0.0"
M.description = "A simple test module for package library testing"

-- Simple data
M.data = {
    counter = 0,
    messages = {},
    config = {
        debug = false,
        max_items = 100
    }
}

-- Simple functions
function M.increment()
    M.data.counter = M.data.counter + 1
    return M.data.counter
end

function M.decrement()
    M.data.counter = M.data.counter - 1
    return M.data.counter
end

function M.get_counter()
    return M.data.counter
end

function M.reset_counter()
    M.data.counter = 0
end

function M.add_message(msg)
    if type(msg) == "string" then
        table.insert(M.data.messages, msg)
        return #M.data.messages
    end
    return nil
end

function M.get_messages()
    return M.data.messages
end

function M.clear_messages()
    M.data.messages = {}
end

function M.greet(name)
    name = name or "World"
    return "Hello, " .. name .. "!"
end

function M.calculate(a, b, operation)
    operation = operation or "add"
    if operation == "add" then
        return a + b
    elseif operation == "subtract" then
        return a - b
    elseif operation == "multiply" then
        return a * b
    elseif operation == "divide" then
        if b ~= 0 then
            return a / b
        else
            error("Division by zero")
        end
    else
        error("Unknown operation: " .. tostring(operation))
    end
end

-- Configuration functions
function M.set_debug(enabled)
    M.data.config.debug = enabled
end

function M.is_debug()
    return M.data.config.debug
end

function M.set_max_items(max)
    if type(max) == "number" and max > 0 then
        M.data.config.max_items = max
        return true
    end
    return false
end

function M.get_max_items()
    return M.data.config.max_items
end

-- Return the module table
return M
