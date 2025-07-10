-- Debug for loop increment
print("=== Testing for loop increment ===")
for i = 1, 3 do
    print("Start of iteration: i = " .. i)
    print("Type of i: " .. type(i))
    -- Don't do any local variables, just test the loop increment
end
print("Loop completed")
