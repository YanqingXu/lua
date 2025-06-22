# 测试框架迁移指南

## 概述

本指南将帮助您从旧的测试结构迁移到新的模块化测试框架。新框架提供了更好的职责分离、可维护性和可扩展性。

## 主要变化

### 1. 目录结构变化

**旧结构:**
```
src/tests/
├── test_main.hpp          # 主测试入口
├── test_utils.hpp          # 测试工具
├── formatting/             # 格式化模块
├── check_naming.py         # 命名检查工具
└── [各模块测试]/
```

**新结构:**
```
src/
├── test_framework/         # 独立的测试框架
│   ├── core/              # 核心框架组件
│   ├── formatting/        # 格式化模块
│   └── tools/             # 测试工具
└── tests/                 # 纯测试代码
    ├── test_main_new.hpp  # 新的主测试入口
    └── [各模块测试]/
```

### 2. 命名空间变化

**旧命名空间:**
```cpp
namespace Lua {
namespace Tests {
    class TestUtils { ... };
}
}
```

**新命名空间:**
```cpp
namespace Lua {
namespace TestFramework {
    class TestUtils { ... };
}
namespace Tests {
    // 具体的测试代码
}
}
```

### 3. 包含路径变化

**旧包含方式:**
```cpp
#include "test_utils.hpp"
#include "formatting/test_formatter.hpp"
```

**新包含方式:**
```cpp
#include "../test_framework/core/test_utils.hpp"
#include "../test_framework/formatting/test_formatter.hpp"
```

## 迁移步骤

### 步骤 1: 更新包含路径

在您的测试文件中，将旧的包含路径替换为新的路径：

```cpp
// 旧的
#include "test_utils.hpp"

// 新的
#include "../test_framework/core/test_utils.hpp"
```

### 步骤 2: 更新命名空间

```cpp
// 旧的
using namespace Lua::Tests;
TestUtils::printInfo("message");

// 新的
using namespace Lua::TestFramework;
TestUtils::printInfo("message");
```

### 步骤 3: 使用新的测试入口

```cpp
// 旧的
#include "test_main.hpp"
void runTests() {
    Lua::Tests::runAllTests();
}

// 新的
#include "test_main_new.hpp"
void runTests() {
    Lua::Tests::runAllTests(); // 使用新框架的实现
}
```

### 步骤 4: 更新测试宏使用

测试宏的使用方式保持不变，但现在它们来自新的框架：

```cpp
// 这些宏的使用方式不变
RUN_TEST_MODULE("Parser Module", ParserTestSuite);
RUN_TEST_SUITE(ExprTestSuite);
RUN_TEST_GROUP("Binary Expression Tests", testBinaryExpressions);
RUN_TEST(BinaryExprTest, testAddition);
```

## 向后兼容性

为了确保平滑迁移，我们提供了向后兼容性支持：

1. **旧的 `test_utils.hpp`** 现在作为包装器，重定向到新的框架
2. **旧的命名空间** 通过别名继续可用
3. **旧的测试宏** 继续工作

## 新功能

新的测试框架提供了以下增强功能：

### 1. 改进的内存检测

```cpp
// 自动内存泄漏检测
MEMORY_LEAK_TEST_GUARD("TestName");

// 带超时的内存检测
MEMORY_LEAK_TEST_GUARD_WITH_TIMEOUT("TestName", 5000);

// 条件内存检测
CONDITIONAL_MEMORY_LEAK_TEST_GUARD(condition, "TestName");
```

### 2. 测试统计

```cpp
// 获取测试统计信息
auto stats = TestUtils::getStatistics();
std::cout << "Pass rate: " << stats.getPassRate() << "%" << std::endl;

// 打印统计报告
TestUtils::printStatisticsReport();
```

### 3. 模块化测试运行

```cpp
// 运行特定模块
runModuleTests("parser");

// 运行快速测试
runQuickTests();
```

### 4. 改进的配置

```cpp
// 配置颜色输出
TestUtils::setColorEnabled(true);

// 配置内存检测
TestUtils::setMemoryCheckEnabled(true);

// 设置主题
TestUtils::setTheme("dark");
```

## 最佳实践

### 1. 逐步迁移

建议逐个模块进行迁移，而不是一次性迁移所有代码：

1. 先迁移一个小模块（如 lexer）
2. 测试确保功能正常
3. 逐步迁移其他模块

### 2. 利用新功能

在迁移过程中，考虑使用新框架的增强功能：

- 使用内存检测宏确保测试的内存安全
- 利用统计功能监控测试质量
- 使用模块化运行功能提高开发效率

### 3. 保持测试代码简洁

新框架鼓励将测试逻辑与框架代码分离：

- 测试类应该专注于测试逻辑
- 框架相关的配置应该在测试入口点进行
- 避免在测试代码中直接操作框架内部

## 故障排除

### 常见问题

1. **编译错误：找不到头文件**
   - 检查包含路径是否正确
   - 确保新的测试框架文件已正确放置

2. **链接错误：未定义的符号**
   - 确保包含了必要的实现文件
   - 检查命名空间是否正确

3. **运行时错误：测试失败**
   - 检查测试逻辑是否需要适配新框架
   - 确认内存检测设置是否合适

### 调试技巧

1. **启用详细输出**
   ```cpp
   TestUtils::setVerbose(true);
   ```

2. **禁用内存检测进行调试**
   ```cpp
   TestUtils::setMemoryCheckEnabled(false);
   ```

3. **使用单模块测试**
   ```cpp
   runModuleTests("problematic_module");
   ```

## 支持

如果在迁移过程中遇到问题，请：

1. 查看本指南的故障排除部分
2. 检查新框架的文档和示例
3. 在项目中创建 issue 报告问题

## 未来计划

- 在所有模块成功迁移后，旧的兼容性代码将被标记为废弃
- 计划在下一个主要版本中移除向后兼容性支持
- 持续改进测试框架的功能和性能