-- Detailed nil debugging
print("=== Testing step by step ===")

print("Step 1: Simple for loop")
for i = 1, 2 do
    print("  i = " .. i .. " (type: " .. type(i) .. ")")
end

print("")
print("Step 2: For loop with local variable")
for i = 1, 2 do
    print("  Before local: i = " .. i)
    local x = i
    print("  After local: i = " .. i .. ", x = " .. x)
    print("  Types: i=" .. type(i) .. ", x=" .. type(x))
end

print("")
print("Step 3: For loop with arithmetic")
for i = 1, 2 do
    print("  i = " .. i)
    print("  i type = " .. type(i))
    local result = i * i
    print("  result = " .. result)
end
