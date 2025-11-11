# Lua 解释器开发指南

> **面向**: 项目开发者和贡献者
> 
> **目的**: 提供统一的开发规范和最佳实践

---

## 📋 目录

- [开发环境设置](#开发环境设置)
- [编码规范](#编码规范)
- [开发流程](#开发流程)
- [测试指南](#测试指南)
- [调试技巧](#调试技巧)

---

## 🛠️ 开发环境设置

### 必需工具

| 工具 | 版本要求 | 用途 |
|------|---------|------|
| C++编译器 | GCC 9+, Clang 10+, MSVC 2019+ | 编译C++17代码 |
| CMake | 3.15+ | 构建系统 |
| Git | 2.0+ | 版本控制 |

### 推荐工具

| 工具 | 用途 |
|------|------|
| Visual Studio Code | 代码编辑器 |
| CLion / Visual Studio | IDE |
| clang-format | 代码格式化 |
| clang-tidy | 静态分析 |
| Valgrind | 内存检查（Linux） |
| Dr. Memory | 内存检查（Windows） |

### 环境配置

#### Windows (Visual Studio)

```powershell
# 克隆仓库
git clone <repository-url>
cd lua

# 生成Visual Studio项目
cmake -B build -G "Visual Studio 16 2019"

# 打开解决方案
start build/lua.sln
```

#### Linux / macOS

```bash
# 克隆仓库
git clone <repository-url>
cd lua

# 生成Makefile
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# 编译
cmake --build build

# 运行测试
cd build && ctest
```

---

## 📝 编码规范

### 命名约定

#### 文件命名

- **头文件**: `snake_case.hpp`
- **源文件**: `snake_case.cpp`
- **测试文件**: `test_<module>.cpp`

示例:
```
src/core/value.hpp
src/core/value.cpp
tests/core/test_value.cpp
```

#### 代码命名

```cpp
namespace Lua {
    // 类名: PascalCase
    class GarbageCollector { };
    class LuaState { };
    
    // 函数名: camelCase
    void markObject(GCObject* obj);
    Value getValue(int index);
    
    // 变量名: camelCase
    int stackSize;
    GCObject* currentObject;
    
    // 成员变量: camelCase + 下划线后缀
    class Example {
    private:
        int value_;
        std::string name_;
    };
    
    // 常量: UPPER_CASE
    constexpr int MAX_STACK_SIZE = 1000;
    const char* LUA_VERSION = "5.1.5";
    
    // 枚举: PascalCase (类型) + PascalCase (值)
    enum class ValueType {
        Nil,
        Boolean,
        Number
    };
    
    // 类型别名: PascalCase 或 snake_case
    using LuaNumber = double;
    using u8 = uint8_t;
}
```

### 代码风格

#### 缩进和空格

```cpp
// 使用4个空格缩进，不使用Tab
class Example {
public:
    void function() {
        if (condition) {
            // 代码
        }
    }
};
```

#### 大括号风格

```cpp
// 函数: 大括号另起一行
void function()
{
    // 代码
}

// 类/结构体: 大括号另起一行
class Example
{
    // 成员
};

// 控制流: 大括号跟随
if (condition) {
    // 代码
} else {
    // 代码
}

// 单行if可以省略大括号，但建议保留
if (condition) {
    doSomething();
}
```

#### 注释风格

```cpp
/**
 * @brief 函数简要描述
 * 
 * 详细描述函数的功能、算法、注意事项等。
 * 
 * @param param1 参数1的描述
 * @param param2 参数2的描述
 * @return 返回值的描述
 * 
 * @note 特别注意事项
 * @warning 警告信息
 * 
 * @see 相关函数或类
 */
ReturnType functionName(Type1 param1, Type2 param2);

// 单行注释用于简短说明
int value; // 变量说明

/* 
 * 多行注释用于临时禁用代码或详细说明
 */
```

### 头文件组织

```cpp
#pragma once  // 推荐使用pragma once

// 1. 标准库头文件
#include <string>
#include <vector>
#include <memory>

// 2. 第三方库头文件
// #include <third_party/lib.hpp>

// 3. 项目内部头文件
#include "common/types.hpp"
#include "core/value.hpp"

namespace Lua {
    // 前向声明
    class GarbageCollector;
    class LuaState;
    
    // 类定义
    class Example {
    public:
        // 公共接口
        
    private:
        // 私有成员
    };
}
```

---

## 🔄 开发流程

### 功能开发流程

#### 1. 需求分析

- 阅读 `lua_c_analysis` 中对应模块的文档
- 理解原始C实现的设计思路
- 参考 `lua_with_cpp` 的C++实现方案

#### 2. 设计阶段

- 编写设计文档（参考 `spec-kit` 的模板）
- 定义接口和数据结构
- 评审设计方案

#### 3. 测试先行

```cpp
// tests/core/test_value.cpp
#include <gtest/gtest.h>
#include "core/value.hpp"

TEST(ValueTest, NilValue) {
    Lua::Value v;
    EXPECT_TRUE(v.isNil());
    EXPECT_EQ(v.type(), Lua::ValueType::Nil);
}

TEST(ValueTest, BooleanValue) {
    Lua::Value v(true);
    EXPECT_TRUE(v.isBoolean());
    EXPECT_EQ(v.asBoolean(), true);
}
```

#### 4. 实现阶段

```cpp
// src/core/value.cpp
#include "value.hpp"

namespace Lua {
    Value::Value() : data_(std::monostate{}) {}
    
    Value::Value(bool b) : data_(b) {}
    
    bool Value::isNil() const noexcept {
        return std::holds_alternative<std::monostate>(data_);
    }
    
    bool Value::isBoolean() const noexcept {
        return std::holds_alternative<bool>(data_);
    }
    
    bool Value::asBoolean() const {
        if (!isBoolean()) {
            throw std::runtime_error("Value is not a boolean");
        }
        return std::get<bool>(data_);
    }
}
```

#### 5. 测试验证

```bash
# 运行测试
cmake --build build --target test_value
./build/tests/test_value

# 运行所有测试
cd build && ctest
```

#### 6. 代码审查

- 自我审查代码
- 运行静态分析工具
- 检查内存泄漏
- 性能测试

---

## 🧪 测试指南

### 测试分类

#### 单元测试

测试单个类或函数的功能。

```cpp
TEST(ValueTest, NumberValue) {
    Lua::Value v(42.0);
    EXPECT_TRUE(v.isNumber());
    EXPECT_DOUBLE_EQ(v.asNumber(), 42.0);
}
```

#### 集成测试

测试多个模块的协作。

```cpp
TEST(IntegrationTest, TableWithGC) {
    auto gc = std::make_unique<Lua::GarbageCollector>();
    auto table = Lua::make_gc_table();
    
    table->set(Lua::Value(1), Lua::Value(100));
    EXPECT_EQ(table->get(Lua::Value(1)).asNumber(), 100);
    
    gc->collect();
    // 验证table仍然有效
}
```

#### 性能测试

测试关键路径的性能。

```cpp
TEST(PerformanceTest, TableInsert) {
    auto table = Lua::make_gc_table();
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100000; ++i) {
        table->set(Lua::Value(i), Lua::Value(i * 2));
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Insert 100k items: " << duration.count() << "ms\n";
}
```

### 测试覆盖率

目标: 代码覆盖率 > 80%

```bash
# 使用gcov生成覆盖率报告
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
cmake --build build
cd build && ctest
gcov -r ../src/core/*.cpp
```

---

## 🐛 调试技巧

### 使用断言

```cpp
#include <cassert>

void setValue(int index, const Value& value) {
    assert(index >= 0 && index < size_);  // 调试版本检查
    data_[index] = value;
}
```

### 日志输出

```cpp
#ifdef DEBUG
    #define LOG_DEBUG(msg) std::cout << "[DEBUG] " << msg << std::endl
#else
    #define LOG_DEBUG(msg)
#endif

void function() {
    LOG_DEBUG("Entering function");
    // 代码
    LOG_DEBUG("Exiting function");
}
```

### 内存检查

#### Linux (Valgrind)

```bash
valgrind --leak-check=full ./build/tests/test_value
```

#### Windows (Dr. Memory)

```powershell
drmemory.exe -- .\build\tests\test_value.exe
```

### GDB调试

```bash
gdb ./build/tests/test_value
(gdb) break value.cpp:42
(gdb) run
(gdb) print value
(gdb) backtrace
```

---

## 📚 参考资源使用

### lua_c_analysis 使用指南

1. **理解原始设计**
   - 阅读 `docs/wiki.md` 了解整体架构
   - 阅读具体模块的文档了解细节

2. **查看源码实现**
   - 查看 `src/*.h` 了解数据结构
   - 查看 `src/*.c` 了解算法实现

3. **参考注释**
   - C源码中有详细的中文注释
   - 理解每个函数的作用和实现细节

### lua_with_cpp 使用指南

1. **参考C++实现模式**
   - 查看如何用C++实现相同功能
   - 学习现代C++特性的使用

2. **避免直接复制**
   - 理解设计思路，不要直接复制代码
   - 根据我们的架构设计进行调整

### spec-kit 使用指南

1. **应用开发方法论**
   - 使用SDD方法论指导开发
   - 参考模板编写文档

2. **质量保证**
   - 应用最佳实践
   - 确保代码质量

---

## ✅ 检查清单

### 提交代码前

- [ ] 代码通过编译，无警告
- [ ] 所有测试通过
- [ ] 代码符合编码规范
- [ ] 添加了必要的注释
- [ ] 更新了相关文档
- [ ] 运行了静态分析工具
- [ ] 检查了内存泄漏
- [ ] 性能测试通过

---

**下一步**: 开始实现第一个模块 - 基础类型系统

