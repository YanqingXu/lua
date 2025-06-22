# 测试框架内存泄漏检测集成指南

## 概述

本指南介绍了如何在Lua编译器测试框架中使用集成的内存泄漏检测功能。测试框架现在自动集成了内存泄漏检测、超时检测、死锁检测和递归检测功能。

## 功能特性

### 1. 自动内存泄漏检测
- 所有测试宏现在自动包含内存泄漏检测
- 无需手动添加内存检测代码
- 测试结束时自动报告内存泄漏

### 2. 分层检测支持
- **INDIVIDUAL级别**: 单个测试方法的内存检测
- **GROUP级别**: 测试组的内存检测
- **SUITE级别**: 测试套件的内存检测
- **MODULE级别**: 模块级别的内存检测
- **MAIN级别**: 全局测试的内存检测

### 3. 综合检测功能
- 内存泄漏检测
- 超时检测
- 死锁检测
- 递归深度检测

## 测试宏使用指南

### 基础测试宏（自动内存检测）

#### RUN_TEST - 标准测试执行
```cpp
// 自动包含内存泄漏检测
RUN_TEST(BinaryExprTest, testAddition);
```

#### SAFE_RUN_TEST - 安全测试执行
```cpp
// 异常安全版本，包含内存泄漏检测
SAFE_RUN_TEST(BinaryExprTest, testAddition);
```

### 综合检测宏

#### RUN_COMPREHENSIVE_TEST - 全面检测
```cpp
// 包含所有检测功能，自定义超时
RUN_COMPREHENSIVE_TEST(BinaryExprTest, testComplexOperation, 5000); // 5秒超时
```

#### RUN_COMPREHENSIVE_TEST_DEFAULT - 默认超时
```cpp
// 使用默认30秒超时的全面检测
RUN_COMPREHENSIVE_TEST_DEFAULT(BinaryExprTest, testStandardOperation);
```

#### SAFE_RUN_COMPREHENSIVE_TEST - 安全全面检测
```cpp
// 异常安全的全面检测
SAFE_RUN_COMPREHENSIVE_TEST(BinaryExprTest, testRiskyOperation, 10000);
```

### 增强的分层测试宏

#### RUN_TEST_GROUP_WITH_MEMORY_CHECK - 组级内存检测
```cpp
// 为整个测试组添加内存检测
RUN_TEST_GROUP_WITH_MEMORY_CHECK("Binary Expression Tests", testBinaryExpressions);
```

#### RUN_TEST_SUITE_WITH_MEMORY_CHECK - 套件级内存检测
```cpp
// 为整个测试套件添加内存检测
RUN_TEST_SUITE_WITH_MEMORY_CHECK(ExprTestSuite);
```

## 实际使用示例

### 示例1: 基础测试类
```cpp
namespace Lua {
namespace Tests {

class ParserTest {
public:
    // 测试方法不需要手动添加内存检测代码
    static void testBasicParsing() {
        // 测试逻辑
        std::string source = "local x = 1";
        auto result = parseSource(source);
        assert(result.isValid());
    }
    
    static void testComplexParsing() {
        // 复杂测试逻辑
        std::string source = R"(
            function factorial(n)
                if n <= 1 then
                    return 1
                else
                    return n * factorial(n - 1)
                end
            end
        )";
        auto result = parseSource(source);
        assert(result.isValid());
    }
};

// 测试组函数
void runParserBasicTests() {
    RUN_TEST(ParserTest, testBasicParsing);           // 自动内存检测
    RUN_TEST(ParserTest, testComplexParsing);         // 自动内存检测
}

void runParserAdvancedTests() {
    // 使用综合检测进行复杂测试
    RUN_COMPREHENSIVE_TEST(ParserTest, testComplexParsing, 15000); // 15秒超时
}

} // namespace Tests
} // namespace Lua
```

### 示例2: 测试套件结构
```cpp
namespace Lua {
namespace Tests {

class ParserTestSuite {
public:
    static void runAllTests() {
        // 使用增强的组级内存检测
        RUN_TEST_GROUP_WITH_MEMORY_CHECK("Basic Parser Tests", runParserBasicTests);
        RUN_TEST_GROUP_WITH_MEMORY_CHECK("Advanced Parser Tests", runParserAdvancedTests);
        RUN_TEST_GROUP_WITH_MEMORY_CHECK("Error Handling Tests", runParserErrorTests);
    }
};

} // namespace Tests
} // namespace Lua
```

### 示例3: 模块级测试
```cpp
// 在test_main.hpp中
void runAllTests() {
    // 使用增强的套件级内存检测
    RUN_TEST_SUITE_WITH_MEMORY_CHECK(ParserTestSuite);
    RUN_TEST_SUITE_WITH_MEMORY_CHECK(LexerTestSuite);
    RUN_TEST_SUITE_WITH_MEMORY_CHECK(VMTestSuite);
}
```

## 内存检测报告

### 正常情况输出
```
[INFO] Running ParserTest::testBasicParsing...
[PASS] ParserTest::testBasicParsing
[MEMORY] No memory leaks detected in ParserTest::testBasicParsing
```

### 内存泄漏检测输出
```
[INFO] Running ParserTest::testWithLeak...
[PASS] ParserTest::testWithLeak
[MEMORY LEAK] Memory leak detected in ParserTest::testWithLeak:
  - 1 allocation(s) not freed
  - Total leaked: 64 bytes
  - Location: parser.cpp:123 in function parseExpression
```

### 综合检测输出
```
[INFO] Running comprehensive test ParserTest::testComplexOperation...
[PASS] ParserTest::testComplexOperation
[MEMORY] No memory leaks detected
[TIMEOUT] Test completed within timeout (2.5s / 5.0s)
[DEADLOCK] No deadlocks detected
[RECURSION] Maximum recursion depth: 15 (safe)
```

## 最佳实践

### 1. 选择合适的测试宏
- **简单测试**: 使用 `RUN_TEST` 或 `SAFE_RUN_TEST`
- **复杂测试**: 使用 `RUN_COMPREHENSIVE_TEST`
- **长时间运行**: 使用自定义超时的综合测试
- **批量测试**: 使用组级或套件级内存检测

### 2. 超时设置建议
- **快速测试**: 1-5秒
- **标准测试**: 5-15秒
- **复杂测试**: 15-60秒
- **性能测试**: 60秒以上

### 3. 内存检测策略
- 在开发阶段使用综合检测
- 在CI/CD中使用标准内存检测
- 对关键模块使用套件级检测

### 4. 错误处理
- 使用 `SAFE_RUN_*` 宏进行批量测试
- 使用标准 `RUN_*` 宏进行关键测试
- 根据测试重要性选择异常处理策略

## 迁移指南

### 从手动内存检测迁移

**旧代码**:
```cpp
void testFunction() {
    AUTO_MEMORY_LEAK_TEST_GUARD();
    // 测试逻辑
}

void runTests() {
    RUN_TEST(TestClass, testFunction);
}
```

**新代码**:
```cpp
void testFunction() {
    // 不需要手动添加内存检测
    // 测试逻辑
}

void runTests() {
    RUN_TEST(TestClass, testFunction); // 自动包含内存检测
}
```

### 升级到综合检测

**标准检测**:
```cpp
RUN_TEST(TestClass, testMethod);
```

**综合检测**:
```cpp
RUN_COMPREHENSIVE_TEST(TestClass, testMethod, 10000);
```

## 配置选项

### 编译时配置
- 在Release模式下，检测功能会被自动禁用
- 在Debug模式下，所有检测功能默认启用

### 运行时配置
- 可以通过环境变量控制检测行为
- 支持动态启用/禁用特定检测功能

## 故障排除

### 常见问题

1. **编译错误**: 确保包含了正确的头文件
2. **链接错误**: 确保链接了内存检测库
3. **误报**: 检查是否有静态变量或全局变量
4. **性能影响**: 在Release模式下检测会被禁用

### 调试技巧

1. 使用 `MEMORY_CHECKPOINT` 添加检查点
2. 使用 `ASSERT_NO_MEMORY_LEAKS` 进行断言检查
3. 查看详细的内存分配报告
4. 使用调试器跟踪内存分配

## 总结

通过集成内存泄漏检测到测试框架中，我们实现了:

1. **自动化**: 无需手动添加检测代码
2. **分层化**: 支持不同级别的检测
3. **综合化**: 集成多种检测功能
4. **易用性**: 简单的宏接口
5. **可配置**: 灵活的配置选项

这种集成方式大大提高了测试的质量和开发效率，帮助及早发现和解决内存相关问题。