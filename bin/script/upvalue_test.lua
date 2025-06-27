-- Upvalue string concatenation test
print("=== Upvalue String Concatenation Test ===")

-- Test 1: Global variables
GLOBAL_NAME = "Global"
GLOBAL_VERSION = "1.0"

function testGlobal()
    print("Global test: " .. GLOBAL_NAME .. " v" .. GLOBAL_VERSION)
end

print("Test 1: Global variables")
testGlobal()

-- Test 2: Local variables (upvalues)
local LOCAL_NAME = "Local"
local LOCAL_VERSION = "2.0"

function testUpvalue()
    print("Upvalue test: " .. LOCAL_NAME .. " v" .. LOCAL_VERSION)
end

print("Test 2: Upvalue variables")
testUpvalue()

print("=== Test completed ===")
