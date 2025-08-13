print("=== If Debug Test ===")

local i = 1
local limit = 3

print("i =", i)
print("limit =", limit)

-- Test direct boolean values
local cond1 = (i <= limit)
print("cond1 = (i <= limit) =", cond1)

if cond1 then
    print("cond1 is true in if statement")
else
    print("cond1 is false in if statement")
end

-- Test direct comparison in if
if i <= limit then
    print("i <= limit is true in direct if")
else
    print("i <= limit is false in direct if")
end

print("=== If Debug Test Complete ===")
