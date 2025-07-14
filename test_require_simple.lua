-- Simple require test
print("Testing require function...")

-- Test 1: Check if require exists
print("require function exists:", type(require))

-- Test 2: Check if package table exists
print("package table exists:", type(package))

if type(package) == "table" then
    print("package.path:", package.path)
    print("package.loaded type:", type(package.loaded))
    print("package.preload type:", type(package.preload))
end

-- Test 3: Try to require a standard library
print("Trying to require string library...")
local str = require("string")
print("string library loaded:", type(str))

-- Test 4: Try to require the test module
print("Trying to require test_module_example...")
local test_mod = require("test_module_example")
print("test_module_example loaded:", type(test_mod))

if type(test_mod) == "table" then
    print("Module name:", test_mod.name)
    print("Module version:", test_mod.version)
    print("Greeting:", test_mod.greet("Lua"))
end

print("Basic require test completed.")
