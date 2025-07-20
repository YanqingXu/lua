-- Debug break statement
print("=== Break Statement Debug ===")

-- Test break in for loop
local sum = 0
for i = 1, 10 do
    print("Loop iteration:", i)
    if i > 3 then
        print("Breaking at i =", i)
        break
    end
    sum = sum + i
    print("Sum so far:", sum)
end
print("Final sum:", sum)
print("Expected sum: 6 (1+2+3)")

print("=== End ===")
