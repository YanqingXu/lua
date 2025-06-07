# Modern C++ Lua Interpreter

This project is a reimplementation of the Lua interpreter using modern C++ techniques and features. The goal is to provide a clean, efficient, and maintainable codebase while preserving the functionality and behavior of the original Lua interpreter.

## Features

- Fully compatible with Lua syntax and semantics.
- Written in modern C++ (C++17 or later).
- Modular and extensible design.
- Improved performance and memory management.
- Comprehensive test suite.

## Getting Started

### Prerequisites

To build and run the project, you will need:

- A C++ compiler that supports C++17 or later (e.g., GCC, Clang, or MSVC).
- CMake (version 3.15 or later).
- A build system (e.g., Make, Ninja, or Visual Studio).

### Building the Project

1. Clone the repository:
   ```bash
   git clone https://github.com/YanqingXu/lua
   cd lua
   ```

2. Create a build directory and navigate to it:
   ```bash
   mkdir build
   cd build
   ```

3. Configure the project with CMake:
   ```bash
   cmake ..
   ```

4. Build the project:
   ```bash
   cmake --build .
   ```

### Running the Interpreter

After building, you can run the Lua interpreter:

```bash
./lua
```

This will start the interactive Lua shell, where you can execute Lua scripts and commands.

### Running Tests

To ensure everything is working correctly, you can run the test suite:

```bash
ctest
```

## Contributing

Contributions are welcome! If you'd like to contribute, please fork the repository, create a feature branch, and submit a pull request. Make sure to follow the project's coding style and include tests for your changes.

## License

This project is licensed under the MIT License. See the [LICENSE](./LICENSE) file for details.

## Parser 模块重构说明

### 文件结构

#### AST 节点定义 (`ast/` 目录)

- **`ast_base.hpp`**: 包含基础的 AST 节点定义
  - `ExprType` 枚举：表达式类型
  - `StmtType` 枚举：语句类型
  - `Expr` 基类：所有表达式的基类
  - `Stmt` 基类：所有语句的基类

- **`expressions.hpp`**: 包含所有表达式类的定义
  - `LiteralExpr`: 字面量表达式
  - `VariableExpr`: 变量表达式
  - `UnaryExpr`: 一元表达式
  - `BinaryExpr`: 二元表达式
  - `CallExpr`: 函数调用表达式
  - `MemberExpr`: 成员访问表达式
  - `TableExpr`: 表构造表达式
  - `IndexExpr`: 索引访问表达式
  - `FunctionExpr`: 函数表达式
  - `TableField`: 表字段结构体

- **`statements.hpp`**: 包含所有语句类的定义
  - `ExprStmt`: 表达式语句
  - `BlockStmt`: 块语句
  - `LocalStmt`: 局部变量声明语句
  - `AssignStmt`: 赋值语句
  - `IfStmt`: if 语句
  - `WhileStmt`: while 语句
  - `ReturnStmt`: return 语句
  - `BreakStmt`: break 语句

#### Parser 实现文件

- **`parser_utils.cpp`**: Parser 类的辅助方法
  - 构造函数
  - `advance()`, `check()`, `match()`, `consume()` 等辅助方法
  - `error()`, `synchronize()`, `isAtEnd()` 错误处理方法
  - `parse()` 主解析方法
  - `isValidAssignmentTarget()` 赋值目标验证

- **`expression_parser.cpp`**: 表达式解析方法
  - `expression()`: 主表达式解析入口
  - `logicalOr()`, `logicalAnd()`: 逻辑运算符解析
  - `equality()`, `comparison()`: 比较运算符解析
  - `concatenation()`: 字符串连接解析
  - `simpleExpression()`, `term()`: 算术运算符解析
  - `unary()`: 一元运算符解析
  - `power()`: 幂运算和后缀表达式解析
  - `primary()`: 基础表达式解析
  - `finishCall()`: 函数调用解析
  - `tableConstructor()`: 表构造解析
  - `functionExpression()`: 函数表达式解析

- **`statement_parser.cpp`**: 语句解析方法
  - `statement()`: 主语句解析入口
  - `expressionStatement()`: 表达式语句解析
  - `localDeclaration()`: 局部变量声明解析
  - `assignmentStatement()`: 赋值语句解析
  - `ifStatement()`: if 语句解析
  - `whileStatement()`: while 语句解析
  - `blockStatement()`: 块语句解析
  - `returnStatement()`: return 语句解析
  - `breakStatement()`: break 语句解析

#### 新的头文件

- **`parser_new.hpp`**: 新的 Parser 头文件
  - 包含所有 AST 节点定义
  - Parser 类声明
  - 保持与原有接口的兼容性

### 优势

#### 1. 代码组织更清晰
- AST 节点定义按功能分类到不同文件
- Parser 实现按功能模块分离
- 每个文件职责单一，便于理解和维护

#### 2. 编译效率提升
- 修改单个 AST 节点类型时，只需重新编译相关文件
- 修改解析逻辑时，只需重新编译对应的解析器文件
- 减少了不必要的重新编译

#### 3. 团队协作友好
- 不同开发者可以同时修改不同的解析器模块
- 减少代码冲突的可能性
- 便于代码审查和测试

#### 4. 可扩展性增强
- 添加新的 AST 节点类型时，只需修改对应的头文件
- 添加新的解析功能时，可以创建新的解析器文件
- 便于实现插件化的解析器扩展

#### 5. 向后兼容
- 保持了原有的 Parser 类接口
- 现有代码无需修改即可使用新的文件结构
- 可以逐步迁移到新的文件结构

### 使用方法

#### 包含新的头文件
```cpp
#include "parser/parser_new.hpp"  // 替代原来的 parser.hpp
```

#### 编译时包含新的源文件
在 CMakeLists.txt 或构建脚本中添加：
```cmake
set(PARSER_SOURCES
    src/parser/parser_utils.cpp
    src/parser/expression_parser.cpp
    src/parser/statement_parser.cpp
)
```

### 迁移建议

1. **逐步迁移**: 可以先使用 `parser_new.hpp` 替代 `parser.hpp`
2. **测试验证**: 确保所有现有测试用例通过
3. **更新构建脚本**: 添加新的源文件到构建配置中
4. **文档更新**: 更新相关的开发文档和注释

### 注意事项

- 新的文件结构需要更新构建配置
- 确保所有依赖关系正确设置
- 在迁移过程中保持原有文件的备份
- 建议在独立分支中进行迁移和测试

## Acknowledgments

This project is inspired by the original Lua interpreter. Special thanks to the Lua team for their work on the Lua programming language.
