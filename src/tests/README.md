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
│   ├── state_test.cpp          # 状态管理测试
│   ├── state_test.hpp
│   ├── value_test.cpp          # 值系统测试
│   └── value_test.hpp
├── gc/                         # 垃圾回收器测试
│   ├── gc_integration_test.cpp         # GC 集成测试
│   ├── gc_integration_test.hpp
│   ├── string_pool_demo_test.cpp       # 字符串池演示测试
│   ├── string_pool_demo_test.hpp
│   └── string_pool_test.cpp            # 字符串池测试
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
- AST 构建
- 语法结构解析（循环、函数、条件语句等）
- 语法错误恢复

### compiler/ - 编译器测试
测试编译器的功能，包括：
- 表达式编译
- 语句编译
- 符号表管理
- 代码生成和优化

### vm/ - 虚拟机测试
测试虚拟机核心功能，包括：
- 值系统
- 状态管理
- 栈操作
- 指令执行

### gc/ - 垃圾回收器测试
测试垃圾回收器功能，包括：
- 内存管理
- 对象标记和清除
- 字符串池管理
- GC 集成测试

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
// 运行编译器测试
#include "compiler/conditional_compilation_test.hpp"
ConditionalCompilationTest::runAllTests();

// 运行 GC 测试
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
