-- Base Library Function Tests
-- Testing core Lua functions

print("=== Base Library Tests ===")

-- Test 1: print function
print("Test 1: print function")
print("Hello, World!")
print("Multiple", "arguments", "test")
print(42)
print(true)
print(nil)

-- Test 2: type function
print("\nTest 2: type function")
print("type(42) =", type(42))
print("type('hello') =", type("hello"))
print("type(true) =", type(true))
print("type(nil) =", type(nil))
print("type({}) =", type({}))

-- Test 3: tostring function
print("\nTest 3: tostring function")
print("tostring(42) =", tostring(42))
print("tostring(3.14) =", tostring(3.14))
print("tostring(true) =", tostring(true))
print("tostring(false) =", tostring(false))
print("tostring(nil) =", tostring(nil))
print("tostring('hello') =", tostring("hello"))

-- Test 4: tonumber function
print("\nTest 4: tonumber function")
print("tonumber('42') =", tonumber("42"))
print("tonumber('3.14') =", tonumber("3.14"))
print("tonumber('hello') =", tonumber("hello"))
print("tonumber('') =", tonumber(""))

-- Test 5: Basic error handling
print("\nTest 5: Basic error handling")
print("Testing error function...")
-- error("This is a test error")  -- Uncomment to test error function

print("\n=== Base Library Tests Complete ===")
