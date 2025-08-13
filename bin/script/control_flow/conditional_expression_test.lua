print("=== Conditional Expression Test ===")

-- Test conditional expressions with working comparisons
print("Testing conditional expressions:")
local val = 10
print("  val =", val)

-- This should now work since comparisons are fixed
local big = val > 5
print("  val > 5 =", big)

local small = val < 5  
print("  val < 5 =", small)

-- Test ternary-like expression
-- local msg = val > 5 and "big" or "small"
-- print("  val > 5 ? 'big' : 'small' =", msg)

print("Test complete")
