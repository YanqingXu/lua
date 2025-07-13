-- 测试第一个例子
print("测试1:")
local function test1(...)
    local first = ...
    print("第一个参数:", first)
end
test1("hello", "world", 123)
