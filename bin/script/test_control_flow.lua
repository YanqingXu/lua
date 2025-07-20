-- Control Flow Features Test
-- Tests if/else, loops, and control structures

print("=== Control Flow Features Test ===")

local test_count = 0
local passed_count = 0

function test_assert(condition, test_name)
    test_count = test_count + 1
    if condition then
        passed_count = passed_count + 1
        print("[OK] " .. test_name)
    else
        print("[FAILED] " .. test_name)
    end
end

-- If-Else Statements
print("Testing if-else statements...")

local x = 10
local result1 = ""
if x > 5 then
    result1 = "greater"
else
    result1 = "lesser"
end
test_assert(result1 == "greater", "if-else (condition true)")

local y = 3
local result2 = ""
if y > 5 then
    result2 = "greater"
else
    result2 = "lesser"
end
test_assert(result2 == "lesser", "if-else (condition false)")

-- Nested If Statements
print("Testing nested if statements...")

local score = 85
local grade = ""
if score >= 90 then
    grade = "A"
else
    if score >= 80 then
        grade = "B"
    else
        if score >= 70 then
            grade = "C"
        else
            grade = "F"
        end
    end
end
test_assert(grade == "B", "Nested if statements")

-- For Loops (numeric)
print("Testing for loops...")

local sum = 0
for i = 1, 5 do
    sum = sum + i
end
test_assert(sum == 15, "For loop (1 to 5)")

local product = 1
for i = 1, 4 do
    product = product * i
end
test_assert(product == 24, "For loop (factorial-like)")

-- For Loops with Step
print("Testing for loops with step...")

local even_sum = 0
for i = 2, 10, 2 do
    even_sum = even_sum + i
end
test_assert(even_sum == 30, "For loop with step (even numbers)")

-- While Loops
print("Testing while loops...")

local count = 0
local total = 0
while count < 4 do
    count = count + 1
    total = total + count
end
test_assert(total == 10, "While loop")

local countdown = 5
local countdown_result = 0
while countdown > 0 do
    countdown_result = countdown_result + countdown
    countdown = countdown - 1
end
test_assert(countdown_result == 15, "While loop countdown")

-- Repeat-Until Loops
print("Testing repeat-until loops...")

local repeat_count = 0
local repeat_sum = 0
repeat
    repeat_count = repeat_count + 1
    repeat_sum = repeat_sum + repeat_count
until repeat_count >= 3
test_assert(repeat_sum == 6, "Repeat-until loop")

-- Break Statement
print("Testing break statement...")

local break_sum = 0
for i = 1, 10 do
    if i > 5 then
        break
    end
    break_sum = break_sum + i
end
test_assert(break_sum == 15, "Break statement in for loop")

-- Multiple Conditions
print("Testing multiple conditions...")

local a = 5
local b = 10
local multi_result = ""
if a > 0 and b > 0 then
    multi_result = "both_positive"
else
    multi_result = "not_both_positive"
end
test_assert(multi_result == "both_positive", "Multiple conditions (and)")

local c = 5
local d = -2
local or_result = ""
if c > 0 or d > 0 then
    or_result = "at_least_one_positive"
else
    or_result = "both_negative"
end
test_assert(or_result == "at_least_one_positive", "Multiple conditions (or)")

-- Summary
print("")
print("=== Control Flow Features Test Summary ===")
print("Total tests: " .. tostring(test_count))
print("Passed tests: " .. tostring(passed_count))

local failed_count = test_count - passed_count
print("Failed tests: " .. tostring(failed_count))

if passed_count == test_count then
    print("All control flow features working correctly!")
else
    print("Some control flow features failed")
end

print("=== Control Flow Features Test Completed ===")
