-- 测试__tostring元方法
print("=== __tostring Metamethod Test ===")

-- 测试1: 基础tostring功能
print("\n--- Test 1: Basic tostring functionality ---")
print("tostring(42):", tostring(42))
print("tostring(true):", tostring(true))
print("tostring('hello'):", tostring("hello"))
print("tostring(nil):", tostring(nil))

-- 测试2: 表的默认字符串表示
print("\n--- Test 2: Default table string representation ---")
local table1 = {a = 1, b = 2}
print("tostring(table1):", tostring(table1))

-- 测试3: 自定义__tostring元方法
print("\n--- Test 3: Custom __tostring metamethod ---")
local obj = {name = "TestObject", value = 123}
setmetatable(obj, {
    __tostring = function(self)
        return "Object{name=" .. self.name .. ", value=" .. self.value .. "}"
    end
})

print("tostring(obj):", tostring(obj))

-- 测试4: 复杂的__tostring元方法
print("\n--- Test 4: Complex __tostring metamethod ---")
local person = {name = "Alice", age = 30, city = "Beijing"}
setmetatable(person, {
    __tostring = function(self)
        return self.name .. " (age " .. self.age .. ") from " .. self.city
    end
})

print("tostring(person):", tostring(person))

-- 测试5: __tostring返回非字符串（应该报错）
print("\n--- Test 5: __tostring returning non-string (should error) ---")
local badObj = {}
setmetatable(badObj, {
    __tostring = function(self)
        return 42  -- 返回数字而不是字符串
    end
})

-- 这应该会报错
local success, result = pcall(function()
    return tostring(badObj)
end)

if success then
    print("tostring(badObj):", result)
else
    print("Error (expected):", result)
end

print("\n=== Test completed ===")
