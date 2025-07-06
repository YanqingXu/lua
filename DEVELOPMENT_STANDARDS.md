# Lua 解释器项目开发规范

## 📋 概述

本文档规定了 Lua 解释器项目的代码开发标准，旨在确保代码质量、一致性和可维护性。所有开发者都必须严格遵循这些标准。

---

## 🎯 核心原则

1. **质量优先**: 代码质量永远优于开发速度
2. **标准统一**: 所有代码必须遵循统一的编码标准
3. **类型安全**: 使用统一的类型系统，避免类型错误
4. **文档同步**: 代码与文档必须保持同步更新
5. **测试驱动**: 每个功能都必须有对应的测试用例
6. **现代化设计**: 充分利用现代C++特性和最佳实践
7. **安全保障**: 确保内存安全、线程安全和异常安全

---

## 📝 代码规范

### 1. 类型系统 (强制要求)

**所有代码必须使用 `types.hpp` 中定义的类型系统**

#### ✅ 正确示例
```cpp
#include "common/types.hpp"

// 使用统一类型定义
Str functionName;
i32 argumentCount;
f64 resultValue;
bool isValid;
StrView description;
u64 memorySize;

// 函数参数和返回值
Value processFunction(State* state, StrView name, i32 args) {
    // implementation...
    return Value(result);
}
```

#### ❌ 错误示例
```cpp
// 禁止使用原生类型
std::string functionName;        // 应使用 Str
int argumentCount;               // 应使用 i32
double resultValue;              // 应使用 f64
unsigned long long memorySize;   // 应使用 u64
const char* description;         // 应使用 StrView
```

#### 类型映射表
| 原生类型 | 项目类型 | 说明 |
|---------|---------|------|
| `std::string` | `Str` | 字符串类型 |
| `std::string_view` | `StrView` | 字符串视图 |
| `int` | `i32` | 32位有符号整数 |
| `unsigned int` | `u32` | 32位无符号整数 |
| `long long` | `i64` | 64位有符号整数 |
| `unsigned long long` | `u64` | 64位无符号整数 |
| `float` | `f32` | 32位浮点数 |
| `double` | `f64` | 64位浮点数 |
| `bool` | `bool` | 布尔类型 (保持不变) |
| `std::vector<T>` | `Vec<T>` | 动态数组容器 |
| `std::unordered_map<K,V>` | `HashMap<K,V>` | 哈希映射容器 |
| `std::unordered_set<T>` | `HashSet<T>` | 哈希集合容器 |
| `std::unique_ptr<T>` | `UPtr<T>` | 独占智能指针 |
| `std::shared_ptr<T>` | `Ptr<T>` | 共享智能指针 |
| `std::weak_ptr<T>` | `WPtr<T>` | 弱引用智能指针 |
| `std::optional<T>` | `Opt<T>` | 可选类型 |
| `std::variant<T...>` | `Var<T...>` | 变体类型 |
| `std::mutex` | `Mtx` | 互斥锁 |
| `std::shared_mutex` | `SharedMtx` | 共享互斥锁 |

### 2. 注释规范 (强制要求)

**所有代码注释必须使用全英文**

#### ✅ 正确示例
```cpp
/**
 * Base library implementation using the new framework
 * Provides essential Lua functions like print, type, etc.
 */
class BaseLib : public LibModule {
public:
    /**
     * Get module name
     * @return Module name as string view
     */
    StrView getName() const noexcept override;
    
    /**
     * Register functions to registry
     * @param registry Function registry to register to
     * @param context Library context for configuration
     */
    void registerFunctions(LibFuncRegistry& registry, const LibContext& context) override;
    
private:
    // Flag to track initialization status
    bool initialized_ = false;
    
    // Helper function to validate input parameters
    static bool validateInput(StrView input);
};
```

#### ❌ 错误示例
```cpp
/**
 * 基础库实现，使用新的框架
 * 提供像print, type等基本Lua函数
 */
class BaseLib : public LibModule {
public:
    // 获取模块名称
    StrView getName() const noexcept override;
    
    // 注册函数到注册表
    void registerFunctions(LibFuncRegistry& registry, const LibContext& context) override;
    
private:
    bool initialized_ = false;  // 初始化状态标志
};
```

#### 注释风格指南
- **类和函数**: 使用 Doxygen 风格的 `/** */` 注释
- **行内注释**: 使用 `//` 进行简短说明
- **参数说明**: 使用 `@param` 标记
- **返回值说明**: 使用 `@return` 标记
- **异常说明**: 使用 `@throws` 标记
- **示例代码**: 使用 `@example` 标记

### 3. 命名规范

#### 类名 (PascalCase)
```cpp
class LibraryManager { };
class BaseLib { };
class FunctionRegistry { };
```

#### 函数名 (camelCase)
```cpp
void registerFunction();
bool isModuleLoaded();
Value callFunction();
```

#### 变量名 (camelCase)
```cpp
Str moduleName;
i32 argumentCount;
bool isInitialized;
```

#### 常量名 (UPPER_SNAKE_CASE)
```cpp
const i32 MAX_FUNCTION_COUNT = 1000;
const StrView DEFAULT_MODULE_NAME = "base";
```

#### 私有成员变量 (camelCase + 下划线后缀)
```cpp
class Example {
private:
    Str name_;
    i32 count_;
    bool initialized_;
};
```

#### 命名空间 (PascalCase)
```cpp
namespace Lua {
    namespace Lib {
        // implementation
    }
}
```

### 4. 文件组织规范

#### 头文件结构
```cpp
#pragma once

// 系统头文件
#include <memory>
#include <vector>
#include <unordered_map>

// 项目基础头文件
#include "common/types.hpp"

// 项目其他头文件
#include "lib_define.hpp"
#include "error_handling.hpp"

namespace Lua {
    namespace Lib {
        /**
         * Class documentation
         */
        class ClassName {
        public:
            // Public interface

        private:
            // Private implementation
        };
    }
}
```

#### 实现文件结构
```cpp
#include "header_name.hpp"

// 系统头文件 (如果需要)
#include <algorithm>
#include <iostream>

// 项目头文件 (如果需要)
#include "other_headers.hpp"

namespace Lua {
    namespace Lib {
        // Implementation
    }
}
```

### 5. 现代C++特性要求

#### 智能指针使用
```cpp
// 使用智能指针管理内存
UPtr<LibModule> module = make_unique<BaseLib>();
Ptr<LibContext> context = make_ptr<LibContext>();

// 禁止裸指针所有权
LibModule* module = new BaseLib();  // ❌ 错误
```

#### RAII 原则
```cpp
class ResourceManager {
public:
    ResourceManager() {
        // Acquire resources in constructor
        file_ = fopen("config.txt", "r");
    }
    
    ~ResourceManager() {
        // Release resources in destructor
        if (file_) {
            fclose(file_);
        }
    }
    
    // Delete copy constructor and assignment
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    
    // Move constructor and assignment if needed
    ResourceManager(ResourceManager&& other) noexcept : file_(other.file_) {
        other.file_ = nullptr;
    }

private:
    FILE* file_ = nullptr;
};
```

#### 异常安全
```cpp
class SafeOperation {
public:
    void performOperation() {
        auto backup = createBackup();  // Strong exception safety
        try {
            doRiskyOperation();
            backup.release();  // Success, don't need backup
        } catch (...) {
            restoreFromBackup(backup);  // Restore on failure
            throw;  // Re-throw
        }
    }
};
```

### 6. 错误处理规范

#### 异常使用
```cpp
// 使用项目定义的异常类型
throw LibException(LibErrorCode::InvalidArgument, 
                  "Function not found: " + functionName);

// 不要使用std::exception的派生类
throw std::runtime_error("error");  // ❌ 错误
```

#### 错误检查
```cpp
// 函数参数验证
Value BaseLib::print(State* state, i32 nargs) {
    if (!state) {
        throw LibException(LibErrorCode::InvalidArgument, 
                          "State cannot be null");
    }
    
    if (nargs < 0) {
        throw LibException(LibErrorCode::InvalidArgument, 
                          "Argument count cannot be negative");
    }
    
    // Function implementation...
    return Value();
}
```

### 7. 性能规范

#### 避免不必要的拷贝
```cpp
// 使用引用传递大对象
void processData(const Vec<Value>& data);  // ✅ 正确

// 避免值传递
void processData(Vec<Value> data);  // ❌ 错误

// 使用 StrView 避免字符串拷贝
void setName(StrView name);  // ✅ 正确
void setName(const Str& name);  // 可以，但 StrView 更好
```

#### 使用移动语义
```cpp
class Container {
public:
    void addItem(Value&& item) {
        items_.emplace_back(std::move(item));  // ✅ 正确
    }
    
    void addItem(const Value& item) {
        items_.push_back(item);  // 也可以，但效率较低
    }

private:
    Vec<Value> items_;
};
```

### 8. 线程安全规范

#### 使用适当的同步原语
```cpp
class ThreadSafeRegistry {
public:
    void registerFunction(StrView name, LibFunction func) {
        UniqueLock lock(mutex_);
        functions_[Str(name)] = std::move(func);
    }
    
    LibFunction getFunction(StrView name) const {
        SharedLock lock(mutex_);
        auto it = functions_.find(Str(name));
        return it != functions_.end() ? it->second : nullptr;
    }

private:
    mutable SharedMtx mutex_;
    HashMap<Str, LibFunction> functions_;
};
```

---

## 🧪 测试规范

### 1. 测试文件命名
- 源文件: `example.cpp` → 测试文件: `example_test.cpp`
- 头文件: `example.hpp` → 测试头文件: `example_test.hpp`

### 2. 测试用例结构
```cpp
#include "base_lib_test.hpp"
#include "base_lib.hpp"
#include <gtest/gtest.h>

namespace Lua {
namespace Lib {
namespace Test {

class BaseLibTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test setup
        state_ = createTestState();
        baseLib_ = make_unique<BaseLib>();
    }
    
    void TearDown() override {
        // Test cleanup
        destroyTestState(state_);
    }
    
    State* state_ = nullptr;
    UPtr<BaseLib> baseLib_;
};

TEST_F(BaseLibTest, PrintFunction_ValidInput_ReturnsSuccess) {
    // Arrange
    pushString(state_, "Hello, World!");
    
    // Act
    Value result = baseLib_->print(state_, 1);
    
    // Assert
    EXPECT_TRUE(result.isValid());
    EXPECT_EQ(getStackSize(state_), 0);
}

TEST_F(BaseLibTest, PrintFunction_NullState_ThrowsException) {
    // Arrange & Act & Assert
    EXPECT_THROW(
        baseLib_->print(nullptr, 1),
        LibException
    );
}

} // namespace Test
} // namespace Lib
} // namespace Lua
```

### 3. 测试覆盖要求
- **核心函数**: 100% 代码覆盖率
- **公共API**: 100% 代码覆盖率
- **私有函数**: 建议80%以上覆盖率
- **异常路径**: 必须测试所有异常情况

---

## 📁 项目结构规范

### 目录结构
```
src/
├── common/                   # 通用组件
│   └── types.hpp            # 类型系统定义 (核心)
├── vm/                      # 虚拟机核心
├── gc/                      # 垃圾回收器
├── lib/                     # 标准库
│   ├── lib_define.hpp       # 库定义
│   ├── lib_context.hpp      # 上下文管理
│   ├── lib_func_registry.hpp # 函数注册
│   ├── lib_manager.hpp      # 库管理器
│   ├── base_lib.hpp         # 基础库
│   └── ...                  # 其他库
├── lexer/                   # 词法分析器
├── parser/                  # 语法分析器
└── tests/                   # 测试代码
    ├── lib/                 # 库测试
    ├── vm/                  # VM测试
    └── ...                  # 其他测试

docs/                        # 文档目录
tests/                       # 集成测试
tools/                       # 开发工具
```

### 包含文件顺序
1. 对应的头文件 (对于 .cpp 文件)
2. 系统标准库头文件
3. 第三方库头文件
4. 项目基础头文件 (`common/types.hpp` 等)
5. 项目其他头文件

---

## 🔍 代码审查标准

### 检查清单

#### ✅ 类型系统
- [ ] 是否使用了 `types.hpp` 中定义的类型
- [ ] 是否避免了原生类型的使用
- [ ] 函数参数和返回值类型是否正确

#### ✅ 注释文档
- [ ] 是否使用全英文注释
- [ ] 公共接口是否有完整的文档注释
- [ ] 复杂逻辑是否有适当的解释

#### ✅ 命名规范
- [ ] 类名是否使用 PascalCase
- [ ] 函数名是否使用 camelCase
- [ ] 变量名是否清晰表达含义
- [ ] 私有成员是否有下划线后缀

#### ✅ 现代C++
- [ ] 是否使用智能指针管理资源
- [ ] 是否遵循 RAII 原则
- [ ] 是否使用移动语义优化性能
- [ ] 是否正确处理异常安全

#### ✅ 性能考虑
- [ ] 是否避免了不必要的拷贝
- [ ] 是否使用了合适的容器类型
- [ ] 大对象是否使用引用传递
- [ ] 字符串是否使用 StrView 优化

#### ✅ 线程安全
- [ ] 共享数据是否有适当的同步
- [ ] 是否使用了正确的锁类型
- [ ] 是否避免了竞态条件

---

## 🚀 最佳实践

### 1. 错误处理最佳实践
```cpp
// 输入验证
Value validateAndProcess(StrView input) {
    if (input.empty()) {
        throw LibException(LibErrorCode::InvalidArgument, 
                          "Input cannot be empty");
    }
    
    if (input.size() > MAX_INPUT_SIZE) {
        throw LibException(LibErrorCode::InvalidArgument, 
                          "Input too long: " + std::to_string(input.size()));
    }
    
    return processInput(input);
}
```

### 2. 资源管理最佳实践
```cpp
class ResourceHandler {
public:
    // Use RAII for automatic resource management
    explicit ResourceHandler(StrView filename) 
        : file_(make_unique<FileWrapper>(filename)) {
        if (!file_->isOpen()) {
            throw LibException(LibErrorCode::FileError, 
                             "Failed to open file: " + Str(filename));
        }
    }
    
    // Deleted copy operations to prevent resource duplication
    ResourceHandler(const ResourceHandler&) = delete;
    ResourceHandler& operator=(const ResourceHandler&) = delete;
    
    // Move operations for efficient transfer
    ResourceHandler(ResourceHandler&&) = default;
    ResourceHandler& operator=(ResourceHandler&&) = default;

private:
    UPtr<FileWrapper> file_;
};
```

### 3. 接口设计最佳实践
```cpp
// Clear, type-safe interface
class FunctionRegistry {
public:
    /**
     * Register a function with metadata
     * @param metadata Function metadata including name, description, parameters
     * @param function The function implementation
     * @throws LibException if function already exists or metadata is invalid
     */
    void registerFunction(const FunctionMetadata& metadata, LibFunction function);
    
    /**
     * Call a registered function
     * @param name Function name to call
     * @param state Lua state for execution
     * @param args Number of arguments on stack
     * @return Function result
     * @throws LibException if function not found or execution fails
     */
    Value callFunction(StrView name, State* state, i32 args);
    
    /**
     * Check if function exists
     * @param name Function name to check
     * @return true if function is registered, false otherwise
     */
    bool hasFunction(StrView name) const noexcept;
};
```

---

## 📊 质量度量

### 代码质量指标
- **编译警告**: 零警告 (使用 `-Werror`)
- **静态分析**: 通过 clang-tidy 检查
- **内存检查**: 通过 AddressSanitizer/Valgrind 检查
- **测试覆盖率**: 核心模块 ≥ 90%
- **文档覆盖率**: 公共API 100%

### 性能指标
- **编译时间**: 增量编译 < 30秒
- **运行时性能**: 关键路径性能回归 < 5%
- **内存使用**: 内存泄漏检测通过
- **启动时间**: 库加载时间 < 100ms

---

## 🔧 开发工具配置

### 编译器标志
```bash
# Debug 构建
-std=c++17 -Wall -Wextra -Werror -g -O0 -fsanitize=address

# Release 构建  
-std=c++17 -Wall -Wextra -Werror -O3 -DNDEBUG
```

### clang-format 配置
```yaml
# .clang-format
BasedOnStyle: Google
IndentWidth: 4
ColumnLimit: 100
AllowShortFunctionsOnASingleLine: Empty
BreakBeforeBraces: Attach
```

### clang-tidy 检查
```yaml
# .clang-tidy
Checks: '-*,readability-*,performance-*,modernize-*,bugprone-*'
```

---

## 📝 版本控制规范

### 提交信息格式
```
<type>(<scope>): <subject>

<body>

<footer>
```

#### 类型 (type)
- `feat`: 新功能
- `fix`: 错误修复
- `refactor`: 代码重构
- `perf`: 性能优化
- `test`: 测试相关
- `docs`: 文档更新
- `style`: 代码风格修改

#### 示例
```
feat(lib): implement BaseLib print function

- Add print function implementation with proper error handling
- Support multiple arguments with automatic string conversion
- Add comprehensive test coverage for edge cases

Closes #123
```

---

## 📚 参考资源

### 相关文档
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [项目类型系统文档](src/common/types.hpp)
- [标准库框架设计](docs/comprehensive_standard_library_refactoring_plan.md)

### 工具链
- **编译器**: GCC 9+ 或 Clang 10+
- **构建系统**: CMake 3.15+
- **测试框架**: Google Test
- **静态分析**: clang-tidy, cppcheck
- **内存检查**: AddressSanitizer, Valgrind
- **代码格式化**: clang-format

---

**版本**: 1.0  
**创建日期**: 2025年7月6日  
**最后更新**: 2025年7月6日  
**维护者**: 项目开发团队  
**状态**: ✅ 正式发布

所有团队成员都必须严格遵循本规范，违反规范的代码不得合并到主分支。
