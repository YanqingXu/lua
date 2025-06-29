# 现代C++标准库框架设计指南

## 概述

本文档介绍了基于Lua 5.1官方标准库设计，结合现代C++特性的全新标准库框架。该框架提供了优良的设计、易扩展性和良好的可读性。

## 设计原则

### 1. 遵循Lua 5.1官方设计模式
- **函数签名**: 采用类似`lua_CFunction`的签名 `int(State*)`
- **参数处理**: 遵循Lua栈操作模式，参数从栈底开始
- **返回值**: 返回值数量，而非直接返回Value对象
- **错误处理**: 使用异常机制，但保持与Lua错误模型兼容

### 2. 现代C++特性应用
- **智能指针**: 自动内存管理，避免内存泄漏
- **模板元编程**: 类型安全的参数检查和转换
- **RAII**: 资源自动管理
- **std::function**: 灵活的函数注册机制
- **完美转发**: 高效的参数传递

### 3. 模块化设计
- **接口分离**: 清晰的模块接口定义
- **依赖注入**: 支持配置和依赖管理
- **懒加载**: 按需加载模块，提高启动性能
- **生命周期管理**: 完整的模块生命周期控制

## 核心组件

### 1. LibraryModule - 模块基类
```cpp
class LibraryModule {
public:
    virtual std::string_view getName() const noexcept = 0;
    virtual void registerFunctions(FunctionRegistry& registry, const LibraryContext& context) = 0;
    virtual void initialize(State* state, const LibraryContext& context) {}
    virtual void cleanup(State* state, const LibraryContext& context) {}
};
```

**特点**:
- 简洁的接口设计，只包含必要方法
- 使用`std::string_view`避免不必要的字符串拷贝
- 可选的初始化和清理钩子
- 支持依赖注入的上下文参数

### 2. FunctionRegistry - 函数注册表
```cpp
class FunctionRegistry {
public:
    void registerFunction(const FunctionMetadata& meta, NativeFunction func);
    std::optional<int> callFunction(std::string_view name, State* state) const;
    bool hasFunction(std::string_view name) const noexcept;
};
```

**特点**:
- 支持函数元数据，便于调试和文档生成
- 使用`std::unordered_map`优化查找性能
- 类型安全的函数注册
- 异常安全的函数调用

### 3. LibraryManager - 库管理器
```cpp
class LibraryManager {
public:
    void registerModule(std::unique_ptr<LibraryModule> module, LoadStrategy strategy);
    bool loadModule(std::string_view name, State* state);
    void loadAllModules(State* state);
};
```

**特点**:
- 支持多种加载策略（立即、懒加载、手动）
- 依赖关系解析
- 模块状态跟踪
- 统计信息和调试支持

### 4. LibraryContext - 库上下文
```cpp
class LibraryContext {
public:
    template<typename T>
    void setConfig(std::string_view key, T&& value);
    
    template<typename T>
    void addDependency(std::shared_ptr<T> dependency);
};
```

**特点**:
- 类型安全的配置管理
- 依赖注入支持
- 模板化设计，支持任意类型

## 使用指南

### 1. 创建自定义库模块

```cpp
class MyCustomLibrary : public LibraryModule {
public:
    std::string_view getName() const noexcept override {
        return "mycustom";
    }

    void registerFunctions(FunctionRegistry& registry, const LibraryContext& context) override {
        registry.registerFunction(
            FunctionMetadata("hello")
                .withDescription("Print hello message")
                .withArgs(0, 1),
            [](State* s) { return hello(s); }
        );
    }

private:
    LUA_FUNCTION(hello) {
        std::string name = "World";
        if (state->getTop() >= 1) {
            name = ArgUtils::checkString(state, 1, "hello").asString();
        }
        
        std::cout << "Hello, " << name << "!" << std::endl;
        return 0;
    }
};
```

### 2. 设置库管理器

```cpp
void setupLibraries(State* state) {
    // 创建库管理器
    auto manager = std::make_unique<LibraryManager>();
    
    // 注册标准库
    manager->registerModule(std::make_unique<BaseLibrary>());
    
    // 注册自定义库
    manager->registerModule(std::make_unique<MyCustomLibrary>());
    
    // 加载所有库
    manager->loadAllModules(state);
}
```

### 3. 高级配置

```cpp
void advancedSetup() {
    // 创建上下文
    auto context = std::make_shared<LibraryContext>();
    
    // 设置配置
    context->setConfig("debug_mode", true);
    context->setConfig("max_stack_size", 1000);
    
    // 添加依赖
    auto logger = std::make_shared<Logger>();
    context->addDependency(logger);
    
    // 创建管理器
    auto manager = std::make_unique<LibraryManager>(context);
    
    // 启用调试模式
    manager->setDebugMode(true);
    
    // 注册模块工厂（懒加载）
    manager->registerModuleFactory("advanced", 
        []() { return std::make_unique<AdvancedLibrary>(); },
        LibraryManager::LoadStrategy::Lazy
    );
}
```

## 最佳实践

### 1. 错误处理
```cpp
LUA_FUNCTION(myFunction) {
    return ErrorUtils::protectedCall(state, [&]() {
        // 检查参数
        ArgUtils::checkArgCount(state, 2, "myFunction");
        
        auto arg1 = ArgUtils::checkNumber(state, 1, "myFunction");
        auto arg2 = ArgUtils::checkString(state, 2, "myFunction");
        
        // 执行逻辑
        // ...
        
        return 1; // 返回值数量
    });
}
```

### 2. 类型转换
```cpp
// 使用类型工具进行安全转换
auto number = TypeUtils::getValue<double>(value);
if (number) {
    // 使用 *number
}

// 创建返回值
state->push(TypeUtils::createValue(42));
state->push(TypeUtils::createValue(std::string("hello")));
```

### 3. 函数元数据
```cpp
registry.registerFunction(
    FunctionMetadata("complexFunction")
        .withDescription("Performs complex calculation")
        .withArgs(2, 4)  // 2-4个参数
        .withVariadic()  // 支持可变参数
        .withArgTypes({"number", "string", "table?", "function?"})
        .withReturnTypes({"number", "string"}),
    complexFunction
);
```

## 性能优化

### 1. 函数查找优化
- 使用`std::unordered_map`进行O(1)查找
- 函数名使用`std::string_view`减少拷贝
- 预分配容器大小

### 2. 内存管理优化
- 使用智能指针自动管理内存
- RAII确保异常安全
- 移动语义减少拷贝开销

### 3. 编译时优化
- 模板元编程进行编译时类型检查
- `constexpr`函数在编译时计算
- 内联函数减少调用开销

## 扩展性设计

### 1. 插件系统
```cpp
// 支持动态加载插件
class PluginManager {
public:
    void loadPlugin(const std::string& path);
    void unloadPlugin(const std::string& name);
};
```

### 2. 自定义类型支持
```cpp
// 支持注册自定义C++类型
template<typename T>
void registerUserType(LibraryManager& manager) {
    // 自动生成绑定代码
}
```

### 3. 异步支持
```cpp
// 支持异步函数调用
LUA_FUNCTION(asyncFunction) {
    return AsyncUtils::createAsyncCall(state, [](auto callback) {
        // 异步操作
        std::thread([callback]() {
            // 执行异步任务
            callback(result);
        }).detach();
    });
}
```

## 调试和诊断

### 1. 调试信息
- 函数调用跟踪
- 参数类型检查
- 性能统计

### 2. 错误报告
- 详细的错误消息
- 调用栈信息
- 参数验证失败详情

### 3. 监控工具
- 模块加载状态
- 函数调用统计
- 内存使用情况

## 总结

这个现代C++标准库框架结合了Lua 5.1的经典设计和现代C++的强大特性，提供了：

1. **优良的设计**: 清晰的接口分离，模块化架构
2. **易扩展性**: 支持插件、自定义类型、异步操作
3. **可读性好**: 丰富的元数据、清晰的命名、完整的文档

该框架为Lua解释器的标准库开发提供了坚实的基础，既保持了与Lua传统的兼容性，又充分利用了现代C++的优势。
