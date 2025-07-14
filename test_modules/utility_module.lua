-- Utility module demonstrating more advanced module patterns
-- This module shows how to create utility functions and use standard libraries

local M = {}

-- Module info
M._NAME = "utility_module"
M._VERSION = "2.1.0"
M._DESCRIPTION = "Utility functions for testing package system"

-- String utilities
M.string_utils = {}

function M.string_utils.trim(str)
    if type(str) ~= "string" then
        return nil
    end
    return string.match(str, "^%s*(.-)%s*$")
end

function M.string_utils.split(str, delimiter)
    if type(str) ~= "string" then
        return {}
    end
    delimiter = delimiter or "%s"
    local result = {}
    for match in string.gmatch(str, "([^" .. delimiter .. "]+)") do
        table.insert(result, match)
    end
    return result
end

function M.string_utils.starts_with(str, prefix)
    if type(str) ~= "string" or type(prefix) ~= "string" then
        return false
    end
    return string.sub(str, 1, string.len(prefix)) == prefix
end

function M.string_utils.ends_with(str, suffix)
    if type(str) ~= "string" or type(suffix) ~= "string" then
        return false
    end
    return string.sub(str, -string.len(suffix)) == suffix
end

function M.string_utils.capitalize(str)
    if type(str) ~= "string" or str == "" then
        return str
    end
    return string.upper(string.sub(str, 1, 1)) .. string.lower(string.sub(str, 2))
end

-- Table utilities
M.table_utils = {}

function M.table_utils.copy(t)
    if type(t) ~= "table" then
        return t
    end
    local copy = {}
    for k, v in pairs(t) do
        copy[k] = v
    end
    return copy
end

function M.table_utils.deep_copy(t)
    if type(t) ~= "table" then
        return t
    end
    local copy = {}
    for k, v in pairs(t) do
        if type(v) == "table" then
            copy[k] = M.table_utils.deep_copy(v)
        else
            copy[k] = v
        end
    end
    return copy
end

function M.table_utils.merge(t1, t2)
    local result = M.table_utils.copy(t1)
    for k, v in pairs(t2) do
        result[k] = v
    end
    return result
end

function M.table_utils.keys(t)
    if type(t) ~= "table" then
        return {}
    end
    local keys = {}
    for k, _ in pairs(t) do
        table.insert(keys, k)
    end
    return keys
end

function M.table_utils.values(t)
    if type(t) ~= "table" then
        return {}
    end
    local values = {}
    for _, v in pairs(t) do
        table.insert(values, v)
    end
    return values
end

function M.table_utils.size(t)
    if type(t) ~= "table" then
        return 0
    end
    local count = 0
    for _ in pairs(t) do
        count = count + 1
    end
    return count
end

-- Math utilities
M.math_utils = {}

function M.math_utils.clamp(value, min_val, max_val)
    return math.max(min_val, math.min(max_val, value))
end

function M.math_utils.round(value, decimals)
    decimals = decimals or 0
    local multiplier = 10 ^ decimals
    return math.floor(value * multiplier + 0.5) / multiplier
end

function M.math_utils.is_even(n)
    return type(n) == "number" and n % 2 == 0
end

function M.math_utils.is_odd(n)
    return type(n) == "number" and n % 2 == 1
end

function M.math_utils.factorial(n)
    if type(n) ~= "number" or n < 0 or n ~= math.floor(n) then
        return nil
    end
    if n == 0 or n == 1 then
        return 1
    end
    local result = 1
    for i = 2, n do
        result = result * i
    end
    return result
end

function M.math_utils.gcd(a, b)
    if type(a) ~= "number" or type(b) ~= "number" then
        return nil
    end
    a, b = math.abs(a), math.abs(b)
    while b ~= 0 do
        a, b = b, a % b
    end
    return a
end

-- Validation utilities
M.validation = {}

function M.validation.is_email(str)
    if type(str) ~= "string" then
        return false
    end
    return string.match(str, "^[%w%._%+%-]+@[%w%._%+%-]+%.%w+$") ~= nil
end

function M.validation.is_number_string(str)
    if type(str) ~= "string" then
        return false
    end
    return tonumber(str) ~= nil
end

function M.validation.is_positive_integer(n)
    return type(n) == "number" and n > 0 and n == math.floor(n)
end

function M.validation.is_in_range(value, min_val, max_val)
    return type(value) == "number" and value >= min_val and value <= max_val
end

-- File utilities (using io library)
M.file_utils = {}

function M.file_utils.file_exists(filename)
    if type(filename) ~= "string" then
        return false
    end
    local file = io.open(filename, "r")
    if file then
        file:close()
        return true
    end
    return false
end

function M.file_utils.read_lines(filename)
    if not M.file_utils.file_exists(filename) then
        return nil
    end
    local lines = {}
    for line in io.lines(filename) do
        table.insert(lines, line)
    end
    return lines
end

-- Date/time utilities (using os library)
M.time_utils = {}

function M.time_utils.get_timestamp()
    return os.time()
end

function M.time_utils.format_date(timestamp, format)
    timestamp = timestamp or os.time()
    format = format or "%Y-%m-%d %H:%M:%S"
    return os.date(format, timestamp)
end

function M.time_utils.get_current_date()
    return M.time_utils.format_date(nil, "%Y-%m-%d")
end

function M.time_utils.get_current_time()
    return M.time_utils.format_date(nil, "%H:%M:%S")
end

-- Module initialization function
function M.init(config)
    config = config or {}
    M._config = M.table_utils.merge({
        debug = false,
        log_level = "info"
    }, config)
    
    if M._config.debug then
        print("Utility module initialized with debug mode enabled")
    end
end

-- Module info function
function M.get_info()
    return {
        name = M._NAME,
        version = M._VERSION,
        description = M._DESCRIPTION,
        config = M._config
    }
end

-- Return the module
return M
