-- Test basic function calls to isolate the issue
print("=== Basic Function Call Test ===")

-- Test 1: Call type function (should work)
print("\nTest 1: type function")
local t = type(42)
print("type(42) =", t)

-- Test 2: Call tostring function (should work)
print("\nTest 2: tostring function")
local s = tostring(42)
print("tostring(42) =", s)

-- Test 3: Check if string library exists
print("\nTest 3: string library")
if string then
    print("string library exists")
    print("string type:", type(string))
    if string.len then
        print("string.len exists")
        local len = string.len("hello")
        print("string.len('hello') =", len)
    end
else
    print("string library not found")
end

-- Test 4: Try require with different approach
print("\nTest 4: require test")
print("require type:", type(require))
if require then
    print("require function exists")
    -- Try to call require directly without pcall first
    print("About to call require...")
    local str_lib = require("string")
    print("require call completed")
    print("result type:", type(str_lib))
else
    print("require function not found")
end

print("\n=== Basic Function Test Complete ===")
