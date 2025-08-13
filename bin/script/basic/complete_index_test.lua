print("=== Complete __index Test ===")

-- Test data
local data = {x = 10, y = 20, z = 30}

-- Create proxy table
local proxy = {}

-- Create metatable with __index function
local mt = {
    __index = function(table, key)
        print("__index called!")
        print("  table type:", type(table))
        print("  key:", key)
        
        -- Check if key exists in data
        if data[key] then
            print("  found in data:", data[key])
            return data[key]
        else
            print("  not found, returning default")
            return "default_value"
        end
    end
}

-- Set metatable
setmetatable(proxy, mt)

-- Test different keys
print("\nTesting proxy.x:")
local result1 = proxy.x
print("Result:", result1)

print("\nTesting proxy.y:")
local result2 = proxy.y
print("Result:", result2)

print("\nTesting proxy.unknown:")
local result3 = proxy.unknown
print("Result:", result3)

print("\n=== Complete Test Finished ===")
