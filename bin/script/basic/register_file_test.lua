-- RegisterFile功能测试脚本
-- 测试基本的寄存器操作和VM指令执行

print("=== RegisterFile功能测试 ===")

-- 测试1：基本变量赋值（测试寄存器操作）
print("测试1：基本变量赋值")
local a = 42
local b = "hello"
local c = true
print("a =", a)
print("b =", b)
print("c =", c)

-- 测试2：变量间赋值（测试MOVE指令）
print("\n测试2：变量间赋值")
local x = a
local y = b
print("x = a:", x)
print("y = b:", y)

-- 测试3：nil值处理（测试LOADNIL指令）
print("\n测试3：nil值处理")
local n1, n2, n3 = nil, nil, nil
print("n1 =", n1)
print("n2 =", n2)
print("n3 =", n3)

-- 测试4：多重赋值（测试寄存器范围操作）
print("\n测试4：多重赋值")
local p, q, r = 1, 2, 3
print("p =", p)
print("q =", q)
print("r =", r)

-- 测试5：函数调用（测试寄存器在函数调用中的使用）
print("\n测试5：函数调用")
local function test_func(param)
    local local_var = param * 2
    return local_var
end

local result = test_func(10)
print("result =", result)

print("\n=== RegisterFile功能测试完成 ===")
