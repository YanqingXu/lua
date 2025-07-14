-- Debug require function
print("Testing require function debugging...")

-- Test type checking
local test_string = "string"
print("test_string type:", type(test_string))

-- Test getting package information
print("package type:", type(package))
if package then
    print("package.loaded type:", type(package.loaded))
    if package.loaded then
        print("package.loaded.string exists:", package.loaded.string ~= nil)
    end
end

-- Test require exists
print("require type:", type(require))
print("require function:", require)

-- Test calling require with explicit string
print("About to call require...")
local success, result = pcall(require, "string")
print("pcall result:", success, result)

if not success then
    print("Error message:", result)
end

print("Debug test completed.")
