-- 最终综合测试

print("=== Final Comprehensive Test ===")

-- 测试1：基本算术和变量
print("Test 1: Basic arithmetic")
local a = 15
local b = 4
print("a =", a, ", b =", b)
print("a + b =", a + b)
print("a - b =", a - b)
print("a * b =", a * b)
print("a / b =", a / b)

-- 测试2：比较操作
print("\nTest 2: Comparison operations")
print("a > b:", a > b)
print("a < b:", a < b)
print("a == b:", a == b)
print("a >= b:", a >= b)
print("a <= b:", a <= b)

-- 测试3：函数定义和调用
print("\nTest 3: Functions")
function max(x, y)
    if x > y then
        return x
    else
        return y
    end
end

function factorial(n)
    if n <= 1 then
        return 1
    else
        return n * factorial(n - 1)
    end
end

print("max(10, 7) =", max(10, 7))
print("max(3, 8) =", max(3, 8))
print("factorial(5) =", factorial(5))

-- 测试4：字符串操作
print("\nTest 4: String operations")
local name = "Lua"
local version = "5.1"
local message = "Welcome to " .. name .. " " .. version .. "!"
print("message =", message)

-- 测试5：循环
print("\nTest 5: Loops")
print("For loop (1 to 5):")
for i = 1, 5 do
    print("  i =", i, ", i^2 =", i * i)
end

print("\nWhile loop (countdown from 3):")
local count = 3
while count > 0 do
    print("  count =", count)
    count = count - 1
end

print("\n=== All tests completed successfully! ===")
