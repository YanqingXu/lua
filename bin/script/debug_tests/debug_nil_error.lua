-- Debug nil error
print("Testing for loop with local variable:")
for i = 1, 3 do
    print("i = " .. i)
    local square = i * i
    print("square = " .. square)
    print("  " .. i .. "^2 = " .. square)
end
print("Done")
