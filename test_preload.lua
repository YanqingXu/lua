-- Test package.preload functionality
print("Testing package.preload...")

-- Add a preload module
package.preload["test_preload"] = function()
    local mod = {}
    mod.name = "test_preload"
    mod.version = "1.0.0"
    mod.message = "Hello from preload!"
    
    function mod.greet(name)
        return "Hello, " .. (name or "World") .. " from preload module!"
    end
    
    return mod
end

-- Test requiring the preload module
print("Requiring preload module...")
local preload_mod = require("test_preload")

print("Preload module loaded:", type(preload_mod))
if preload_mod then
    print("Module name:", preload_mod.name)
    print("Module version:", preload_mod.version)
    print("Module message:", preload_mod.message)
    print("Greet function:", preload_mod.greet("Lua"))
end

-- Test that it's cached
print("Testing preload caching...")
local preload_mod2 = require("test_preload")
print("Same module object:", preload_mod == preload_mod2)

-- Check if it's in package.loaded
print("Module in package.loaded:", package.loaded["test_preload"] ~= nil)

print("Preload test completed.")
