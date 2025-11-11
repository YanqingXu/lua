# Lua 解释器开发指南

> **面向**: 项目开发者和贡献者
> 
> **目的**: 提供统一的开发规范和最佳实践

---

## 📋 目录

- [开发环境设置](#开发环境设置)
- [编码规范](#编码规范)
- [类型系统使用规范](#类型系统使用规范)
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

## 🎯 类型系统使用规范

### 强制使用类型别名

**重要规则**: 项目中的所有代码**必须**使用 `src/common/types.hpp` 中定义的类型别名，**禁止**直接使用原始类型。

#### 为什么要使用类型别名？

1. **代码一致性**: 统一的类型命名风格，提高代码可读性
2. **类型安全**: 明确的类型语义，避免混淆
3. **易于维护**: 如需修改底层类型实现，只需修改 `types.hpp` 一处
4. **简洁性**: 更短的类型名称，减少代码冗余
5. **可移植性**: 便于跨平台适配

### 常用类型映射表

#### 基础类型

| ❌ 禁止使用 | ✅ 必须使用 | 说明 |
|------------|-----------|------|
| `int8_t` | `i8` | 8位有符号整数 |
| `int16_t` | `i16` | 16位有符号整数 |
| `int32_t` | `i32` | 32位有符号整数 |
| `int64_t` | `i64` | 64位有符号整数 |
| `uint8_t` | `u8` | 8位无符号整数 |
| `uint16_t` | `u16` | 16位无符号整数 |
| `uint32_t` | `u32` | 32位无符号整数 |
| `uint64_t` | `u64` | 64位无符号整数 |
| `float` | `f32` | 32位浮点数 |
| `double` | `f64` | 64位浮点数 |
| `size_t` | `usize` | 无符号大小类型 |
| `ptrdiff_t` | `isize` | 有符号差值类型 |

#### 字符串类型

| ❌ 禁止使用 | ✅ 必须使用 | 说明 |
|------------|-----------|------|
| `std::string` | `Str` | 标准字符串 |
| `std::string_view` | `StrView` | 字符串视图（C++17） |

#### 容器类型

| ❌ 禁止使用 | ✅ 必须使用 | 说明 |
|------------|-----------|------|
| `std::vector<T>` | `Vec<T>` | 动态数组 |
| `std::unordered_map<K, V>` | `HashMap<K, V>` | 哈希映射 |
| `std::unordered_set<T>` | `HashSet<T>` | 哈希集合 |

#### 现代C++类型

| ❌ 禁止使用 | ✅ 必须使用 | 说明 |
|------------|-----------|------|
| `std::variant<Types...>` | `Var<Types...>` | 变体类型 |
| `std::optional<T>` | `Opt<T>` | 可选类型 |
| `std::function<Sig>` | `Func<Sig>` | 函数对象 |

#### 智能指针类型

| ❌ 禁止使用 | ✅ 必须使用 | 说明 |
|------------|-----------|------|
| `std::shared_ptr<T>` | `Ptr<T>` | 共享指针 |
| `std::unique_ptr<T>` | `UniquePtr<T>` | 独占指针 |
| `std::weak_ptr<T>` | `WeakPtr<T>` | 弱引用指针 |

### 代码示例对比

#### ❌ 错误示例（禁止）

```cpp
#include <string>
#include <vector>
#include <unordered_map>

class StringPool {
private:
    std::unordered_map<std::string, GCString*> pool_;
    std::vector<std::string> cache_;

public:
    GCString* intern(std::string_view str);
    std::string_view find(const std::string& key);
    size_t size() const;
};
```

#### ✅ 正确示例（必须）

```cpp
#include "common/types.hpp"

class StringPool {
private:
    HashMap<Str, GCString*> pool_;
    Vec<Str> cache_;

public:
    GCString* intern(StrView str);
    StrView find(const Str& key);
    usize size() const;
};
```

### 违规处理

违反类型系统使用规范的代码将：

1. **代码审查不通过**: Pull Request 将被要求修改
2. **需要重构**: 已有代码发现违规需立即修正
3. **编译警告**: 未来可能添加编译时检查
4. **影响评估**: 严重违规可能影响代码贡献者评级

### 特殊情况

以下情况可以例外使用原始类型：

1. **与C API交互**: 必须使用C标准类型时
2. **第三方库接口**: 第三方库要求特定类型时
3. **平台特定代码**: 需要使用平台特定类型时

**注意**: 即使在特殊情况下，也应在接口边界处尽快转换为项目类型别名。

### 检查清单

在提交代码前，请确认：

- [ ] 所有整数类型使用 `i8/i16/i32/i64` 或 `u8/u16/u32/u64`
- [ ] 所有大小类型使用 `usize`，差值类型使用 `isize`
- [ ] 所有字符串使用 `Str`，字符串视图使用 `StrView`
- [ ] 所有容器使用 `Vec/HashMap/HashSet` 等别名
- [ ] 所有现代C++类型使用 `Var/Opt/Func` 等别名
- [ ] 所有智能指针使用 `Ptr/UniquePtr/WeakPtr` 等别名
- [ ] 注释中的类型名称也使用别名
- [ ] 文档中的类型说明使用别名

### 参考资源

- **类型定义文件**: `src/common/types.hpp`
- **示例代码**: `src/core/gc_string.hpp`, `src/core/string_pool.hpp`
- **类型映射完整列表**: 查看 `types.hpp` 文件中的详细注释

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

