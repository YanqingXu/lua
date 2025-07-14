-- Test require function and package library issues
print("=== Testing require function and package library ===")

-- Test 1: Check if package table exists
print("\n1. Testing package table structure:")
if package then
    print("✓ package table exists")
    print("  package.path =", package.path)
    print("  package.loaded type =", type(package.loaded))
    print("  package.preload type =", type(package.preload))
    print("  package.loaders type =", type(package.loaders))
else
    print("✗ package table missing")
end

-- Test 2: Check if require function exists
print("\n2. Testing require function:")
if require then
    print("✓ require function exists, type =", type(require))
else
    print("✗ require function missing")
end

-- Test 3: Test standard library loading
print("\n3. Testing standard library loading:")
local libs = {"string", "table", "math", "io", "os", "debug"}
for _, lib in ipairs(libs) do
    local success, result = pcall(require, lib)
    if success then
        print("✓ require('" .. lib .. "') successful, type =", type(result))
    else
        print("✗ require('" .. lib .. "') failed:", result)
    end
end

-- Test 4: Check package.loaded entries
print("\n4. Testing package.loaded entries:")
if package and package.loaded then
    print("package.loaded contents:")
    for k, v in pairs(package.loaded) do
        print("  " .. tostring(k) .. " =", type(v))
    end
else
    print("✗ package.loaded not available")
end

-- Test 5: Test package.searchpath function
print("\n5. Testing package.searchpath:")
if package and package.searchpath then
    print("✓ package.searchpath exists")
    local path = package.searchpath("nonexistent", "./?.lua")
    print("  searchpath for nonexistent module:", path)
else
    print("✗ package.searchpath missing")
end

-- Test 6: Create a simple test module and try to load it
print("\n6. Testing file module loading:")

-- Create a simple test module file
local test_module_content = [[
-- Simple test module
local M = {}
M.name = "test_module"
M.version = "1.0"
M.greet = function(name)
    return "Hello, " .. (name or "World")
end
return M
]]

-- Write test module to file
local file = io.open("test_module.lua", "w")
if file then
    file:write(test_module_content)
    file:close()
    print("✓ Created test_module.lua")
    
    -- Try to require the test module
    local success, result = pcall(require, "test_module")
    if success then
        print("✓ require('test_module') successful")
        print("  Module name:", result.name)
        print("  Module version:", result.version)
        print("  Greet function:", result.greet("Lua"))
    else
        print("✗ require('test_module') failed:", result)
    end
else
    print("✗ Failed to create test_module.lua")
end

-- Test 7: Test loadfile function
print("\n7. Testing loadfile function:")
if loadfile then
    print("✓ loadfile function exists")
    local func, err = loadfile("test_module.lua")
    if func then
        print("✓ loadfile('test_module.lua') successful, type =", type(func))
    else
        print("✗ loadfile('test_module.lua') failed:", err)
    end
else
    print("✗ loadfile function missing")
end

-- Test 8: Test dofile function
print("\n8. Testing dofile function:")
if dofile then
    print("✓ dofile function exists")
    local success, result = pcall(dofile, "test_module.lua")
    if success then
        print("✓ dofile('test_module.lua') successful, type =", type(result))
    else
        print("✗ dofile('test_module.lua') failed:", result)
    end
else
    print("✗ dofile function missing")
end

-- Test 9: Test error cases
print("\n9. Testing error cases:")

-- Test require with invalid argument
local success, err = pcall(require, 123)
if not success then
    print("✓ require(123) correctly failed:", err)
else
    print("✗ require(123) should have failed")
end

-- Test require with non-existent module
success, err = pcall(require, "nonexistent_module_xyz")
if not success then
    print("✓ require('nonexistent_module_xyz') correctly failed:", err)
else
    print("✗ require('nonexistent_module_xyz') should have failed")
end

print("\n=== Test completed ===")
