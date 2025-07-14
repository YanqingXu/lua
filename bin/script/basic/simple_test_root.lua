-- Very simple test module
local simple = {}

simple.name = "simple"
simple.version = "1.0"

function simple.hello()
    return "Hello from simple module!"
end

return simple
