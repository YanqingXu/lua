-- Debug register conflict
print("=== Test 1: No local variables ===")
for i = 1, 3 do
    print("i = " .. i)
end

print("")
print("=== Test 2: Local variable without using loop variable ===")
for i = 1, 3 do
    local x = 42
    print("i = " .. i .. ", x = " .. x)
end

print("")
print("=== Test 3: Local variable using loop variable ===")
for i = 1, 3 do
    local x = i
    print("i = " .. i .. ", x = " .. x)
end

print("")
print("=== Test 4: Arithmetic with loop variable ===")
for i = 1, 3 do
    print("Before arithmetic: i = " .. i)
    local result = i * 2
    print("After arithmetic: i = " .. i .. ", result = " .. result)
end
