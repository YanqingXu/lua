# 标准库核心框架组件实现计划

## 🎯 实现目标

基于统一架构设计，实现LibraryManager、LibContext、LibFuncRegistry等核心框架组件，建立完整的标准库管理和运行基础。

---

## 📦 核心组件清单

### 1. LibraryManager - 中央库管理器

**文件**: `src/lib/lib_manager.hpp/cpp`  
**状态**: 🔄 需要完善实现  
**优先级**: 🔥 高

#### 功能需求
- **模块注册与发现**: 支持静态注册和工厂函数注册
- **依赖关系管理**: 自动解析和加载依赖模块
- **生命周期控制**: 管理模块的初始化、运行、清理
- **延迟加载机制**: 按需加载减少启动开销
- **性能监控**: 统计加载时间和调用频率
- **错误处理**: 统一的错误报告和恢复机制

#### 实现要点
```cpp
class LibraryManager {
private:
    // 单例实现
    static std::unique_ptr<LibraryManager> instance_;
    static std::once_flag initFlag_;
    
    // 模块管理
    std::unordered_map<Str, ModuleInfo> modules_;
    std::shared_ptr<LibContext> globalContext_;
    
    // 线程安全
    mutable std::shared_mutex mutex_;
    
    // 性能统计
    LibraryStats stats_;
    
public:
    // 核心接口
    static LibraryManager& getInstance();
    void registerModule(std::unique_ptr<LibModule> module);
    bool loadModule(StrView name, State* state, const LibContext& context);
    
    // 批量操作
    void loadCoreModules(State* state, const LibContext& context);
    void loadAllModules(State* state, const LibContext& context);
    
    // 状态查询
    bool isRegistered(StrView name) const;
    bool isLoaded(StrView name) const;
    LibModule* getModule(StrView name) const;
};
```

### 2. LibContext - 配置和上下文管理

**文件**: `src/lib/lib_context.hpp`  
**状态**: 🔄 需要完善实现  
**优先级**: 🔥 高

#### 功能需求
- **类型安全配置**: 支持各种类型的配置参数
- **依赖注入**: 管理服务和对象的依赖关系
- **环境变量**: 系统环境和运行时设置
- **安全控制**: 沙箱模式和权限管理
- **批量配置**: 从文件或字符串加载配置

#### 实现要点
```cpp
class LibContext {
private:
    // 配置存储
    std::unordered_map<Str, std::any> config_;
    
    // 依赖注入
    std::unordered_map<std::type_index, std::shared_ptr<void>> dependencies_;
    
    // 环境设置
    std::unordered_map<Str, Str> environment_;
    
    // 安全配置
    bool safeMode_ = false;
    SandboxLevel sandboxLevel_ = SandboxLevel::None;
    
    // 线程安全
    mutable std::shared_mutex mutex_;
    
public:
    // 配置管理
    template<typename T>
    void setConfig(StrView key, T&& value);
    
    template<typename T>
    std::optional<T> getConfig(StrView key) const;
    
    // 依赖注入
    template<typename T>
    void addDependency(std::shared_ptr<T> dependency);
    
    template<typename T>
    std::shared_ptr<T> getDependency() const;
};
```

### 3. LibFuncRegistry - 函数注册表

**文件**: `src/lib/lib_func_registry.hpp/cpp`  
**状态**: 🔄 需要完善实现  
**优先级**: 🔥 高

#### 功能需求
- **高效查找**: 优化的函数查找算法
- **元数据支持**: 函数签名、参数类型、描述信息
- **批量注册**: 支持批量注册和模板化API
- **调试支持**: 调用统计和性能监控
- **类型安全**: 编译时类型检查

#### 实现要点
```cpp
class LibFuncRegistry {
private:
    // 函数存储
    std::unordered_map<Str, FunctionInfo> functions_;
    
    // 性能缓存
    mutable std::unordered_map<Str, LuaFunction> cache_;
    
    // 统计信息
    mutable std::unordered_map<Str, CallStats> callStats_;
    
    // 线程安全
    mutable std::shared_mutex mutex_;
    
public:
    // 函数注册
    void registerFunction(StrView name, LuaFunction func, const FunctionMetadata& meta = {});
    
    template<typename F>
    void registerFunction(StrView name, F&& func, const FunctionMetadata& meta = {});
    
    // 函数调用
    LuaFunction getFunction(StrView name) const;
    bool hasFunction(StrView name) const;
    
    // 批量操作
    void registerFunctions(const std::vector<FunctionRegistration>& functions);
    std::vector<Str> getAllFunctionNames() const;
    
    // 统计和调试
    CallStats getFunctionStats(StrView name) const;
    void resetStats();
};
```

---

## 🔄 实现计划

### 第1天: LibraryManager基础实现
**任务**:
- [ ] 实现单例模式和基础结构
- [ ] 实现模块注册功能
- [ ] 实现基础的模块加载机制
- [ ] 添加线程安全保护

**预期成果**:
- 可以注册和加载基础模块
- 基本的错误处理机制
- 线程安全的访问接口

### 第2天: LibContext配置系统
**任务**:
- [ ] 实现类型安全的配置存储
- [ ] 实现依赖注入基础功能
- [ ] 添加环境变量支持
- [ ] 实现配置文件加载

**预期成果**:
- 完整的配置管理功能
- 基础的依赖注入支持
- 配置文件解析功能

### 第3天: LibFuncRegistry优化
**任务**:
- [ ] 实现高效的函数查找算法
- [ ] 添加函数元数据支持
- [ ] 实现批量注册功能
- [ ] 添加性能统计机制

**预期成果**:
- 高性能的函数注册和查找
- 完整的元数据支持
- 调试和监控功能

### 第4天: 集成测试和优化
**任务**:
- [ ] 集成三个核心组件
- [ ] 编写单元测试
- [ ] 性能基准测试
- [ ] 错误处理完善

**预期成果**:
- 完整的核心框架可以运行
- 通过基础测试验证
- 性能指标达到预期

---

## 🧪 测试策略

### 单元测试
```cpp
// LibraryManager测试
TEST(LibraryManagerTest, SingletonAccess) {
    auto& manager1 = LibraryManager::getInstance();
    auto& manager2 = LibraryManager::getInstance();
    EXPECT_EQ(&manager1, &manager2);
}

TEST(LibraryManagerTest, ModuleRegistration) {
    auto& manager = LibraryManager::getInstance();
    auto module = std::make_unique<TestModule>();
    manager.registerModule(std::move(module));
    EXPECT_TRUE(manager.isRegistered("test"));
}

// LibContext测试
TEST(LibContextTest, ConfigManagement) {
    LibContext context;
    context.setConfig("test_int", 42);
    context.setConfig("test_string", std::string("hello"));
    
    EXPECT_EQ(context.getConfig<int>("test_int"), 42);
    EXPECT_EQ(context.getConfig<std::string>("test_string"), "hello");
}

// LibFuncRegistry测试
TEST(LibFuncRegistryTest, FunctionRegistration) {
    LibFuncRegistry registry;
    registry.registerFunction("test_func", [](State* s) { return 0; });
    
    EXPECT_TRUE(registry.hasFunction("test_func"));
    auto func = registry.getFunction("test_func");
    EXPECT_NE(func, nullptr);
}
```

### 集成测试
```cpp
TEST(IntegrationTest, FullWorkflow) {
    // 创建管理器和上下文
    auto& manager = LibraryManager::getInstance();
    LibContext context;
    
    // 配置设置
    context.setConfig("debug_mode", true);
    
    // 注册模块
    manager.registerModule(std::make_unique<BaseLib>());
    
    // 加载模块
    State* state = createTestState();
    EXPECT_TRUE(manager.loadModule("base", state, context));
    
    // 验证功能
    EXPECT_TRUE(manager.isLoaded("base"));
    auto module = manager.getModule("base");
    EXPECT_NE(module, nullptr);
}
```

### 性能测试
```cpp
TEST(PerformanceTest, FunctionLookupSpeed) {
    LibFuncRegistry registry;
    
    // 注册大量函数
    for (int i = 0; i < 1000; ++i) {
        registry.registerFunction("func_" + std::to_string(i), 
                                 [](State* s) { return 0; });
    }
    
    // 测试查找性能
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10000; ++i) {
        registry.getFunction("func_500");
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    EXPECT_LT(duration.count(), 1000); // 应该在1ms内完成
}
```

---

## 📊 性能指标

### 目标指标
- **函数查找**: 平均 < 100ns per lookup
- **模块加载**: < 10ms per module  
- **内存使用**: < 1MB for framework overhead
- **并发性能**: 支持10+线程同时访问

### 优化策略
- **缓存机制**: 热点函数查找缓存
- **延迟初始化**: 减少启动时间
- **内存池**: 减少动态分配
- **无锁设计**: 读多写少的场景优化

---

## 🔒 错误处理策略

### 错误分类
1. **配置错误**: 无效的配置参数或类型不匹配
2. **依赖错误**: 模块依赖不满足或循环依赖
3. **加载错误**: 模块加载失败或初始化错误
4. **运行时错误**: 函数调用时的参数或状态错误

### 处理机制
```cpp
// 统一错误类型
enum class LibErrorCode {
    ConfigurationError,
    DependencyError,
    LoadingError,
    RuntimeError
};

// 异常类
class LibException : public std::exception {
public:
    LibException(LibErrorCode code, const std::string& message);
    LibErrorCode getErrorCode() const;
    const char* what() const override;
};

// 错误恢复
class ErrorRecovery {
public:
    static bool tryRecover(LibErrorCode code, const std::string& context);
    static void reportError(LibErrorCode code, const std::string& message);
};
```

---

## 🚀 下一步计划

### 本周任务
1. **实现LibraryManager** - 完整的模块管理功能
2. **完善LibContext** - 配置和依赖注入系统
3. **优化LibFuncRegistry** - 高性能函数注册
4. **集成测试** - 确保组件协同工作

### 下周计划
1. **与BaseLib集成** - 使用新框架重构BaseLib
2. **性能优化** - 基准测试和优化
3. **错误处理完善** - 完整的错误恢复机制
4. **文档完善** - API文档和使用指南

---

**文档版本**: 1.0  
**创建日期**: 2025年6月29日  
**负责人**: AI Assistant  
**状态**: 🔄 实施计划就绪
