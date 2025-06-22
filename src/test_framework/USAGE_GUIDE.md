# Lua测试框架使用指南

## 概述

Lua测试框架2.0是一个现代化、模块化的C++测试框架，专为Lua解释器项目设计。它提供了丰富的测试功能、内存安全检测、美观的输出格式和灵活的配置选项。

## 快速开始

### 1. 基本使用

```cpp
#include "test_framework/test_framework.hpp"

class MyTestSuite {
public:
    static void runAllTests() {
        RUN_TEST_GROUP("Basic Tests", runBasicTests);
    }
    
private:
    static void runBasicTests() {
        RUN_TEST(MyTestClass, testAddition);
        RUN_TEST(MyTestClass, testSubtraction);
    }
};

class MyTestClass {
public:
    static void testAddition() {
        int result = 2 + 3;
        if (result != 5) {
            throw std::runtime_error("Addition test failed");
        }
    }
    
    static void testSubtraction() {
        int result = 5 - 3;
        if (result != 2) {
            throw std::runtime_error("Subtraction test failed");
        }
    }
};

int main() {
    RUN_ALL_TESTS(MyTestSuite);
    return 0;
}
```

### 2. 使用便捷宏

```cpp
// 快速初始化
QUICK_INIT_LUA_TEST_FRAMEWORK();

// 运行所有测试
RUN_ALL_TESTS(MyTestSuite);

// 运行模块测试
RUN_MODULE_TESTS(Parser, ParserTestSuite);

// 运行快速测试（适用于CI/CD）
RUN_QUICK_TESTS(QuickTestSuite);

// 运行内存安全测试
RUN_MEMORY_SAFE_TESTS(MemoryTestSuite);
```

## 核心概念

### 测试层次结构

测试框架支持多层次的测试组织：

1. **MAIN** - 主测试级别（整个测试套件）
2. **MODULE** - 模块级别（如lexer、parser等）
3. **SUITE** - 测试套件级别（一组相关测试）
4. **GROUP** - 测试组级别（功能分组）
5. **INDIVIDUAL** - 单个测试级别

### 测试宏

#### 基础测试宏

```cpp
// 运行单个测试
RUN_TEST(TestClass, testMethod);

// 运行主测试
RUN_MAIN_TEST(MainTestClass, runAllTests);

// 运行模块测试
RUN_TEST_MODULE(ModuleTestClass, runModuleTests);

// 运行测试套件
RUN_TEST_SUITE(TestSuiteClass);

// 运行测试组
RUN_TEST_GROUP("Group Name", TestClass::runGroupTests);
```

#### 内存检测宏

```cpp
// 带内存检测的测试组
RUN_TEST_GROUP_WITH_MEMORY_CHECK("Memory Tests", TestClass::runMemoryTests);

// 带内存检测的测试套件
RUN_TEST_SUITE_WITH_MEMORY_CHECK(MemoryTestSuite);

// 内存泄漏检测守卫
MEMORY_LEAK_TEST_GUARD("Test Name");

// 带超时的内存检测
MEMORY_LEAK_TEST_GUARD_WITH_TIMEOUT("Test Name", 5000); // 5秒超时

// 条件内存检测
CONDITIONAL_MEMORY_LEAK_TEST_GUARD(condition, "Test Name");

// 内存使用报告
MEMORY_USAGE_REPORT("Operation Name");
```

## 高级功能

### 1. 内存安全测试

```cpp
class MemoryTestClass {
public:
    static void testMemoryAllocation() {
        MEMORY_LEAK_TEST_GUARD("Memory Allocation Test");
        
        // 分配内存
        int* ptr = new int[100];
        
        // 使用内存
        for (int i = 0; i < 100; ++i) {
            ptr[i] = i;
        }
        
        // 释放内存
        delete[] ptr;
        
        // 框架会自动检测内存泄漏
    }
    
    static void testWithTimeout() {
        MEMORY_LEAK_TEST_GUARD_WITH_TIMEOUT("Timeout Test", 3000);
        
        // 执行可能耗时的操作
        // 如果超过3秒会自动终止
    }
};
```

### 2. 自定义输出和格式化

```cpp
// 设置颜色主题
TestUtils::setTheme(ColorTheme::MODERN);
TestUtils::setTheme(ColorTheme::CLASSIC);
TestUtils::setTheme(ColorTheme::MINIMAL);

// 启用/禁用颜色输出
TestUtils::setColorEnabled(true);
TestUtils::setColorEnabled(false);

// 打印不同级别的信息
TestUtils::printInfo("Information message");
TestUtils::printWarning("Warning message");
TestUtils::printError("Error message");
TestUtils::printException("Exception message");

// 打印层次化标题
TestUtils::printLevelHeader(TestLevel::MODULE, "Parser Module");
TestUtils::printLevelFooter(TestLevel::MODULE, "Parser Module Completed");
```

### 3. 测试统计和报告

```cpp
// 获取测试统计信息
auto stats = TestUtils::getTestStatistics();
std::cout << "Total tests: " << stats.totalTests << std::endl;
std::cout << "Passed: " << stats.passedTests << std::endl;
std::cout << "Failed: " << stats.failedTests << std::endl;

// 生成测试报告
TestUtils::generateTestReport();
```

## 最佳实践

### 1. 测试组织

```cpp
// 推荐的测试组织结构
class ParserTestSuite {
public:
    static void runAllTests() {
        // 按功能分组
        RUN_TEST_GROUP("Expression Parsing", runExpressionTests);
        RUN_TEST_GROUP("Statement Parsing", runStatementTests);
        RUN_TEST_GROUP("Error Recovery", runErrorRecoveryTests);
        
        // 内存安全测试单独分组
        RUN_TEST_GROUP_WITH_MEMORY_CHECK("Memory Safety", runMemorySafetyTests);
    }
    
private:
    static void runExpressionTests() {
        RUN_TEST(ExpressionParser, testBinaryExpression);
        RUN_TEST(ExpressionParser, testUnaryExpression);
        RUN_TEST(ExpressionParser, testFunctionCall);
    }
    
    // ... 其他测试组
};
```

### 2. 错误处理

```cpp
class TestClass {
public:
    static void testWithProperErrorHandling() {
        try {
            // 执行可能失败的操作
            performRiskyOperation();
            
            // 验证结果
            if (!verifyResult()) {
                throw std::runtime_error("Result verification failed");
            }
            
            TestUtils::printInfo("Test passed successfully");
            
        } catch (const std::exception& e) {
            TestUtils::printError("Test failed: " + std::string(e.what()));
            throw; // 重新抛出异常让框架处理
        }
    }
};
```

### 3. 内存管理

```cpp
class MemoryAwareTest {
public:
    static void testWithRAII() {
        MEMORY_LEAK_TEST_GUARD("RAII Test");
        
        // 使用RAII管理资源
        {
            std::unique_ptr<int[]> ptr(new int[1000]);
            // 使用ptr
            ptr[0] = 42;
        } // 自动释放
        
        // 或使用自定义RAII类
        {
            ResourceManager manager;
            manager.allocateResources();
            // 使用资源
        } // 析构函数自动清理
    }
};
```

## 配置选项

### 1. 颜色主题

```cpp
// 现代主题（默认）
TestUtils::setTheme(ColorTheme::MODERN);

// 经典主题
TestUtils::setTheme(ColorTheme::CLASSIC);

// 最小主题
TestUtils::setTheme(ColorTheme::MINIMAL);
```

### 2. 输出控制

```cpp
// 详细输出模式
TestUtils::setVerbose(true);

// 静默模式（仅显示错误）
TestUtils::setSilent(true);

// 禁用颜色（适用于CI/CD）
TestUtils::setColorEnabled(false);
```

### 3. 内存检测配置

```cpp
// 设置内存检测超时
MemoryTestUtils::setDefaultTimeout(5000); // 5秒

// 启用/禁用内存检测
MemoryTestUtils::setMemoryCheckEnabled(true);

// 设置内存泄漏阈值
MemoryTestUtils::setLeakThreshold(1024); // 1KB
```

## 迁移指南

### 从旧框架迁移

1. **更新包含路径**：
   ```cpp
   // 旧方式
   #include "tests/test_utils.hpp"
   
   // 新方式
   #include "test_framework/test_framework.hpp"
   ```

2. **更新命名空间**：
   ```cpp
   // 旧方式
   TestFormatting::TestFormatter::printInfo("message");
   
   // 新方式
   Lua::TestFramework::TestUtils::printInfo("message");
   // 或使用别名
   Lua::Test::TestUtils::printInfo("message");
   ```

3. **使用新的便捷宏**：
   ```cpp
   // 旧方式
   int main() {
       runAllTests();
       return 0;
   }
   
   // 新方式
   int main() {
       RUN_ALL_TESTS(MainTestSuite);
       return 0;
   }
   ```

## 故障排除

### 常见问题

1. **编译错误**：
   - 确保包含了正确的头文件
   - 检查命名空间是否正确
   - 验证测试类和方法是否为静态

2. **内存检测误报**：
   - 检查是否有未释放的资源
   - 确保使用了正确的内存管理模式
   - 考虑调整内存泄漏阈值

3. **测试超时**：
   - 增加超时时间
   - 优化测试代码性能
   - 检查是否有死循环

### 调试技巧

```cpp
// 启用详细输出
TestUtils::setVerbose(true);

// 添加调试信息
TestUtils::printInfo("Debug: Entering test function");
TestUtils::printInfo("Debug: Variable value = " + std::to_string(value));

// 使用内存使用报告
MEMORY_USAGE_REPORT("Before allocation");
// ... 分配内存
MEMORY_USAGE_REPORT("After allocation");
```

## 示例项目

查看 `examples/example_test.hpp` 文件获取完整的使用示例。

## 版本信息

当前版本：2.0.0

要查看版本信息：
```cpp
Lua::TestFramework::Version::printVersion();
```

## 支持和贡献

如果遇到问题或有改进建议，请：
1. 查看本文档和迁移指南
2. 检查示例代码
3. 提交issue或pull request

---

*本文档会随着框架的更新而持续改进。*