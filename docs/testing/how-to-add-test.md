# How to Add a Test — 如何添加测试

## 1. 添加 C++ 单元测试

```cpp
// 1. 在 tests/unit/<module>/ 下创建文件
// tests/unit/compiler/test_my_feature.cpp

#include "test_framework/test_framework.hpp"

TEST_CASE(MyFeatureSuite, TestBasic) {
    // 准备
    LuaState* L = LuaState::create();
    
    // 执行
    VM::execute(L, ...);
    
    // 验证
    ASSERT_EQ(L->getStack().top(), 42.0);
}

// 将测试源文件加入项目测试源清单，保持 CMake 与 Visual Studio 清单一致。
```

## 2. 添加 Lua 脚本测试

```lua
-- 1. 创建脚本
-- tests/lua/regressions/test_my_feature.lua

local result = myFunction(1, 2)
assert(result == 3, "expected 3, got " .. tostring(result))

print("Test passed!")
```

## 3. Test Suite 注册

```cpp
// 测试通过宏自动注册:
TEST_CASE(SuiteName, TestName) {
    // 测试代码
}

// 展开后:
// 1. 创建 TestSuite 对象
// 2. 在构造函数中向 TestRegistry 注册
// 3. main() 自动发现并运行
```

## 4. 常用断言

```cpp
ASSERT_TRUE(condition);
ASSERT_FALSE(condition);
ASSERT_EQ(a, b);
ASSERT_NE(a, b);
ASSERT_GT(a, b);   // a > b
ASSERT_LT(a, b);   // a < b
ASSERT_THROWS(expr, ExceptionType);
```
