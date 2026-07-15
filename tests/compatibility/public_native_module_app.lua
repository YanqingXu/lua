local module_path = assert(arg[1], "missing native module path")

local missing_file, file_message, file_where =
    package.loadlib(module_path .. ".missing", "luaopen_publicfixture")
assert(missing_file == nil)
assert(type(file_message) == "string")
assert(file_where == "open")

local missing_symbol, symbol_message, symbol_where =
    package.loadlib(module_path, "luaopen_publicfixture_missing")
assert(missing_symbol == nil)
assert(type(symbol_message) == "string")
assert(symbol_where == "init")

local loader, message, where = package.loadlib(module_path, "luaopen_publicfixture")
assert(loader, tostring(where) .. ": " .. tostring(message))

local first = loader("publicfixture")
local second = loader("publicfixture")

assert(first.source == "public-lua-h-only")
assert(first.state_calls == 1)
assert(first.module_calls == 1)
assert(first.protected_value == 42.5)
assert(second.state_calls == 2)
assert(second.module_calls == 2)
assert(second.protected_value == 42.5)
