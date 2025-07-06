-- Debug condition evaluation
print("Testing condition evaluation:")

local i = 1
print("i = " .. i)
print("i <= 2: " .. tostring(i <= 2))

if i <= 2 then
    print("Condition is true, entering if block")
else
    print("Condition is false, not entering if block")
end

print("Testing simple while with debug:")
local count = 1
print("Before while: count = " .. count)
print("Condition count <= 1: " .. tostring(count <= 1))

while count <= 1 do
    print("Inside while: count = " .. count)
    count = count + 1
    print("After increment: count = " .. count)
    print("New condition count <= 1: " .. tostring(count <= 1))
end

print("After while: count = " .. count)
