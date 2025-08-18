-- Medium range for loop test
print("Starting medium range for loop test")
local sum = 0
for i = 1, 10 do
    sum = sum + i
end
print("Sum from 1 to 10 =", sum)
print("Expected = 55")

-- Test larger range
sum = 0
for i = 1, 100 do
    sum = sum + i
end
print("Sum from 1 to 100 =", sum)
print("Expected = 5050")
