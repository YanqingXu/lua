# Lua插件系统

## 概述

Lua插件系统为Lua解释器提供了强大的扩展能力，支持动态加载、热重载、安全沙箱、依赖管理等高级特性。该系统基于现有的LibModule架构扩展，保持向后兼容性的同时提供了更丰富的功能。

## 特性

### 核心特性
- **动态加载**: 支持运行时加载和卸载插件
- **热重载**: 支持插件的热更新，无需重启应用
- **依赖管理**: 自动解析和管理插件依赖关系
- **版本控制**: 支持插件版本管理和兼容性检查
- **安全沙箱**: 提供资源限制和权限控制
- **插件间通信**: 支持插件之间的消息传递
- **配置管理**: 灵活的插件配置系统
- **性能监控**: 实时监控插件性能和资源使用
- **错误隔离**: 插件错误不会影响主程序稳定性

### 高级特性
- **事件系统**: 插件生命周期事件通知
- **审计日志**: 详细的操作日志记录
- **调试支持**: 丰富的调试和诊断工具
- **批量操作**: 支持批量加载、卸载插件
- **查询系统**: 强大的插件查询和过滤功能

## 架构设计

### 核心组件

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   PluginSystem  │────│ PluginManager   │────│  PluginLoader   │
│   (统一接口)     │    │   (生命周期)     │    │   (动态加载)     │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         │                       │                       │
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│ PluginRegistry  │    │ PluginContext   │    │ PluginSandbox   │
│   (注册管理)     │    │   (运行环境)     │    │   (安全隔离)     │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

### 类层次结构

```cpp
IPlugin (接口)
├── LibModule (基类)
└── 具体插件实现
    ├── ExamplePlugin
    ├── MathPlugin
    └── StringPlugin

IPluginFactory (工厂接口)
└── TypedPluginFactory<T> (模板工厂)
    ├── ExamplePluginFactory
    ├── MathPluginFactory
    └── StringPluginFactory
```

## 快速开始

### 1. 基本使用

```cpp
#include "plugin/plugin.hpp"

// 创建插件系统
auto pluginSystem = Lua::PluginSystemFactory::create(luaState, libManager);

// 初始化
Lua::PluginSystemConfig config = Lua::PluginSystemFactory::createDefaultConfig();
if (!pluginSystem->initialize(config)) {
    // 处理初始化错误
    std::cerr << "Failed to initialize plugin system: " 
              << pluginSystem->getLastError() << std::endl;
    return false;
}

// 扫描可用插件
auto availablePlugins = pluginSystem->scanPlugins();
std::cout << "Found " << availablePlugins.size() << " plugins" << std::endl;

// 加载插件
if (pluginSystem->loadPlugin("ExamplePlugin")) {
    std::cout << "Plugin loaded successfully" << std::endl;
} else {
    std::cerr << "Failed to load plugin: " 
              << pluginSystem->getLastError() << std::endl;
}

// 使用插件
auto plugin = pluginSystem->getPlugin("ExamplePlugin");
if (plugin && pluginSystem->isPluginEnabled("ExamplePlugin")) {
    // 插件已加载并启用
}

// 关闭系统
pluginsystem->shutdown();
```

### 2. 开发插件

#### 创建插件类

```cpp
#include "plugin/plugin_interface.hpp"

class MyPlugin : public Lua::IPlugin {
public:
    // 实现必需的接口方法
    Lua::StrView getName() const override {
        return "MyPlugin";
    }
    
    void registerFunctions(Lua::FunctionRegistry& registry) override {
        // 注册Lua函数
        registry.registerFunction("my_function", myLuaFunction);
    }
    
    bool onLoad(Lua::PluginContext* context) override {
        context_ = context;
        context_->logInfo("MyPlugin loaded");
        return true;
    }
    
    bool onUnload(Lua::PluginContext* context) override {
        context_->logInfo("MyPlugin unloaded");
        return true;
    }
    
    // 实现其他接口方法...
    
private:
    Lua::PluginContext* context_ = nullptr;
    
    static int myLuaFunction(lua_State* L) {
        // Lua函数实现
        lua_pushstring(L, "Hello from MyPlugin!");
        return 1;
    }
};
```

#### 创建插件工厂

```cpp
class MyPluginFactory : public Lua::IPluginFactory {
public:
    Lua::UPtr<Lua::IPlugin> createPlugin() override {
        return std::make_unique<MyPlugin>();
    }
    
    Lua::StrView getPluginName() const override {
        return "MyPlugin";
    }
    
    Lua::PluginVersion getPluginVersion() const override {
        return {1, 0, 0};
    }
};
```

#### 注册插件

```cpp
// 静态注册（编译时）
LUA_REGISTER_STATIC_PLUGIN(MyPlugin, new MyPluginFactory());

// 或者动态注册（运行时）
auto factory = std::make_unique<MyPluginFactory>();
pluginsystem->getPluginManager()->registerFactory("MyPlugin", std::move(factory));
```

### 3. 高级配置

#### 自定义配置

```cpp
Lua::PluginSystemConfig config;

// 启用安全沙箱
config.enableSandbox = true;
config.strictMode = true;

// 配置搜索路径
config.searchPaths.systemPaths.push_back("./plugins");
config.searchPaths.userPaths.push_back("~/.myapp/plugins");

// 设置资源限制
config.defaultLimits.maxMemoryUsage = 32 * 1024 * 1024; // 32MB
config.defaultLimits.maxExecutionTime = 5000; // 5秒

// 配置权限
config.defaultPermissions.permissions[Lua::PermissionType::FileRead] = true;
config.defaultPermissions.permissions[Lua::PermissionType::NetworkAccess] = false;

// 启用热重载
config.enableHotReload = true;

// 初始化系统
pluginsystem->initialize(config);
```

#### 插件加载选项

```cpp
Lua::PluginLoadOptions options;
options.enableSandbox = true;
options.checkDependencies = true;
options.autoLoadDependencies = true;
options.permissions = {"file_read", "network_access"};
options.config = {{"debug", "true"}, {"log_level", "info"}};

pluginsystem->loadPlugin("MyPlugin", options);
```

## API参考

### PluginSystem类

主要的插件系统接口，提供统一的插件管理功能。

#### 核心方法

```cpp
// 系统生命周期
bool initialize(const PluginSystemConfig& config = {});
void shutdown();
PluginSystemState getState() const;

// 插件管理
bool loadPlugin(StrView name, const PluginLoadOptions& options = {});
bool unloadPlugin(StrView name);
bool reloadPlugin(StrView name);
bool enablePlugin(StrView name);
bool disablePlugin(StrView name);

// 查询
IPlugin* getPlugin(StrView name) const;
bool isPluginLoaded(StrView name) const;
Vec<Str> getLoadedPlugins() const;
Vec<PluginMetadata> getAvailablePlugins() const;

// 批量操作
Vec<Str> loadPlugins(const Vec<Str>& names, const PluginLoadOptions& options = {});
Vec<Str> autoLoadPlugins(const PluginLoadOptions& options = {});
void unloadAllPlugins();
```

### IPlugin接口

所有插件必须实现的基础接口。

```cpp
class IPlugin : public LibModule {
public:
    // 生命周期方法
    virtual bool onLoad(PluginContext* context) = 0;
    virtual bool onUnload(PluginContext* context) = 0;
    virtual bool onEnable(PluginContext* context) = 0;
    virtual bool onDisable(PluginContext* context) = 0;
    
    // 状态管理
    virtual PluginState getState() const = 0;
    virtual void setState(PluginState state) = 0;
    
    // 元数据
    virtual const PluginMetadata& getMetadata() const = 0;
    
    // 配置
    virtual bool configure(const HashMap<Str, Str>& config) = 0;
    virtual HashMap<Str, Str> getConfiguration() const = 0;
    
    // 消息处理
    virtual bool handleMessage(StrView message, const HashMap<Str, Str>& data) = 0;
};
```

### PluginContext类

为插件提供运行时环境和服务。

```cpp
// 基础服务
State* getLuaState() const;
PluginManager* getPluginManager() const;
StrView getPluginName() const;

// 日志服务
void log(PluginLogLevel level, StrView message);
void logInfo(StrView message);
void logError(StrView message);

// 配置服务
Str getConfig(StrView key, StrView defaultValue = "") const;
void setConfig(StrView key, StrView value);
bool saveConfig();

// 插件间通信
IPlugin* findPlugin(StrView name) const;
bool sendMessage(StrView targetPlugin, StrView message, const HashMap<Str, Str>& data = {});

// 函数注册
template<typename F>
void registerFunction(StrView name, F&& func);
void unregisterFunction(StrView name);
```

## 示例

### 数学插件示例

```cpp
class MathPlugin : public Lua::IPlugin {
public:
    void registerFunctions(Lua::FunctionRegistry& registry) override {
        registry.registerFunction("factorial", luaFactorial);
        registry.registerFunction("fibonacci", luaFibonacci);
        registry.registerFunction("is_prime", luaIsPrime);
    }
    
private:
    static int luaFactorial(lua_State* L) {
        int n = luaL_checkinteger(L, 1);
        if (n < 0) {
            return luaL_error(L, "factorial: negative number");
        }
        
        long long result = 1;
        for (int i = 2; i <= n; ++i) {
            result *= i;
        }
        
        lua_pushinteger(L, result);
        return 1;
    }
    
    static int luaFibonacci(lua_State* L) {
        int n = luaL_checkinteger(L, 1);
        if (n < 0) {
            return luaL_error(L, "fibonacci: negative number");
        }
        
        if (n <= 1) {
            lua_pushinteger(L, n);
            return 1;
        }
        
        long long a = 0, b = 1;
        for (int i = 2; i <= n; ++i) {
            long long temp = a + b;
            a = b;
            b = temp;
        }
        
        lua_pushinteger(L, b);
        return 1;
    }
    
    static int luaIsPrime(lua_State* L) {
        int n = luaL_checkinteger(L, 1);
        if (n < 2) {
            lua_pushboolean(L, 0);
            return 1;
        }
        
        for (int i = 2; i * i <= n; ++i) {
            if (n % i == 0) {
                lua_pushboolean(L, 0);
                return 1;
            }
        }
        
        lua_pushboolean(L, 1);
        return 1;
    }
};
```

### Lua使用示例

```lua
-- 加载数学插件后，可以在Lua中使用
print(factorial(5))  -- 输出: 120
print(fibonacci(10)) -- 输出: 55
print(is_prime(17))  -- 输出: true
```

## 最佳实践

### 1. 插件设计原则

- **单一职责**: 每个插件应该专注于一个特定的功能领域
- **最小依赖**: 尽量减少对外部库的依赖
- **错误处理**: 妥善处理所有可能的错误情况
- **资源管理**: 正确管理内存和其他资源
- **线程安全**: 考虑多线程环境下的安全性

### 2. 性能优化

- **延迟加载**: 只在需要时加载插件
- **缓存机制**: 合理使用缓存提高性能
- **资源限制**: 设置合适的资源限制
- **批量操作**: 使用批量操作减少开销

### 3. 安全考虑

- **沙箱隔离**: 在生产环境中启用沙箱
- **权限控制**: 遵循最小权限原则
- **输入验证**: 验证所有外部输入
- **审计日志**: 记录重要操作

### 4. 调试技巧

- **详细日志**: 使用详细的日志记录
- **调试模式**: 在开发时启用调试模式
- **错误追踪**: 保留错误历史记录
- **性能监控**: 监控插件性能指标

## 故障排除

### 常见问题

1. **插件加载失败**
   - 检查插件文件是否存在
   - 验证插件接口实现
   - 查看错误日志

2. **依赖解析失败**
   - 检查依赖插件是否可用
   - 验证版本兼容性
   - 检查循环依赖

3. **权限拒绝**
   - 检查沙箱配置
   - 验证权限设置
   - 查看审计日志

4. **性能问题**
   - 检查资源使用统计
   - 优化插件代码
   - 调整资源限制

### 调试工具

```cpp
// 获取系统诊断信息
auto diagnostics = pluginsystem->getDiagnostics();
for (const auto& [key, value] : diagnostics) {
    std::cout << key << ": " << value << std::endl;
}

// 获取插件统计
auto stats = pluginsystem->getPluginStatistics();
std::cout << "Total plugins: " << stats.totalPlugins << std::endl;
std::cout << "Loaded plugins: " << stats.loadedPlugins << std::endl;

// 获取性能统计
auto perfStats = pluginsystem->getPerformanceStatistics();
for (const auto& [plugin, metrics] : perfStats) {
    std::cout << "Plugin: " << plugin << std::endl;
    for (const auto& [metric, value] : metrics) {
        std::cout << "  " << metric << ": " << value << std::endl;
    }
}
```

## 版本历史

### v1.0.0 (当前版本)
- 初始版本发布
- 基础插件系统实现
- 动态加载和热重载支持
- 安全沙箱和权限控制
- 依赖管理和版本控制
- 插件间通信机制
- 配置管理系统
- 性能监控和错误处理

## 贡献指南

欢迎贡献代码和建议！请遵循以下步骤：

1. Fork项目
2. 创建功能分支
3. 编写测试
4. 提交更改
5. 创建Pull Request

## 许可证

本项目采用MIT许可证，详见LICENSE文件。