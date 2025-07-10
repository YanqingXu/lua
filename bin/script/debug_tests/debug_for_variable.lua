-- Debug for loop variable
print("=== Testing simple for loop ===")
for i = 1, 3 do
    print("i = " .. i)
    print("type(i) = " .. type(i))
    local result = i * i
    print("i * i = " .. result)
end
print("Done")
