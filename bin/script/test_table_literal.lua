-- 测试表字面量键值对赋值问题
print("=== 表字面量键值对赋值测试 ===")

-- 测试1: 基本的表字面量
print("\n1. 基本表字面量测试:")
local t1 = {a = 1, b = 2, c = 3}
print("t1.a =", t1.a)
print("t1.b =", t1.b) 
print("t1.c =", t1.c)

-- 测试2: 混合索引类型
print("\n2. 混合索引类型测试:")
local t2 = {
    name = "test",
    [1] = "first",
    [2] = "second",
    ["key"] = "value"
}
print("t2.name =", t2.name)
print("t2[1] =", t2[1])
print("t2[2] =", t2[2])
print("t2['key'] =", t2["key"])

-- 测试3: 嵌套表字面量
print("\n3. 嵌套表字面量测试:")
local t3 = {
    inner = {x = 10, y = 20},
    data = {a = 1, b = 2}
}
print("t3.inner.x =", t3.inner.x)
print("t3.inner.y =", t3.inner.y)
print("t3.data.a =", t3.data.a)
print("t3.data.b =", t3.data.b)

-- 测试4: 表达式作为值
print("\n4. 表达式作为值测试:")
local t4 = {
    sum = 1 + 2,
    product = 3 * 4,
    str = "hello" .. " world"
}
print("t4.sum =", t4.sum)
print("t4.product =", t4.product)
print("t4.str =", t4.str)

-- 测试5: 函数作为值
print("\n5. 函数作为值测试:")
local t5 = {
    func = function(x) return x * 2 end,
    add = function(a, b) return a + b end
}
print("t5.func(5) =", t5.func(5))
print("t5.add(3, 4) =", t5.add(3, 4))

-- 测试6: 空表和nil值
print("\n6. 空表和nil值测试:")
local t6 = {
    empty = {},
    nilval = nil,
    zero = 0,
    false_val = false
}
print("t6.empty =", t6.empty)
print("t6.nilval =", t6.nilval)
print("t6.zero =", t6.zero)
print("t6.false_val =", t6.false_val)

print("\n=== 测试完成 ===")
