-- 测试普通参数传递
print("测试普通参数传递")

local function test(a, b, c)
    print("参数 a:", a)
    print("参数 b:", b)
    print("参数 c:", c)
end

print("调用 test(1, 2, 3)")
test(1, 2, 3)

print("测试完成")
