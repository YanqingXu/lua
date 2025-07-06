# Lua 解释器标准库框架重构设计方案

## 🎯 重构目标

基于对现有代码的深度分析，我们需要建立一个统一、现代化、高度模块化的标准库框架，以解决当前架构分散、依赖混乱、功能不完整的问题。

---

## 🏗️ 框架整体架构

### 架构分层设计

```
┌─────────────────────────────────────────────────────────────┐
│                    应用层 (Application Layer)               │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │
│  │   Lexer     │  │   Parser    │  │   Runtime   │        │
│  └─────────────┘  └─────────────┘  └─────────────┘        │
├─────────────────────────────────────────────────────────────┤
│                  库接口层 (Library API Layer)               │
│  ┌─────────────────────────────────────────────────────────┐│
│  │                 LibraryAPI                              ││
│  │  • registerBaseLib()     • registerMathLib()           ││
│  │  • registerStringLib()   • registerIOLib()             ││
│  │  • initializeStandardLibs() • configureLibs()          ││
│  └─────────────────────────────────────────────────────────┘│
├─────────────────────────────────────────────────────────────┤
│                  库管理层 (Library Management Layer)        │
│  ┌─────────────────────────────────────────────────────────┐│
│  │                LibraryManager                           ││
│  │  • 库的注册与发现  • 依赖关系管理  • 生命周期控制       ││
│  │  • 延迟加载机制    • 错误处理     • 性能监控            ││
│  └─────────────────────────────────────────────────────────┘│
├─────────────────────────────────────────────────────────────┤
│                  核心框架层 (Core Framework Layer)          │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │
│  │ LibModule   │  │ LibContext  │  │FuncRegistry │        │
│  │   接口      │  │   上下文    │  │  注册表     │        │
│  └─────────────┘  └─────────────┘  └─────────────┘        │
├─────────────────────────────────────────────────────────────┤
│                 标准库模块层 (Standard Library Modules)     │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │
│  │   BaseLib   │  │ StringLib   │  │  MathLib    │        │
│  └─────────────┘  └─────────────┘  └─────────────┘        │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │
│  │  TableLib   │  │   IOLib     │  │   OSLib     │        │
│  └─────────────┘  └─────────────┘  └─────────────┘        │
├─────────────────────────────────────────────────────────────┤
│                   基础层 (Foundation Layer)                 │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │
│  │    State    │  │    Value    │  │   Utils     │        │
│  │  虚拟机状态 │  │   值系统    │  │ 工具函数    │        │
│  └─────────────┘  └─────────────┘  └─────────────┘        │
└─────────────────────────────────────────────────────────────┘
```

---

## 📦 核心组件设计

### 1. LibModule - 统一模块接口

```cpp
namespace Lua {
namespace Lib {

/**
 * 标准库模块统一接口
 * 所有标准库模块都必须实现此接口
 */
class LibModule {
public:
    virtual ~LibModule() = default;
    
    // === 模块基础信息 ===
    virtual StrView getName() const noexcept = 0;
    virtual StrView getVersion() const noexcept = 0;
    virtual StrView getDescription() const noexcept = 0;
    
    // === 模块依赖管理 ===
    virtual std::vector<StrView> getDependencies() const { return {}; }
    virtual bool isCompatibleWith(StrView version) const { return true; }
    
    // === 模块生命周期 ===
    virtual void configure(LibContext& context) {}
    virtual void registerFunctions(LibFuncRegistry& registry, const LibContext& context) = 0;
    virtual void initialize(State* state, const LibContext& context) {}
    virtual void cleanup(State* state, const LibContext& context) {}
    
    // === 模块状态查询 ===
    virtual bool isInitialized() const { return initialized_; }
    virtual std::vector<StrView> getExportedFunctions() const = 0;
    
protected:
    bool initialized_ = false;
};

}} // namespace Lua::Lib
```

### 2. LibraryManager - 中央管理器

```cpp
namespace Lua {
namespace Lib {

/**
 * 标准库管理器 - 负责所有库模块的注册、加载、管理
 * 采用单例模式确保全局一致性
 */
class LibraryManager {
public:
    // === 单例访问 ===
    static LibraryManager& getInstance();
    
    // === 模块注册 ===
    void registerModule(std::unique_ptr<LibModule> module);
    void registerModuleFactory(StrView name, 
                              std::function<std::unique_ptr<LibModule>()> factory,
                              LoadStrategy strategy = LoadStrategy::Lazy);
    
    // === 模块加载 ===
    bool loadModule(StrView name, State* state, const LibContext& context = {});
    void loadCoreModules(State* state, const LibContext& context = {});
    void loadAllModules(State* state, const LibContext& context = {});
    void loadModulesFromConfig(State* state, const LibraryConfig& config);
    
    // === 模块查询 ===
    bool isRegistered(StrView name) const;
    bool isLoaded(StrView name) const;
    LibModule* getModule(StrView name) const;
    std::vector<StrView> getRegisteredModules() const;
    std::vector<StrView> getLoadedModules() const;
    
    // === 依赖管理 ===
    bool resolveDependencies(StrView moduleName, State* state, const LibContext& context);
    std::vector<StrView> getDependencyChain(StrView moduleName) const;
    
    // === 配置管理 ===
    void setGlobalContext(std::shared_ptr<LibContext> context);
    LibContext& getGlobalContext();
    
    // === 性能监控 ===
    LibraryStats getStatistics() const;
    void enablePerformanceMonitoring(bool enable);
    
private:
    LibraryManager() = default;
    
    struct ModuleInfo {
        std::unique_ptr<LibModule> module;
        std::function<std::unique_ptr<LibModule>()> factory;
        LoadStrategy strategy;
        bool loaded = false;
        std::chrono::steady_clock::time_point loadTime;
    };
    
    std::unordered_map<Str, ModuleInfo> modules_;
    std::shared_ptr<LibContext> globalContext_;
    mutable std::shared_mutex mutex_;
    LibraryStats stats_;
};

/** 加载策略枚举 */
enum class LoadStrategy {
    Immediate,  // 立即加载
    Lazy,       // 延迟加载（首次使用时）
    Manual      // 手动加载
};

}} // namespace Lua::Lib
```

### 3. LibContext - 上下文与配置

```cpp
namespace Lua {
namespace Lib {

/**
 * 库上下文 - 管理配置、依赖注入、环境设置
 */
class LibContext {
public:
    // === 配置管理 ===
    template<typename T>
    void setConfig(StrView key, T&& value);
    
    template<typename T>
    std::optional<T> getConfig(StrView key) const;
    
    void setConfigFromFile(StrView filename);
    void setConfigFromString(StrView configStr);
    
    // === 依赖注入 ===
    template<typename T>
    void addDependency(std::shared_ptr<T> dependency);
    
    template<typename T>
    std::shared_ptr<T> getDependency() const;
    
    // === 环境设置 ===
    void setDebugMode(bool enable);
    void setSafeMode(bool enable);
    void setSandboxMode(bool enable);
    void setPerformanceMode(bool enable);
    
    bool isDebugMode() const;
    bool isSafeMode() const;
    bool isSandboxMode() const;
    bool isPerformanceMode() const;
    
    // === 日志和监控 ===
    void setLogger(std::shared_ptr<Logger> logger);
    std::shared_ptr<Logger> getLogger() const;
    
    void setProfiler(std::shared_ptr<Profiler> profiler);
    std::shared_ptr<Profiler> getProfiler() const;

private:
    std::unordered_map<Str, std::any> config_;
    std::unordered_map<std::type_index, std::any> dependencies_;
    
    bool debugMode_ = false;
    bool safeMode_ = false;
    bool sandboxMode_ = false;
    bool performanceMode_ = false;
    
    std::shared_ptr<Logger> logger_;
    std::shared_ptr<Profiler> profiler_;
};

}} // namespace Lua::Lib
```

### 4. LibFuncRegistry - 函数注册表

```cpp
namespace Lua {
namespace Lib {

/**
 * 函数注册表 - 管理Lua函数的注册、查找、调用
 */
class LibFuncRegistry {
public:
    // === 函数注册 ===
    void registerFunction(StrView name, LibFunction func);
    void registerFunction(const FunctionMetadata& meta, LibFunction func);
    void registerFunctionBatch(const std::vector<FunctionRegistration>& functions);
    
    // === 函数查找和调用 ===
    LibFunction findFunction(StrView name) const;
    Value callFunction(StrView name, State* state, i32 nargs) const;
    bool hasFunction(StrView name) const;
    
    // === 元数据查询 ===
    const FunctionMetadata* getFunctionMetadata(StrView name) const;
    std::vector<StrView> getAllFunctionNames() const;
    std::vector<StrView> getFunctionsByModule(StrView moduleName) const;
    
    // === 性能统计 ===
    const FunctionStats& getFunctionStats(StrView name) const;
    void resetStats();
    
private:
    struct FunctionEntry {
        LibFunction function;
        FunctionMetadata metadata;
        mutable FunctionStats stats;
    };
    
    std::unordered_map<Str, FunctionEntry> functions_;
    mutable std::shared_mutex mutex_;
};

/** 函数元数据 */
struct FunctionMetadata {
    Str name;
    Str description;
    Str moduleName;
    i32 minArgs;
    i32 maxArgs;    // -1 表示可变参数
    std::vector<Str> paramNames;
    std::vector<Str> paramTypes;
    Str returnType;
    Str version;
    
    // 链式配置API
    FunctionMetadata& withDescription(StrView desc);
    FunctionMetadata& withArgs(i32 min, i32 max = -1);
    FunctionMetadata& withParam(StrView name, StrView type);
    FunctionMetadata& withReturn(StrView type);
    FunctionMetadata& withVersion(StrView ver);
};

}} // namespace Lua::Lib
```

---

## 📚 标准库模块重构

### 核心库优先级排序

| 优先级 | 库名称 | 状态 | 完成度 | 预计工期 |
|--------|--------|------|--------|----------|
| 🔥 P0 | BaseLib | 🔄 重构中 | 70% | 1周 |
| 🔥 P0 | ErrorHandling | ✅ 基本完成 | 85% | 3天 |
| 🔥 P1 | StringLib | 🔄 需重构 | 60% | 1周 |
| 🔥 P1 | TableLib | 🔄 需重构 | 40% | 1.5周 |
| 📊 P2 | MathLib | 🔄 需重构 | 50% | 1周 |
| 📊 P2 | IOLib | ❌ 未开始 | 0% | 2周 |
| 📊 P3 | OSLib | ❌ 未开始 | 0% | 1.5周 |
| 🧪 P4 | DebugLib | ❌ 可选 | 0% | 1周 |
| 🧪 P4 | CoroutineLib | ❌ 可选 | 0% | 2周 |

### 1. BaseLib 重构完善

**当前状态**: 已有新架构框架 (`base_lib_new.hpp/cpp`)，需要完善实现

**重构重点**:
```cpp
class BaseLib : public LibModule {
public:
    // === 核心基础函数 (P0) ===
    LUA_FUNCTION(print);      // ✅ 优先实现
    LUA_FUNCTION(type);       // ✅ 优先实现  
    LUA_FUNCTION(tostring);   // ✅ 优先实现
    LUA_FUNCTION(tonumber);   // ✅ 优先实现
    LUA_FUNCTION(error);      // ✅ 优先实现
    LUA_FUNCTION(assert_func);// ✅ 优先实现
    
    // === 表操作函数 (P1) ===
    LUA_FUNCTION(pairs);      // 🔄 需要完善
    LUA_FUNCTION(ipairs);     // 🔄 需要完善
    LUA_FUNCTION(next);       // 🔄 需要完善
    
    // === 元表操作 (P1) ===
    LUA_FUNCTION(getmetatable);
    LUA_FUNCTION(setmetatable);
    
    // === 原始访问 (P2) ===
    LUA_FUNCTION(rawget);
    LUA_FUNCTION(rawset);
    LUA_FUNCTION(rawlen);
    LUA_FUNCTION(rawequal);
    
    // === 错误处理 (P2) ===
    LUA_FUNCTION(pcall);
    LUA_FUNCTION(xpcall);
    
    // === 工具函数 (P3) ===
    LUA_FUNCTION(select);
    LUA_FUNCTION(unpack);
    
    // === 代码加载 (P3) ===
    LUA_FUNCTION(loadstring);
    LUA_FUNCTION(load);
};
```

### 2. StringLib 标准化重构

```cpp
class StringLib : public LibModule {
public:
    StrView getName() const noexcept override { return "string"; }
    StrView getVersion() const noexcept override { return "1.0.0"; }
    StrView getDescription() const noexcept override { 
        return "Lua 5.1 String Library"; 
    }
    
    void registerFunctions(LibFuncRegistry& registry, const LibContext& context) override;
    
    // === 基础字符串函数 ===
    LUA_FUNCTION(len);        // string.len
    LUA_FUNCTION(sub);        // string.sub
    LUA_FUNCTION(upper);      // string.upper
    LUA_FUNCTION(lower);      // string.lower
    LUA_FUNCTION(rep);        // string.rep
    LUA_FUNCTION(reverse);    // string.reverse
    
    // === 查找和替换 ===
    LUA_FUNCTION(find);       // string.find
    LUA_FUNCTION(match);      // string.match
    LUA_FUNCTION(gsub);       // string.gsub
    LUA_FUNCTION(gmatch);     // string.gmatch
    
    // === 格式化 ===
    LUA_FUNCTION(format);     // string.format
    
    // === 字符操作 ===
    LUA_FUNCTION(byte);       // string.byte
    LUA_FUNCTION(char);       // string.char
};
```

### 3. TableLib 全新设计

```cpp
class TableLib : public LibModule {
public:
    StrView getName() const noexcept override { return "table"; }
    
    // === 表操作函数 ===
    LUA_FUNCTION(insert);     // table.insert
    LUA_FUNCTION(remove);     // table.remove
    LUA_FUNCTION(sort);       // table.sort
    LUA_FUNCTION(concat);     // table.concat
    
    // === 高级表操作 ===
    LUA_FUNCTION(maxn);       // table.maxn
    LUA_FUNCTION(foreach);    // table.foreach
    LUA_FUNCTION(foreachi);   // table.foreachi
};
```

---

## 🚀 高级特性设计

### 1. 插件系统 (Phase 2)

```cpp
namespace Lua {
namespace Lib {

/**
 * 插件管理器 - 支持动态加载第三方库
 */
class PluginManager {
public:
    // === 插件加载 ===
    bool loadPlugin(StrView path);
    bool loadPluginFromMemory(const void* data, size_t size);
    void unloadPlugin(StrView name);
    
    // === 插件查询 ===
    bool isPluginLoaded(StrView name) const;
    std::vector<StrView> getLoadedPlugins() const;
    PluginInfo getPluginInfo(StrView name) const;
    
    // === 插件安全 ===
    void setSandboxMode(bool enable);
    void setPluginPermissions(StrView name, const PermissionSet& permissions);
    
private:
    std::unordered_map<Str, std::shared_ptr<Plugin>> plugins_;
    bool sandboxMode_ = false;
};

}} // namespace Lua::Lib
```

### 2. 异步支持 (Phase 3)

```cpp
namespace Lua {
namespace Lib {

/**
 * 异步函数支持
 */
class AsyncSupport {
public:
    // 创建异步调用
    template<typename F>
    static Value createAsyncCall(State* state, F&& func);
    
    // 等待异步结果
    static Value awaitResult(State* state, AsyncHandle handle);
    
    // 异步回调
    static void setAsyncCallback(AsyncHandle handle, LibFunction callback);
};

// 异步函数宏
#define LUA_ASYNC_FUNCTION(name) \
    static Value name##_async(State* state, i32 nargs); \
    static Value name(State* state, i32 nargs) { \
        return AsyncSupport::createAsyncCall(state, name##_async); \
    } \
    static Value name##_async(State* state, i32 nargs)

}} // namespace Lua::Lib
```

### 3. 性能监控系统

```cpp
namespace Lua {
namespace Lib {

/**
 * 性能监控和统计
 */
class PerformanceMonitor {
public:
    // === 性能统计 ===
    void recordFunctionCall(StrView funcName, std::chrono::nanoseconds duration);
    void recordMemoryUsage(StrView moduleName, size_t bytes);
    void recordErrorCount(StrView moduleName, ErrorType type);
    
    // === 统计查询 ===
    FunctionStats getFunctionStats(StrView funcName) const;
    ModuleStats getModuleStats(StrView moduleName) const;
    GlobalStats getGlobalStats() const;
    
    // === 性能分析 ===
    std::vector<StrView> getSlowFunctions(f64 threshold) const;
    std::vector<StrView> getMemoryHungryModules() const;
    std::vector<StrView> getErrorProneModules() const;
    
    // === 报告生成 ===
    Str generatePerformanceReport() const;
    void exportStatsToFile(StrView filename) const;
};

}} // namespace Lua::Lib
```

---

## 📋 实施计划

### Phase 1: 核心框架完善 (2周)

**Week 1**: 
- ✅ 完善 BaseLib 核心函数实现 (print, type, tostring, tonumber, error, assert)
- ✅ 修复编译依赖问题
- ✅ 建立基础测试用例

**Week 2**:
- 🔄 实现 BaseLib 表操作函数 (pairs, ipairs, next)
- 🔄 完善 StringLib 重构
- 🔄 集成测试和性能验证

### Phase 2: 标准库完善 (4周)

**Week 3-4**: 
- TableLib 完整实现
- MathLib 重构和优化
- 错误处理框架完善

**Week 5-6**:
- IOLib 核心功能实现
- OSLib 基础功能实现
- 性能优化和内存管理

### Phase 3: 高级特性 (3周)

**Week 7-8**:
- 插件系统实现
- 异步支持机制
- 性能监控系统

**Week 9**:
- 全面测试和文档完善
- 性能基准测试
- 向后兼容性验证

---

## 🎯 成功指标

### 技术指标
- **编译成功率**: 100%
- **测试覆盖率**: ≥ 90%
- **性能提升**: 相比重构前提升 ≥ 20%
- **内存使用**: 优化 ≥ 15%

### 架构指标
- **模块独立性**: 每个库模块可独立编译
- **依赖清晰性**: 依赖关系图清晰无环
- **扩展便利性**: 新增库模块 ≤ 100行样板代码
- **配置灵活性**: 支持运行时配置热更新

### 质量指标
- **代码规范**: 100% 符合项目编码规范
- **文档完整性**: 所有公开API都有完整文档
- **向后兼容**: 100% 兼容现有Lua 5.1脚本
- **错误处理**: 所有错误都有明确的错误码和消息

---

**最后更新**: 2025年7月6日  
**设计负责人**: AI Assistant  
**状态**: 🔄 设计完成，准备实施
