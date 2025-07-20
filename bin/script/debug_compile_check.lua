-- Check if there are compilation issues
print("=== Compilation Check ===")

-- Try to load the comprehensive test file without executing it
local success, error_msg = pcall(function()
    -- This will compile the file but not execute it
    local chunk = loadfile("bin/script/comprehensive_validation_test.lua")
    if chunk then
        print("File compiled successfully")
        return true
    else
        print("File compilation failed")
        return false
    end
end)

print("Load attempt result:", success)
if not success then
    print("Error:", error_msg)
end

print("=== Compilation Check Complete ===")
