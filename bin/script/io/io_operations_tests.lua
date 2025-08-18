-- IO operations tests
-- Test various features of the input/output library

print("=== IO Operations Tests ===")

-- Test 1: Basic output
print("Test 1: Basic output")
print("  This is the output of the print function")

-- Test 2: io.write function
print("\nTest 2: io.write function")
if io and io.write then
    io.write("  This is the output of io.write\n")
    io.write("  Multiple", " ", "arguments", " ", "output\n")
else
    print("  io.write function is not available")
end

-- Test 3: Output different types of values
print("\nTest 3: Output different types of values")
local number = 42
local string = "test string"
local boolean = true
local nilValue = nil

print("  number:", number)
print("  string:", string)
print("  boolean:", boolean)
print("  nil value:", nilValue)

-- Test 4: Formatted output
print("\nTest 4: Formatted output")
local name = "John"
local age = 25
local score = 95.5

if string and string.format then
    local formatted = string.format("Name: %s, Age: %d, Score: %.1f", name, age, score)
    print("  " .. formatted)
else
    print("  string.format is not available, using concatenation:")
    print("  Name: " .. name .. ", Age: " .. age .. ", Score: " .. score)
end

-- Test 5: Table output
print("\nTest 5: Table output")
local table1 = {a = 1, b = 2, c = 3}
print("  table object:", table1)  -- Will display the address of the table

-- Custom table printing function
local function printTable(t, indent)
    indent = indent or ""
    for k, v in pairs(t) do
        if type(v) == "table" then
            print(indent .. k .. ":")
            printTable(v, indent .. "  ")
        else
            print(indent .. k .. ": " .. tostring(v))
        end
    end
end

print("  table content:")
printTable(table1, "    ")

-- Test 6: Output of nested tables
print("\nTest 6: Output of nested tables")
local nestedTable = {
    person = {
        name = "Mike",
        age = 30,
        address = {
            city = "Beijing",
            district = "Chaoyang"
        }
    },
    scores = {90, 85, 92}
}

print("  nested table content:")
printTable(nestedTable, "    ")

-- Test 7: Array output
print("\nTest 7: Array output")
local array = {10, 20, 30, 40, 50}
print("  array length:", #array)
print("  array content:")
for i = 1, #array do
    print("    [" .. i .. "] = " .. array[i])
end

-- Test 8: Function output
print("\nTest 8: Function output")
local function testFunc()
    return "test function"
end

print("  function object:", testFunc)
print("  function call result:", testFunc())

-- Test 9: Error output
print("\nTest 9: Error output")
local success, error = pcall(function()
    error("This is a test error")
end)

if not success then
    print("  Caught error:", error)
end

-- Test 10: Large data output test
print("\nTest 10: Large data output test")
print("  Output squares of 1 to 10:")
for i = 1, 10 do
    print("    " .. i .. "^2 = " .. (i * i))
end

-- Test 11: Special character output
print("\nTest 11: Special character output")
print("  Tab:\tHere is a tab")
print("  Newline:\nHere is a newline")
print("  Quotes: \"double quote\" 'single quote'")
print("  Backslash: \\This is a backslash\\")

-- Test 12: Unicode character output
print("\nTest 12: Unicode character output")
print("  Chinese: 你好世界")
print("  Japanese: こんにちは")
print("  Korean: 안녕하세요")
print("  Emoji: 😀 🎉 ❤️")

-- Test 13: Output numbers in different bases
print("\nTest 13: Output numbers in different bases")
local num = 255
if string and string.format then
    print("  Decimal:", num)
    print("  Hex:", string.format("%x", num))
    print("  Octal:", string.format("%o", num))
else
    print("  Decimal:", num)
    print("  (Other bases require string.format support)")
end

print("\n=== IO Operations Tests Complete ===")
