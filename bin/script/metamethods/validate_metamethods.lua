-- 元表和元方法正确性验证测试
print("=== 元表和元方法正确性验证 ===")

local test_count = 0
local pass_count = 0

-- 测试辅助函数
local function test(name, condition, expected)
    test_count = test_count + 1
    local result = condition
    local success = (result == expected)
    if success then
        pass_count = pass_count + 1
        print("✓ " .. name .. ": PASS")
    else
        print("✗ " .. name .. ": FAIL (expected " .. tostring(expected) .. ", got " .. tostring(result) .. ")")
    end
    return success
end

-- 测试1: 基本元表操作
print("\n--- 测试1: 基本元表操作 ---")
local obj = {value = 42}
local meta = {name = "TestMeta"}

-- 设置元表
setmetatable(obj, meta)
local retrieved = getmetatable(obj)
test("setmetatable/getmetatable", retrieved == meta, true)

-- 测试2: __index 元方法 (表形式)
print("\n--- 测试2: __index 元方法 (表形式) ---")
local t1 = {existing = "original"}
local mt1 = {}
local fallback = {default_value = "from_fallback", missing_key = "found_it"}

mt1.__index = fallback
setmetatable(t1, mt1)

test("__index - 存在的键", t1.existing, "original")
test("__index - 回退查找", t1.default_value, "from_fallback")
test("__index - 不存在的键", t1.nonexistent, nil)

-- 测试3: __newindex 元方法 (表形式)
print("\n--- 测试3: __newindex 元方法 (表形式) ---")
local t2 = {existing = "original"}
local mt2 = {}
local storage = {}

mt2.__newindex = storage
setmetatable(t2, mt2)

-- 修改已存在的键 (应该直接修改)
t2.existing = "modified"
test("__newindex - 修改已存在键", t2.existing, "modified")

-- 添加新键 (应该存储到storage中)
t2.new_key = "new_value"
test("__newindex - 新键存储位置", storage.new_key, "new_value")
test("__newindex - 新键在原表中", t2.new_key, nil)

-- 测试4: 基本算术运算 (无元方法)
print("\n--- 测试4: 基本算术运算 ---")
test("数字加法", 5 + 3, 8)
test("数字减法", 5 - 3, 2)
test("数字乘法", 5 * 3, 15)
test("数字除法", 6 / 2, 3)

-- 测试5: 字符串连接
print("\n--- 测试5: 字符串连接 ---")
test("字符串连接", "Hello" .. " " .. "World", "Hello World")
test("数字字符串连接", "Value: " .. 42, "Value: 42")

-- 测试6: 基本比较运算
print("\n--- 测试6: 基本比较运算 ---")
test("数字相等", 5 == 5, true)
test("数字不等", 5 == 3, false)
test("数字小于", 3 < 5, true)
test("数字大于等于", 5 >= 3, true)
test("字符串相等", "hello" == "hello", true)
test("字符串小于", "abc" < "def", true)

-- 测试7: nil 和 boolean 比较
print("\n--- 测试7: nil 和 boolean 比较 ---")
test("nil 相等", nil == nil, true)
test("boolean 相等", true == true, true)
test("boolean 不等", true == false, false)
test("nil 与其他类型", nil == 0, false)

-- 测试8: 表的默认行为
print("\n--- 测试8: 表的默认行为 ---")
local table1 = {a = 1}
local table2 = {a = 1}
local table3 = table1

test("表引用相等", table1 == table3, true)
test("表内容不等 (不同引用)", table1 == table2, false)

-- 测试9: 元方法查找机制
print("\n--- 测试9: 元方法查找机制 ---")
local t3 = {}
local mt3 = {}
local mt3_meta = {}

-- 创建元表的元表
mt3_meta.__index = {inherited_value = "from_meta_meta"}
setmetatable(mt3, mt3_meta)

mt3.__index = mt3  -- 元表指向自己
setmetatable(t3, mt3)

test("元表继承查找", t3.inherited_value, "from_meta_meta")

-- 测试10: rawget/rawset 行为
print("\n--- 测试10: rawget/rawset 行为 ---")
local t4 = {direct = "direct_value"}
local mt4 = {__index = {fallback = "fallback_value"}}
setmetatable(t4, mt4)

test("rawget - 存在的键", rawget(t4, "direct"), "direct_value")
test("rawget - 不存在的键", rawget(t4, "fallback"), nil)
test("普通访问 - 回退查找", t4.fallback, "fallback_value")

rawset(t4, "raw_key", "raw_value")
test("rawset 设置", t4.raw_key, "raw_value")

-- 测试结果统计
print("\n=== 测试结果统计 ===")
print("总测试数: " .. test_count)
print("通过测试: " .. pass_count)
print("失败测试: " .. (test_count - pass_count))
print("通过率: " .. string.format("%.1f%%", (pass_count / test_count) * 100))

if pass_count == test_count then
    print("🎉 所有测试通过！元表和元方法实现正确。")
else
    print("⚠️  有测试失败，需要检查实现。")
end

print("\n=== 验证完成 ===")
