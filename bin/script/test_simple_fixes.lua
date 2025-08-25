-- Simple fix verification test
print("=== Simple Fix Verification ===")

-- Test comparison instructions
print("1. Comparison instruction test:")
if 5 == 5 then
    print("  EQ: 5 == 5 correct")
end

if 3 < 7 then
    print("  LT: 3 < 7 correct")
end

if 4 <= 4 then
    print("  LE: 4 <= 4 correct")
end

-- Test string concatenation
print("2. String concatenation test:")
local hello = "Hello"
local world = "World"
local exclaim = "!"
local result = hello .. world .. exclaim
print("  Concatenation result: " .. result)

-- Test simple loop
print("3. Loop test:")
for i = 1, 3 do
    if i <= 3 then
        print("  Loop " .. i .. " correct")
    end
end

print("=== Verification complete ===")
