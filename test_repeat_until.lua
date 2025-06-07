-- Test repeat-until loop syntax

-- Basic repeat-until loop
repeat
    x = x + 1
until x > 10

-- Multiple statements in body
repeat
    print(i)
    i = i + 1
until i >= 5

-- Local variable in body
repeat
    local temp = getValue()
until temp ~= nil

-- Complex condition
repeat
    doSomething()
until condition == true and flag == false

-- Nested repeat-until
repeat
    repeat
        y = y * 2
    until y > 100
until x < 0