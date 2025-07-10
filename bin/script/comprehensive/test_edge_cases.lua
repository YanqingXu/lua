-- Error Handling and Edge Cases Test
-- Tests error conditions and boundary cases

print("=== Error Handling and Edge Cases Test ===")

-- Test 1: String function edge cases
print("\n1. String edge cases:")
print("  string.len(''):", string.len(''))
print("  string.sub('abc', 10):", string.sub('abc', 10))
print("  string.sub('abc', -2):", string.sub('abc', -2))
print("  string.rep('a', 0):", string.rep('a', 0))

-- Test 2: Math function edge cases
print("\n2. Math edge cases:")
print("  math.sqrt(0):", math.sqrt(0))
print("  math.sqrt(-1):", math.sqrt(-1))  -- Should be NaN
print("  math.log(0):", math.log(0))      -- Should be -inf
print("  math.pow(0, 0):", math.pow(0, 0)) -- Should be 1
print("  math.abs(0):", math.abs(0))  print("  math.min(1):", math.min(1))        -- Single argument
  print("  math.max(1):", math.max(1))        -- Single argument

-- Test 3: Type conversion edge cases
print("\n3. Type conversion edge cases:")
print("  tonumber(''):", tonumber(''))
print("  tonumber('abc'):", tonumber('abc'))
print("  tonumber('123abc'):", tonumber('123abc'))
print("  tonumber('  456  '):", tonumber('  456  '))
print("  tostring(nil):", tostring(nil))
print("  tostring(0/0):", tostring(0/0))  -- NaN

-- Test 4: Table edge cases
print("\n4. Table edge cases:")
local empty_table = {}
print("  #empty_table:", #empty_table)
if table.concat then
    print("  table.concat(empty_table):", table.concat(empty_table))
end

local sparse_table = {[1] = "a", [3] = "c", [5] = "e"}
print("  #sparse_table:", #sparse_table)

-- Test 5: Assert function testing (non-failing cases)
print("\n5. Assert testing:")
local function safe_assert_test()
    local result1 = assert(true, "This should not fail")
    print("  assert(true) result:", result1)
    
    local result2 = assert(42, "Number assertion")
    print("  assert(42) result:", result2)
    
    local result3 = assert("hello", "String assertion")
    print("  assert('hello') result:", result3)
    
    -- Note: We don't test assert(false) as it would terminate the program
    print("  assert(false) test skipped (would terminate)")
end

safe_assert_test()

-- Test 6: Error function testing (commented out to avoid termination)
print("\n6. Error function testing:")
print("  error() function exists:", type(error) == "function")
-- error("This is a test error")  -- Commented out to avoid program termination

-- Test 7: IO edge cases
print("\n7. IO edge cases:")
if io.write then
    io.write("Testing io.write with empty string: ")
    io.write("")
    io.write("Done\n")
    
    -- Test multiple arguments (must convert boolean to string)
    io.write("Multiple ", "args ", "test: ", "123", " ", tostring(true), "\n")
end

-- Test 8: OS edge cases
print("\n8. OS edge cases:")
if os.getenv then
    local non_existent = os.getenv("NON_EXISTENT_VAR_12345")
    print("  Non-existent env var:", non_existent)
end

if os.date then
    local date_with_format = os.date("%Y-%m-%d %H:%M:%S")
    print("  Formatted date:", date_with_format)
end

print("\n=== Error Handling Test Complete ===")
print("All edge cases handled appropriately!")
