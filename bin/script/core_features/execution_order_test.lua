-- Test execution order
print("=== Execution Order Test ===")

print("Step 1: Before defining x")
local x = 42
print("Step 2: After defining x, x =", x)

print("Step 3: Before defining function")
function test()
    print("Step 5: Inside function, x =", x)
    return x
end
print("Step 4: After defining function")

print("Step 6: Before calling function")
local result = test()
print("Step 7: After calling function, result =", result)

print("=== Execution order test completed ===")
