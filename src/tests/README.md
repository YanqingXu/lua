# 测试文件组织结构

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
├── lib/                        # 标准库测试 (预留)
└── integration/                # 集成测试 (预留)
```

## 模块说明

### lexer/ - 词法分析器测试
测试词法分析器的功能，包括：
- Token 识别和分类
- 词法错误处理
- 特殊字符和关键字处理

### parser/ - 语法分析器测试
测试语法分析器的功能，包括：
- **test_parser.hpp/.cpp**: 语法分析器测试统一入口，调用所有解析器相关测试
- **parser_test**: 基础语法分析和 AST 构建测试
- **function_test**: 函数定义和参数解析测试
- **if_statement_test**: 条件语句（if-then-else）解析测试
- **forin_test**: for-in 循环语句解析测试
- **repeat_test**: repeat-until 循环语句解析测试

语法分析器测试覆盖了从基础表达式到复杂语言结构的解析，确保 AST 能够正确构建。

### compiler/ - 编译器测试
测试编译器的功能，包括：
- **test_compiler.hpp/.cpp**: 编译器测试统一入口，调用所有编译器相关测试
- **symbol_table_test**: 符号表管理和作用域处理
- **literal_compiler_test**: 字面量（nil、boolean、number、string）编译
- **variable_compiler_test**: 变量访问和解析编译
- **binary_expression_test**: 二元表达式（算术、比较、逻辑）编译
- **expression_compiler_test**: 复杂表达式编译器
- **conditional_compilation_test**: 条件语句和短路逻辑编译

编译器测试按功能模块组织，从基础的符号表到复杂的表达式编译，确保编译过程的每个环节都能正确工作。

### vm/ - 虚拟机测试
测试虚拟机核心功能，包括：
- **test_vm.hpp/.cpp**: 虚拟机测试统一入口，调用所有虚拟机相关测试
- **value_test**: 值系统测试（nil、boolean、number、string、table 等）
- **state_test**: 状态管理测试（全局变量、栈操作、函数调用等）

虚拟机测试专注于运行时环境的正确性，包括值的表示、状态管理和执行环境。

### gc/ - 垃圾回收器测试
测试垃圾回收器功能，包括：
- **test_gc.hpp/.cpp**: GC 测试统一入口，调用所有垃圾回收器相关测试
- **string_pool_demo_test**: 字符串池演示和基本功能测试
- **gc_integration_test**: GC 与核心类型的集成测试，复杂引用模式测试

GC 测试模块专注于内存管理的正确性，包括对象标记、清除、字符串池管理等关键功能。

### lib/ - 标准库测试 (预留)
未来用于测试 Lua 标准库实现

### integration/ - 集成测试 (预留)
未来用于端到端的集成测试

## 使用方法

### 运行所有测试
```cpp
#include "test_main.hpp"
Lua::Tests::runAllTests();
```

### 运行特定模块测试
```cpp
// 运行所有编译器测试（推荐）
#include "compiler/test_compiler.hpp"
CompilerTest::runAllTests();

// 运行所有 GC 测试（推荐）
#include "gc/test_gc.hpp"
GCTest::runAllTests();

// 运行所有语法分析器测试（推荐）
#include "parser/test_parser.hpp"
ParserTestSuite::runAllTests();

// 运行所有虚拟机测试（推荐）
#include "vm/test_vm.hpp"
VMTestSuite::runAllTests();

// 运行单个编译器测试
#include "compiler/conditional_compilation_test.hpp"
ConditionalCompilationTest::runAllTests();

// 运行单个 GC 测试
#include "gc/gc_integration_test.hpp"
GCIntegrationTest::runAllTests();
```

## 测试命名规范

1. **测试类命名**: `<Module><Function>Test`
   - 例如: `ConditionalCompilationTest`, `GCIntegrationTest`

2. **测试方法命名**: `test<SpecificFeature>()`
   - 例如: `testSimpleIfStatement()`, `testBasicStringInterning()`

3. **主入口方法**: `runAllTests()`
   - 每个测试类都应该有这个静态方法

## 文件组织原则

1. **按功能模块分类**: 每个目录对应解释器的一个主要模块
2. **头文件和源文件配对**: `.hpp` 和 `.cpp` 文件放在同一目录
3. **测试类结构统一**: 使用静态类和 `runAllTests()` 方法
4. **清晰的依赖关系**: 通过目录结构体现模块间的关系

这种组织方式使得测试代码更加模块化、易于维护和扩展。

### 编译器测试模块详细说明

编译器测试模块现在提供了统一的测试入口 `CompilerTest`，它按逻辑顺序执行所有编译器相关测试：

```cpp
// 完整的编译器测试流程
#include "compiler/test_compiler.hpp"

// 这将按以下顺序执行所有编译器测试：
// 1. SymbolTableTest - 符号表和作用域管理
// 2. LiteralCompilerTest - 字面量编译
// 3. VariableCompilerTest - 变量访问编译  
// 4. BinaryExpressionTest - 二元表达式编译
// 5. ExpressionCompilerTest - 复杂表达式编译
// 6. ConditionalCompilationTest - 条件语句编译
CompilerTest::runAllTests();
```

每个子测试都可以单独运行，用于调试特定的编译器功能。

### GC 测试模块详细说明

GC 测试模块现在提供了统一的测试入口 `GCTest`，它按逻辑顺序执行所有垃圾回收器相关测试：

```cpp
// 完整的 GC 测试流程
#include "gc/test_gc.hpp"

// 这将按以下顺序执行所有 GC 测试：
// 1. StringPoolDemoTest - 字符串池演示和基本功能
// 2. GCIntegrationTest - GC 与核心类型集成，复杂引用模式测试
GCTest::runAllTests();
```

每个子测试都可以单独运行，用于调试特定的 GC 功能。

### Parser 测试模块详细说明

Parser 测试模块现在提供了统一的测试入口 `ParserTestSuite`，它按逻辑顺序执行所有语法分析器相关测试：

```cpp
// 完整的 Parser 测试流程
#include "parser/test_parser.hpp"

// 这将按以下顺序执行所有 Parser 测试：
// 1. ParserTest - 基础语法分析和 AST 构建
// 2. FunctionTest - 函数定义和参数解析
// 3. IfStatementTest - 条件语句解析
// 4. ForInTest - for-in 循环解析
// 5. RepeatTest - repeat-until 循环解析
ParserTestSuite::runAllTests();
```

### VM 测试模块详细说明

VM 测试模块现在提供了统一的测试入口 `VMTestSuite`，它按逻辑顺序执行所有虚拟机相关测试：

```cpp
// 完整的 VM 测试流程
#include "vm/test_vm.hpp"

// 这将按以下顺序执行所有 VM 测试：
// 1. ValueTest - 值系统测试（各种数据类型）
// 2. StateTest - 状态管理测试（全局变量、栈操作、函数调用）
VMTestSuite::runAllTests();
```

### 测试模块组织总结

项目现在采用了完全分层的测试组织结构：

#### 主层级 - 完整测试套件
- **主测试入口**: `test_main.hpp/.cpp` - 运行所有测试模块

#### 模块层级 - 各功能模块统一入口
1. **词法分析器**: `lexer/lexer_test.hpp/.cpp` - 词法分析功能测试
2. **语法分析器**: `parser/test_parser.hpp/.cpp` - 所有语法分析相关测试
3. **编译器**: `compiler/test_compiler.hpp/.cpp` - 所有编译器相关测试
4. **虚拟机**: `vm/test_vm.hpp/.cpp` - 所有虚拟机相关测试
5. **垃圾回收器**: `gc/test_gc.hpp/.cpp` - 所有垃圾回收器相关测试

#### 子模块层级 - 具体功能测试
每个模块下包含多个具体的测试类，负责测试特定功能。

#### 测试执行层次
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
└── GCTest::runAllTests()
    ├── StringPoolDemoTest::runAllTests()
    └── GCIntegrationTest::runAllTests()
```

这种结构提供了极大的灵活性：
- ✅ **完整测试**: 一次运行所有测试
- ✅ **模块测试**: 针对特定模块的测试
- ✅ **功能测试**: 针对具体功能的精确测试
- ✅ **易于维护**: 清晰的组织结构和统一的接口
- ✅ **易于扩展**: 新测试可以轻松添加到对应模块

### 构建和运行说明

**注意**: 当前项目有两个测试目录：

1. **`tests/`** - 基于 Google Test 的原始测试文件，在 CMakeLists.txt 中已配置
2. **`src/tests/`** - 新组织的模块化测试文件（本文档描述的结构）

要使用新的测试组织结构，你可以：

#### 方法 1: 直接在代码中调用
```cpp
#include "src/tests/test_main.hpp"

int main() {
    Lua::Tests::runAllTests();
    return 0;
}
```

#### 方法 2: 编译为独立可执行文件
```bash
# 在项目根目录执行
g++ -std=c++17 -I src src/tests/test_organization_demo.cpp -o test_demo
./test_demo
```

#### 方法 3: 集成到现有 CMakeLists.txt 
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
