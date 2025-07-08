# Lua 项目测试框架规范文档

**版本**: v2.0  
**最后更新**: 2025年7月7日  
**状态**: 正式版本

## 📋 概述

本文档定义了 Lua 项目测试框架的完整规范，包括文件组织、命名规范、代码结构、测试层次和最佳实践。

### 🔑 核心原则：层次化调用传播

**测试框架的核心设计原则是严格的层次化调用传播机制：**

```
MAIN (test_main.hpp)
  ↓ 只能调用 MODULE 级别
MODULE (test_module.hpp)
  ↓ 只能调用 SUITE 级别
SUITE (module_test_suite.hpp)
  ↓ 只能调用 GROUP 级别
GROUP (功能分组函数)
  ↓ 只能调用 INDIVIDUAL 级别
INDIVIDUAL (具体测试方法)
```

**每一层只能调用其直接下级，严禁跨级调用。这确保了：**
- 🎯 **清晰的测试结构** - 测试组织层次分明
- 🔍 **易于调试定位** - 问题可以快速定位到具体层次
- 🛡️ **错误隔离控制** - 错误不会跨层传播
- 📊 **统计报告准确** - 每层的测试统计独立准确
- 🔧 **维护成本降低** - 修改测试时影响范围可控

## 🏗️ 测试框架架构

### 核心组件结构
```
src/test_framework/
├── core/                        # 测试框架核心
│   ├── test_macros.hpp          # 测试宏定义
│   ├── test_utils.hpp           # 测试工具类
│   ├── test_memory.hpp/.cpp     # 内存检测工具
├── formatting/                  # 结果格式化
│   ├── format_formatter.hpp/.cpp # 格式化器
│   ├── format_config.hpp/.cpp   # 配置管理
│   ├── format_colors.hpp/.cpp   # 颜色输出
│   ├── format_strategies.hpp/.cpp # 格式化策略
│   └── format_define.hpp        # 格式化定义
├── examples/                    # 测试示例
│   ├── example_test.hpp         # 示例测试
│   └── lexer_test.cpp          # 具体示例
└── tools/                       # 测试工具
    ├── check_naming.py          # 命名检查工具
    ├── check_naming_simple.py   # 简化命名检查
    └── check_naming.bat         # Windows批处理
```

### 测试目录结构
```
src/tests/
├── test_main.hpp                # 主测试入口
├── comprehensive_test_suite.hpp # 综合测试套件
├── lexer/                       # 词法分析器测试
│   └── test_lexer.hpp
├── parser/                      # 语法分析器测试
│   └── test_parser.hpp
├── vm/                          # 虚拟机测试
│   └── test_vm.hpp
├── compiler/                    # 编译器测试
│   └── test_compiler.hpp
├── gc/                          # 垃圾回收测试
│   └── test_gc.hpp
├── lib/                         # 标准库测试
│   ├── test_lib.hpp             # 库测试主入口
│   ├── base_lib_test.hpp        # 基础库测试
│   ├── string_lib_test.hpp      # 字符串库测试
│   ├── math_lib_test.hpp        # 数学库测试
│   ├── table_lib_test.hpp       # 表库测试
│   ├── io_lib_test.hpp          # IO库测试
│   ├── os_lib_test.hpp          # OS库测试
│   ├── debug_lib_test.hpp       # 调试库测试
│   └── standard_library_test_suite.hpp # 完整标准库测试套件
└── localization/                # 本地化测试
    └── localization_test.hpp
```

## 📝 文件命名规范

### 1. 主测试文件命名
- **格式**: `test_{模块名}_{子模块名}.hpp`
- **示例**: 
  - `test_lexer_tokenizer.hpp`
  - `test_parser_expression.hpp`
  - `test_vm_execution.hpp`

### 2. 子模块测试文件命名
- **格式**: `{模块名}_{功能名}_test.hpp` 或 `{模块名}_{功能名}_test.cpp`
- **示例**:
  - `base_lib_test.hpp`
  - `string_operations_test.cpp`
  - `memory_management_test.hpp`

### 3. 测试套件文件命名
- **格式**: `{功能域}_test_suite.hpp`
- **示例**:
  - `standard_library_test_suite.hpp`
  - `compiler_test_suite.hpp`

### 4. 命名规则
- 只使用小写字母、数字和下划线
- 避免使用缩写，使用完整的描述性名称
- 测试文件必须以 `_test` 结尾或以 `test_` 开头

## 🔧 测试层次结构

### 测试层次定义
```cpp
enum class TestLevel {
    MAIN,        // 主测试级别 - 整个测试套件
    MODULE,      // 模块级别 - 特定功能模块
    SUITE,       // 套件级别 - 相关测试的集合
    GROUP,       // 组级别 - 测试组
    INDIVIDUAL   // 个体级别 - 单个测试用例
};
```

### 层次使用规范
1. **MAIN**: 用于 `main()` 函数或顶级测试入口
2. **MODULE**: 用于模块测试（如 Parser、Lexer、VM）
3. **SUITE**: 用于测试套件（如 BaseLibTestSuite）
4. **GROUP**: 用于测试组（如功能相关的测试集合）
5. **INDIVIDUAL**: 用于单个测试方法

### 🔄 层次调用传播机制 ⭐ **核心规范**

**测试框架采用严格的层次化调用结构，每一层只能调用其直接下级，形成清晰的调用链：**

```
MAIN (test_main.hpp)
  ↓ 调用所有 MODULE 测试
MODULE (test_module.hpp)
  ↓ 调用所有 SUITE 测试
SUITE (module_test_suite.hpp)
  ↓ 调用所有 GROUP 测试
GROUP (功能分组)
  ↓ 调用所有 INDIVIDUAL 测试
INDIVIDUAL (具体测试方法)
```

#### 调用传播规则

1. **MAIN → MODULE**: 主测试入口调用所有模块测试
   ```cpp
   // test_main.hpp
   static void runAllTests() {
       RUN_TEST_MODULE("Lexer Module", LexerTestSuite);
       RUN_TEST_MODULE("Parser Module", ParserTestSuite);
       RUN_TEST_MODULE("VM Module", VMTestSuite);
       RUN_TEST_MODULE("Library Module", LibTestSuite);
   }
   ```

2. **MODULE → SUITE**: 模块测试调用所有子功能测试套件
   ```cpp
   // test_lib.hpp (模块级)
   static void runAllTests() {
       RUN_TEST_SUITE("Base Library", BaseLibTestSuite);
       RUN_TEST_SUITE("String Library", StringLibTestSuite);
       RUN_TEST_SUITE("Math Library", MathLibTestSuite);
   }
   ```

3. **SUITE → GROUP**: 测试套件调用所有功能分组测试
   ```cpp
   // base_lib_test.hpp (套件级)
   static void runAllTests() {
       RUN_TEST_GROUP("Basic Functions", runBasicTests);
       RUN_TEST_GROUP("Type Operations", runTypeTests);
       RUN_TEST_GROUP("Error Handling", runErrorTests);
   }
   ```

4. **GROUP → INDIVIDUAL**: 测试组调用所有具体测试方法
   ```cpp
   // 测试组函数 (组级)
   static void runBasicTests() {
       RUN_TEST(BaseLibTest, testPrint);
       RUN_TEST(BaseLibTest, testType);
       RUN_TEST(BaseLibTest, testToString);
   }
   ```

#### 禁止跨级调用 ❌

**严格禁止跨级调用，例如：**
- ❌ MAIN 直接调用 SUITE 或 GROUP
- ❌ MODULE 直接调用 GROUP 或 INDIVIDUAL
- ❌ SUITE 直接调用 INDIVIDUAL

**正确的调用必须逐级传播，确保测试层次清晰可控。**

## 🎯 测试宏规范

### 核心测试宏
```cpp
// 运行单个测试方法 (INDIVIDUAL 级别)
RUN_TEST(TestClass, TestMethod)

// 运行主测试 (MAIN 级别)
RUN_MAIN_TEST(TestName, TestFunction)

// 运行模块测试 (MODULE 级别)
RUN_TEST_MODULE(ModuleName, ModuleTestClass)

// 运行测试套件 (SUITE 级别)
RUN_TEST_SUITE(SuiteName, SuiteClass)

// 运行测试组 (GROUP 级别)
RUN_TEST_GROUP(GroupName, GroupFunction)

// 带内存检查的测试组
RUN_TEST_GROUP_WITH_MEMORY_CHECK(GroupName, GroupFunction)
```

### 🔗 宏使用的层次调用规范 ⭐ **强制要求**

**每个测试宏只能在其对应的层次中使用，并且只能调用直接下级的宏：**

#### 1. MAIN 级别 (test_main.hpp)
```cpp
class MainTestSuite {
public:
    static void runAllTests() {
        // ✅ 只能使用 RUN_TEST_MODULE 调用模块测试
        RUN_TEST_MODULE("Lexer Module", LexerTestSuite);
        RUN_TEST_MODULE("Parser Module", ParserTestSuite);
        RUN_TEST_MODULE("VM Module", VMTestSuite);
        RUN_TEST_MODULE("Library Module", LibTestSuite);

        // ❌ 禁止直接调用其他级别
        // RUN_TEST_SUITE("Base Library", BaseLibTestSuite);  // 错误！
        // RUN_TEST_GROUP("Basic Tests", runBasicTests);      // 错误！
    }
};
```

#### 2. MODULE 级别 (test_module.hpp)
```cpp
class LibTestSuite {
public:
    static void runAllTests() {
        // ✅ 只能使用 RUN_TEST_SUITE 调用套件测试
        RUN_TEST_SUITE("Base Library", BaseLibTestSuite);
        RUN_TEST_SUITE("String Library", StringLibTestSuite);
        RUN_TEST_SUITE("Math Library", MathLibTestSuite);

        // ❌ 禁止跨级调用
        // RUN_TEST_GROUP("Basic Tests", runBasicTests);      // 错误！
        // RUN_TEST(BaseLibTest, testPrint);                  // 错误！
    }
};
```

#### 3. SUITE 级别 (module_test_suite.hpp)
```cpp
class BaseLibTestSuite {
public:
    static void runAllTests() {
        // ✅ 只能使用 RUN_TEST_GROUP 调用组测试
        RUN_TEST_GROUP("Basic Functions", runBasicTests);
        RUN_TEST_GROUP("Type Operations", runTypeTests);
        RUN_TEST_GROUP("Error Handling", runErrorTests);

        // ❌ 禁止跨级调用
        // RUN_TEST(BaseLibTest, testPrint);                  // 错误！
    }
};
```

#### 4. GROUP 级别 (功能分组函数)
```cpp
static void runBasicTests() {
    // ✅ 只能使用 RUN_TEST 调用具体测试
    RUN_TEST(BaseLibTest, testPrint);
    RUN_TEST(BaseLibTest, testType);
    RUN_TEST(BaseLibTest, testToString);

    // ❌ 不能调用其他级别的宏
    // RUN_TEST_GROUP("Sub Group", subFunction);           // 错误！
}
```

#### 5. INDIVIDUAL 级别 (具体测试方法)
```cpp
class BaseLibTest {
public:
    static void testPrint() {
        // ✅ 具体的测试实现，不调用其他测试宏
        // 测试逻辑...
    }
};
```

### 断言宏
```cpp
// 基础断言
TEST_ASSERT(condition, message)
TEST_ASSERT_EQ(expected, actual, message)
TEST_ASSERT_TRUE(condition, message)
TEST_ASSERT_FALSE(condition, message)

// 内存检测宏
MEMORY_LEAK_TEST_GUARD(testName)
```

## 📋 测试类结构规范

### 1. 测试套件类结构
```cpp
namespace Lua::Tests {

class ModuleTestSuite {
public:
    /**
     * @brief 运行所有测试
     */
    static void runAllTests() {
        // 使用适当的测试宏运行测试组
        RUN_TEST_GROUP("Basic Tests", runBasicTests);
        RUN_TEST_GROUP("Advanced Tests", runAdvancedTests);
    }

private:
    /**
     * @brief 基础测试组
     */
    static void runBasicTests() {
        RUN_TEST(TestClass, testMethod1);
        RUN_TEST(TestClass, testMethod2);
    }
    
    /**
     * @brief 高级测试组
     */
    static void runAdvancedTests() {
        RUN_TEST(TestClass, testAdvancedMethod1);
        RUN_TEST(TestClass, testAdvancedMethod2);
    }
};

} // namespace Lua::Tests
```

### 2. 测试类结构
```cpp
class TestClass {
public:
    /**
     * @brief 测试方法1
     */
    static void testMethod1() {
        TestUtils::printInfo("Testing method1...");
        
        // 测试逻辑
        if (condition) {
            throw std::runtime_error("Test failed: reason");
        }
        
        TestUtils::printInfo("Method1 test passed");
    }
    
    /**
     * @brief 测试方法2
     */
    static void testMethod2() {
        // 测试逻辑
    }
};
```

## 🎨 格式化和输出规范

### 1. 输出工具使用
```cpp
// 基础输出
TestUtils::printInfo("信息消息");
TestUtils::printWarning("警告消息");
TestUtils::printError("错误消息");

// 测试结果
TestUtils::printTestResult("测试名称", true/false);

// 层次化输出
TestUtils::printLevelHeader(TestLevel::MODULE, "模块名", "描述");
TestUtils::printLevelFooter(TestLevel::MODULE, "总结");

// 异常处理
TestUtils::printException(e, "上下文");
TestUtils::printUnknownException("上下文");
```

### 2. 颜色和主题配置
```cpp
// 启用颜色输出
TestUtils::setColorEnabled(true);

// 设置主题
TestUtils::setTheme("default");

// 获取配置
auto& config = TestUtils::getConfig();
```

## 🧠 内存检测规范

### 1. 内存检测使用
```cpp
// 启用内存检测
TestUtils::setMemoryCheckEnabled(true);

// 手动内存检测
TestUtils::startMemoryCheck("测试名称");
// ... 测试代码 ...
bool hasLeak = TestUtils::endMemoryCheck("测试名称");

// 自动内存检测（推荐）
MEMORY_LEAK_TEST_GUARD("测试名称");
```

### 2. 内存安全测试组
```cpp
RUN_TEST_GROUP_WITH_MEMORY_CHECK("Memory Safe Tests", runMemorySafeTests);
```

## 📊 测试统计和报告

### 统计结构
```cpp
struct TestStatistics {
    int totalTests = 0;
    int passedTests = 0;
    int failedTests = 0;
    int skippedTests = 0;
    
    double getPassRate() const;
    bool allPassed() const;
};
```

## 🔍 命名检查工具

### 使用方法
```bash
# 检查所有测试文件
python src/test_framework/tools/check_naming.py src/tests

# 简化检查
python src/test_framework/tools/check_naming_simple.py src/tests

# Windows批处理
src/test_framework/tools/check_naming.bat
```

## ✅ 最佳实践

### 1. 层次调用规范 ⭐ **最重要**
- **严格遵循层次调用**: 每层只能调用直接下级，禁止跨级调用
- **调用链完整性**: MAIN → MODULE → SUITE → GROUP → INDIVIDUAL
- **文件组织对应**: 文件结构应反映测试层次结构
- **命名一致性**: 文件名和类名应明确表示其在层次中的位置

#### 层次调用检查清单
```
✅ test_main.hpp 只调用 RUN_TEST_MODULE
✅ test_module.hpp 只调用 RUN_TEST_SUITE
✅ module_test_suite.hpp 只调用 RUN_TEST_GROUP
✅ 测试组函数只调用 RUN_TEST
✅ 测试方法不调用其他测试宏
```

### 2. 测试组织
- 按功能模块组织测试
- 使用清晰的层次结构
- 每个测试方法只测试一个功能点
- 确保测试文件的层次结构与调用关系一致

### 3. 错误处理
- 使用异常来报告测试失败
- 提供清晰的错误消息
- 包含足够的上下文信息
- 在每个层次提供适当的错误上下文

### 4. 内存管理
- 对所有涉及动态内存的测试启用内存检测
- 确保测试后正确清理资源
- 使用 RAII 模式管理资源

### 5. 代码风格
- 遵循项目的代码风格规范
- 使用描述性的测试名称
- 添加适当的注释和文档
- 明确标注测试的层次级别

### 6. 测试独立性
- 每个测试应该独立运行
- 不依赖其他测试的执行顺序
- 避免全局状态的影响
- 每个层次的测试应该能够独立执行

## 🚀 集成指南

### 1. 添加新测试模块 (MODULE 级别)
1. 在 `src/tests/` 下创建模块目录
2. 创建主测试头文件 `test_{模块名}.hpp`
3. 在模块测试类中只调用 `RUN_TEST_SUITE`
4. 在 `test_main.hpp` 中使用 `RUN_TEST_MODULE` 集成

```cpp
// 示例：test_new_module.hpp
class NewModuleTestSuite {
public:
    static void runAllTests() {
        // ✅ 只调用 SUITE 级别
        RUN_TEST_SUITE("Feature A", FeatureATestSuite);
        RUN_TEST_SUITE("Feature B", FeatureBTestSuite);
    }
};

// 在 test_main.hpp 中集成
RUN_TEST_MODULE("New Module", NewModuleTestSuite);
```

### 2. 添加新测试套件 (SUITE 级别)
1. 创建测试套件类 `{功能名}TestSuite`
2. 实现 `runAllTests()` 静态方法，只调用 `RUN_TEST_GROUP`
3. 在对应的模块测试中使用 `RUN_TEST_SUITE` 集成

```cpp
// 示例：feature_test_suite.hpp
class FeatureTestSuite {
public:
    static void runAllTests() {
        // ✅ 只调用 GROUP 级别
        RUN_TEST_GROUP("Basic Operations", runBasicTests);
        RUN_TEST_GROUP("Advanced Operations", runAdvancedTests);
    }
private:
    static void runBasicTests() {
        // ✅ 只调用 INDIVIDUAL 级别
        RUN_TEST(FeatureTest, testBasicFunction1);
        RUN_TEST(FeatureTest, testBasicFunction2);
    }
};
```

### 3. 添加新测试组 (GROUP 级别)
1. 在测试套件中创建静态测试组函数
2. 函数中只使用 `RUN_TEST` 调用具体测试方法
3. 在套件的 `runAllTests()` 中使用 `RUN_TEST_GROUP` 调用

### 4. 层次集成检查清单
在添加任何新测试时，必须检查：

```
✅ 文件层次正确：test_main.hpp → test_module.hpp → suite.hpp
✅ 调用层次正确：MAIN → MODULE → SUITE → GROUP → INDIVIDUAL
✅ 宏使用正确：每层只使用对应级别的宏
✅ 命名规范正确：文件名反映其在层次中的位置
✅ 包含关系正确：上级文件包含下级文件
```

### 5. 运行测试
```cpp
// 在 main() 函数中 (MAIN 级别)
RUN_MAIN_TEST("Complete Test Suite", MainTestSuite::runAllTests);
```

### 6. 常见错误和修正 ❌→✅

#### 错误示例：跨级调用
```cpp
// ❌ 错误：在 MODULE 级别直接调用 GROUP
class ModuleTestSuite {
public:
    static void runAllTests() {
        RUN_TEST_GROUP("Basic Tests", runBasicTests);  // 错误！
    }
};

// ✅ 正确：通过 SUITE 级别调用
class ModuleTestSuite {
public:
    static void runAllTests() {
        RUN_TEST_SUITE("Feature Suite", FeatureTestSuite);  // 正确！
    }
};
```

#### 错误示例：层次混乱
```cpp
// ❌ 错误：在 SUITE 级别直接调用 INDIVIDUAL
class SuiteTestSuite {
public:
    static void runAllTests() {
        RUN_TEST(TestClass, testMethod);  // 错误！
    }
};

// ✅ 正确：通过 GROUP 级别调用
class SuiteTestSuite {
public:
    static void runAllTests() {
        RUN_TEST_GROUP("Test Group", runTestGroup);  // 正确！
    }
private:
    static void runTestGroup() {
        RUN_TEST(TestClass, testMethod);  // 正确！
    }
};
```

---

## 📋 重要提醒：层次调用传播机制

### 🎯 核心要求总结

**测试框架最重要的规范是层次调用传播机制，必须严格遵循：**

1. **test_main.hpp** (MAIN级别)
   - ✅ 只能调用 `RUN_TEST_MODULE`
   - ✅ 调用所有模块测试
   - ❌ 禁止直接调用 SUITE、GROUP、INDIVIDUAL

2. **test_module.hpp** (MODULE级别)
   - ✅ 只能调用 `RUN_TEST_SUITE`
   - ✅ 调用所有子功能测试套件
   - ❌ 禁止直接调用 GROUP、INDIVIDUAL

3. **module_test_suite.hpp** (SUITE级别)
   - ✅ 只能调用 `RUN_TEST_GROUP`
   - ✅ 调用所有分组测试
   - ❌ 禁止直接调用 INDIVIDUAL

4. **测试分组函数** (GROUP级别)
   - ✅ 只能调用 `RUN_TEST`
   - ✅ 调用所有具体测试方法
   - ❌ 禁止调用其他级别

5. **具体测试方法** (INDIVIDUAL级别)
   - ✅ 实现具体测试逻辑
   - ❌ 不调用任何测试宏

### 🔍 违规检查方法

在代码审查时，检查以下内容：
- [ ] 每个文件只使用其对应级别的测试宏
- [ ] 调用关系形成完整的层次链
- [ ] 没有跨级调用的情况
- [ ] 文件命名反映其在层次中的位置

### ⚠️ 常见违规示例

```cpp
// ❌ 错误：MODULE 级别跨级调用 GROUP
class ModuleTestSuite {
    static void runAllTests() {
        RUN_TEST_GROUP("Basic Tests", runBasicTests);  // 违规！
    }
};

// ❌ 错误：SUITE 级别跨级调用 INDIVIDUAL
class SuiteTestSuite {
    static void runAllTests() {
        RUN_TEST(TestClass, testMethod);  // 违规！
    }
};
```

**遵循层次调用传播机制是测试框架正确运行的基础，违反此规范将导致测试结构混乱、错误定位困难、统计不准确等问题。**

---

**注意**: 本规范是基于现有测试框架代码分析总结而成，应与实际代码保持同步更新。层次调用传播机制是测试框架的核心设计原则，必须严格遵循。
