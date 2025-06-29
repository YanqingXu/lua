-- Debug local variable scope in for loop
print("=== Test 1: For loop without local variables ===")
for i = 1, 3 do
    print("i = " .. i)
    print("i * i = " .. (i * i))
end

print("")
print("=== Test 2: For loop with local variables ===")
for i = 1, 3 do
    print("i = " .. i)
    local square = i * i
    print("square = " .. square)
end

print("")
print("=== Test 3: For loop with multiple local variables ===")
for i = 1, 3 do
    print("i = " .. i)
    local temp1 = i
    local temp2 = temp1 * temp1
    print("temp2 = " .. temp2)
end
