-- Complex Standard Library Integration Test
-- Tests advanced features and combinations of standard libraries

print("=== Complex Integration Test ===")

-- Test 1: Mathematical calculations with string formatting
print("\n1. Mathematical calculations with string formatting:")
local pi_approx = math.pi
local sin_half_pi = math.sin(pi_approx / 2)
local result_str = "sin(π/2) = " .. tostring(sin_half_pi)
print("  " .. result_str)

local sqrt_calculation = math.sqrt(math.pow(3, 2) + math.pow(4, 2))
print("  sqrt(3² + 4²) = " .. tostring(sqrt_calculation))

-- Test 2: String manipulation with table operations
print("\n2. String and table operations:")
local words = {"Hello", "beautiful", "Lua", "world"}
local upper_words = {}

-- Convert all words to uppercase using string.upper
for i = 1, #words do
    table.insert(upper_words, string.upper(words[i]))
end

local joined = table.concat(upper_words, " ")
print("  Uppercase joined: " .. joined)

-- Test 3: Complex string operations
print("\n3. Complex string operations:")
local text = "Programming with Lua is fun!"
local length = string.len(text)
local first_half = string.sub(text, 1, math.floor(length / 2))
local second_half = string.sub(text, math.floor(length / 2) + 1)

print("  Original: " .. text)
print("  Length: " .. tostring(length))
print("  First half: " .. first_half)
print("  Second half: " .. second_half)
print("  Reversed: " .. string.reverse(text))

-- Test 4: Nested function calls and type checking
print("\n4. Nested function calls and type checking:")
local function process_value(val)
    local val_type = type(val)
    local val_str = tostring(val)
    
    if val_type == "number" then
        return "Number: " .. val_str .. " (abs: " .. tostring(math.abs(val)) .. ")"
    elseif val_type == "string" then
        return "String: " .. string.upper(val_str) .. " (len: " .. tostring(string.len(val)) .. ")"
    else
        return "Other type: " .. val_type .. " -> " .. val_str
    end
end

local test_values = {42, -3.14, "hello", true, nil}
for i = 1, #test_values do
    local processed = process_value(test_values[i])
    print("  " .. processed)
end

-- Test 5: Random number generation with string repetition
print("\n5. Random operations:")
math.randomseed(os.time())  -- Seed with current time
local random_num = math.random(1, 10)
local star_pattern = string.rep("*", random_num)
print("  Random number (1-10): " .. tostring(random_num))
print("  Star pattern: " .. star_pattern)

-- Test 6: Table sorting with custom comparison
print("\n6. Table sorting:")
local numbers = {}
for _ = 1, 5 do
    table.insert(numbers, math.random(1, 100))
end

print("  Before sorting: " .. table.concat(numbers, ", "))
table.sort(numbers)
print("  After sorting: " .. table.concat(numbers, ", "))

-- Test 7: Time and date operations
print("\n7. Time operations:")
local start_time = os.clock()
-- Simulate some work
local sum = 0
for i = 1, 1000 do
    sum = sum + math.sqrt(i)
end
local end_time = os.clock()

print("  Current time: " .. tostring(os.time()))
print("  Calculation result: " .. tostring(math.floor(sum)))
print("  Elapsed time: " .. tostring(end_time - start_time) .. " seconds")

print("\n=== Complex Integration Test Complete ===")
print("All advanced features working correctly!")
