# Modern C++ Lua 解释器测试框架

本目录包含了 Modern C++ Lua 解释器项目的所有测试文件，按模块进行了分类组织。

## 目录结构

```
tests/
├── README.md                    # 本文件 - 测试目录说明
├── test_main.cpp               # 主测试入口
├── test_main.hpp               # 主测试头文件
├── lexer/                      # 词法分析器测试
│   ├── lexer_test.cpp
│   └── lexer_test.hpp
├── parser/                     # 语法分析器测试
│   ├── test_parser.cpp                 # 语法分析器测试统一入口
│   ├── test_parser.hpp                 # 语法分析器测试统一入口头文件
│   ├── parser_test.cpp
│   ├── parser_test.hpp
│   ├── forin_test.cpp          # for-in 循环测试
│   ├── forin_test.hpp
│   ├── function_test.cpp       # 函数测试
│   ├── function_test.hpp
│   ├── if_statement_test.cpp   # if 语句测试
│   ├── if_statement_test.hpp
│   ├── repeat_test.cpp         # repeat-until 循环测试
│   └── repeat_test.hpp
├── compiler/                   # 编译器测试
│   ├── test_compiler.cpp               # 编译器测试统一入口
│   ├── test_compiler.hpp               # 编译器测试统一入口头文件
│   ├── binary_expression_test.cpp      # 二元表达式编译测试
│   ├── binary_expression_test.hpp
│   ├── conditional_compilation_test.cpp # 条件编译测试
│   ├── conditional_compilation_test.hpp
│   ├── expression_compiler_test.cpp     # 表达式编译器测试
│   ├── expression_compiler_test.hpp
│   ├── literal_compiler_test.cpp        # 字面量编译测试
│   ├── literal_compiler_test.hpp
│   ├── variable_compiler_test.cpp       # 变量编译测试
│   ├── variable_compiler_test.hpp
│   ├── symbol_table_test.cpp           # 符号表测试
│   └── symbol_table_test.hpp
├── vm/                         # 虚拟机测试
│   ├── test_vm.cpp                     # 虚拟机测试统一入口
│   ├── test_vm.hpp                     # 虚拟机测试统一入口头文件
│   ├── state_test.cpp          # 状态管理测试
│   ├── state_test.hpp
│   ├── value_test.cpp          # 值系统测试
│   └── value_test.hpp
├── gc/                         # 垃圾回收器测试
│   ├── test_gc.cpp                     # GC 测试统一入口
│   ├── test_gc.hpp                     # GC 测试统一入口头文件
│   ├── gc_integration_test.cpp         # GC 集成测试
│   ├── gc_integration_test.hpp
│   ├── string_pool_demo_test.cpp       # 字符串池演示测试
│   └── string_pool_demo_test.hpp
├── lib/                        # 标准库测试
│   ├── test_lib.cpp                    # 标准库测试统一入口
│   ├── test_lib.hpp                    # 标准库测试统一入口头文件
│   ├── table_lib_test.cpp              # 表库测试
│   └── table_lib_test.hpp
└── localization/               # 本地化测试 (示例)
    ├── localization_test.cpp           # 本地化功能测试
    └── localization_test.hpp
```

## 如何添加新的测试文件

### 步骤 1: 确定测试类型和位置

根据你要测试的功能，选择合适的目录：

- **lexer/**: 词法分析相关功能
- **parser/**: 语法分析相关功能  
- **compiler/**: 编译器相关功能
- **vm/**: 虚拟机运行时功能
- **gc/**: 垃圾回收器功能
- **lib/**: 标准库功能
- **新模块/**: 如果是全新的模块，创建新目录

### 步骤 2: 创建测试文件

#### 2.1 创建头文件 (.hpp)

```cpp
#ifndef YOUR_TEST_NAME_HPP
#define YOUR_TEST_NAME_HPP

#include <iostream>
#include "../../path/to/your/module.hpp"  // 包含被测试的模块

namespace Lua {
namespace Tests {

/**
 * @brief 你的测试类描述
 * 
 * 详细说明这个测试类测试什么功能
 */
class YourTestName {
public:
    /**
     * @brief 运行所有测试
     * 
     * 执行这个测试类中的所有测试用例
     */
    static void runAllTests();
    
private:
    // 私有测试方法
    static void testSpecificFeature1();
    static void testSpecificFeature2();
    static void testErrorHandling();
    
    // 辅助方法
    static void printTestResult(const std::string& testName, bool passed);
};

} // namespace Tests
} // namespace Lua

#endif // YOUR_TEST_NAME_HPP
```

#### 2.2 创建实现文件 (.cpp)

```cpp
#include "your_test_name.hpp"

namespace Lua {
namespace Tests {

void YourTestName::runAllTests() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Running Your Test Name Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 运行所有测试方法
    testSpecificFeature1();
    testSpecificFeature2();
    testErrorHandling();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Your Test Name Tests Completed" << std::endl;
    std::cout << "========================================" << std::endl;
}

void YourTestName::testSpecificFeature1() {
    std::cout << "\nTesting Specific Feature 1:" << std::endl;
    
    try {
        // 测试代码
        // 例如：创建对象，调用方法，验证结果
        
        // 示例测试逻辑
        bool testPassed = true; // 根据实际测试结果设置
        
        printTestResult("Specific Feature 1", testPassed);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void YourTestName::testSpecificFeature2() {
    std::cout << "\nTesting Specific Feature 2:" << std::endl;
    
    // 类似的测试实现
}

void YourTestName::testErrorHandling() {
    std::cout << "\nTesting Error Handling:" << std::endl;
    
    // 测试错误情况的处理
}

void YourTestName::printTestResult(const std::string& testName, bool passed) {
    if (passed) {
        std::cout << "[PASS] " << testName << " test passed" << std::endl;
    } else {
        std::cout << "[FAIL] " << testName << " test failed" << std::endl;
    }
}

} // namespace Tests
} // namespace Lua
```

### 步骤 3: 集成到测试套件

#### 3.1 如果是现有模块的新测试

将你的测试添加到对应模块的统一入口文件中：

**在模块的 test_xxx.hpp 中添加包含：**
```cpp
#include "your_test_name.hpp"
```

**在模块的 test_xxx.cpp 中添加调用：**
```cpp
void ModuleTest::runAllTests() {
    // ... 现有测试 ...
    
    // 添加你的测试
    printSectionHeader("Your Test Name Tests");
    YourTestName::runAllTests();
    printSectionFooter();
    
    // ... 其他测试 ...
}
```

#### 3.2 如果是全新模块的测试

**创建模块统一入口文件：**

`your_module/test_your_module.hpp`:
```cpp
#ifndef TEST_YOUR_MODULE_HPP
#define TEST_YOUR_MODULE_HPP

#include "your_test_name.hpp"
// 包含其他测试文件

namespace Lua {
namespace Tests {

class YourModuleTest {
public:
    static void runAllTests();
    
private:
    static void printSectionHeader(const std::string& title);
    static void printSectionFooter();
};

} // namespace Tests
} // namespace Lua

#endif // TEST_YOUR_MODULE_HPP
```

`your_module/test_your_module.cpp`:
```cpp
#include "test_your_module.hpp"
#include <iostream>
#include <string>

namespace Lua {
namespace Tests {

void YourModuleTest::runAllTests() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "          YOUR MODULE TEST SUITE" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Running all your-module-related tests..." << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    try {
        printSectionHeader("Your Test Name Tests");
        YourTestName::runAllTests();
        printSectionFooter();
        
        // 添加其他测试...
        
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "    [OK] ALL YOUR MODULE TESTS COMPLETED SUCCESSFULLY" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "\n[ERROR] Your Module test suite failed: " << e.what() << std::endl;
    }
}

void YourModuleTest::printSectionHeader(const std::string& title) {
    std::cout << "\n" << std::string(40, '-') << std::endl;
    std::cout << title << std::endl;
    std::cout << std::string(40, '-') << std::endl;
}

void YourModuleTest::printSectionFooter() {
    std::cout << std::string(40, '-') << std::endl;
}

} // namespace Tests
} // namespace Lua
```

### 步骤 4: 更新主测试入口

**在 test_main.hpp 中添加包含：**
```cpp
#include "your_module/test_your_module.hpp"
```

**在 test_main.cpp 中添加调用：**
```cpp
void runAllTests() {
    std::cout << "=== Running Lua Interpreter Tests ===" << std::endl;
    try {
        // ... 现有测试 ...
        
        // 添加你的模块测试
        YourModuleTest::runAllTests();
        
        std::cout << "\n=== All Tests Completed ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "\nTest execution failed with exception: " << e.what() << std::endl;
    }
}
```

## 测试编写最佳实践

### 1. 测试命名规范

- **测试类命名**: `<ModuleName>Test`
  - 例如: `LocalizationTest`, `ErrorHandlerTest`

- **测试方法命名**: `test<SpecificFeature>()`
  - 例如: `testLanguageSwitching()`, `testErrorMessageFormatting()`

- **主入口方法**: `runAllTests()`
  - 每个测试类都应该有这个静态方法

### 2. 代码注释规范

**重要**: 所有测试代码中的注释必须使用英文编写，包括但不限于：
- 类和方法的文档注释
- 行内注释
- 测试说明注释
- 代码块解释注释

这确保了代码的国际化和团队协作的一致性。

### 3. 测试结构模式

```cpp
// 1. Setup test environment
// 2. Execute the operation being tested
// 3. Verify results
// 4. Cleanup resources (if needed)
```

### 4. 错误处理

- 使用 try-catch 块捕获异常
- 提供清晰的错误信息
- 测试正常情况和异常情况

### 5. 输出格式

- 使用统一的输出格式
- 明确标识测试通过/失败
- 提供有意义的测试描述

### 6. 测试独立性

- 每个测试应该独立运行
- 不依赖其他测试的执行顺序
- 清理测试产生的副作用

## 示例：添加本地化测试

假设我们要为本地化功能添加测试，以下是完整的实现示例：

### localization/localization_test.hpp
```cpp
#ifndef LOCALIZATION_TEST_HPP
#define LOCALIZATION_TEST_HPP

#include <iostream>
#include "../../localization/localization_manager.hpp"

namespace Lua {
namespace Tests {

class LocalizationTest {
public:
    static void runAllTests();
    
private:
    static void testBasicLocalization();
    static void testLanguageSwitching();
    static void testMissingTranslation();
    static void printTestResult(const std::string& testName, bool passed);
};

} // namespace Tests
} // namespace Lua

#endif // LOCALIZATION_TEST_HPP
```

### localization/localization_test.cpp
```cpp
#include "localization_test.hpp"

namespace Lua {
namespace Tests {

void LocalizationTest::runAllTests() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Running Localization Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    testBasicLocalization();
    testLanguageSwitching();
    testMissingTranslation();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Localization Tests Completed" << std::endl;
    std::cout << "========================================" << std::endl;
}

void LocalizationTest::testBasicLocalization() {
    std::cout << "\nTesting Basic Localization:" << std::endl;
    
    try {
        LocalizationManager manager;
        
        // 测试英文消息
        std::string englishMsg = manager.getMessage("error.syntax", "en");
        bool englishTest = !englishMsg.empty();
        printTestResult("English Message Retrieval", englishTest);
        
        // 测试中文消息
        std::string chineseMsg = manager.getMessage("error.syntax", "zh");
        bool chineseTest = !chineseMsg.empty();
        printTestResult("Chinese Message Retrieval", chineseTest);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] Basic localization test failed: " << e.what() << std::endl;
    }
}

void LocalizationTest::testLanguageSwitching() {
    std::cout << "\nTesting Language Switching:" << std::endl;
    
    try {
        LocalizationManager manager;
        
        // 设置为英文
        manager.setLanguage("en");
        std::string msg1 = manager.getCurrentMessage("error.syntax");
        
        // 切换到中文
        manager.setLanguage("zh");
        std::string msg2 = manager.getCurrentMessage("error.syntax");
        
        // 验证消息不同
        bool switchTest = (msg1 != msg2);
        printTestResult("Language Switching", switchTest);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] Language switching test failed: " << e.what() << std::endl;
    }
}

void LocalizationTest::testMissingTranslation() {
    std::cout << "\nTesting Missing Translation Handling:" << std::endl;
    
    try {
        LocalizationManager manager;
        
        // 测试不存在的消息键
        std::string missingMsg = manager.getMessage("nonexistent.key", "en");
        
        // 应该返回默认消息或键名
        bool missingTest = !missingMsg.empty();
        printTestResult("Missing Translation Handling", missingTest);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] Missing translation test failed: " << e.what() << std::endl;
    }
}

void LocalizationTest::printTestResult(const std::string& testName, bool passed) {
    if (passed) {
        std::cout << "[PASS] " << testName << " test passed" << std::endl;
    } else {
        std::cout << "[FAIL] " << testName << " test failed" << std::endl;
    }
}

} // namespace Tests
} // namespace Lua
```

然后在主测试入口中添加对本地化测试的调用。

## 测试执行层次

```
runAllTests()
├── LexerTest::runAllTests()
├── VMTestSuite::runAllTests()
│   ├── ValueTest::runAllTests()
│   └── StateTest::runAllTests()
├── ParserTestSuite::runAllTests()
│   ├── ParserTest::runAllTests()
│   ├── FunctionTest::runAllTests()
│   ├── IfStatementTest::runAllTests()
│   ├── ForInTest::runAllTests()
│   └── RepeatTest::runAllTests()
├── CompilerTest::runAllTests()
│   ├── SymbolTableTest::runAllTests()
│   ├── LiteralCompilerTest::runAllTests()
│   ├── VariableCompilerTest::runAllTests()
│   ├── BinaryExpressionTest::runAllTests()
│   ├── ExpressionCompilerTest::runAllTests()
│   └── ConditionalCompilationTest::runAllTests()
├── GCTest::runAllTests()
│   ├── StringPoolDemoTest::runAllTests()
│   └── GCIntegrationTest::runAllTests()
├── LibTestSuite::runAllTests()
│   └── TableLibTest::runAllTests()
└── YourModuleTest::runAllTests()  # 新添加的模块
    ├── YourTest1::runAllTests()
    └── YourTest2::runAllTests()
```

这种结构提供了极大的灵活性：
- ✅ **完整测试**: 一次运行所有测试
- ✅ **模块测试**: 针对特定模块的测试
- ✅ **功能测试**: 针对具体功能的精确测试
- ✅ **易于维护**: 清晰的组织结构和统一的接口
- ✅ **易于扩展**: 新测试可以轻松添加到对应模块

## 构建和运行说明

**注意**: 当前项目有两个测试目录：

1. **`tests/`** - 基于 Google Test 的原始测试文件，在 CMakeLists.txt 中已配置
2. **`src/tests/`** - 新组织的模块化测试文件（本文档描述的结构）

要使用新的测试组织结构，你可以：

### 方法 1: 直接在代码中调用
```cpp
#include "src/tests/test_main.hpp"

int main() {
    Lua::Tests::runAllTests();
    return 0;
}
```

### 方法 2: 编译为独立可执行文件
```bash
# 在项目根目录执行
g++ -std=c++17 -I src src/tests/test_main.cpp src/tests/*/*.cpp -o test_runner
./test_runner
```

### 方法 3: 集成到现有 CMakeLists.txt 
将以下内容添加到 CMakeLists.txt：
```cmake
# 新的模块化测试
add_executable(modular_tests
  src/tests/test_main.cpp
  src/tests/compiler/test_compiler.cpp
  src/tests/gc/test_gc.cpp
  # 添加其他测试源文件...
)
target_link_libraries(modular_tests PRIVATE lua_lib)
```

## 总结

通过遵循这个指南，你可以：

1. **快速添加新测试**: 使用标准化的模板和结构
2. **保持代码一致性**: 遵循统一的命名和组织规范
3. **便于维护**: 清晰的模块化结构
4. **灵活运行**: 支持全量测试、模块测试和单个测试
5. **易于调试**: 清晰的输出格式和错误处理

记住：好的测试不仅验证功能正确性，还能作为代码的活文档，帮助其他开发者理解系统的行为和预期。
