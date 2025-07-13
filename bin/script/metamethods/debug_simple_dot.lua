-- Debug simple dot access
print("=== Debug Simple Dot Access ===")

-- Test 1: Simple table with function
local t = {
    f = function() return "test" end
}

print("t.f:", t.f)
print("t.f ~= nil:", t.f ~= nil)
print("t['f'] ~= nil:", t["f"] ~= nil)

print("\n=== Debug completed ===")
