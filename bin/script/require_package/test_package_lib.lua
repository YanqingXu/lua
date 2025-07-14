-- Test package library basic functionality
print("Testing package library...")

-- Check package table
print("package table exists:", package ~= nil)

if package then
    print("package.path:", package.path)
    print("package.loaded exists:", package.loaded ~= nil)
    print("package.preload exists:", package.preload ~= nil)
    print("package.loaders exists:", package.loaders ~= nil)
    
    -- Check some loaded modules
    if package.loaded then
        print("string in package.loaded:", package.loaded.string ~= nil)
        print("table in package.loaded:", package.loaded.table ~= nil)
        print("math in package.loaded:", package.loaded.math ~= nil)
    end
    
    -- Test package.searchpath if it exists
    if package.searchpath then
        print("package.searchpath exists:", type(package.searchpath))
    end
end

-- Test require function
print("require function exists:", require ~= nil)

-- Test loadfile function
print("loadfile function exists:", loadfile ~= nil)

-- Test dofile function
print("dofile function exists:", dofile ~= nil)

print("Package library test completed.")
