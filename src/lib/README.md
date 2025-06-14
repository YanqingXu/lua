# Lua 标准库框架

本目录包含了 Lua 解释器的标准库实现，采用现代 C++ 设计模式，提供了模块化、可扩展的库管理系统。

## 架构概览

### 核心组件

1. **lib_common.hpp** - 标准库公共接口和宏定义
2. **lib_utils.hpp/cpp** - 标准库工具函数和辅助类
3. **lib_manager.hpp/cpp** - 库管理器，负责库的注册、加载和管理
4. **lib_init.hpp/cpp** - 库初始化接口和配置
5. **base_lib.hpp/cpp** - 基础库实现
6. **string_lib.hpp/cpp** - 字符串库实现（示例）

### 设计模式

#### 1. 模块接口模式

所有标准库模块都继承自 `LibModule` 接口：

```cpp
class LibModule {
public:
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual void registerModule(State* state) = 0;
    virtual bool isLoaded() const { return loaded_; }
    // ...
};
```

#### 2. 单例管理器模式

`LibManager` 采用单例模式管理所有库模块：

```cpp
LibManager& manager = LibManager::getInstance();
manager.registerLib(std::make_unique<BaseLib>());
manager.loadLib("base", state);
```

#### 3. 工厂注册模式

使用宏简化库的注册：

```cpp
REGISTER_LIB(BaseLib);
REGISTER_LIB(StringLib);
```

## 使用指南

### 创建新的库模块

1. **继承 LibModule 接口**：

```cpp
class MyLib : public LibModule {
public:
    std::string getName() const override { return "mylib"; }
    std::string getVersion() const override { return "1.0.0"; }
    void registerModule(State* state) override;
    
    // 库函数声明
    static Value myFunction(State* state, int nargs);
};
```

2. **实现库函数**：

```cpp
Value MyLib::myFunction(State* state, int nargs) {
    LibUtils::ArgChecker checker(state, nargs);
    
    if (!checker.checkMinArgs(1)) {
        return Value(nullptr);
    }
    
    auto val = checker.getValue();
    // 函数逻辑...
    
    return Value(result);
}
```

3. **注册库函数**：

```cpp
void MyLib::registerModule(State* state) {
    registerFunction(state, "myFunction", myFunction);
    setLoaded(true);
}
```

4. **注册库模块**：

```cpp
REGISTER_LIB(MyLib);
```

### 初始化标准库

#### 基本初始化

```cpp
// 初始化核心库
Lib::initCoreLibraries(state);

// 初始化所有库
Lib::initAllLibraries(state);
```

#### 高级初始化

```cpp
// 使用配置选项
InitOptions options;
options.loadCoreLibs = true;
options.loadExtendedLibs = false;
options.safeMode = true;

Lib::initLibrariesWithOptions(state, options);
```

### 工具函数使用

#### 参数检查

```cpp
LibUtils::ArgChecker checker(state, nargs);

// 检查参数数量
if (!checker.checkMinArgs(2)) {
    return Value(nullptr);
}

// 检查参数类型
if (!checker.checkType(1, ValueType::String)) {
    return Value(nullptr);
}

// 获取参数值
auto val = checker.getValue();
```

#### 类型转换

```cpp
// 值转字符串
std::string str = LibUtils::Convert::toString(value);

// 值转数字
auto num = LibUtils::Convert::toNumber(value);
if (num) {
    double result = *num;
}
```

#### 字符串操作

```cpp
// 字符串工具
bool starts = LibUtils::String::startsWith(str, "prefix");
std::string trimmed = LibUtils::String::trim(str);
auto parts = LibUtils::String::split(str, ",");
```

#### 表操作

```cpp
// 表工具
size_t length = LibUtils::TableUtils::getArrayLength(table);
auto keys = LibUtils::TableUtils::getKeys(table);
bool empty = LibUtils::TableUtils::isEmpty(table);
```

#### 错误处理

```cpp
// 抛出错误
LibUtils::Error::throwError(state, "error message");
LibUtils::Error::throwTypeError(state, 1, "string", "number");
LibUtils::Error::throwArgError(state, 2, "invalid value");
```

## 库模块列表

### 已实现的库

- **base** - 基础库（print, type, tonumber, tostring 等）
- **string** - 字符串库（len, sub, find, gsub 等）

### 计划实现的库

- **table** - 表操作库
- **math** - 数学函数库
- **io** - 输入输出库
- **os** - 操作系统库
- **debug** - 调试库
- **coroutine** - 协程库

## 配置选项

### 初始化选项

```cpp
struct InitOptions {
    bool loadCoreLibs = true;        // 加载核心库
    bool loadExtendedLibs = true;    // 加载扩展库
    bool loadAdvancedLibs = false;   // 加载高级库
    bool safeMode = false;           // 安全模式
    bool sandboxMode = false;        // 沙箱模式
    std::vector<std::string> excludeLibs;  // 排除的库
    std::vector<std::string> includeLibs;  // 包含的库
};
```

### 库配置

```cpp
// 设置库搜索路径
Lib::Config::setLibraryPath("/path/to/libs");

// 设置加载超时
Lib::Config::setLoadTimeout(5000);

// 启用日志
Lib::Config::enableLogging(true);
```

## 性能考虑

1. **延迟加载** - 库只在需要时才加载
2. **缓存机制** - 已加载的库会被缓存
3. **依赖管理** - 自动处理库之间的依赖关系
4. **内存管理** - 使用智能指针管理库生命周期

## 扩展性

1. **插件系统** - 支持动态加载外部库
2. **版本管理** - 支持库的版本控制
3. **兼容性检查** - 自动检查库的兼容性
4. **热重载** - 支持库的热重载（开发模式）

## 安全性

1. **沙箱模式** - 限制库的访问权限
2. **安全模式** - 禁用危险的库函数
3. **权限控制** - 细粒度的权限管理
4. **输入验证** - 严格的参数验证

## 调试支持

1. **详细日志** - 库加载和执行的详细日志
2. **错误追踪** - 完整的错误堆栈信息
3. **性能分析** - 库函数的性能统计
4. **内存监控** - 库的内存使用监控

## 最佳实践

1. **错误处理** - 始终检查参数和返回值
2. **内存管理** - 正确管理 C++ 对象的生命周期
3. **异常安全** - 确保异常安全的代码
4. **文档编写** - 为每个库函数编写详细文档
5. **单元测试** - 为每个库函数编写单元测试

## 示例代码

查看 `string_lib.cpp` 了解完整的库实现示例。

## 标准库框架重构计划

### 重构背景

当前标准库框架虽然功能完整，但存在以下问题需要改进：

1. **架构复杂度高** - 多层抽象导致理解和维护困难
2. **性能考虑不足** - 函数查找和调用存在性能瓶颈
3. **扩展性限制** - 难以支持动态库加载和第三方扩展
4. **错误处理不完善** - 缺乏统一的错误处理机制

### 重构目标

- **简化架构** - 减少不必要的抽象层次
- **提升性能** - 优化函数注册和调用机制
- **增强扩展性** - 支持插件化和动态加载
- **完善错误处理** - 建立统一的错误处理框架
- **现代化代码** - 采用更多现代C++特性

### 重构计划

#### 第一阶段：接口简化与性能优化（1-2个月）

**目标**：简化核心接口，优化基础性能

**主要任务**：

1. **重新设计LibModule接口**
   ```cpp
   // 新的简化接口
   class LibModule {
   public:
       virtual ~LibModule() = default;
       virtual std::string_view getName() const noexcept = 0;
       virtual void registerFunctions(FunctionRegistry& registry) = 0;
   };
   ```

2. **重构LibManager架构**
   - 移除单例模式，改为依赖注入
   - 使用std::unordered_map优化函数查找
   - 实现函数调用缓存机制

3. **优化函数注册机制**
   ```cpp
   // 新的注册方式
   class FunctionRegistry {
   public:
       template<typename F>
       void registerFunction(std::string_view name, F&& func);
       
       Value callFunction(std::string_view name, State* state, int nargs);
   private:
       std::unordered_map<std::string, std::function<Value(State*, int)>> functions_;
   };
   ```

4. **简化宏定义**
   ```cpp
   #define REGISTER_FUNCTION(name, func) \
       registry.registerFunction(#name, [](State* s, int n) { return func(s, n); })
   ```

**里程碑**：
- [ ] 完成新接口设计和实现
- [ ] 重构现有base_lib和string_lib
- [ ] 性能测试显示20%以上的性能提升
- [ ] 所有现有测试通过

#### 第二阶段：扩展性增强与错误处理（2-3个月）

**目标**：建立插件化架构，完善错误处理

**主要任务**：

1. **设计插件接口标准**
   ```cpp
   class LibraryPlugin {
   public:
       virtual ~LibraryPlugin() = default;
       virtual std::string_view getName() const = 0;
       virtual std::string_view getVersion() const = 0;
       virtual bool isCompatible(const Version& interpreterVersion) const = 0;
       virtual void initialize(PluginContext& context) = 0;
       virtual void cleanup() = 0;
   };
   ```

2. **实现动态库加载**
   ```cpp
   class PluginManager {
   public:
       bool loadPlugin(const std::filesystem::path& pluginPath);
       void unloadPlugin(std::string_view name);
       std::vector<std::string> getLoadedPlugins() const;
   private:
       std::unordered_map<std::string, std::unique_ptr<PluginHandle>> plugins_;
   };
   ```

3. **建立统一错误处理框架**
   ```cpp
   enum class LibErrorCode {
       Success = 0,
       InvalidArgument,
       TypeMismatch,
       OutOfRange,
       RuntimeError,
       SystemError
   };
   
   class LibException : public std::exception {
   public:
       LibException(LibErrorCode code, std::string message, 
                   std::source_location loc = std::source_location::current());
       
       LibErrorCode getErrorCode() const noexcept { return code_; }
       const std::string& getMessage() const noexcept { return message_; }
       const std::source_location& getLocation() const noexcept { return location_; }
   };
   ```

4. **实现库的热重载机制**
   ```cpp
   class HotReloadManager {
   public:
       void enableHotReload(std::string_view libraryName);
       void disableHotReload(std::string_view libraryName);
       bool reloadLibrary(std::string_view libraryName);
   };
   ```

**里程碑**：
- [ ] 完成插件接口设计和实现
- [ ] 实现至少一个外部插件示例
- [ ] 建立完整的错误处理机制
- [ ] 实现热重载功能

#### 第三阶段：性能优化与现代化（3-4个月）

**目标**：全面性能优化，代码现代化

**主要任务**：

1. **函数调用优化**
   ```cpp
   // 使用concepts进行类型约束
   template<typename F>
   concept LibraryFunction = requires(F f, State* s, int n) {
       { f(s, n) } -> std::convertible_to<Value>;
   };
   
   // 编译时函数注册
   template<LibraryFunction F>
   constexpr void registerFunction(std::string_view name, F&& func);
   ```

2. **内存管理优化**
   ```cpp
   class MemoryPool {
   public:
       template<typename T, typename... Args>
       T* allocate(Args&&... args);
       
       void deallocate(void* ptr);
       void reset();
   };
   ```

3. **缓存机制实现**
   ```cpp
   class FunctionCache {
   public:
       Value getCachedResult(std::string_view funcName, 
                           const std::vector<Value>& args);
       void cacheResult(std::string_view funcName, 
                       const std::vector<Value>& args, 
                       const Value& result);
   };
   ```

4. **引入现代C++特性**
   - 使用std::span替代原始指针
   - 使用std::optional处理可选返回值
   - 使用std::expected处理错误（C++23）
   - 使用ranges库简化算法操作

**里程碑**：
- [ ] 完成所有性能优化
- [ ] 代码现代化完成
- [ ] 建立完整的性能测试套件
- [ ] 性能相比原版提升50%以上

### 质量保证措施

#### 测试策略

1. **单元测试**
   - 每个新功能都要有对应的单元测试
   - 测试覆盖率要求达到90%以上
   - 使用Google Test框架

2. **集成测试**
   - 测试库之间的交互
   - 测试插件加载和卸载
   - 测试热重载功能

3. **性能测试**
   - 建立基准测试套件
   - 每次重构后进行性能回归测试
   - 使用Google Benchmark框架

4. **兼容性测试**
   - 确保向后兼容性
   - 测试不同平台的兼容性
   - 测试不同编译器的兼容性

#### 代码审查

1. **审查流程**
   - 所有代码变更都需要代码审查
   - 至少需要两个审查者批准
   - 自动化代码质量检查

2. **审查标准**
   - 代码风格一致性
   - 性能影响评估
   - 安全性检查
   - 文档完整性

#### 风险管理

1. **回滚计划**
   - 每个阶段都有明确的回滚点
   - 保持旧版本的兼容性
   - 建立快速回滚机制

2. **渐进式迁移**
   - 支持新旧接口并存
   - 提供迁移工具和指南
   - 逐步废弃旧接口

### 实施时间线

```
月份    1    2    3    4    5    6    7
阶段一  [====接口简化====][性能优化]
阶段二       [====扩展性====][错误处理]
阶段三            [====性能优化====][现代化]
测试    [==========持续测试==========]
文档    [==========文档更新==========]
```

### 成功指标

1. **性能指标**
   - 函数调用性能提升50%以上
   - 内存使用减少30%以上
   - 库加载时间减少40%以上

2. **质量指标**
   - 代码覆盖率达到90%以上
   - 静态分析无严重问题
   - 所有平台编译通过

3. **可用性指标**
   - API简化程度（减少50%的样板代码）
   - 文档完整性（100%的API有文档）
   - 示例代码覆盖率（所有功能都有示例）

### 后续维护

1. **持续集成**
   - 自动化构建和测试
   - 性能回归检测
   - 代码质量监控

2. **版本管理**
   - 语义化版本控制
   - 变更日志维护
   - 兼容性矩阵

3. **社区支持**
   - 开发者文档
   - 示例和教程
   - 问题跟踪和支持

## 贡献指南

1. 遵循现有的代码风格
2. 添加适当的错误处理
3. 编写单元测试
4. 更新文档
5. 提交 Pull Request
6. **新增**：参与重构计划的开发者请先阅读重构计划文档
7. **新增**：重构相关的PR需要额外的性能测试报告