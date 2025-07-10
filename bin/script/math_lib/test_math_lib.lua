-- Math Library Function Tests
-- Testing mathematical functions

print("=== Math Library Tests ===")

-- Test 1: Basic math functions
print("Test 1: Basic math functions")
print("math.abs(-5) =", math.abs(-5))
print("math.abs(5) =", math.abs(5))
print("math.floor(3.7) =", math.floor(3.7))
print("math.ceil(3.2) =", math.ceil(3.2))
print("math.sqrt(16) =", math.sqrt(16))
print("math.sqrt(2) =", math.sqrt(2))

-- Test 2: Power and logarithmic functions
print("\nTest 2: Power and logarithmic functions")
print("math.pow(2, 3) =", math.pow(2, 3))
print("math.exp(1) =", math.exp(1))
print("math.log(math.exp(1)) =", math.log(math.exp(1)))
print("math.log10(100) =", math.log10(100))

-- Test 3: Trigonometric functions
print("\nTest 3: Trigonometric functions")
print("math.sin(0) =", math.sin(0))
print("math.cos(0) =", math.cos(0))
print("math.tan(0) =", math.tan(0))
print("math.sin(math.pi/2) =", math.sin(math.pi/2))
print("math.cos(math.pi) =", math.cos(math.pi))

-- Test 4: Inverse trigonometric functions
print("\nTest 4: Inverse trigonometric functions")
print("math.asin(0) =", math.asin(0))
print("math.acos(1) =", math.acos(1))
print("math.atan(0) =", math.atan(0))
print("math.atan2(0, 1) =", math.atan2(0, 1))

-- Test 5: Min/Max functions
print("\nTest 5: Min/Max functions")
print("math.min(1, 2, 3) =", math.min(1, 2, 3))
print("math.max(1, 2, 3) =", math.max(1, 2, 3))
print("math.min(-1, 0, 1) =", math.min(-1, 0, 1))
print("math.max(-1, 0, 1) =", math.max(-1, 0, 1))

-- Test 6: Utility functions
print("\nTest 6: Utility functions")
print("math.fmod(7, 3) =", math.fmod(7, 3))
print("math.deg(math.pi) =", math.deg(math.pi))
print("math.rad(180) =", math.rad(180))

-- Test 7: Random functions
print("\nTest 7: Random functions")
math.randomseed(12345)
print("math.random() =", math.random())
print("math.random(10) =", math.random(10))
print("math.random(5, 15) =", math.random(5, 15))

-- Test 8: Advanced functions
print("\nTest 8: Advanced functions")
print("math.modf(3.14) =", math.modf(3.14))
print("math.frexp(8) =", math.frexp(8))
print("math.ldexp(0.5, 4) =", math.ldexp(0.5, 4))

print("\n=== Math Library Tests Complete ===")
