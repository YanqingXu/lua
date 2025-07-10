-- 函数调用测试
print("=== Function Call Test ===")

-- 测试1：简单函数定义和调用
function add(a, b)
    return a + b
end

local result = add(5, 3)
print("add(5, 3) =", result)

-- 测试2：无参数函数
function sayHello()
    return "Hello from function!"
end

local greeting = sayHello()
print("sayHello() =", greeting)

-- 测试3：字符串返回函数
function getName()
    return "Lua"
end

local name = getName()
print("getName() =", name)

print("=== Function test completed ===")
