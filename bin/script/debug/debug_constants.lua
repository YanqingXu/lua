-- Debug constants and table construction
print("=== Debug Constants ===")

-- Test 1: Simple string constant
print("Test 1: String constant")
local str = "hello"
print("str = " .. str)

-- Test 2: Simple number constant
print("Test 2: Number constant")
local num = 42
print("num = " .. tostring(num))

-- Test 3: Very simple table
print("Test 3: Very simple table")
local t = {}
t.a = 1
print("t.a = " .. tostring(t.a))

print("=== Debug completed ===")
