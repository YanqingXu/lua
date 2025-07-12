-- Core functionality test
print("=== Core Functionality Test ===")

-- Test 1: Table literal initialization
print("Test 1: Table literal initialization")
local t1 = {x = 10, y = 20}
print("t1.x should be 10, actual:", t1.x)
print("t1.y should be 20, actual:", t1.y)

-- Test 2: Dynamic field assignment
print("Test 2: Dynamic field assignment")
local t2 = {}
t2.a = 30
t2.b = 40
print("t2.a should be 30, actual:", t2.a)
print("t2.b should be 40, actual:", t2.b)

-- Test 3: String field values
print("Test 3: String field values")
local t3 = {name = "test", type = "example"}
print("t3.name should be test, actual:", t3.name)
print("t3.type should be example, actual:", t3.type)

-- Test 4: Mixed types
print("Test 4: Mixed types")
local t4 = {}
t4.num = 100
t4.str = "hello"
t4.bool = true
print("t4.num should be 100, actual:", t4.num)
print("t4.str should be hello, actual:", t4.str)
print("t4.bool should be true, actual:", t4.bool)

-- Test 5: Simple metatable test
print("Test 5: Simple metatable test")
local obj = {}
local meta = {}
obj.existing = "exists"
meta.metamethod = "present"

print("Before setmetatable:")
print("obj.existing =", obj.existing)
print("meta.metamethod =", meta.metamethod)

setmetatable(obj, meta)
print("setmetatable completed")

print("=== Core functionality test completed ===")
