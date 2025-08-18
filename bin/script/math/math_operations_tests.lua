-- Math operations tests
-- Test various functions of the math library

print("=== Math operations tests ===")

-- Test 1: Basic arithmetic operations
print("Test 1: Basic arithmetic operations")
local a = 10
local b = 3
print("  a =", a, ", b =", b)
print("  a + b =", a + b)
print("  a - b =", a - b)
print("  a * b =", a * b)
print("  a / b =", a / b)
print("  a % b =", a % b)
print("  a ^ b =", a ^ b)

-- Test 2: Unary operations
print("\nTest 2: Unary operations")
local x = 5
print("  x =", x)
print("  -x =", -x)

-- Test 3: math.abs function
print("\nTest 3: math.abs function")
if math and math.abs then
    print("  math.abs(-5) =", math.abs(-5))
    print("  math.abs(5) =", math.abs(5))
    print("  math.abs(0) =", math.abs(0))
else
    print("  math.abs function not available")
end

-- Test 4: math.max and math.min
print("\nTest 4: math.max and math.min")
if math and math.max and math.min then
    print("  math.max(1, 5, 3) =", math.max(1, 5, 3))
    print("  math.min(1, 5, 3) =", math.min(1, 5, 3))
    print("  math.max(-1, -5, -3) =", math.max(-1, -5, -3))
else
    print("  math.max/min functions not available")
end

-- Test 5: math.floor and math.ceil
print("\nTest 5: math.floor and math.ceil")
if math and math.floor and math.ceil then
    local num = 3.7
    print("  num =", num)
    print("  math.floor(num) =", math.floor(num))
    print("  math.ceil(num) =", math.ceil(num))
    
    local negNum = -3.7
    print("  negNum =", negNum)
    print("  math.floor(negNum) =", math.floor(negNum))
    print("  math.ceil(negNum) =", math.ceil(negNum))
else
    print("  math.floor/ceil functions not available")
end

-- Test 6: math.sqrt
print("\nTest 6: math.sqrt")
if math and math.sqrt then
    print("  math.sqrt(16) =", math.sqrt(16))
    print("  math.sqrt(2) =", math.sqrt(2))
    print("  math.sqrt(0) =", math.sqrt(0))
else
    print("  math.sqrt function not available")
end

-- Test 7: Trigonometric functions
print("\nTest 7: Trigonometric functions")
if math and math.sin and math.cos and math.tan then
    local angle = math.pi / 4  -- 45 degrees
    print("  angle = π/4")
    print("  math.sin(angle) =", math.sin(angle))
    print("  math.cos(angle) =", math.cos(angle))
    print("  math.tan(angle) =", math.tan(angle))
else
    print("  Trigonometric functions not available")
end

-- Test 8: math.pi constant
print("\nTest 8: math.pi constant")
if math and math.pi then
    print("  math.pi =", math.pi)
    print("  2 * math.pi =", 2 * math.pi)
else
    print("  math.pi constant not available")
end

-- Test 9: math.exp and math.log
print("\nTest 9: math.exp and math.log")
if math and math.exp and math.log then
    print("  math.exp(1) =", math.exp(1))  -- e
    print("  math.log(math.exp(1)) =", math.log(math.exp(1)))  -- should be 1
    print("  math.log(10) =", math.log(10))
else
    print("  math.exp/log functions not available")
end

-- Test 10: math.random
print("\nTest 10: math.random")
if math and math.random then
    print("  math.random() =", math.random())
    print("  math.random(10) =", math.random(10))
    print("  math.random(5, 15) =", math.random(5, 15))
    
    -- Set random seed
    if math.randomseed then
        math.randomseed(12345)
        print("  After setting seed, math.random() =", math.random())
    end
else
    print("  math.random function not available")
end

-- Test 11: math.deg and math.rad
print("\nTest 11: math.deg and math.rad")
if math and math.deg and math.rad then
    print("  math.deg(math.pi) =", math.deg(math.pi))  -- 180 degrees
    print("  math.rad(180) =", math.rad(180))  -- π radians
else
    print("  math.deg/rad functions not available")
end

-- Test 12: math.fmod
print("\nTest 12: math.fmod")
if math and math.fmod then
    print("  math.fmod(10, 3) =", math.fmod(10, 3))
    print("  math.fmod(-10, 3) =", math.fmod(-10, 3))
    print("  10 % 3 =", 10 % 3)
    print("  -10 % 3 =", -10 % 3)
else
    print("  math.fmod function not available")
end

-- Test 13: Numeric precision test
print("\nTest 13: Numeric precision test")
local small = 0.1 + 0.2
print("  0.1 + 0.2 =", small)
print("  0.1 + 0.2 == 0.3:", small == 0.3)

-- Test 14: Big number operations
print("\nTest 14: Big number operations")
local big1 = 1e15
local big2 = 2e15
print("  1e15 + 2e15 =", big1 + big2)
print("  1e15 * 2 =", big1 * 2)

print("\n=== Math operations tests completed ===")
