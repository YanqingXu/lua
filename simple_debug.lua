-- Simple debug tests

print("=== Testing Basic Operations ===")
local a = 0
local b = 1
print("a = " .. a)
print("b = " .. b)

local temp = a + b
print("temp = a + b = " .. temp)
a = b
print("a = b, now a = " .. a)
b = temp
print("b = temp, now b = " .. b)

print("")
print("=== Testing Array Access ===")
local arr = {1, 5, 3, 9, 2, 8, 4}
print("arr[1] = " .. arr[1])
print("arr[2] = " .. arr[2])
print("arr[3] = " .. arr[3])
print("arr[4] = " .. arr[4])
print("#arr = " .. #arr)

print("")
print("=== Testing Comparison ===")
local x = 5
local y = 9
print("x = " .. x)
print("y = " .. y)
print("x > y: " .. tostring(x > y))
print("y > x: " .. tostring(y > x))

print("")
print("=== Testing Simple While Loop ===")
local count = 3
print("Initial count = " .. count)
while count > 0 do
    print("In loop, count = " .. count)
    count = count - 1
    print("After decrement, count = " .. count)
end
print("After loop")

print("")
print("=== Testing Simple Repeat-Until ===")
local z = 1
print("Initial z = " .. z)
repeat
    print("In repeat, z = " .. z)
    z = z * 2
    print("After multiply, z = " .. z)
    print("z > 4? " .. tostring(z > 4))
until z > 4
print("After repeat")
