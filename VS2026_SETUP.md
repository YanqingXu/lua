# Visual Studio 2026 手动编译指南

本文档说明如何在 Visual Studio 2026 中手动编译和运行 Lua C++ 解释器的测试程序。

## 方法1：使用构建脚本（推荐）

最简单的方法是使用提供的构建脚本：

```powershell
# 进入项目目录
cd lua

# 编译并运行main.cpp（Debug版本）
.\build_main.bat debug

# 编译并运行main.cpp（Release版本）
.\build_main.bat release
```

这个脚本会自动编译所有必要的文件并运行测试。

## 方法2：在Visual Studio IDE中手动配置

如果你想在Visual Studio IDE中进行开发和调试，请按照以下步骤操作：

### 步骤1：创建新项目

1. 打开 Visual Studio 2026
2. 选择 "创建新项目"
3. 选择 "空项目" 或 "控制台应用"
4. 项目名称：`LuaInterpreter`
5. 位置：选择 `lua/` 目录的父目录

### 步骤2：配置项目属性

1. 右键点击项目 → 属性
2. 配置属性 → C/C++ → 常规 → 附加包含目录，添加：
   ```
   $(ProjectDir)src
   $(ProjectDir)tests\unit
   ```
3. 配置属性 → C/C++ → 语言 → C++语言标准，选择：
   ```
   ISO C++17 标准 (/std:c++17)
   ```
4. 配置属性 → C/C++ → 预处理器 → 预处理器定义，添加：
   ```
   _CRT_SECURE_NO_WARNINGS
   ```

### 步骤3：添加源文件

将以下文件添加到项目中：

#### 核心类（src/core/）
- `value.cpp`
- `gc_object.cpp`
- `gc_string.cpp`
- `string_pool.cpp`
- `table.cpp`
- `function.cpp`
- `userdata.cpp`
- `upvalue.cpp`

#### 垃圾回收（src/gc/）
- `garbage_collector.cpp`

#### 虚拟机（src/vm/）
- `global_state.cpp`
- `stack.cpp`
- `lua_state.cpp`
- `vm.cpp`

#### 编译器（src/compiler/）
- `lexer.cpp`
- `ast.cpp`
- `parser.cpp`
- `opcode.cpp`
- `codegen.cpp`

#### 标准库（src/lib/）
- `baselib.cpp`

#### 测试文件（tests/unit/）
- `test_value.cpp`
- `test_gc_string.cpp`
- `test_table.cpp`
- `test_vm_core.cpp`
- `test_function.cpp`
- `test_gc.cpp`

#### 主程序
- `src/main.cpp`

### 步骤4：编译和运行

1. 选择配置：Debug 或 Release
2. 按 F7 编译项目
3. 按 F5 运行（带调试）或 Ctrl+F5 运行（不带调试）

## 代码结构说明

### main.cpp

`src/main.cpp` 是主程序入口，它：
- 包含测试框架（`test_framework.hpp`）
- 调用所有测试注册函数
- 运行所有单元测试
- 输出测试报告

**重要**：`main.cpp` 现在复用 `tests/unit/` 目录下的测试框架和测试用例，避免代码重复。

### 测试框架

测试框架位于 `tests/unit/` 目录：
- `test_framework.hpp` - 测试框架核心（TestSuite、TestRegistry、断言宏）
- `test_registry.hpp` - 测试注册函数声明
- `test_*.cpp` - 各个模块的测试实现

### 相对路径

`main.cpp` 使用相对路径包含测试框架：
```cpp
#include "../tests/unit/test_framework.hpp"
#include "../tests/unit/test_registry.hpp"
```

这就是为什么需要在项目属性中添加 `tests\unit` 到包含目录。

## 常见问题

### Q: 编译时找不到头文件

**A**: 确保在项目属性中正确配置了包含目录：
- `$(ProjectDir)src`
- `$(ProjectDir)tests\unit`

### Q: 链接错误：无法解析的外部符号

**A**: 确保所有必要的 `.cpp` 文件都已添加到项目中，特别是：
- 所有 `src/core/*.cpp` 文件
- 所有 `tests/unit/test_*.cpp` 文件

### Q: 运行时崩溃

**A**: 
1. 确保使用 Debug 配置进行调试
2. 检查是否所有测试文件都正确编译和链接
3. 使用 F5（带调试）运行，查看崩溃位置

## 输出示例

成功运行后，你应该看到类似以下的输出：

```
========================================
Lua C++ Interpreter - Unit Test Suite
========================================
Test Framework: Custom Lightweight Framework
Build: Visual Studio 2026 Manual Compilation
Date: 2025-11-14
========================================

[INFO] Registering tests...
[INFO] All tests registered.
[INFO] Starting test execution...

========================================
Test Suite: Value
========================================
  [PASS] Nil value creation
  [PASS] Boolean value creation
  ...
----------------------------------------
Total: 16 | Pass: 16 | Fail: 0
========================================

...

========================================
Test Summary
========================================
Total Tests: 111
Passed: 111
Failed: 0
========================================

✓ ALL TESTS PASSED!
```

## 下一步

成功编译和运行测试后，你可以：
1. 在 `src/main.cpp` 中设置断点进行调试
2. 修改测试文件添加新的测试用例
3. 开始实现新的功能模块

