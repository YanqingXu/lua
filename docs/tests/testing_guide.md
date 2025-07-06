# Modern C++ Lua 解释器测试框架

> **版本**: v2.1 | **最后更新**: 2025年6月 | **维护者**: YanqingXu

## 📋 快速导航

| 章节 | 内容 | 适用场景 |
|------|------|----------|
| [🚀 快速开始](#快速开始) | 5分钟上手指南 | 新手入门 |
| [📋 规范速查](#规范速查表) | 核心规范一览 | 日常开发 |
| [❓ 常见问题](#常见问题faq) | 故障排除指南 | 问题解决 |
| [🏗️ 测试层次结构](#测试层次结构) | 框架架构说明 | 架构理解 |
| [📁 模块测试规范](#模块测试文件规范) | 文件组织规范 | 规范开发 |
| [🎯 命名约定](#命名约定) | 命名规则详解 | 标准化开发 |
| [🔧 编译规范](#编译规范) | 编译验证流程 | 质量保证 |
| [🚀 最佳实践](#测试编写最佳实践) | 高级开发技巧 | 进阶开发 |

---

# Lua 编译器测试目录

本目录包含了 Modern C++ Lua 解释器项目的所有测试文件，按模块进行了分类组织。

## 🚀 快速开始

### 5分钟上手指南

1. **创建新测试文件**
   ```bash
   # 模块测试文件
   touch src/tests/lexer/test_lexer.hpp
   
   # 子模块测试文件
   touch src/tests/parser/expr/parser_binary_expr_test.cpp
   ```

2. **基本文件结构**
   ```cpp
   // test_lexer.hpp
   #pragma once
   #include "../test_base.hpp"
   
   namespace lua::tests::lexer {
       class LexerTest : public TestBase {
       public:
           void run_all_tests() override;
       private:
           void test_tokenize();
       };
   }
   ```

3. **编译验证**
   ```bash
   g++ -std=c++17 -I. test_lexer.cpp -o test_lexer && ./test_lexer
   ```

4. **运行测试**
   ```bash
   cd src/tests && ./run_tests.sh
   ```

## 📋 规范速查表

### 核心命名规范

| 类型 | 格式 | 示例 | 说明 |
|------|------|------|------|
| **模块测试文件** | `test_{module}.hpp` | `test_lexer.hpp` | 主测试文件 |
| **子模块测试文件** | `{module}_{feature}_test.cpp` | `parser_binary_expr_test.cpp` | 具体功能测试 |
| **主测试文件** | `test_{主模块}_{子模块}.hpp` | `test_parser_expr.hpp` | 子模块主测试 |
| **测试类** | `{Module}Test` | `LexerTest` | 测试类命名 |
| **命名空间** | `lua::tests::{module}` | `lua::tests::lexer` | 命名空间层级 |

### 快速检查清单

- [ ] 文件名符合命名规范
- [ ] 包含正确的命名空间
- [ ] 继承自 `TestBase`
- [ ] 实现 `run_all_tests()` 方法
- [ ] 使用 `TEST_ASSERT` 宏
- [ ] 编译通过验证
- [ ] 测试独立运行

## ❓ 常见问题(FAQ)

### Q1: 编译错误 "undefined reference to TestBase"
**解决方案**: 确保包含了正确的基类头文件
```cpp
#include "../test_base.hpp"  // 相对路径要正确
```

### Q2: 命名空间冲突
**解决方案**: 检查命名空间层级是否正确
```cpp
namespace lua::tests::parser {  // 正确
namespace lua::parser::tests {  // 错误
```

### Q3: 测试文件找不到
**解决方案**: 检查文件路径和包含语句
```cpp
#include "parser_binary_expr_test.hpp"  // 同目录
#include "../lexer/test_lexer.hpp"      // 上级目录
```

### Q4: 如何添加新的测试模块？
**步骤**:
1. 在 `src/tests/` 下创建模块目录
2. 创建 `test_{module}.hpp` 主测试文件
3. 添加具体的 `{module}_{feature}_test.cpp` 文件
4. 更新主测试文件的包含列表

### Q5: 测试运行失败但编译成功
**排查步骤**:
1. 检查测试逻辑是否正确
2. 确认测试数据是否有效
3. 验证断言条件是否合理
4. 查看错误输出信息

---

## 测试重构总结

### 完成的工作 ✅

1. **代码结构统一化**: 将所有测试文件从函数式结构转换为类式结构，统一使用 `namespace Lua::Tests` 命名空间，所有测试类都实现 `static void runAllTests()` 方法

2. **目录结构模块化**: 按功能模块重新组织测试目录结构（lexer、parser、compiler、vm、gc等）

3. **测试统一入口创建**: 为每个模块创建了统一的测试入口文件

4. **主测试入口更新**: 更新 `test_main.hpp/.cpp` 以使用新的模块化测试结构

5. **文档完善**: 提供完整的使用示例和构建说明

### 项目优势

- **模块化**: 测试按功能模块清晰组织
- **可维护性**: 统一的代码结构和命名规范
- **可扩展性**: 新测试可以轻松添加到相应模块
- **灵活性**: 支持运行全部测试、模块测试或单个测试
- **清晰性**: 每个测试的作用和位置都有明确的文档说明

## 测试层次结构

我们的测试框架采用5级层次结构：

```
MAIN (主测试)
├── MODULE (模块测试)
│   ├── SUITE (套件测试)
│   │   ├── GROUP (组测试)
│   │   │   └── INDIVIDUAL (个别测试)
```

### 层次说明和宏使用

1. **MAIN 级别**: 整个项目的最高级别测试入口点
   - 宏: `RUN_MAIN_TEST(TestName, TestFunction)`
   - 使用场景: 在主测试文件中调用，运行整个项目的所有测试

2. **MODULE 级别**: 特定功能模块的测试协调器
   - 宏: `RUN_TEST_MODULE(ModuleName, ModuleTestClass)`
   - 使用场景: 在主测试函数中调用各个模块，组织不同功能模块的测试

3. **SUITE 级别**: 模块内相关功能的测试套件
   - 宏: `RUN_TEST_SUITE(TestSuiteName)`
   - 使用场景: 在模块测试类中调用，组织相关功能的测试

4. **GROUP 级别**: 套件内特定功能区域的测试组
   - 宏: `RUN_TEST_GROUP(GroupName, GroupFunction)`
   - 使用场景: 在测试套件类中调用，组织特定功能的测试

5. **INDIVIDUAL 级别**: 单个测试用例的执行
   - 宏: `RUN_TEST(ClassName, methodName)` 或 `SAFE_RUN_TEST(ClassName, methodName)`
   - 使用场景: 在测试组函数中调用，执行具体的测试方法

### 命名约定

- **模块类**: `[ModuleName]TestSuite` (如 `ParserTestSuite`)
- **套件类**: `[FeatureName]TestSuite` (如 `ExprTestSuite`)
- **测试类**: `[SpecificFeature]Test` (如 `BinaryExprTest`)
- **测试方法**: `test[SpecificCase]` (如 `testAddition`)

## 模块测试文件规范

### 📁 模块测试文件结构

为了简化文件管理和提高代码可维护性，**模块测试文件**(如 `test_lib.hpp`, `test_parser.hpp` 等)应遵循以下规范：

#### ✅ 推荐结构：单一头文件实现

**模块测试文件的实现函数应直接写在头文件中，无需单独的 `.cpp` 文件**

```cpp
// test_module.hpp - 推荐的模块测试文件结构
#pragma once

#include "../../test_framework/core/test_utils.hpp"
#include "specific_test1.hpp"
#include "specific_test2.hpp"
// ... 其他测试套件包含

namespace Lua::Tests {

/**
 * @brief Module test suite
 * 
 * Coordinates all module related tests
 */
class ModuleTestSuite {
public:
    /**
     * @brief Run all module tests
     * 
     * Execute all test suites in this module
     */
    static void runAllTests() {
        RUN_TEST_SUITE(SpecificTest1);
        RUN_TEST_SUITE(SpecificTest2);
        RUN_TEST_SUITE(SpecificTest3);
        
        // TODO: Add other test suites here when available
    }
};

} // namespace Lua::Tests
```

#### ❌ 避免的结构：分离的头文件和实现文件

```cpp
// ❌ 不推荐：test_module.hpp + test_module.cpp 的分离结构
// test_module.hpp
class ModuleTestSuite {
public:
    static void runAllTests(); // 仅声明
};

// test_module.cpp  
void ModuleTestSuite::runAllTests() {
    // 实现代码...
}
```

### 🎯 模块测试文件的作用

模块测试文件作为**MODULE级别**的测试协调器，主要职责是：

1. **统一入口**: 提供模块内所有测试的单一入口点
2. **测试协调**: 使用 `RUN_TEST_SUITE` 宏协调各个测试套件
3. **依赖管理**: 包含模块内所有测试套件的头文件
4. **层次组织**: 在测试框架的5级层次中处于MODULE级别

### 📋 模块测试文件规范要点

#### 1. **文件命名**
- 模块测试文件命名：`test_[module_name].hpp`
- 例如：`test_lib.hpp`, `test_parser.hpp`, `test_compiler.hpp`

#### 2. **类命名**
- 模块测试类命名：`[ModuleName]TestSuite`
- 例如：`LibTestSuite`, `ParserTestSuite`, `CompilerTestSuite`

#### 3. **命名空间**
- 统一使用：`namespace Lua::Tests`

#### 4. **必需方法**
- 每个模块测试类必须实现：`static void runAllTests()`

#### 5. **测试宏使用**
- 在模块测试文件中使用：`RUN_TEST_SUITE(TestSuiteName)`
- 不使用：`RUN_TEST_GROUP` 或 `RUN_TEST`（这些属于更低层级）

#### 6. **头文件包含**
```cpp
#include "../../test_framework/core/test_utils.hpp"           // 测试框架工具
#include "specific_test1.hpp"          // 模块内的具体测试套件
#include "specific_test2.hpp"          // 更多测试套件...
#include "../../common/types.hpp"      // 如需要的公共类型
```

### 🔧 模块测试文件重构示例

#### 重构前（不推荐）
```cpp
// test_lib.hpp
class LibTestSuite {
public:
    static void runAllTests();
private:
    static void printSectionHeader(const std::string& title);
    static void printSectionFooter();
};

// test_lib.cpp
void LibTestSuite::runAllTests() {
    std::cout << "=== Standard Library Tests ===" << std::endl;
    BaseLibTest::runAllTests();
    TableLibTest::runAllTests();
    // ... 自定义格式化代码
}
```

#### 重构后（推荐）
```cpp
// test_lib.hpp
#pragma once

#include "../../test_framework/core/test_utils.hpp"
#include "base_lib_test.hpp"
#include "table_lib_test.hpp"

namespace Lua::Tests {

class LibTestSuite {
public:
    static void runAllTests() {
        RUN_TEST_SUITE(BaseLibTestSuite);
        RUN_TEST_SUITE(TableLibTest);
        RUN_TEST_SUITE(StringLibTest);
        RUN_TEST_SUITE(MathLibTest);
    }
};

} // namespace Lua::Tests
```

### 📈 规范优势

1. **🗂️ 文件简化**: 减少文件数量，简化项目结构
2. **🎯 职责明确**: 模块测试文件专注于测试协调，不处理具体测试逻辑
3. **🔧 维护便利**: 内联实现便于快速查看和修改
4. **📊 规范统一**: 所有模块测试文件结构一致
5. **🚀 构建优化**: 减少编译单元，可能提升构建速度

### 🔍 识别模块测试文件

模块测试文件通常具有以下特征：
- 文件名以 `test_` 开头
- 类名以 `TestSuite` 结尾  
- 主要包含 `RUN_TEST_SUITE` 调用
- 作为模块内多个测试套件的协调器
- 位于各模块目录的根层级

### 📝 实施建议

1. **新建模块**: 直接采用单一头文件结构
2. **现有模块**: 逐步重构，将 `.cpp` 内容合并到 `.hpp` 并删除 `.cpp` 文件
3. **保持一致**: 确保所有模块测试文件都遵循相同规范
4. **文档更新**: 重构后及时更新相关文档和注释

# Test Formatting System

这是一个模块化的测试输出格式化系统，提供了层级化的测试输出、颜色支持和可配置的格式化选项。

## 特性

- **层级化输出**: 支持 MAIN、SUITE、GROUP、INDIVIDUAL 四个层级
- **颜色支持**: 自动检测终端颜色支持，提供多种颜色主题
- **可配置**: 支持配置文件和环境变量配置
- **跨平台**: 支持 Windows 和 Unix-like 系统
- **模块化设计**: 清晰的代码结构，易于维护和扩展

## 文件结构

```
tests/
├── test_main.hpp               # 主接口文件
├── example_usage.cpp           # 使用示例
├── test_format_config.txt      # 配置文件示例
├── README.md                   # 说明文档
└── formatting/                 # 格式化模块
    ├── test_formatter.hpp      # 核心格式化器
    ├── test_formatter.cpp
    ├── test_config.hpp         # 配置管理
    ├── test_config.cpp
    ├── test_colors.hpp         # 颜色管理
    ├── test_colors.cpp
    ├── format_strategies.hpp   # 格式化策略
    └── format_strategies.cpp
```

## 快速开始

### 1. 编译示例

```bash
cd tests
# 使用您首选的构建系统编译项目
```

### 2. 运行示例

```bash
./example_usage
```

或者直接运行：

```bash
./example_usage
```

## 使用方法

### 运行所有测试

```cpp
#include "test_main.hpp"
Lua::Tests::runAllTests();
```

### 运行特定模块测试

```cpp
// 编译器模块
#include "compiler/test_compiler.hpp"
CompilerTest::runAllTests();

// GC 模块
#include "gc/test_gc.hpp"
GCTest::runAllTests();

// Parser 模块
#include "parser/test_parser.hpp"
ParserTestSuite::runAllTests();

// VM 模块
#include "vm/test_vm.hpp"
VMTestSuite::runAllTests();
```

### 运行单个测试类

```cpp
#include "parser/function_test.hpp"
FunctionTest::runAllTests();
```

### 基本输出格式使用

```cpp
#include "../test_framework/core/test_utils.hpp"

// 使用宏进行层级化测试
RUN_MAIN_TEST("主测试", []() {
    RUN_TEST_SUITE(MyTestSuite);
});

// 手动使用不同层级
TestUtils::printLevelHeader(TestUtils::TestLevel::MAIN, "主测试");
TestUtils::printLevelHeader(TestUtils::TestLevel::SUITE, "测试套件");
TestUtils::printLevelHeader(TestUtils::TestLevel::GROUP, "测试组");
TestUtils::printLevelHeader(TestUtils::TestLevel::INDIVIDUAL, "单个测试");

// 输出测试结果
TestUtils::printTestResult("测试名称", true);  // 通过
TestUtils::printTestResult("测试名称", false); // 失败

// 输出信息
TestUtils::printInfo("信息消息");
TestUtils::printWarning("警告消息");
TestUtils::printError("错误消息");
```

### 输出格式规范

#### 层次结构
- **测试套件级别**: 使用 `printSimpleSectionHeader/Footer`
- **测试组级别**: 使用 `printSectionHeader/Footer`
- **单个测试级别**: 使用 `printTestResult`
- **信息级别**: 使用 `printInfo/Warning/Error`

#### 缩进规范
- 节标题：无缩进
- 测试结果：4个空格缩进
- 信息消息：4个空格缩进

#### 标识符规范
- `[PASS]`：测试通过
- `[FAIL]`：测试失败
- `[INFO]`：信息消息
- `[WARN]`：警告消息
- `[ERROR]`：错误消息
- `[OK]`：节完成

### 宏函数使用

#### RUN_TEST 宏
用于执行单个测试方法，自动处理异常和输出格式：

```cpp
RUN_TEST(SynchronizeTest, testBasicSynchronization);
```

#### RUN_TEST_SUITE 宏
用于执行整个测试套件：

```cpp
RUN_TEST_SUITE(SynchronizeTest);
```

#### SAFE_RUN_TEST 宏
用于安全执行测试，不会重新抛出异常：

```cpp
SAFE_RUN_TEST(SynchronizeTest, testBasicSynchronization);
```

### 配置系统

#### 使用配置文件

创建 `test_format_config.txt`：

```
# 启用颜色
colorEnabled=true

# 设置主题 (default, dark, light, mono)
theme=dark
```

在代码中加载：

```cpp
TestUtils::loadConfig("test_format_config.txt");
```

#### 使用环境变量

```bash
# 禁用颜色
export NO_COLOR=1

# 强制启用颜色
export FORCE_COLOR=1

# 设置主题
export TEST_THEME=dark
```

#### 运行时配置

```cpp
// 设置颜色
TestUtils::setColorEnabled(true);

// 设置主题
TestUtils::setTheme("dark");
```

### 可用的颜色主题

- **default**: 标准颜色方案
- **dark**: 适合深色背景的鲜艳颜色
- **light**: 适合浅色背景的柔和颜色
- **mono**: 单色方案，只使用文本格式化

## 输出层级说明

### MAIN 层级
- 用于最顶层的测试执行
- 使用双线边框和强调色
- 适合整个测试程序的开始和结束

### SUITE 层级
- 用于测试套件
- 使用单线边框
- 适合一组相关测试的分组

### GROUP 层级
- 用于测试组
- 使用简洁的树形结构
- 适合套件内的子分组

### INDIVIDUAL 层级
- 用于单个测试
- 使用简单的项目符号
- 适合最小的测试单元

### 输出格式示例

```
╔══════════════════════════════════════════════════════════════════════════════╗
║                           MAIN: Lua Compiler Tests                          ║
╚══════════════════════════════════════════════════════════════════════════════╝

┌─────────────────────────────────────────────────────────────────────────────┐
│                            MODULE: Parser Module                            │
└─────────────────────────────────────────────────────────────────────────────┘

  ┌───────────────────────────────────────────────────────────────────────────┐
  │                        SUITE: ExprTestSuite                              │
  └───────────────────────────────────────────────────────────────────────────┘

    ┌─────────────────────────────────────────────────────────────────────────┐
    │                    GROUP: Binary Expression Tests                      │
    └─────────────────────────────────────────────────────────────────────────┘

      [OK] INDIVIDUAL: BinaryExprTest::testAddition
      [OK] INDIVIDUAL: BinaryExprTest::testSubtraction
      ✗ INDIVIDUAL: BinaryExprTest::testDivision
```

## 跨平台支持

### Windows
- 自动启用 ANSI 转义序列支持（Windows 10+）
- 检测 Windows Terminal 和现代终端
- 提供降级支持

### Unix-like 系统
- 自动检测终端颜色支持
- 支持标准的 TERM 环境变量
- 兼容各种终端模拟器

## 扩展开发

### 添加新的格式化策略

1. 在 `format_strategies.hpp` 中定义新的策略类
2. 实现 `IFormatStrategy` 接口
3. 在 `TestFormatter` 中注册新策略

### 添加新的颜色主题

1. 在 `test_colors.cpp` 的 `initializeColorSchemes()` 中添加新主题
2. 定义各种颜色类型的 ANSI 代码

### 添加新的配置选项

1. 在 `TestConfig` 类中添加新的成员变量
2. 在配置文件解析和环境变量检查中添加支持
3. 提供相应的 getter/setter 方法

## 性能考虑

- 使用单例模式避免重复初始化
- 颜色检测只在启动时进行一次
- 策略模式允许高效的格式化选择
- 最小化字符串操作和内存分配

## 故障排除

### 颜色不显示
1. 检查 `NO_COLOR` 环境变量
2. 确认终端支持 ANSI 转义序列
3. 尝试设置 `FORCE_COLOR=1`

### 编译错误
1. 确保使用 C++14 或更高版本
2. 检查包含路径设置
3. 确认所有源文件都已编译

### 配置文件不生效
1. 检查文件路径是否正确
2. 确认文件格式（key=value）
3. 查看是否有语法错误

## 许可证

本项目遵循与主项目相同的许可证。

## 目录结构

```
tests/
├── README.md                    # 本文件 - 测试目录说明
├── test_main.cpp               # 主测试入口
├── test_main.hpp               # 主测试头文件
├── test_main.hpp               # 测试主入口
├── formatting/                 # 格式化模块
│   ├── test_formatter.hpp      # 核心格式化器
│   ├── test_formatter.cpp
│   ├── test_config.hpp         # 配置管理
│   ├── test_config.cpp
│   ├── test_colors.hpp         # 颜色管理
│   ├── test_colors.cpp
│   ├── format_strategies.hpp   # 格式化策略
│   └── format_strategies.cpp
├── lexer/                      # 词法分析器测试
│   ├── test_lexer_basic.cpp
│   └── test_lexer.hpp
├── parser/                     # 语法分析器测试
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
│   ├── test_vm.hpp                     # 虚拟机测试统一入口头文件
│   ├── state_test.cpp          # 状态管理测试
│   ├── state_test.hpp
│   ├── value_test.cpp          # 值系统测试
│   └── value_test.hpp
├── gc/                         # 垃圾回收器测试
│   ├── test_gc.hpp                     # GC 测试统一入口头文件
│   ├── gc_integration_test.cpp         # GC 集成测试
│   ├── gc_integration_test.hpp
│   ├── string_pool_demo_test.cpp       # 字符串池演示测试
│   └── string_pool_demo_test.hpp
├── lib/                        # 标准库测试
│   ├── test_lib.hpp                    # 标准库测试统一入口头文件
│   ├── base_lib_test.hpp               # 基础库测试
│   ├── base_lib_test.cpp               # 基础库测试实现
│   ├── table_lib_test.cpp              # 表库测试
│   ├── table_lib_test.hpp
│   ├── string_lib_test.cpp             # 字符串库测试
│   ├── string_lib_test.hpp
│   ├── math_lib_test.cpp               # 数学库测试
│   └── math_lib_test.hpp
├── localization/               # 本地化测试 (示例)
│   ├── localization_test.cpp           # 本地化功能测试
│   └── localization_test.hpp
├── integration/                # 集成测试
└── plugin/                     # 插件系统测试
```

### 统一测试入口文件

根据模块测试文件规范，统一测试入口文件现在采用单一头文件实现：

- **编译器测试统一入口**: `compiler/test_compiler.hpp` - 包含所有编译器相关测试的统一调用
- **GC 测试统一入口**: `gc/test_gc.hpp` - 包含所有垃圾回收器相关测试的统一调用  
- **Parser 测试统一入口**: `parser/test_parser.hpp` - 包含所有语法分析器相关测试的统一调用
- **VM 测试统一入口**: `vm/test_vm.hpp` - 包含所有虚拟机相关测试的统一调用
- **标准库测试统一入口**: `lib/test_lib.hpp` - 包含所有标准库相关测试的统一调用

**注意**: 这些模块测试文件的实现直接写在头文件中，不再需要对应的 `.cpp` 文件。

## 最佳实践

1. **保持层次清晰**: 每个层次都有明确的职责
2. **使用正确的宏**: 根据层次选择合适的宏
3. **添加文档**: 为测试类添加层次说明
4. **逐步迁移**: 可以逐步迁移现有代码，不需要一次性全部更改
5. **测试独立性**: 确保每个层次的测试都是独立的

## 如何添加新的测试文件

### 步骤 1: 确定测试类型和位置

根据你要测试的功能，选择合适的目录：

- **lexer/**: 词法分析相关功能
- **parser/**: 语法分析相关功能  
- **compiler/**: 编译器相关功能
- **vm/**: 虚拟机运行时功能
- **gc/**: 垃圾回收器功能
- **lib/**: 标准库功能
- **localization/**: 本地化和国际化功能
- **plugin/**: 插件系统功能
- **integration/**: 集成测试功能
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

### 1. 测试文件命名规范 🎯

> **📋 快速识别指南**  
> **L1 模块**: `test_<module>.hpp` (主协调器)  
> **L2 功能**: `<module>_<feature>_test.hpp` (功能测试)  
> **L3 细节**: `<module>_<feature>_<detail>_test.hpp` (详细测试)  
> **类名**: `<Module><Feature>Test` (PascalCase)  
> **方法**: `test<SpecificCase>()` (camelCase)

#### 1.1 🏗️ 功能层级架构

测试文件按照三层功能架构组织：

```
📁 <module>/                     # L1: 模块层 (Module Level)
├── 📄 test_<module>.hpp         # 主测试套件 (Test Suite Coordinator)
├── 📄 test_<module>.cpp         # 主测试套件实现
├── 📄 <module>_<feature>_test.* # L2: 功能层 (Feature Level)
├── 📄 <module>_<detail>_test.*  # L3: 细节层 (Detail Level)
└── 📁 <submodule>/              # 子模块 (可选)
    ├── 📄 <module>_<submodule>_<feature>_test.*
    └── 📄 test_<module>_<submodule>.hpp
```

#### 1.2 📝 命名规范矩阵

| 层级 | 文件命名格式 | 类命名格式 | 用途 | 示例 |
|------|-------------|------------|------|------|
| **L1 模块** | `test_<module>.hpp` | `<Module>TestSuite` | 主协调器，统一入口 | `test_parser.hpp` → `ParserTestSuite` |
| **L2 功能** | `<module>_<feature>_test.hpp` | `<Module><Feature>Test` | 核心功能测试 | `parser_expression_test.hpp` → `ParserExpressionTest` |
| **L3 细节** | `<module>_<feature>_<detail>_test.hpp` | `<Module><Feature><Detail>Test` | 具体实现测试 | `parser_expression_binary_test.hpp` → `ParserExpressionBinaryTest` |
| **错误处理** | `<module>_error_test.hpp` | `<Module>ErrorTest` | 错误场景测试 | `parser_error_test.hpp` → `ParserErrorTest` |
| **性能测试** | `<module>_performance_test.hpp` | `<Module>PerformanceTest` | 性能基准测试 | `parser_performance_test.hpp` → `ParserPerformanceTest` |
| **集成测试** | `<module>_integration_test.hpp` | `<Module>IntegrationTest` | 模块间集成 | `parser_integration_test.hpp` → `ParserIntegrationTest` |

#### 1.3 🎨 标准功能分类

每个模块应包含以下标准功能测试：

| 功能类型 | 命名后缀 | 测试内容 | 优先级 |
|----------|----------|----------|--------|
| **基础功能** | `_basic_test` | 核心API、基本操作 | 🔴 必须 |
| **高级功能** | `_advanced_test` | 复杂场景、组合操作 | 🟡 重要 |
| **错误处理** | `_error_test` | 异常情况、边界条件 | 🔴 必须 |
| **内存管理** | `_memory_test` | 内存分配、泄漏检测 | 🟡 重要 |
| **性能测试** | `_performance_test` | 性能基准、压力测试 | 🟢 可选 |
| **集成测试** | `_integration_test` | 模块间协作 | 🟡 重要 |

#### 1.4 📂 目录结构示例

##### 简单模块 (如 lexer)
```
lexer/
├── test_lexer.hpp              # L1: LexerTestSuite
├── test_lexer.cpp
├── lexer_basic_test.hpp        # L2: LexerBasicTest
├── lexer_basic_test.cpp
├── test_lexer_error.hpp        # L2: LexerErrorTestSuite
└── test_lexer_error.cpp
```

##### 复杂模块 (如 parser)
```
parser/
├── test_parser.hpp             # L1: ParserTestSuite
├── test_parser.cpp
├── parser_basic_test.hpp       # L2: ParserBasicTest
├── parser_error_test.hpp       # L2: ParserErrorTest
├── expr/                       # 子模块: 表达式解析
│   ├── test_expr.hpp           # L1: ExprTestSuite
│   ├── binary_expr_test.hpp    # L2: BinaryExprTest
│   ├── unary_expr_test.hpp     # L2: UnaryExprTest
│   └── literal_expr_test.hpp   # L2: LiteralExprTest
└── stmt/                       # 子模块: 语句解析
    ├── test_stmt.hpp           # L1: StmtTestSuite
    └── return_stmt_test.hpp    # L2: ReturnStmtTest
```

##### 超复杂模块 (如 vm)
```
vm/
├── test_vm.hpp                 # L1: VMTestSuite
├── vm_basic_test.hpp           # L2: VMBasicTest
├── vm_error_test.hpp           # L2: VMErrorTest
├── closure/                    # 子模块: 闭包
│   ├── test_closure.hpp        # L1: ClosureTestSuite
│   ├── closure_basic_test.hpp  # L2: ClosureBasicTest
│   ├── closure_advanced_test.hpp # L2: ClosureAdvancedTest
│   ├── closure_memory_test.hpp # L2: ClosureMemoryTest
│   └── closure_error_test.hpp  # L2: ClosureErrorTest
└── state/                      # 子模块: 状态管理
    ├── test_state.hpp          # L1: StateTestSuite
    ├── state_basic_test.hpp    # L2: StateBasicTest
    ├── state_stack_test.hpp    # L2: StateStackTest
    └── state_gc_test.hpp       # L2: StateGCTest
```

#### 1.5 🏷️ 类和方法命名规范

##### 类命名规范
```cpp
// L1 主测试套件
class ParserTestSuite {          // <Module>TestSuite
    static void runAllTests();
};

// L2 功能测试类
class ParserExpressionTest {     // <Module><Feature>Test
    static void runAllTests();
    static void testBinaryExpression();
    static void testUnaryExpression();
};

// L3 详细测试类
class ParserExpressionBinaryTest { // <Module><Feature><Detail>Test
    static void runAllTests();
    static void testArithmeticOperators();
    static void testLogicalOperators();
};
```

##### 方法命名规范
```cpp
class CompilerTest {
public:
    // 主入口方法 (必须)
    static void runAllTests();
    
private:
    // 功能测试方法
    static void testBasicCompilation();     // test<SpecificFeature>()
    static void testExpressionCompilation();
    static void testStatementCompilation();
    
    // 辅助方法
    static void printTestResult(const std::string& testName, bool passed);
    static bool compileAndVerify(const std::string& source);
};
```

#### 1.6 🔍 命名空间规范

```cpp
namespace Lua {
namespace Tests {
        class ParserTestSuite { /* ... */ };
        class ParserExprTestSuite { /* ... */ };
        class ParserStmtTestSuite { /* ... */ };

        class CompilerTestSuite { /* ... */ };
        class CompilerExprTestSuit { /* ... */ };
        class CompilerStmtTestSuit { /* ... */ };
    }
}
```

#### 1.7 ✅ 命名规范检查清单

**文件命名检查:**
- [ ] L1主测试文件: `test_<module>.hpp`
- [ ] L2功能测试: `<module>_<feature>_test.hpp`
- [ ] L3详细测试: `<module>_<feature>_<detail>_test.hpp`
- [ ] 错误测试: `<module>_error_test.hpp`
- [ ] 文件名全小写，使用下划线分隔

**类命名检查:**
- [ ] 主测试套件: `<Module>TestSuite`
- [ ] 功能测试类: `<Module><Feature>Test`
- [ ] 类名使用PascalCase
- [ ] 避免缩写，使用完整单词

**方法命名检查:**
- [ ] 主入口方法: `runAllTests()`
- [ ] 测试方法: `test<SpecificFeature>()`
- [ ] 方法名使用camelCase
- [ ] 方法名描述具体测试内容

**目录结构检查:**
- [ ] 模块目录名与功能对应
- [ ] 子模块目录结构清晰
- [ ] 文件组织符合功能层级

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

#### 测试调用规范

- **主测试文件调用其他主测试文件时**：使用 `RUN_TEST_SUITE(TestClass)` 宏
- **调用子测试文件时**：使用 `RUN_TEST(TestClass, TestMethod)` 宏

```cpp
// 示例：主测试文件调用其他主测试文件
RUN_TEST_SUITE(ParserTestSuite);
RUN_TEST_SUITE(LexerTestSuite);

// 示例：调用具体的测试方法
RUN_TEST(BasicParserTest, testTokenParsing);
RUN_TEST(BasicParserTest, testExpressionParsing);
```

### 4. 错误处理

- 使用 try-catch 块捕获异常
- 提供清晰的错误信息
- 测试正常情况和异常情况

### 5. 输出格式

详细的输出格式指南请参考：[OUTPUT_FORMAT_GUIDE.md](OUTPUT_FORMAT_GUIDE.md)

- 使用统一的输出格式
- 明确标识测试通过/失败
- 提供有意义的测试描述

### 6. 测试独立性

- 每个测试应该独立运行
- 不依赖其他测试的执行顺序
- 清理测试产生的副作用

### 7. 🔧 编译规范

> **⚡ 快速开始（30秒理解）**
> 
> **核心原则**: 每个测试文件必须能独立编译 ✅  
> **验证命令**: `g++ -std=c++17 -I../../../ your_test.cpp -o test_output` 🚀  
> **关键步骤**: 添加临时main → 编译验证 → 运行测试 → 修复问题 → 清理代码 🔄

#### 7.1 📋 5步编译验证流程

| 步骤 | 操作 | 命令/代码 | 说明 |
|------|------|-----------|------|
| **1️⃣** | **添加临时main函数** | 在测试文件末尾添加 | 用于独立编译测试 |
| **2️⃣** | **独立编译测试** | `g++ -std=c++17 -I../../../ your_test.cpp -o test_output` | 验证编译通过 |
| **3️⃣** | **运行测试验证** | `./test_output` | 确保测试正常执行 |
| **4️⃣** | **修复编译问题** | 根据错误信息修复 | 参考下方错误速查表 |
| **5️⃣** | **清理临时代码** | 删除临时main函数 | 保持代码整洁 |

##### 📝 临时main函数模板
```cpp
// Temporary main function for compilation testing
int main() {
    Lua::Tests::YourTestClass::runAllTests();
    return 0;
}
```

##### 🖥️ 编译命令模板
```bash
# Windows (PowerShell)
g++ -std=c++17 -I../../../ your_test.cpp -o test_output.exe

# Linux/macOS
g++ -std=c++17 -I../../../ your_test.cpp -o test_output
```

#### 7.2 ⚠️ 常见编译错误速查表

| 错误类型 | 错误症状 | 解决方案 | 示例 |
|---------|----------|----------|------|
| **🔗 缺失头文件** | `fatal error: 'xxx.hpp' file not found` | 添加 `#include "xxx.hpp"` | `#include "../../compiler/compiler.hpp"` |
| **❓ 未定义符号** | `undefined reference to 'ClassName::method'` | 检查类名和方法名拼写 | 确认 `CompilerTest::runAllTests()` 存在 |
| **📁 路径错误** | `No such file or directory` | 修正 `#include` 路径 | 使用相对路径 `../../module/file.hpp` |
| **🔧 链接错误** | `undefined symbol` | 添加依赖的 .cpp 文件到编译命令 | `g++ test.cpp dependency.cpp` |
| **🏷️ 命名空间错误** | `'Tests' is not a member of 'Lua'` | 检查命名空间声明 | 确保使用 `namespace Lua::Tests` |
| **📦 类型不匹配** | `cannot convert 'int' to 'std::string'` | 检查参数类型 | 修正函数调用参数 |

#### 7.3 ✅ 编译检查清单

**编译前检查:**
- [ ] 包含所有必要的头文件
- [ ] 使用正确的命名空间 `Lua::Tests`
- [ ] 相对路径指向正确
- [ ] 临时main函数已添加

**编译后验证:**
- [ ] 编译无错误无警告
- [ ] 可执行文件生成成功
- [ ] 测试运行正常
- [ ] 临时main函数已移除

#### 7.4 🚀 一键编译验证脚本

**Windows PowerShell:**
```powershell
# 快速编译验证脚本
$testFile = "your_test.cpp"
$outputFile = "test_output.exe"

Write-Host "🔧 编译测试文件: $testFile" -ForegroundColor Yellow
g++ -std=c++17 -I../../../ $testFile -o $outputFile

if ($LASTEXITCODE -eq 0) {
    Write-Host "✅ 编译成功，运行测试..." -ForegroundColor Green
    .\$outputFile
    Remove-Item $outputFile
    Write-Host "🧹 清理完成" -ForegroundColor Blue
} else {
    Write-Host "❌ 编译失败，请检查错误信息" -ForegroundColor Red
}
```

**Linux/macOS Bash:**
```bash
#!/bin/bash
# 快速编译验证脚本
TEST_FILE="your_test.cpp"
OUTPUT_FILE="test_output"

echo "🔧 编译测试文件: $TEST_FILE"
g++ -std=c++17 -I../../../ $TEST_FILE -o $OUTPUT_FILE

if [ $? -eq 0 ]; then
    echo "✅ 编译成功，运行测试..."
    ./$OUTPUT_FILE
    rm $OUTPUT_FILE
    echo "🧹 清理完成"
else
    echo "❌ 编译失败，请检查错误信息"
fi
```

#### 7.5 🎯 编译规范核心要点

| 要点 | 说明 | 重要性 |
|------|------|--------|
| **🔍 完整性检查** | 确保子测试文件包含所有必要的依赖 | ⭐⭐⭐ |
| **🔬 独立性验证** | 每个子测试文件都应该能够独立编译 | ⭐⭐⭐ |
| **🛠️ 错误修复** | 及时修复编译过程中发现的问题 | ⭐⭐⭐ |
| **🧹 清理工作** | 完成验证后移除临时代码 | ⭐⭐ |
| **📚 文档更新** | 记录编译依赖和特殊要求 | ⭐⭐ |

> **💡 提示**: 这个编译验证流程确保了测试文件的质量和可维护性，是测试开发的重要环节。

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
│   ├── CompilerSymbolTableTest::runAllTests()
│   ├── CompilerLiteralTest::runAllTests()
│   ├── CompilerVariableTest::runAllTests()
│   ├── CompilerBinaryExpressionTest::runAllTests()
│   ├── CompilerExpressionTest::runAllTests()
│   ├── CompilerConditionalTest::runAllTests()
│   └── CompilerMultiReturnTest::runAllTests()
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
- ✅ **易于扩展**: 新测试可以轻松添加到相应模块

## 构建和运行说明

**注意**: 当前项目有两个测试目录：

1. **`tests/`** - 基于 Google Test 的原始测试文件
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

### 方法 3: 集成到构建系统
根据您使用的构建系统，添加相应的配置：
```
# 新的模块化测试
add_executable(modular_tests
  src/tests/test_main.cpp
  src/tests/compiler/test_compiler.cpp
  src/tests/gc/test_gc.cpp
  # 添加其他测试源文件...
)
target_link_libraries(modular_tests PRIVATE lua_lib)
```

## 子模块测试规范：命名空间隔离方案

### 🎯 问题背景

在复杂的项目中，不同模块可能包含相似的测试套件名称，例如：
- `parser/expr/ExprTestSuite` - 解析器表达式测试
- `compiler/expr/ExprTestSuite` - 编译器表达式测试
- `vm/expr/ExprTestSuite` - 虚拟机表达式测试

传统的平坦命名方式无法有效区分这些测试套件的归属，容易造成命名冲突和维护困难。

### 🏗️ 解决方案：命名空间隔离

采用**命名空间隔离**方案，通过层级化的命名空间明确区分不同模块和子模块的测试套件。

#### 命名空间层级结构

```cpp
namespace Lua {
    namespace Tests {
        namespace Parser {        // 解析器模块
            namespace Expr {      // 表达式子模块
                class ExprTestSuite { /* ... */ };
                class BinaryExprTest { /* ... */ };
                class UnaryExprTest { /* ... */ };
            }
            namespace Stmt {      // 语句子模块
                class StmtTestSuite { /* ... */ };
                class IfStmtTest { /* ... */ };
            }
        }
        namespace Compiler {      // 编译器模块
            namespace Expr {
                class ExprTestSuite { /* ... */ };
                class ExprCompilerTest { /* ... */ };
            }
        }
        namespace VM {            // 虚拟机模块
            namespace Expr {
                class ExprTestSuite { /* ... */ };
                class ExprExecutorTest { /* ... */ };
            }
        }
    }
}
```

#### 调用方式

```cpp
// 明确的命名空间调用
Lua::Tests::Parser::Expr::ExprTestSuite::runAllTests();
Lua::Tests::Compiler::Expr::ExprTestSuite::runAllTests();
Lua::Tests::VM::Expr::ExprTestSuite::runAllTests();

// 或使用 using 声明简化
using namespace Lua::Tests::Parser;
Expr::ExprTestSuite::runAllTests();
Stmt::StmtTestSuite::runAllTests();
```

### 📁 文件组织结构

#### 目录结构
```
src/tests/
├── parser/
│   ├── test_parser.hpp                    # Lua::Tests::Parser::ParserTestSuite
│   ├── expr/
│   │   ├── test_parser_expr.hpp           # Lua::Tests::Parser::Expr::ExprTestSuite
│   │   ├── parser_binary_expr_test.hpp    # Lua::Tests::Parser::Expr::ParserBinaryExprTest (声明)
│   │   ├── parser_binary_expr_test.cpp    # Lua::Tests::Parser::Expr::ParserBinaryExprTest (实现)
│   │   ├── parser_unary_expr_test.hpp     # Lua::Tests::Parser::Expr::ParserUnaryExprTest (声明)
│   │   ├── parser_unary_expr_test.cpp     # Lua::Tests::Parser::Expr::ParserUnaryExprTest (实现)
│   │   ├── parser_literal_expr_test.hpp   # Lua::Tests::Parser::Expr::ParserLiteralExprTest (声明)
│   │   └── parser_literal_expr_test.cpp   # Lua::Tests::Parser::Expr::ParserLiteralExprTest (实现)
│   └── stmt/
│       ├── test_parser_stmt.hpp           # Lua::Tests::Parser::Stmt::StmtTestSuite
│       ├── parser_if_stmt_test.hpp        # Lua::Tests::Parser::Stmt::ParserIfStmtTest (声明)
│       ├── parser_if_stmt_test.cpp        # Lua::Tests::Parser::Stmt::ParserIfStmtTest (实现)
│       ├── parser_while_stmt_test.hpp     # Lua::Tests::Parser::Stmt::ParserWhileStmtTest (声明)
│       ├── parser_while_stmt_test.cpp     # Lua::Tests::Parser::Stmt::ParserWhileStmtTest (实现)
│       ├── parser_function_stmt_test.hpp  # Lua::Tests::Parser::Stmt::ParserFunctionStmtTest (声明)
│       └── parser_function_stmt_test.cpp  # Lua::Tests::Parser::Stmt::ParserFunctionStmtTest (实现)
├── compiler/
│   ├── test_compiler.hpp                  # Lua::Tests::Compiler::CompilerTestSuite
│   └── expr/
│       ├── test_compiler_expr.hpp         # Lua::Tests::Compiler::Expr::ExprTestSuite
│       ├── compiler_binary_expr_test.hpp  # Lua::Tests::Compiler::Expr::CompilerBinaryExprTest (声明)
│       ├── compiler_binary_expr_test.cpp  # Lua::Tests::Compiler::Expr::CompilerBinaryExprTest (实现)
│       ├── compiler_unary_expr_test.hpp   # Lua::Tests::Compiler::Expr::CompilerUnaryExprTest (声明)
│       ├── compiler_unary_expr_test.cpp   # Lua::Tests::Compiler::Expr::CompilerUnaryExprTest (实现)
│       ├── compiler_literal_expr_test.hpp # Lua::Tests::Compiler::Expr::CompilerLiteralExprTest (声明)
│       └── compiler_literal_expr_test.cpp # Lua::Tests::Compiler::Expr::CompilerLiteralExprTest (实现)
└── vm/
    ├── test_vm.hpp                        # Lua::Tests::VM::VMTestSuite
    └── expr/
        ├── test_vm_expr.hpp               # Lua::Tests::VM::Expr::ExprTestSuite
        ├── vm_binary_expr_test.hpp        # Lua::Tests::VM::Expr::VMBinaryExprTest (声明)
        ├── vm_binary_expr_test.cpp        # Lua::Tests::VM::Expr::VMBinaryExprTest (实现)
        ├── vm_unary_expr_test.hpp         # Lua::Tests::VM::Expr::VMUnaryExprTest (声明)
        ├── vm_unary_expr_test.cpp         # Lua::Tests::VM::Expr::VMUnaryExprTest (实现)
        ├── vm_literal_expr_test.hpp       # Lua::Tests::VM::Expr::VMLiteralExprTest (声明)
        └── vm_literal_expr_test.cpp       # Lua::Tests::VM::Expr::VMLiteralExprTest (实现)
```

#### 实现示例

**`src/tests/parser/expr/test_expr.hpp`**
```cpp
#pragma once

#include "../../../test_framework/core/test_utils.hpp"
#include "binary_expr_test.hpp"
#include "unary_expr_test.hpp"
#include "literal_expr_test.hpp"
#include "call_expr_test.hpp"
#include "table_expr_test.hpp"
#include "member_expr_test.hpp"
#include "variable_expr_test.hpp"

namespace Lua {
namespace Tests {
namespace Parser {
namespace Expr {

/**
 * @brief Parser Expression Test Suite
 * 
 * Coordinates all parser expression-related tests
 * Namespace: Lua::Tests::Parser::Expr
 */
class ExprTestSuite {
public:
    /**
     * @brief Run all parser expression tests
     * 
     * Execute all expression parsing test suites
     */
    static void runAllTests() {
        RUN_TEST_SUITE(BinaryExprTest);
        RUN_TEST_SUITE(UnaryExprTest);
        RUN_TEST_SUITE(LiteralExprTest);
        RUN_TEST_SUITE(CallExprTest);
        RUN_TEST_SUITE(TableExprTest);
        RUN_TEST_SUITE(MemberExprTest);
        RUN_TEST_SUITE(VariableExprTest);
    }
};

} // namespace Expr
} // namespace Parser
} // namespace Tests
} // namespace Lua
```

**`src/tests/parser/expr/test_expr.cpp`**
```cpp
#include "test_expr.hpp"

// 实现已在头文件中内联定义，此文件可选
// 如需要额外的实现代码，可在此添加
```

**`src/tests/parser/test_parser.hpp`**
```cpp
#pragma once

#include "../../test_framework/core/test_utils.hpp"
#include "expr/test_expr.hpp"
#include "stmt/test_stmt.hpp"

namespace Lua {
namespace Tests {
namespace Parser {

/**
 * @brief Parser Test Suite
 * 
 * Coordinates all parser-related tests
 * Namespace: Lua::Tests::Parser
 */
class ParserTestSuite {
public:
    /**
     * @brief Run all parser tests
     * 
     * Execute all parser test suites including expressions and statements
     */
    static void runAllTests() {
        RUN_TEST_SUITE(Expr::ExprTestSuite);
        RUN_TEST_SUITE(Stmt::StmtTestSuite);
    }
};

} // namespace Parser
} // namespace Tests
} // namespace Lua
```

### 🎯 规范要点

#### 1. **命名空间层级**
- **L1**: `Lua::Tests` - 项目根命名空间
- **L2**: `Lua::Tests::<Module>` - 模块命名空间（如 `Parser`, `Compiler`, `VM`）
- **L3**: `Lua::Tests::<Module>::<SubModule>` - 子模块命名空间（如 `Expr`, `Stmt`）
- **L4**: 可根据需要进一步细分

#### 2. **类命名规范**
- **测试套件**: `<Feature>TestSuite` （如 `ExprTestSuite`, `StmtTestSuite`）
- **具体测试**: `<Specific>Test` （如 `BinaryExprTest`, `IfStmtTest`）

#### 3. **文件命名规范**
- **测试套件文件**: `test_<feature>.hpp` （如 `test_expr.hpp`, `test_stmt.hpp`）
- **具体测试文件**: `<specific>_test.hpp` （如 `binary_expr_test.hpp`）

#### 4. **前缀命名策略**（避免跨模块文件重名）

为了避免不同模块间测试文件重名问题，采用**前缀命名策略**：

##### 命名规则
- **Parser 模块测试文件**: `parser_<feature>_test.hpp/cpp`
- **Compiler 模块测试文件**: `compiler_<feature>_test.hpp/cpp`
- **VM 模块测试文件**: `vm_<feature>_test.hpp/cpp`
- **其他模块**: `<module>_<feature>_test.hpp/cpp`

##### 示例对比

**传统命名（可能重名）**:
```
src/tests/parser/expr/binary_expr_test.hpp
src/tests/compiler/expr/binary_expr_test.hpp  # 重名！
src/tests/vm/expr/binary_expr_test.hpp         # 重名！
```

**前缀命名（避免重名）**:
```
src/tests/parser/expr/parser_binary_expr_test.hpp
src/tests/compiler/expr/compiler_binary_expr_test.hpp
src/tests/vm/expr/vm_binary_expr_test.hpp
```

##### 类名对应关系
```cpp
// Parser 模块
namespace Lua::Tests::Parser::Expr {
    class ParserBinaryExprTest { /* ... */ };  // 文件: parser_binary_expr_test.hpp
    class ParserLiteralExprTest { /* ... */ }; // 文件: parser_literal_expr_test.hpp
}

// Compiler 模块
namespace Lua::Tests::Compiler::Expr {
    class CompilerBinaryExprTest { /* ... */ }; // 文件: compiler_binary_expr_test.hpp
    class CompilerLiteralExprTest { /* ... */ }; // 文件: compiler_literal_expr_test.hpp
}
```

##### 优势
1. **🎯 唯一性保证**: 确保跨模块文件名不重复
2. **🔍 快速识别**: 文件名即可识别所属模块
3. **🛠️ IDE友好**: 支持更好的文件搜索和导航
4. **📁 组织清晰**: 保持目录结构的同时避免命名冲突
5. **🔧 维护便利**: 重构时减少文件名冲突的风险
6. **📄 头文件分离**: 采用.hpp/.cpp文件对，符合C++最佳实践

##### 头文件/实现文件分离规范

**文件组织方式**:
- **头文件(.hpp)**: 包含类声明、接口定义、内联函数
- **实现文件(.cpp)**: 包含具体的测试实现、测试逻辑

**示例结构**:
```cpp
// parser_binary_expr_test.hpp - 测试类声明
#pragma once
#include "../test_base.hpp"

namespace lua::tests::parser {
    class ParserBinaryExprTest : public TestBase {
    public:
        void run_all_tests() override;
        
    private:
        void test_addition_expression();
        void test_subtraction_expression();
        void test_multiplication_expression();
        void test_division_expression();
        void test_nested_expressions();
        void test_operator_precedence();
        void test_error_handling();
    };
}
```

```cpp
// parser_binary_expr_test.cpp - 测试实现
#include "parser_binary_expr_test.hpp"
#include "../../parser/expression_parser.hpp"

namespace lua::tests::parser {
    
void ParserBinaryExprTest::run_all_tests() {
    test_addition_expression();
    test_subtraction_expression();
    test_multiplication_expression();
    test_division_expression();
    test_nested_expressions();
    test_operator_precedence();
    test_error_handling();
}

void ParserBinaryExprTest::test_addition_expression() {
    // 具体的测试实现
    auto parser = ExpressionParser();
    auto result = parser.parse("1 + 2");
    TEST_ASSERT(result.is_valid(), "Addition expression should parse successfully");
}

// ... 其他测试方法的实现

} // namespace lua::tests::parser
```

**分离的优势**:
1. **🚀 编译效率**: 头文件变更时只需重新编译相关文件
2. **📖 接口清晰**: 头文件提供清晰的测试接口概览
3. **🔧 维护性**: 实现细节与接口分离，便于维护
4. **📦 模块化**: 支持更好的模块化设计
5. **🎯 复用性**: 头文件可以被其他测试文件引用
6. **📏 代码组织**: 符合C++项目的标准组织方式

#### 5. **主测试文件命名规范**（子模块测试协调器）

为了与子测试文件的前缀命名策略保持一致，**子模块的主测试文件**也应采用前缀命名：

##### 命名规则
- **格式**: `test_{主模块名}_{子模块名}.hpp`
- **作用**: 作为子模块内所有测试的协调器和统一入口

##### 示例对比

**传统命名**:
```
src/tests/parser/expr/test_expr.hpp          # 不明确所属主模块
src/tests/compiler/expr/test_expr.hpp        # 可能重名
src/tests/vm/instruction/test_instruction.hpp # 不一致
```

**前缀命名（推荐）**:
```
src/tests/parser/expr/test_parser_expr.hpp
src/tests/compiler/expr/test_compiler_expr.hpp
src/tests/vm/instruction/test_vm_instruction.hpp
```

##### 文件内容结构
```cpp
// test_parser_expr.hpp
#ifndef TEST_PARSER_EXPR_HPP
#define TEST_PARSER_EXPR_HPP

#include "../../../test_framework/core/test_utils.hpp"
#include "parser_binary_expr_test.hpp"
#include "parser_literal_expr_test.hpp"
// ... 其他子测试文件

namespace Lua {
namespace Tests {
namespace Parser {
namespace Expr {

/**
 * @brief Parser Expression Test Suite
 * 
 * Coordinates all parser expression-related tests
 * Namespace: Lua::Tests::Parser::Expr
 */
class ExprTestSuite {
public:
    static void runAllTests() {
        TestUtils::printInfo("Running Parser Expression Tests...");
        
        RUN_TEST_SUITE(Lua::Tests::Parser::Expr::ParserBinaryExprTest);
        RUN_TEST_SUITE(Lua::Tests::Parser::Expr::ParserLiteralExprTest);
        // ... 其他测试套件
        
        TestUtils::printInfo("Parser Expression Tests completed.");
    }
};

} // namespace Expr
} // namespace Parser
} // namespace Tests
} // namespace Lua

#endif // TEST_PARSER_EXPR_HPP
```

##### 命名一致性
- **主测试文件**: `test_parser_expr.hpp`
- **子测试文件**: `parser_binary_expr_test.hpp`, `parser_literal_expr_test.hpp`
- **测试类名**: `ExprTestSuite` (主协调器), `ParserBinaryExprTest` (具体测试)
- **命名空间**: `Lua::Tests::Parser::Expr`

##### 优势
1. **🎯 命名一致性**: 与子测试文件的前缀策略保持一致
2. **🔍 模块识别**: 文件名清晰标识所属的主模块和子模块
3. **🚫 避免重名**: 防止不同模块的主测试文件重名
4. **📁 层次清晰**: 文件名体现了模块的层次结构
5. **🔧 维护友好**: IDE中更容易搜索和识别文件

#### 6. **调用规范**
- **完整调用**: `Lua::Tests::<Module>::<SubModule>::<TestSuite>::runAllTests()`
- **简化调用**: 使用 `using namespace` 或 `using` 声明

### 🚀 优势

1. **🎯 命名清晰**: 通过命名空间明确区分不同模块的测试
2. **🔒 避免冲突**: 消除同名测试套件的命名冲突
3. **📁 结构清晰**: 文件组织与命名空间结构一致
4. **🔧 易于维护**: 模块化的组织便于代码维护
5. **📈 可扩展**: 支持任意层级的子模块扩展
6. **🎨 IDE友好**: 现代IDE能够很好地支持命名空间导航

### 📋 迁移指南

#### 现有代码迁移步骤

1. **添加命名空间**
   ```cpp
   // 迁移前
   class ExprTestSuite { /* ... */ };
   
   // 迁移后
   namespace Lua::Tests::Parser::Expr {
       class ExprTestSuite { /* ... */ };
   }
   ```

2. **更新调用代码**
   ```cpp
   // 迁移前
   ExprTestSuite::runAllTests();
   
   // 迁移后
   Lua::Tests::Parser::Expr::ExprTestSuite::runAllTests();
   ```

3. **更新包含文件**
   ```cpp
   // 确保包含路径正确
   #include "parser/expr/test_expr.hpp"
   ```

#### 新代码开发规范

1. **确定模块归属**: 明确测试属于哪个模块和子模块
2. **选择合适的命名空间**: 遵循项目的命名空间层级
3. **保持一致性**: 与同模块的其他测试保持命名风格一致
4. **文档注释**: 在类注释中明确标注命名空间信息

### 🔍 最佳实践

1. **命名空间注释**: 在每个命名空间结束处添加注释
   ```cpp
   } // namespace Expr
   } // namespace Parser
   } // namespace Tests
   } // namespace Lua
   ```

2. **using声明**: 在需要频繁调用时使用using声明
   ```cpp
   using ParserExprTests = Lua::Tests::Parser::Expr;
   ParserExprTests::ExprTestSuite::runAllTests();
   ```

3. **文档说明**: 在类文档中明确标注命名空间
   ```cpp
   /**
    * @brief Expression Test Suite for Parser Module
    * @namespace Lua::Tests::Parser::Expr
    */
   class ExprTestSuite { /* ... */ };
   ```

4. **一致性检查**: 定期检查命名空间使用的一致性

通过采用命名空间隔离方案，我们能够构建一个清晰、可维护、可扩展的测试体系，有效避免命名冲突，提高代码的可读性和维护性。

## 总结

通过遵循这个指南，你可以：

1. **快速添加新测试**: 使用标准化的模板和结构
2. **保持代码一致性**: 遵循统一的命名和组织规范
3. **便于维护**: 清晰的模块化结构
4. **灵活运行**: 支持全量测试、模块测试和单个测试
5. **易于调试**: 清晰的输出格式和错误处理
6. **避免命名冲突**: 通过命名空间隔离确保测试套件的唯一性
7. **提高可维护性**: 层级化的命名空间结构便于代码组织和维护

记住：好的测试不仅验证功能正确性，还能作为代码的活文档，帮助其他开发者理解系统的行为和预期。通过合理的命名空间设计，我们能够构建一个更加清晰、可维护的测试体系。

---

## 📚 附录

### 🔧 工具支持

#### 自动化脚本

**测试文件生成器**
```bash
#!/bin/bash
# generate_test.sh - 快速生成测试文件模板

MODULE=$1
FEATURE=$2

if [ -z "$MODULE" ] || [ -z "$FEATURE" ]; then
    echo "Usage: $0 <module> <feature>"
    echo "Example: $0 parser binary_expr"
    exit 1
fi

# 创建测试文件
cat > "${MODULE}_${FEATURE}_test.cpp" << EOF
#pragma once
#include "../test_base.hpp"

namespace lua::tests::${MODULE} {
    class ${MODULE^}${FEATURE^}Test : public TestBase {
    public:
        void run_all_tests() override;
    private:
        void test_basic_functionality();
        void test_edge_cases();
        void test_error_handling();
    };
}
EOF

echo "Generated ${MODULE}_${FEATURE}_test.cpp"
```

**规范检查工具**
```python
#!/usr/bin/env python3
# check_naming.py - 检查命名规范

import os
import re
from pathlib import Path

def check_test_files(directory):
    """检查测试文件命名规范"""
    issues = []
    
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith(('.hpp', '.cpp')):
                if not check_naming_convention(file, root):
                    issues.append(f"{root}/{file}")
    
    return issues

def check_naming_convention(filename, path):
    """检查单个文件的命名规范"""
    # 主测试文件规范
    if filename.startswith('test_'):
        return re.match(r'test_[a-z_]+\.hpp$', filename)
    
    # 子模块测试文件规范
    if filename.endswith('_test.cpp'):
        return re.match(r'[a-z]+_[a-z_]+_test\.cpp$', filename)
    
    return True

if __name__ == "__main__":
    issues = check_test_files("src/tests")
    if issues:
        print("命名规范问题:")
        for issue in issues:
            print(f"  - {issue}")
    else:
        print("✅ 所有文件都符合命名规范")
```

#### IDE 配置

**VS Code 配置**
```json
// .vscode/settings.json
{
    "files.associations": {
        "*test*.hpp": "cpp",
        "*test*.cpp": "cpp"
    },
    "C_Cpp.default.includePath": [
        "${workspaceFolder}/src",
        "${workspaceFolder}/src/tests"
    ],
    "editor.rulers": [80, 120],
    "files.trimTrailingWhitespace": true
}
```

**代码片段**
```json
// .vscode/snippets/cpp.json
{
    "Test Class Template": {
        "prefix": "testclass",
        "body": [
            "#pragma once",
            "#include \"../test_base.hpp\"",
            "",
            "namespace lua::tests::${1:module} {",
            "    class ${2:Module}${3:Feature}Test : public TestBase {",
            "    public:",
            "        void run_all_tests() override;",
            "    private:",
            "        void test_${4:basic_functionality}();",
            "        void test_edge_cases();",
            "        void test_error_handling();",
            "    };",
            "}"
        ],
        "description": "Create a test class template"
    }
}
```

### 📈 版本历史

| 版本 | 日期 | 更新内容 | 贡献者 |
|------|------|----------|--------|
| **v2.1** | 2024-12 | 文档结构优化、快速导航、FAQ | 团队 |
| **v2.0** | 2024-11 | 主测试文件命名规范、工具支持 | 团队 |
| **v1.5** | 2024-10 | 子模块测试规范、前缀命名策略 | 团队 |
| **v1.0** | 2024-09 | 基础测试框架、模块化结构 | 团队 |

### 🤝 贡献指南

#### 如何贡献

1. **报告问题**
   - 在 GitHub Issues 中报告 bug
   - 提供详细的重现步骤
   - 包含相关的错误信息

2. **提出改进建议**
   - 在 Issues 中提出功能请求
   - 描述期望的行为和用例
   - 考虑向后兼容性

3. **提交代码**
   - Fork 项目并创建特性分支
   - 遵循现有的代码规范
   - 添加相应的测试用例
   - 更新相关文档

#### 代码审查清单

- [ ] 遵循命名规范
- [ ] 包含适当的测试
- [ ] 更新相关文档
- [ ] 通过所有现有测试
- [ ] 代码风格一致
- [ ] 性能影响评估

### 📞 支持与联系

- **项目主页**: [GitHub Repository](https://github.com/YanqingXu/lua)
- **文档**: [在线文档](https://your-org.github.io/lua/docs)
- **问题反馈**: [GitHub Issues](https://github.com/YanqingXu/lua/issues)
- **讨论**: [GitHub Discussions](https://github.com/YanqingXu/lua/discussions)

### 📄 许可证

本项目采用 [MIT License](LICENSE) 开源许可证。

---

**最后更新**: 2025年6月 | **文档版本**: v2.1 | **维护者**: YanqingXu
