# 测试输出格式统一指南

本文档说明如何在测试代码中使用 `TestUtils` 类提供的统一输出格式函数。

## 1. 包含头文件

在测试文件中包含 `test_utils.hpp`：

```cpp
#include "../test_utils.hpp"  // 根据文件位置调整路径
```

## 2. 可用的输出格式函数

### 2.1 节标题格式

```cpp
// 标准节标题（使用破折号）
TestUtils::printSectionHeader("Test Section Name");
// 输出：
// --------------------------------------------------
//   Test Section Name
// --------------------------------------------------

// 简单节标题（使用等号）
TestUtils::printSimpleSectionHeader("Test Section Name");
// 输出：
// === Test Section Name ===
```

### 2.2 节结尾格式

```cpp
// 标准节结尾
TestUtils::printSectionFooter();
// 输出：
// --------------------------------------------------
//   [OK] Section completed

// 简单节结尾
TestUtils::printSimpleSectionFooter("Custom completion message");
// 输出：
// === Custom completion message ===
```

### 2.3 测试结果格式

```cpp
// 测试通过
TestUtils::printTestResult("Basic Functionality Test", true);
// 输出：    [PASS] Basic Functionality Test

// 测试失败
TestUtils::printTestResult("Error Handling Test", false);
// 输出：    [FAIL] Error Handling Test
```

### 2.4 信息消息格式

```cpp
// 信息消息
TestUtils::printInfo("Testing basic synchronization...");
// 输出：    [INFO] Testing basic synchronization...

// 警告消息
TestUtils::printWarning("This test may take longer than expected");
// 输出：    [WARN] This test may take longer than expected

// 错误消息
TestUtils::printError("Test failed with exception: " + std::string(e.what()));
// 输出：    [ERROR] Test failed with exception: ...
```

## 3. 完整示例

```cpp
#include "test_example.hpp"
#include "../test_utils.hpp"
#include <iostream>
#include <cassert>

namespace Lua {
    namespace Tests {
        void ExampleTest::runAllTests() {
            TestUtils::printSimpleSectionHeader("Example Test Suite");
            
            try {
                testBasicFunctionality();
                testErrorHandling();
                testEdgeCases();
                
                TestUtils::printSimpleSectionFooter("All Example tests passed!");
            } catch (const std::exception& e) {
                TestUtils::printError("Example test failed with exception: " + std::string(e.what()));
                throw;
            } catch (...) {
                TestUtils::printError("Example test failed with unknown exception");
                throw;
            }
        }
        
        void ExampleTest::printTestHeader(const std::string& testName) {
            TestUtils::printInfo("Testing " + testName + "...");
        }
        
        void ExampleTest::printTestFooter(const std::string& testName) {
            TestUtils::printTestResult(testName, true);
        }
        
        void ExampleTest::testBasicFunctionality() {
            printTestHeader("Basic Functionality");
            
            // 测试逻辑...
            bool testPassed = true;
            
            if (testPassed) {
                printTestFooter("Basic Functionality");
            } else {
                TestUtils::printTestResult("Basic Functionality", false);
                throw std::runtime_error("Basic functionality test failed");
            }
        }
    }
}
```

## 4. 输出格式规范

### 4.1 层次结构

- **测试套件级别**：使用 `printSimpleSectionHeader/Footer`
- **测试组级别**：使用 `printSectionHeader/Footer`
- **单个测试级别**：使用 `printTestResult`
- **信息级别**：使用 `printInfo/Warning/Error`

### 4.2 缩进规范

- 节标题：无缩进
- 测试结果：4个空格缩进
- 信息消息：4个空格缩进

### 4.3 标识符规范

- `[PASS]`：测试通过
- `[FAIL]`：测试失败
- `[INFO]`：信息消息
- `[WARN]`：警告消息
- `[ERROR]`：错误消息
- `[OK]`：节完成

## 5. 迁移现有测试

将现有测试迁移到统一格式：

### 5.1 替换输出语句

```cpp
// 旧格式
std::cout << "\n=== Test Suite ===\n" << std::endl;

// 新格式
TestUtils::printSimpleSectionHeader("Test Suite");
```

### 5.2 替换错误处理

```cpp
// 旧格式
std::cerr << "Test failed: " << e.what() << std::endl;

// 新格式
TestUtils::printError("Test failed: " + std::string(e.what()));
```

### 5.3 替换测试结果

```cpp
// 旧格式
std::cout << "[OK] " << testName << " passed\n" << std::endl;

// 新格式
TestUtils::printTestResult(testName, true);
```

## 6. 宏函数使用

### 6.1 RUN_TEST 宏

用于执行单个测试方法，自动处理异常和输出格式：

```cpp
// 使用方法
RUN_TEST(SynchronizeTest, testBasicSynchronization);

// 等价于以下代码：
try {
    TestUtils::printInfo("Running SynchronizeTest::testBasicSynchronization...");
    SynchronizeTest::testBasicSynchronization();
    TestUtils::printTestResult("SynchronizeTest::testBasicSynchronization", true);
} catch (const std::exception& e) {
    TestUtils::printTestResult("SynchronizeTest::testBasicSynchronization", false);
    TestUtils::printError("Exception in SynchronizeTest::testBasicSynchronization: " + std::string(e.what()));
    throw;
}
```

### 6.2 RUN_TEST_SUITE 宏

用于执行整个测试套件：

```cpp
// 使用方法
RUN_TEST_SUITE(SynchronizeTest);

// 等价于以下代码：
try {
    TestUtils::printSimpleSectionHeader("SynchronizeTest Test Suite");
    SynchronizeTest::runAllTests();
    TestUtils::printSimpleSectionFooter("SynchronizeTest tests completed successfully");
} catch (const std::exception& e) {
    TestUtils::printError("SynchronizeTest test suite failed with exception: " + std::string(e.what()));
    throw;
}
```

### 6.3 SAFE_RUN_TEST 宏

用于安全执行测试，不会重新抛出异常：

```cpp
// 使用方法
SAFE_RUN_TEST(SynchronizeTest, testBasicSynchronization);

// 这个版本会捕获异常但不重新抛出，适合批量测试
```

### 6.4 宏函数的优势

1. **统一格式**: 自动使用标准的输出格式
2. **异常处理**: 自动捕获和报告异常
3. **简化代码**: 减少重复的try-catch代码
4. **一致性**: 确保所有测试的执行方式一致

### 6.5 使用示例

```cpp
#include "../test_utils.hpp"

namespace Lua {
    namespace Tests {
        void ExampleTestRunner() {
            // 执行整个测试套件
            RUN_TEST_SUITE(SynchronizeTest);
            
            // 执行单个测试
            RUN_TEST(SynchronizeTest, testBasicSynchronization);
            RUN_TEST(SynchronizeTest, testErrorRecovery);
            
            // 安全执行（不会因异常中断）
            SAFE_RUN_TEST(SynchronizeTest, testEdgeCase);
        }
    }
}
```

通过使用这些统一的输出格式函数和宏，可以确保所有测试的输出保持一致性和可读性。