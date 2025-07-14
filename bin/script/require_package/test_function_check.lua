-- Test function existence and callability
print("=== Function Check Test ===")

-- Test 1: Check if require exists and is callable
print("\nTest 1: require function check")
print("require exists:", require ~= nil)
print("require type:", type(require))

-- Test 2: Check if print works (baseline)
print("\nTest 2: print function check")
print("print exists:", print ~= nil)
print("print type:", type(print))

-- Test 3: Check if type works (baseline)
print("\nTest 3: type function check")
print("type exists:", type ~= nil)

-- Test 4: Try calling require with pcall to catch errors
print("\nTest 4: pcall require test")
local success, result = pcall(function()
    return require("string")
end)
print("pcall success:", success)
if success then
    print("result type:", type(result))
else
    print("error:", result)
end

print("\n=== Function Check Complete ===")
