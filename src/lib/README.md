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

## 贡献指南

1. 遵循现有的代码风格
2. 添加适当的错误处理
3. 编写单元测试
4. 更新文档
5. 提交 Pull Request