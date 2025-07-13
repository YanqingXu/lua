-- 简单的可变参数调试
print("开始调试")

local function test(a, ...)
    print("固定参数 a:", a)
    local first_vararg = ...
    print("第一个可变参数:", first_vararg)
end

print("调用 test(1, 2, 3)")
test(1, 2, 3)

print("调试完成")
