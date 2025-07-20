-- Step by step debug
print("=== Step by Step Debug ===")

-- Test individual values
local a = true
local b = false

print("a =", a)
print("b =", b)

-- Test simple assignment
local result1 = a
print("result1 = a:", result1)

local result2 = b  
print("result2 = b:", result2)

-- Test conditional assignment
local result3
if a then
    result3 = b
else
    result3 = a
end
print("if a then b else a:", result3)

local result4
if a then
    result4 = a
else
    result4 = b
end
print("if a then a else b:", result4)

print("=== End ===")
