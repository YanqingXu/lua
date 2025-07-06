-- Debug scope issue with minimal code
print("=== Test 1: Simple for loop ===")
for i = 1, 2 do
    print("i = " .. i)
end

print("")
print("=== Test 2: For loop with one local ===")
for i = 1, 2 do
    local x = i
    print("i = " .. i .. ", x = " .. x)
end

print("")
print("=== Test 3: For loop with two locals ===")
for i = 1, 2 do
    local x = i
    local y = x
    print("i = " .. i .. ", x = " .. x .. ", y = " .. y)
end
