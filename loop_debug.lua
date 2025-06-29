-- Loop debugging

print("=== Testing While Loop ===")
local count = 3
print("Before while loop, count = " .. count)
print("Condition count > 0: " .. tostring(count > 0))

while count > 0 do
    print("Inside while loop, count = " .. count)
    count = count - 1
    print("After decrement, count = " .. count)
end
print("After while loop")

print("")
print("=== Testing Repeat-Until Loop ===")
local x = 1
print("Before repeat loop, x = " .. x)

repeat
    print("Inside repeat loop, x = " .. x)
    x = x * 2
    print("After multiply, x = " .. x)
    print("Condition x > 4: " .. tostring(x > 4))
until x > 4

print("After repeat loop")

print("")
print("=== Testing Array Access ===")
local arr = {1, 5, 3, 9, 2, 8, 4}
print("arr[1] = " .. arr[1])
print("arr[2] = " .. arr[2])
print("arr[3] = " .. arr[3])
print("arr[4] = " .. arr[4])
print("arr[5] = " .. arr[5])
print("arr[6] = " .. arr[6])
print("arr[7] = " .. arr[7])

local max = arr[1]
print("Initial max = " .. max)
for i = 2, #arr do
    print("Checking arr[" .. i .. "] = " .. arr[i])
    print("max = " .. max)
    print("arr[" .. i .. "] > max: " .. tostring(arr[i] > max))
    if arr[i] > max then
        print("Updating max to " .. arr[i])
        max = arr[i]
    end
end
print("Final max = " .. max)
