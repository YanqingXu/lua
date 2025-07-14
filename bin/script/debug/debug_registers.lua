-- 调试寄存器值
print("=== Register Debug Test ===")

-- 测试1: 简单的变量赋值和访问
print("\nTest 1: Simple variable assignment")
local x = 42
print("x =", x)

-- 测试2: 简单的函数调用
print("\nTest 2: Simple function call")
local function testFunc(a)
    print("testFunc called with a =", a)
    return a + 1
end

local result = testFunc(10)
print("testFunc result =", result)

-- 测试3: 表创建和访问
print("\nTest 3: Table creation and access")
local t = {value = 123}
print("t.value =", t.value)

-- 测试4: 最简单的__call测试
print("\nTest 4: Simplest __call test")
local obj = {}
local meta = {}
meta.__call = function(self, arg)
    print("__call: self =", self, "arg =", arg)
    return "ok"
end
setmetatable(obj, meta)

print("About to call obj(999)...")
local callResult = obj(999)
print("Call result =", callResult)

print("\n=== Test completed ===")
