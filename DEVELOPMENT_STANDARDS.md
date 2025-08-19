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

## ⚠️ 重要澄清

### STL函数使用政策
**STL函数在适当时鼓励使用**，与类型系统要求是独立的：

- ✅ **类型系统要求**：使用项目定义的类型别名（Str, Vec<T>, HashMap<K,V>等）
- ✅ **STL函数使用**：鼓励使用STL函数提高代码清晰度和简洁性
- ✅ **两者结合**：在项目类型上使用STL函数是完全正确的

**示例**：
```cpp
// ✅ 完全正确：项目类型 + STL函数
Vec<Str> names = {"Alice", "Bob", "Charlie"};
auto it = std::find(names.begin(), names.end(), "Bob");
Str result = std::min(name1, name2);
std::sort(numbers.begin(), numbers.end());
```

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

### 2. STL函数使用政策 (推荐指南)

**STL函数在适当时鼓励使用**

#### 核心原则
- **优先考虑代码清晰度和简洁性**：当STL函数使代码更可读、可维护和简洁时，应该使用
- **平衡原则**：在提高代码质量时选择STL函数，在需要更透明逻辑或调试可见性时使用显式实现
- **独立性**：STL函数使用与类型系统要求是独立的，使用项目定义的类型别名不禁止STL函数

#### ✅ 推荐使用的STL函数

##### 比较和数值操作
```cpp
// ✅ 推荐：使用STL函数提高可读性
i32 result = std::min(a, b);
i32 maximum = std::max(x, y);
f64 absolute = std::abs(value);

// ❌ 避免：冗长的手动实现
i32 result = (a < b) ? a : b;  // 当逻辑简单时不必要
```

##### 移动语义和性能优化
```cpp
// ✅ 推荐：使用移动语义
container.emplace_back(std::move(item));
auto ptr = std::make_unique<Object>(args);
auto shared = std::make_shared<Resource>();

// ✅ 推荐：完美转发
template<typename T>
void wrapper(T&& arg) {
    target(std::forward<T>(arg));
}
```

##### 类型转换
```cpp
// ✅ 推荐：标准类型转换
Str message = "Value: " + std::to_string(number);
i32 parsed = std::stoi(numberStr);

// ✅ 推荐：类型转换函数
auto result = static_cast<f64>(intValue);
```

##### 算法操作
```cpp
// ✅ 推荐：标准算法（在适当时）
auto it = std::find(container.begin(), container.end(), target);
std::sort(data.begin(), data.end());
std::transform(input.begin(), input.end(), output.begin(), transform_func);

// ✅ 推荐：范围操作
for (const auto& item : container) {
    // 处理item
}
```

#### ⚖️ 平衡考虑

##### 何时使用STL函数
- 逻辑简单且STL函数更清晰
- 性能关键且STL实现优化良好
- 标准操作且不需要特殊定制

##### 何时使用显式实现
- 需要特殊的错误处理逻辑
- 调试时需要步进可见性
- 业务逻辑复杂需要透明度

#### 📝 重要澄清

**STL函数使用与类型系统要求是独立的：**
```cpp
// ✅ 正确：使用项目类型 + STL函数
Vec<Str> names = {"Alice", "Bob", "Charlie"};
auto it = std::find(names.begin(), names.end(), "Bob");
Str result = std::min(name1, name2);

// ✅ 正确：在项目类型上使用STL算法
std::sort(numbers.begin(), numbers.end());
HashMap<Str, i32> map;
auto found = std::find_if(map.begin(), map.end(), predicate);
```

### 3. 注释规范 (强制要求)

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

### 7. 现代C++特性指南

#### 智能指针使用 (强制要求)
```cpp
// ✅ 推荐：使用智能指针管理内存
UPtr<LibModule> module = make_unique<BaseLib>();
Ptr<Resource> shared = make_shared<Resource>();

// ✅ 推荐：工厂函数
template<typename T, typename... Args>
UPtr<T> createUnique(Args&&... args) {
    return make_unique<T>(std::forward<Args>(args)...);
}

// ❌ 避免：裸指针内存管理
LibModule* module = new BaseLib();  // 错误
delete module;                      // 错误
```

#### 移动语义和完美转发 (推荐使用)
```cpp
class Container {
public:
    // ✅ 推荐：移动语义
    void addItem(Value&& item) {
        items_.emplace_back(std::move(item));
    }

    // ✅ 推荐：完美转发
    template<typename T>
    void emplaceItem(T&& item) {
        items_.emplace_back(std::forward<T>(item));
    }

    // ✅ 推荐：移动构造函数
    Container(Container&& other) noexcept
        : items_(std::move(other.items_)) {}

    // ✅ 推荐：移动赋值操作符
    Container& operator=(Container&& other) noexcept {
        if (this != &other) {
            items_ = std::move(other.items_);
        }
        return *this;
    }

private:
    Vec<Value> items_;
};
```

#### Auto类型推导 (推荐使用)
```cpp
// ✅ 推荐：复杂类型使用auto
auto it = container.find(key);
auto result = std::make_unique<ComplexType>();
auto lambda = [](const Value& v) { return v.isValid(); };

// ✅ 推荐：范围for循环
for (const auto& item : container) {
    processItem(item);
}

// ⚠️ 注意：简单类型可以显式声明
i32 count = 0;        // 清晰
f64 ratio = 0.5;      // 清晰
bool found = false;   // 清晰
```

#### Lambda表达式 (推荐使用)
```cpp
// ✅ 推荐：算法中使用lambda
std::sort(items.begin(), items.end(),
    [](const Item& a, const Item& b) {
        return a.priority > b.priority;
    });

// ✅ 推荐：捕获列表
auto processor = [this, &state](const Value& v) {
    return this->processValue(state, v);
};

// ✅ 推荐：泛型lambda (C++14+)
auto comparator = [](const auto& a, const auto& b) {
    return a < b;
};
```

#### Constexpr使用 (推荐使用)
```cpp
// ✅ 推荐：编译时常量
constexpr i32 MAX_STACK_SIZE = 1000;
constexpr f64 PI = 3.14159265359;

// ✅ 推荐：constexpr函数
constexpr i32 factorial(i32 n) {
    return (n <= 1) ? 1 : n * factorial(n - 1);
}

// ✅ 推荐：constexpr if (C++17+)
template<typename T>
void process(T&& value) {
    if constexpr (std::is_integral_v<T>) {
        // 整数处理
    } else {
        // 其他类型处理
    }
}
```

#### 异常安全 (强制要求)
```cpp
// ✅ 推荐：RAII和异常安全
class ResourceManager {
public:
    ResourceManager() : resource_(acquireResource()) {
        if (!resource_) {
            throw ResourceException("Failed to acquire resource");
        }
    }

    ~ResourceManager() noexcept {
        if (resource_) {
            releaseResource(resource_);
        }
    }

    // ✅ 推荐：不抛出异常的移动操作
    ResourceManager(ResourceManager&& other) noexcept
        : resource_(std::exchange(other.resource_, nullptr)) {}

private:
    Resource* resource_;
};
```

### 8. 性能规范

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

### 📚 文档组织规范 ⭐ **新增规范**

#### 1. 文档目录结构要求 (强制执行)

**所有技术文档必须放入 `docs/` 目录下，保持项目根目录整洁**

```
docs/
├── core/                    # 核心架构文档
│   ├── framework_design.md
│   ├── memory_management.md
│   └── gc_implementation.md
├── lib/                     # 标准库文档
│   ├── base_lib_complete.md
│   ├── string_lib_guide.md
│   └── math_lib_api.md
├── vm/                      # 虚拟机文档
│   ├── bytecode_spec.md
│   └── instruction_set.md

├── api/                     # API参考文档
│   └── public_api.md
└── development/             # 开发相关文档
    ├── milestone_reports.md
    └── architecture_decisions.md
```

#### 2. 根目录文档限制 (强制要求)

**项目根目录只允许存放以下核心文档**：
- ✅ `README.md` - 项目介绍和快速开始
- ✅ `README_CN.md` - 中文版项目介绍
- ✅ `current_develop_plan.md` - 当前开发计划
- ✅ `DEVELOPMENT_STANDARDS.md` - 开发规范文档

**所有其他文档必须迁移到 `docs/` 目录下**

#### 3. 功能完结文档要求 (强制执行)

**每完成一个重要功能模块，必须在 `docs/` 目录下创建功能完结总结文档**

##### 文档命名规范
```
docs/
├── [module]_[feature]_complete.md     # 功能完成总结
├── [module]_[feature]_implementation.md # 实现细节
└── [module]_[feature]_test_report.md  # 测试报告
```

##### 功能完结文档必需内容
```markdown
# [Module] [Feature] 功能完结报告

## 📋 功能概述
- 功能描述
- 实现范围
- 关键特性

## ✅ 完成的工作
- [ ] 核心功能实现
- [ ] 功能验证
- [ ] API文档编写
- [ ] 性能优化完成

## 🧪 功能验证
- 功能完整性验证
- 性能基准测试结果
- 内存泄漏检查结果

## 📊 性能指标
- 功能响应时间
- 内存使用情况
- 并发处理能力

## 🔧 技术细节
- 核心算法说明
- 关键数据结构
- 重要设计决策

## 📝 API参考
- 公共接口列表
- 参数说明
- 使用示例

## 🚀 后续优化计划
- 已知限制
- 优化建议
- 扩展方向

## 📅 完成信息
- 完成日期: YYYY-MM-DD
- 负责人: [Name]
- 审查人: [Name]
- 状态: ✅ 已完成
```

#### 4. 文档模块化管理

**当 `docs/` 目录文档过多时，必须按模块创建子目录进行分类管理**

##### 推荐模块划分
- `core/` - 核心架构和设计文档
- `lib/` - 标准库相关文档
- `vm/` - 虚拟机相关文档
- `gc/` - 垃圾回收器文档
- `parser/` - 解析器文档
- `lexer/` - 词法分析器文档
- `api/` - API参考文档
- `development/` - 开发过程文档
- `reports/` - 里程碑和进度报告

#### 5. 文档更新要求

- **同步更新**: 代码变更时必须同时更新相关文档
- **版本标记**: 文档需要标明对应的代码版本
- **审查流程**: 重要文档变更需要经过代码审查
- **索引维护**: 在 `docs/docs_overview.md` 中维护文档索引

##### 文档索引示例
```markdown
# 项目文档索引

## 📚 核心文档
- [框架设计](core/framework_design.md)
- [架构决策](development/architecture_decisions.md)

## 🔧 实现文档
- [Base Library 完成报告](lib/base_lib_complete.md)
- [String Library 实现指南](lib/string_lib_implementation.md)



## 📊 进度报告
- [项目里程碑](reports/milestone_reports.md)
- [性能基准测试](reports/performance_benchmarks.md)
```

### 包含文件顺序
1. 对应的头文件 (对于 .cpp 文件)
2. 系统标准库头文件
3. 第三方库头文件
4. 项目基础头文件 (`common/types.hpp` 等)
5. 项目其他头文件

---

## ⚖️ 规范违规处理

### 文档规范违规处理
1. **文档放置错误**: 发现技术文档在根目录下，要求立即迁移到 `docs/` 目录
2. **缺失功能文档**: 功能完成但未创建完结文档，阻止功能标记为完成
3. **文档不同步**: 代码变更但文档未更新，要求同步更新文档
4. **缺失文档索引**: `docs/` 目录无索引或索引过时，要求更新维护

### 编译规范违规处理
1. **编译失败**: 代码编译错误，要求立即修复
2. **警告存在**: 编译警告未处理，要求全部修复
3. **跳过验证**: 未执行编译验证直接提交，要求补充验证

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

#### ✅ STL函数使用
- [ ] 是否在适当时使用STL函数提高代码清晰度
- [ ] 是否使用std::min/std::max替代冗长的三元操作符
- [ ] 是否使用std::move优化性能
- [ ] 是否使用std::make_unique/std::make_shared创建智能指针
- [ ] 是否在算法操作中合理使用STL函数

#### ✅ 现代C++特性
- [ ] 是否使用智能指针管理资源
- [ ] 是否遵循 RAII 原则
- [ ] 是否使用移动语义优化性能
- [ ] 是否正确处理异常安全
- [ ] 是否合理使用auto类型推导
- [ ] 是否使用范围for循环
- [ ] 是否在适当时使用lambda表达式
- [ ] 是否使用constexpr优化编译时计算

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

### 📋 强制编译验证要求 ⭐ **新增规范**

**每次修改C++代码后必须执行编译验证**：

#### 1. 编译验证流程 (强制执行)
```bash
# 编译单个文件验证语法
g++ -std=c++17 -Wall -Wextra -Werror -c [filename].cpp -o [filename].o

# 示例：编译lib_context.cpp
g++ -std=c++17 -Wall -Wextra -Werror -c src/lib/lib_context.cpp -o src/lib/lib_context.o
```

#### 2. 错误修复要求
- ✅ **必须修复所有编译错误**: 任何编译错误都必须立即修复
- ✅ **必须修复所有警告**: 使用`-Werror`将警告视为错误
- ✅ **必须通过语法检查**: 确保C++17标准兼容性
- ✅ **必须验证头文件依赖**: 确保所有#include正确
- ✅ **编译成功后清理**: 编译成功没有报错则删除生成的目标文件

#### 3. 编译验证检查清单
- [ ] 编译命令执行成功 (返回代码0)
- [ ] 无语法错误
- [ ] 无类型错误
- [ ] 无未定义符号
- [ ] 无缺失头文件
- [ ] 无警告信息
- [ ] 编译成功后删除生成的目标文件

#### 4. 常见编译错误处理
```bash
# 缺失头文件
error: 'std::string' has not been declared
解决: #include <string>

# 类型不匹配
error: cannot convert 'int' to 'size_t'
解决: 使用正确的类型转换或类型定义

# 未定义符号
error: 'function_name' was not declared in this scope
解决: 添加正确的函数声明或包含头文件
```

#### 5. 自动化编译脚本 (推荐)
```bash
#!/bin/bash
# compile_check.sh - 自动编译检查脚本

compile_file() {
    local file=$1
    local output=${file%.cpp}.o
    
    echo "Compiling $file..."
    if g++ -std=c++17 -Wall -Wextra -Werror -c "$file" -o "$output"; then
        echo "✅ $file compiled successfully"
        # Clean up generated object file after successful compilation
        rm -f "$output"
        echo "🗑️ Cleaned up $output"
        return 0
    else
        echo "❌ $file compilation failed"
        return 1
    fi
}

# 使用示例
compile_file "src/lib/lib_context.cpp"
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

**版本**: 1.1  
**创建日期**: 2025年7月6日  
**最后更新**: 2025年8月19日  
**维护者**: 项目开发团队  
**状态**: ✅ 正式发布

## 📝 版本更新记录

### v1.1 (2025年7月6日)
- ✅ **新增**: 强制编译验证要求规范
- ✅ **新增**: 文档组织规范和模块化管理要求
- ✅ **新增**: 功能完结文档强制要求
- ✅ **新增**: 违规处理流程明确化
- ✅ **完善**: 根目录文档限制规则

### v1.0 (2025年7月6日)
- 🎯 初始版本发布
- 📋 基础开发规范制定
- 🔧 类型系统规范确立
- 📝 注释和命名规范制定

---

所有团队成员都必须严格遵循本规范，违反规范的代码不得合并到主分支。
