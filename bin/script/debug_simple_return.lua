-- Debug simple return
print("=== Debug Simple Return ===")

-- Test 1: Function with simple return
function return_five()
    return 5
end

local result1 = return_five()
print("result1 =", result1)
print("Expected: 5")

-- Test 2: Function with variable return
function return_variable()
    local x = 10
    return x
end

local result2 = return_variable()
print("result2 =", result2)
print("Expected: 10")

-- Test 3: Function with arithmetic return
function return_arithmetic()
    return 3 + 4
end

local result3 = return_arithmetic()
print("result3 =", result3)
print("Expected: 7")

print("=== End ===")
