-- Test compiler integration for extended instructions
print("=== Compiler Integration Test ===")

-- Test 1: Simple table creation and access
print("Test 1: Table operations")
local t = {}
t.name = "test"
t.value = 42
print("t.name =", t.name)
print("t.value =", t.value)

-- Test 2: Comparison operations
print("\nTest 2: Comparison operations")
local a = 10
local b = 20
if a < b then
    print("a < b is true")
end

if a == 10 then
    print("a == 10 is true")
end

-- Test 3: Logical operations
print("\nTest 3: Logical operations")
local flag = true
if not flag then
    print("This should not print")
else
    print("not flag is false")
end

-- Test 4: String operations
print("\nTest 4: String operations")
local str = "Hello"
print("Length of str:", #str)

-- Test 5: Modulo operation
print("\nTest 5: Modulo operation")
local result = 17 % 5
print("17 % 5 =", result)

print("\n=== Compiler Integration Test Completed ===")
