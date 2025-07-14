-- Test package.searchpath function
print("Testing package.searchpath...")

-- Test searchpath function
if package.searchpath then
    print("package.searchpath exists:", type(package.searchpath))
    
    -- Test with a simple pattern
    local result = package.searchpath("nonexistent", "./?.lua")
    print("Search for nonexistent module:", result)
    
    -- Test with current module (should find basic_test.lua)
    local result2 = package.searchpath("basic_test", "./?.lua")
    print("Search for basic_test:", result2)
    
else
    print("package.searchpath does not exist")
end

print("Searchpath test completed.")
