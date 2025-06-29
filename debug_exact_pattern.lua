-- Exact pattern from test.lua
print("For Loop (Square Numbers):")
for i = 1, 5 do
    local square = i * i
    print("  " .. i .. "^2 = " .. square)
end
print("")

print("Repeat-until Loop:")
local x = 1
repeat
    print("  x = " .. x)
    x = x * 2
until x > 10
print("")
