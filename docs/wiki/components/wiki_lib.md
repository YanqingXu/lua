# Lua 标准库框架详解

## 概述

Lua 标准库框架是解释器中负责管理和提供标准库功能的核心系统，采用现代 C++ 设计模式，实现了模块化、可扩展、高性能的库管理机制。本文档详细介绍了库框架的设计架构、核心组件、使用方法以及如何添加新的库模块。

## 系统架构

### 核心设计理念

#### 1. 模块化设计
- **接口统一**: 所有库模块都实现统一的 `LibModule` 接口
- **职责分离**: 库管理、初始化、工具函数分离到不同组件
- **松耦合**: 库模块之间相互独立，可单独加载和卸载

#### 2. 工厂模式
- **动态创建**: 使用工厂函数动态创建库实例
- **延迟加载**: 库只在需要时才被创建和加载
- **内存效率**: 避免不必要的库实例化

#### 3. 单例管理
- **全局管理**: `LibManager` 采用单例模式管理所有库
- **状态一致**: 确保库的加载状态在整个系统中一致
- **线程安全**: 支持多线程环境下的安全访问

### 架构图

```
┌─────────────────────────────────────────────────────────────────┐
│                    Lua 标准库框架架构                            │
├─────────────────────────────────────────────────────────────────┤
│  应用层 (Application Layer)                                     │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐            │
│  │   Lexer     │  │   Parser    │  │   Runtime   │            │
│  │   词法分析   │  │   语法分析   │  │   运行时     │            │
│  └─────────────┘  └─────────────┘  └─────────────┘            │
├─────────────────────────────────────────────────────────────────┤
│  初始化层 (Initialization Layer)                                │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                    LibInit                                  ││
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        ││
│  │  │initCore()   │  │initAll()    │  │initCustom() │        ││
│  │  └─────────────┘  └─────────────┘  └─────────────┘        ││
│  └─────────────────────────────────────────────────────────────┘│
├─────────────────────────────────────────────────────────────────┤
│  管理层 (Management Layer)                                      │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                   LibManager                                ││
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        ││
│  │  │register()   │  │load()       │  │unload()     │        ││
│  │  └─────────────┘  └─────────────┘  └─────────────┘        ││
│  └─────────────────────────────────────────────────────────────┘│
├─────────────────────────────────────────────────────────────────┤
│  库模块层 (Library Module Layer)                                │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────┐│
│  │   BaseLib   │  │ StringLib   │  │  TableLib   │  │   ...   ││
│  │             │  │             │  │             │  │         ││
│  │ • print()   │  │ • len()     │  │ • insert()  │  │         ││
│  │ • type()    │  │ • sub()     │  │ • remove()  │  │         ││
│  │ • tonumber()│  │ • find()    │  │ • sort()    │  │         ││
│  └─────────────┘  └─────────────┘  └─────────────┘  └─────────┘│
├─────────────────────────────────────────────────────────────────┤
│  工具层 (Utility Layer)                                         │
│  ┌─────────────────────┐    ┌─────────────────────────────────┐ │
│  │     LibUtils        │    │        LibCommon                │ │
│  │                     │    │                                 │ │
│  │ • ArgChecker        │    │ • LibModule 接口                │ │
│  │ • Convert           │    │ • LibFunction 类型              │ │
│  │ • String            │    │ • 错误处理宏                     │ │
│  │ • Error             │    │ • 参数检查宏                     │ │
│  └─────────────────────┘    └─────────────────────────────────┘ │
├─────────────────────────────────────────────────────────────────┤
│  基础层 (Foundation Layer)                                      │
│  ┌─────────────────────┐    ┌─────────────────────────────────┐ │
│  │       State         │    │         Value                   │ │
│  │     虚拟机状态       │    │        值系统                    │ │
│  └─────────────────────┘    └─────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

## 核心组件详解

### 1. LibModule 接口

#### 接口定义

```cpp
class LibModule {
public:
    virtual ~LibModule() = default;
    
    // 获取模块名称
    virtual const Str& getName() const = 0;
    
    // 注册模块函数到状态
    virtual void registerModule(State* state) = 0;
    
    // 获取模块版本
    virtual const Str& getVersion() const {
        static const Str version = "1.0.0";
        return version;
    }
    
    // 检查模块是否已加载
    virtual bool isLoaded() const { return loaded_; }
    
protected:
    // 注册函数的辅助方法
    void registerFunction(State* state, const Str& name, LibFunction func);
    void registerFunction(State* state, Value table, const Str& name, LibFunction func);
    
    // 标记模块为已加载
    void setLoaded(bool loaded) { loaded_ = loaded; }
    
private:
    bool loaded_ = false;
};
```

#### 设计特点

**1. 纯虚接口**
- **强制实现**: 所有库必须实现核心方法
- **类型安全**: 编译时检查接口完整性
- **多态支持**: 支持运行时多态调用

**2. 辅助方法**
- **简化注册**: 提供便捷的函数注册方法
- **统一格式**: 确保所有库使用相同的注册方式
- **错误处理**: 内置错误处理机制

**3. 状态管理**
- **加载状态**: 跟踪模块的加载状态
- **生命周期**: 管理模块的完整生命周期
- **依赖检查**: 支持模块间的依赖检查

### 2. LibManager 库管理器

#### 核心功能

```cpp
class LibManager {
public:
    // 单例访问
    static LibManager& getInstance();
    
    // 注册库模块
    void registerLibrary(const Str& name, std::function<UPtr<LibModule>()> factory);
    
    // 加载特定库
    bool loadLibrary(State* state, const Str& name);
    
    // 加载所有库
    void loadAllLibraries(State* state);
    
    // 加载核心库
    void loadCoreLibraries(State* state);
    
    // 加载扩展库
    void loadExtendedLibraries(State* state);
    
    // 检查库状态
    bool isRegistered(const Str& name) const;
    bool isLoaded(const Str& name) const;
    
    // 获取库信息
    Vec<Str> getRegisteredLibraries() const;
    Vec<Str> getLoadedLibraries() const;
    LibModule* getModule(const Str& name);
    
    // 卸载库
    void unloadLibrary(const Str& name);
    void unloadAllLibraries();
    
private:
    // 注册的库工厂
    HashMap<Str, std::function<UPtr<LibModule>()>> libraries_;
    
    // 已加载的模块
    HashMap<Str, UPtr<LibModule>> loaded_modules_;
};
```

#### 管理机制

**1. 工厂注册**

```cpp
// 注册库工厂函数
void registerLibrary(const Str& name, std::function<UPtr<LibModule>()> factory) {
    libraries_[name] = factory;
}

// 使用宏简化注册
#define REGISTER_LIB(name, class_name) \
    LibManager::getInstance().registerLibrary(name, []() -> UPtr<LibModule> { \
        return std::make_unique<class_name>(); \
    })
```

**2. 延迟加载**

```cpp
bool loadLibrary(State* state, const Str& name) {
    auto it = libraries_.find(name);
    if (it == libraries_.end()) {
        return false; // 库未注册
    }
    
    // 检查是否已加载
    auto loaded_it = loaded_modules_.find(name);
    if (loaded_it != loaded_modules_.end()) {
        return true; // 已加载
    }
    
    // 创建并注册模块
    auto module = it->second();
    if (module) {
        module->registerModule(state);
        loaded_modules_[name] = std::move(module);
        return true;
    }
    
    return false;
}
```

**3. 优先级加载**

```cpp
// 库加载优先级
int getLibraryPriority(const std::string& name) {
    static const std::unordered_map<std::string, int> priorities = {
        {"base", 1},     // 基础库优先
        {"string", 2},  // 字符串库
        {"table", 3},   // 表库
        {"math", 4},    // 数学库
        {"io", 5},      // IO库
        {"os", 6},      // OS库
        {"debug", 7},   // 调试库
        {"coroutine", 8}, // 协程库
        {"package", 9}  // 包管理库
    };
    
    auto it = priorities.find(name);
    return (it != priorities.end()) ? it->second : 100;
}
```

### 3. LibInit 初始化系统

#### 初始化选项

```cpp
struct InitOptions {
    bool load_base = true;          // 基础库
    bool load_string = true;        // 字符串库
    bool load_table = true;         // 表库
    bool load_math = true;          // 数学库
    bool load_io = false;           // IO库（可选）
    bool load_os = false;           // OS库（可选）
    bool load_debug = false;        // 调试库（可选）
    bool load_coroutine = false;    // 协程库（可选）
    bool load_package = false;      // 包管理库（可选）
    
    // 安全选项
    bool safe_mode = false;         // 安全模式
    bool sandbox_mode = false;      // 沙箱模式
};
```

#### 初始化方法

**1. 预定义初始化**

```cpp
// 初始化核心库
void initCoreLibraries(State* state) {
    const Vec<Str> core_libs = {
        "base", "string", "table", "math"
    };
    
    LibManager& manager = LibManager::getInstance();
    for (const Str& lib : core_libs) {
        manager.loadLibrary(state, lib);
    }
}

// 初始化所有库
void initAllLibraries(State* state) {
    LibManager::getInstance().loadAllLibraries(state);
}

// 初始化最小库集
void initMinimalLibraries(State* state) {
    LibManager::getInstance().loadLibrary(state, "base");
}
```

**2. 自定义初始化**

```cpp
void initLibrariesWithOptions(State* state, const InitOptions& options) {
    LibManager& manager = LibManager::getInstance();
    
    // 根据选项加载库
    if (options.load_base) manager.loadLibrary(state, "base");
    if (options.load_string) manager.loadLibrary(state, "string");
    if (options.load_table) manager.loadLibrary(state, "table");
    if (options.load_math) manager.loadLibrary(state, "math");
    
    // 可选库
    if (options.load_io && !options.safe_mode) {
        manager.loadLibrary(state, "io");
    }
    if (options.load_os && !options.safe_mode) {
        manager.loadLibrary(state, "os");
    }
    
    // 高级库
    if (options.load_debug && !options.sandbox_mode) {
        manager.loadLibrary(state, "debug");
    }
    if (options.load_coroutine) {
        manager.loadLibrary(state, "coroutine");
    }
    if (options.load_package && !options.sandbox_mode) {
        manager.loadLibrary(state, "package");
    }
}
```

**3. 便捷宏定义**

```cpp
// 便捷初始化宏
#define LUA_INIT_CORE(state) Lua::LibInit::initCoreLibraries(state)
#define LUA_INIT_ALL(state) Lua::LibInit::initAllLibraries(state)
#define LUA_INIT_SAFE(state) Lua::LibInit::initLibrariesWithOptions(state, Lua::LibInit::getSafeModeOptions())
#define LUA_INIT_MINIMAL(state) Lua::LibInit::initMinimalLibraries(state)
```

### 4. LibUtils 工具库

#### ArgChecker 参数检查器

```cpp
class ArgChecker {
public:
    ArgChecker(State* state, int nargs) : state_(state), nargs_(nargs), current_arg_(1) {}
    
    // 参数数量检查
    bool checkMinArgs(int min_args);
    bool checkExactArgs(int exact_args);
    
    // 类型化参数获取
    Opt<LuaNumber> getNumber();
    Opt<Str> getString();
    Opt<Table*> getTable();
    Opt<Function*> getFunction();
    Opt<Value> getValue();
    
    // 状态查询
    bool hasMore() const;
    int getCurrentIndex() const;
    int getTotalArgs() const;
    
private:
    State* state_;
    int nargs_;
    int current_arg_;
};
```

**使用示例：**

```cpp
Value StringLib::len(State* state, int nargs) {
    LibUtils::ArgChecker checker(state, nargs);
    
    // 检查参数数量
    if (!checker.checkMinArgs(1)) {
        return Value();
    }
    
    // 获取字符串参数
    auto str = checker.getString();
    if (!str) {
        LibUtils::Error::throwTypeError(state, 1, "string", "other");
        return Value();
    }
    
    // 返回字符串长度
    return Value(static_cast<LuaNumber>(str->length()));
}
```

#### Convert 类型转换工具

```cpp
namespace Convert {
    // 值转换
    Opt<LuaNumber> toNumber(const Value& value);
    Opt<Str> toString(const Value& value);
    Opt<bool> toBoolean(const Value& value);
    
    // 类型名称
    Str typeToString(ValueType type);
    
    // 安全转换
    template<typename T>
    Opt<T> safeCast(const Value& value);
}
```

#### Error 错误处理工具

```cpp
namespace Error {
    // 抛出错误
    void throwError(State* state, const Str& message);
    void throwTypeError(State* state, int arg_index, const Str& expected, const Str& actual);
    void throwArgError(State* state, int arg_index, const Str& message);
    void throwRangeError(State* state, int arg_index, const Str& message);
    
    // 错误格式化
    Str formatError(const Str& function, const Str& message);
    Str formatTypeError(int arg_index, const Str& expected, const Str& actual);
}
```

#### String 字符串工具

```cpp
namespace String {
    // 字符串操作
    bool startsWith(const Str& str, const Str& prefix);
    bool endsWith(const Str& str, const Str& suffix);
    Str trim(const Str& str);
    Str toLower(const Str& str);
    Str toUpper(const Str& str);
    
    // 字符串分割和连接
    Vec<Str> split(const Str& str, const Str& delimiter);
    Str join(const Vec<Str>& parts, const Str& separator);
    
    // 模式匹配
    bool match(const Str& str, const Str& pattern);
    Vec<Str> findAll(const Str& str, const Str& pattern);
}
```

## 现有库模块详解

### 1. BaseLib 基础库

#### 功能概述

BaseLib 提供 Lua 的核心基础函数，是所有 Lua 程序的基础。

```cpp
class BaseLib : public LibModule {
public:
    const std::string& getName() const override {
        static const std::string name = "base";
        return name;
    }
    
    void registerModule(State* state) override;
    
private:
    // 基础函数
    static Value print(State* state, int nargs);
    static Value tonumber(State* state, int nargs);
    static Value tostring(State* state, int nargs);
    static Value type(State* state, int nargs);
    static Value ipairs(State* state, int nargs);
    static Value pairs(State* state, int nargs);
    static Value next(State* state, int nargs);
    static Value getmetatable(State* state, int nargs);
    static Value setmetatable(State* state, int nargs);
    static Value rawget(State* state, int nargs);
    static Value rawset(State* state, int nargs);
    static Value rawlen(State* state, int nargs);
    static Value rawequal(State* state, int nargs);
    static Value pcall(State* state, int nargs);
    static Value xpcall(State* state, int nargs);
    static Value error(State* state, int nargs);
    static Value lua_assert(State* state, int nargs);
    static Value select(State* state, int nargs);
    static Value unpack(State* state, int nargs);
};
```

#### 核心函数实现

**1. print 函数**

```cpp
Value BaseLib::print(State* state, int nargs) {
    LibUtils::ArgChecker checker(state, nargs);
    
    for (int i = 1; i <= nargs; i++) {
        if (i > 1) std::cout << "\t";
        
        Value val = state->get(i);
        if (val.isString()) {
            std::cout << val.asString();
        } else {
            std::cout << val.toString();
        }
    }
    std::cout << std::endl;
    
    return Value(); // nil
}
```

**2. type 函数**

```cpp
Value BaseLib::type(State* state, int nargs) {
    LibUtils::ArgChecker checker(state, nargs);
    
    if (!checker.checkMinArgs(1)) {
        return Value();
    }
    
    auto val = checker.getValue();
    if (!val) {
        return Value("nil");
    }
    
    return Value(LibUtils::Convert::typeToString(val->type()));
}
```

**3. tonumber 函数**

```cpp
Value BaseLib::tonumber(State* state, int nargs) {
    LibUtils::ArgChecker checker(state, nargs);
    
    if (!checker.checkMinArgs(1)) {
        return Value();
    }
    
    auto val = checker.getValue();
    if (!val) {
        return Value(); // nil
    }
    
    auto num = LibUtils::Convert::toNumber(*val);
    return num ? Value(*num) : Value(); // nil if conversion failed
}
```

### 2. StringLib 字符串库

#### 功能概述

StringLib 提供字符串操作相关的函数，支持字符串处理、模式匹配、格式化等功能。

```cpp
class StringLib : public LibModule {
public:
    const Str& getName() const override { 
        static const Str name = "string";
        return name;
    }
    
    void registerModule(State* state) override;
    
private:
    // 字符串函数
    static Value len(State* state, int nargs);
    static Value sub(State* state, int nargs);
    static Value upper(State* state, int nargs);
    static Value lower(State* state, int nargs);
    static Value find(State* state, int nargs);
    static Value gsub(State* state, int nargs);
    static Value match(State* state, int nargs);
    static Value gmatch(State* state, int nargs);
    static Value rep(State* state, int nargs);
    static Value reverse(State* state, int nargs);
    static Value format(State* state, int nargs);
    static Value char_func(State* state, int nargs);
    static Value byte_func(State* state, int nargs);
};
```

#### 注册机制

```cpp
void StringLib::registerModule(State* state) {
    // 创建 string 表
    auto stringTable = GCRef<Table>(new Table());
    
    // 注册字符串函数到表中
    registerFunction(state, Value(stringTable), "len", len);
    registerFunction(state, Value(stringTable), "sub", sub);
    registerFunction(state, Value(stringTable), "upper", upper);
    registerFunction(state, Value(stringTable), "lower", lower);
    registerFunction(state, Value(stringTable), "find", find);
    registerFunction(state, Value(stringTable), "gsub", gsub);
    registerFunction(state, Value(stringTable), "match", match);
    registerFunction(state, Value(stringTable), "gmatch", gmatch);
    registerFunction(state, Value(stringTable), "rep", rep);
    registerFunction(state, Value(stringTable), "reverse", reverse);
    registerFunction(state, Value(stringTable), "format", format);
    registerFunction(state, Value(stringTable), "char", char_func);
    registerFunction(state, Value(stringTable), "byte", byte_func);
    
    // 设置 string 表为全局变量
    state->setGlobal("string", Value(stringTable));
    
    // 标记为已加载
    setLoaded(true);
}
```

### 3. TableLib 表库

#### 功能概述

TableLib 提供表操作相关的函数，包括插入、删除、排序、连接等操作。

```cpp
class TableLib : public LibModule {
public:
    const Str& getName() const override { 
        static const Str name = "table";
        return name;
    }
    
    void registerModule(State* state) override;
    
private:
    // 表函数
    static Value insert(State* state, int nargs);
    static Value remove(State* state, int nargs);
    static Value concat(State* state, int nargs);
    static Value sort(State* state, int nargs);
    static Value pack(State* state, int nargs);
    static Value unpack(State* state, int nargs);
    static Value move(State* state, int nargs);
    static Value maxn(State* state, int nargs);
};
```

## 添加新库的完整指南

### 步骤 1: 创建库头文件

创建 `src/lib/mylib.hpp`：

```cpp
#pragma once

#include "../common/types.hpp"
#include "lib_common.hpp"
#include "lib_utils.hpp"
#include "../vm/value.hpp"

namespace Lua {

    /**
     * @brief 自定义库模块
     * 
     * 提供自定义功能，包括：
     * - mylib.hello: 打印问候语
     * - mylib.add: 数字相加
     * - mylib.info: 获取库信息
     */
    class MyLib : public LibModule {
    public:
        // LibModule 接口实现
        const Str& getName() const override { 
            static const Str name = "mylib";
            return name;
        }
        
        const Str& getVersion() const override { 
            static const Str version = "1.0.0";
            return version;
        }
        
        void registerModule(State* state) override;
        
    private:
        // 库函数声明
        static Value hello(State* state, int nargs);
        static Value add(State* state, int nargs);
        static Value info(State* state, int nargs);
    };
}
```

### 步骤 2: 实现库函数

创建 `src/lib/mylib.cpp`：

```cpp
#include "mylib.hpp"
#include "lib_common.hpp"
#include "lib_utils.hpp"
#include "../vm/state.hpp"
#include "../vm/table.hpp"
#include <iostream>

namespace Lua {

    void MyLib::registerModule(State* state) {
        // 创建 mylib 表
        auto mylibTable = GCRef<Table>(new Table());
        
        // 注册库函数
        registerFunction(state, Value(mylibTable), "hello", hello);
        registerFunction(state, Value(mylibTable), "add", add);
        registerFunction(state, Value(mylibTable), "info", info);
        
        // 设置为全局变量
        state->setGlobal("mylib", Value(mylibTable));
        
        // 标记为已加载
        setLoaded(true);
    }
    
    Value MyLib::hello(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        // 获取可选的名字参数
        Str name = "World";
        if (checker.hasMore()) {
            auto nameOpt = checker.getString();
            if (nameOpt) {
                name = *nameOpt;
            }
        }
        
        // 打印问候语
        std::cout << "Hello, " << name << "!" << std::endl;
        
        return Value(); // nil
    }
    
    Value MyLib::add(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        // 检查参数数量
        if (!checker.checkMinArgs(2)) {
            LibUtils::Error::throwError(state, "add requires at least 2 arguments");
            return Value();
        }
        
        // 获取第一个数字
        auto num1 = checker.getNumber();
        if (!num1) {
            LibUtils::Error::throwTypeError(state, 1, "number", "other");
            return Value();
        }
        
        // 获取第二个数字
        auto num2 = checker.getNumber();
        if (!num2) {
            LibUtils::Error::throwTypeError(state, 2, "number", "other");
            return Value();
        }
        
        // 返回相加结果
        return Value(*num1 + *num2);
    }
    
    Value MyLib::info(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        // 创建信息表
        auto infoTable = GCRef<Table>(new Table());
        
        // 设置库信息
        infoTable->set(Value("name"), Value("mylib"));
        infoTable->set(Value("version"), Value("1.0.0"));
        infoTable->set(Value("author"), Value("Your Name"));
        infoTable->set(Value("description"), Value("A custom library example"));
        
        return Value(infoTable);
    }
}
```

### 步骤 3: 注册库到管理器

在 `src/lib/lib_init.cpp` 中添加注册代码：

```cpp
#include "mylib.hpp"

// 在 registerAllLibraries 函数中添加
void registerAllLibraries() {
    LibManager& manager = LibManager::getInstance();
    
    // 注册现有库
    REGISTER_LIB("base", BaseLib);
    REGISTER_LIB("string", StringLib);
    REGISTER_LIB("table", TableLib);
    
    // 注册新库
    REGISTER_LIB("mylib", MyLib);
}
```

### 步骤 4: 更新初始化选项

在 `src/lib/lib_init.hpp` 中添加新库的初始化选项：

```cpp
struct InitOptions {
    bool load_base = true;
    bool load_string = true;
    bool load_table = true;
    bool load_math = true;
    bool load_io = false;
    bool load_os = false;
    bool load_debug = false;
    bool load_coroutine = false;
    bool load_package = false;
    
    // 添加新库选项
    bool load_mylib = false;  // 自定义库（可选）
    
    bool safe_mode = false;
    bool sandbox_mode = false;
};
```

在 `src/lib/lib_init.cpp` 中更新初始化逻辑：

```cpp
void initLibrariesWithOptions(State* state, const InitOptions& options) {
    LibManager& manager = LibManager::getInstance();
    
    // 现有库初始化...
    
    // 添加新库初始化
    if (options.load_mylib) {
        manager.loadLibrary(state, "mylib");
    }
}
```

### 步骤 5: 编写单元测试

创建 `src/tests/lib/mylib_test.cpp`：

```cpp
#include "../../lib/mylib.hpp"
#include "../../vm/state.hpp"
#include "../../lib/lib_manager.hpp"
#include <cassert>
#include <iostream>

namespace Lua {
    namespace Test {
        
        class MyLibTest {
        public:
            static void runAllTests() {
                std::cout << "Running MyLib tests..." << std::endl;
                
                testHelloFunction();
                testAddFunction();
                testInfoFunction();
                testLibraryLoading();
                
                std::cout << "All MyLib tests passed!" << std::endl;
            }
            
        private:
            static void testHelloFunction() {
                std::cout << "Testing hello function..." << std::endl;
                
                // 创建状态
                auto state = std::make_unique<State>();
                
                // 加载库
                LibManager::getInstance().registerLibrary("mylib", 
                    []() -> std::unique_ptr<LibModule> {
                        return std::make_unique<MyLib>();
                    });
                LibManager::getInstance().loadLibrary(state.get(), "mylib");
                
                // 测试调用（这里需要根据实际的调用机制调整）
                // ...
                
                std::cout << "Hello function test passed." << std::endl;
            }
            
            static void testAddFunction() {
                std::cout << "Testing add function..." << std::endl;
                
                // 创建状态
                auto state = std::make_unique<State>();
                
                // 加载库
                LibManager::getInstance().loadLibrary(state.get(), "mylib");
                
                // 测试数字相加
                // 这里需要根据实际的函数调用机制来测试
                // Value result = MyLib::add(state.get(), 2);
                // assert(result.isNumber());
                // assert(result.asNumber() == 5.0); // 假设传入 2 和 3
                
                std::cout << "Add function test passed." << std::endl;
            }
            
            static void testInfoFunction() {
                std::cout << "Testing info function..." << std::endl;
                
                // 创建状态
                auto state = std::make_unique<State>();
                
                // 加载库
                LibManager::getInstance().loadLibrary(state.get(), "mylib");
                
                // 测试信息获取
                // Value result = MyLib::info(state.get(), 0);
                // assert(result.isTable());
                
                std::cout << "Info function test passed." << std::endl;
            }
            
            static void testLibraryLoading() {
                std::cout << "Testing library loading..." << std::endl;
                
                LibManager& manager = LibManager::getInstance();
                
                // 测试注册
                assert(manager.isRegistered("mylib"));
                
                // 测试加载
                auto state = std::make_unique<State>();
                bool loaded = manager.loadLibrary(state.get(), "mylib");
                assert(loaded);
                assert(manager.isLoaded("mylib"));
                
                // 测试模块获取
                LibModule* module = manager.getModule("mylib");
                assert(module != nullptr);
                assert(module->getName() == "mylib");
                assert(module->getVersion() == "1.0.0");
                assert(module->isLoaded());
                
                std::cout << "Library loading test passed." << std::endl;
            }
        };
    }
}
```

### 步骤 6: 更新构建系统

在项目的构建文件中添加新的源文件：

**构建配置示例:**

```
# 添加新的库源文件到您的构建系统
# 包含以下文件：
    src/lib/lib_common.cpp
    src/lib/lib_manager.cpp
    src/lib/lib_init.cpp
    src/lib/lib_utils.cpp
    src/lib/base_lib.cpp
    src/lib/string_lib.cpp
    src/lib/table_lib.cpp
    src/lib/mylib.cpp  # 新添加的库

# 添加测试文件
    src/tests/lib/base_lib_test.cpp
    src/tests/lib/string_lib_test.cpp
    src/tests/lib/table_lib_test.cpp
    src/tests/lib/mylib_test.cpp  # 新添加的测试
)
```

**Visual Studio 项目文件:**

在 `.vcxproj` 文件中添加：

```xml
<ClCompile Include="src\lib\mylib.cpp" />
<ClInclude Include="src\lib\mylib.hpp" />
<ClCompile Include="src\tests\lib\mylib_test.cpp" />
```

### 步骤 7: 编写文档

创建 `docs/mylib_api.md`：

```markdown
# MyLib API 文档

## 概述

MyLib 是一个示例自定义库，展示了如何在 Lua 解释器中添加新的库模块。

## 函数列表

### mylib.hello([name])

打印问候语。

**参数：**
- `name` (string, 可选): 要问候的名字，默认为 "World"

**返回值：**
- `nil`

**示例：**
```lua
mylib.hello()        -- 输出: Hello, World!
mylib.hello("Lua")   -- 输出: Hello, Lua!
```

### mylib.add(a, b)

计算两个数字的和。

**参数：**
- `a` (number): 第一个数字
- `b` (number): 第二个数字

**返回值：**
- `number`: 两个数字的和

**示例：**
```lua
local result = mylib.add(3, 5)  -- result = 8
```

### mylib.info()

获取库的信息。

**参数：**
- 无

**返回值：**
- `table`: 包含库信息的表

**示例：**
```lua
local info = mylib.info()
print(info.name)     -- 输出: mylib
print(info.version)  -- 输出: 1.0.0
```

## 错误处理

所有函数都会进行参数验证，如果参数类型或数量不正确，会抛出相应的错误。

## 版本历史

- v1.0.0: 初始版本，包含基本功能
```

### 步骤 8: 集成测试

在 `src/tests/test_main.cpp` 中添加新库的测试：

```cpp
#include "lib/mylib_test.cpp"

int main() {
    try {
        // 运行现有测试...
        
        // 运行新库测试
        Lua::Test::MyLibTest::runAllTests();
        
        std::cout << "All tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
```

## 高级特性

### 1. 库依赖管理

#### 依赖声明

```cpp
class AdvancedLib : public LibModule {
public:
    // 声明依赖的库
    Vec<Str> getDependencies() const override {
        return {"base", "string", "table"};
    }
    
    void registerModule(State* state) override {
        // 检查依赖是否已加载
        LibManager& manager = LibManager::getInstance();
        for (const Str& dep : getDependencies()) {
            if (!manager.isLoaded(dep)) {
                throw std::runtime_error("Dependency not loaded: " + dep);
            }
        }
        
        // 注册函数...
    }
};
```

#### 自动依赖解析

```cpp
void LibManager::loadLibraryWithDependencies(State* state, const Str& name) {
    // 获取库的依赖
    auto factory = libraries_[name];
    auto temp_module = factory();
    auto dependencies = temp_module->getDependencies();
    
    // 递归加载依赖
    for (const Str& dep : dependencies) {
        if (!isLoaded(dep)) {
            loadLibraryWithDependencies(state, dep);
        }
    }
    
    // 加载目标库
    loadLibrary(state, name);
}
```

### 2. 库版本管理

#### 版本兼容性检查

```cpp
struct Version {
    int major, minor, patch;
    
    bool isCompatibleWith(const Version& other) const {
        return major == other.major && minor >= other.minor;
    }
    
    static Version parse(const Str& version_str) {
        // 解析版本字符串 "1.2.3"
        // ...
    }
};

class VersionedLib : public LibModule {
public:
    Version getRequiredVersion() const {
        return Version{1, 0, 0};
    }
    
    bool checkCompatibility(const Version& system_version) const {
        return getRequiredVersion().isCompatibleWith(system_version);
    }
};
```

### 3. 模块化加载

#### 模块工厂接口

```cpp
class ModuleFactory {
public:
    virtual ~ModuleFactory() = default;
    virtual UPtr<LibModule> createLibrary() = 0;
    virtual Str getLibraryName() const = 0;
    virtual Version getLibraryVersion() const = 0;
};

// 模块创建函数
namespace ModuleFactories {
    std::unique_ptr<LibModule> createMathModule() {
        return std::make_unique<MathLib>();
    }

    std::unique_ptr<LibModule> createStringModule() {
        return std::make_unique<StringLib>();
    }
}
```

#### 动态加载器

```cpp
class DynamicLibLoader {
public:
    bool loadPlugin(const Str& plugin_path) {
        // 加载动态库
        void* handle = dlopen(plugin_path.c_str(), RTLD_LAZY);
        if (!handle) {
            return false;
        }
        
        // 获取创建函数
        auto create_func = (PluginInterface*(*)())dlsym(handle, "createPlugin");
        if (!create_func) {
            dlclose(handle);
            return false;
        }
        
        // 创建插件实例
        auto plugin = create_func();
        auto library = plugin->createLibrary();
        
        // 注册到管理器
        LibManager::getInstance().registerLibrary(
            plugin->getLibraryName(),
            [library = std::move(library)]() mutable -> UPtr<LibModule> {
                return std::move(library);
            }
        );
        
        return true;
    }
};
```

### 4. 库配置系统

#### 配置接口

```cpp
class ConfigurableLib : public LibModule {
public:
    struct Config {
        bool enable_logging = false;
        int max_operations = 1000;
        Str output_format = "text";
    };
    
    void configure(const Config& config) {
        config_ = config;
    }
    
    const Config& getConfig() const {
        return config_;
    }
    
private:
    Config config_;
};
```

#### 配置管理器

```cpp
class LibConfigManager {
public:
    template<typename ConfigType>
    void setConfig(const Str& lib_name, const ConfigType& config) {
        configs_[lib_name] = std::make_any<ConfigType>(config);
    }
    
    template<typename ConfigType>
    Opt<ConfigType> getConfig(const Str& lib_name) const {
        auto it = configs_.find(lib_name);
        if (it != configs_.end()) {
            try {
                return std::any_cast<ConfigType>(it->second);
            } catch (const std::bad_any_cast&) {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }
    
private:
    HashMap<Str, std::any> configs_;
};
```

### 5. 性能监控

#### 性能统计

```cpp
class LibPerformanceMonitor {
public:
    struct FunctionStats {
        usize call_count = 0;
        std::chrono::nanoseconds total_time{0};
        std::chrono::nanoseconds min_time{std::chrono::nanoseconds::max()};
        std::chrono::nanoseconds max_time{0};
        
        double getAverageTime() const {
            return call_count > 0 ? 
                static_cast<double>(total_time.count()) / call_count : 0.0;
        }
    };
    
    void recordCall(const Str& lib_name, const Str& func_name, 
                   std::chrono::nanoseconds duration) {
        Str key = lib_name + "." + func_name;
        auto& stats = function_stats_[key];
        
        stats.call_count++;
        stats.total_time += duration;
        stats.min_time = std::min(stats.min_time, duration);
        stats.max_time = std::max(stats.max_time, duration);
    }
    
    const FunctionStats& getStats(const Str& lib_name, const Str& func_name) const {
        Str key = lib_name + "." + func_name;
        static const FunctionStats empty_stats;
        
        auto it = function_stats_.find(key);
        return (it != function_stats_.end()) ? it->second : empty_stats;
    }
    
    void printReport() const {
        std::cout << "=== Library Performance Report ===\n";
        for (const auto& [key, stats] : function_stats_) {
            std::cout << key << ":\n";
            std::cout << "  Calls: " << stats.call_count << "\n";
            std::cout << "  Total: " << stats.total_time.count() << " ns\n";
            std::cout << "  Average: " << stats.getAverageTime() << " ns\n";
            std::cout << "  Min: " << stats.min_time.count() << " ns\n";
            std::cout << "  Max: " << stats.max_time.count() << " ns\n";
        }
    }
    
private:
    HashMap<Str, FunctionStats> function_stats_;
};
```

#### 性能装饰器

```cpp
template<typename Func>
auto withPerformanceMonitoring(const Str& lib_name, const Str& func_name, Func&& func) {
    return [=](auto&&... args) {
        auto start = std::chrono::high_resolution_clock::now();
        
        auto result = func(std::forward<decltype(args)>(args)...);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        
        LibPerformanceMonitor::getInstance().recordCall(lib_name, func_name, duration);
        
        return result;
    };
}

// 使用示例
Value MyLib::add(State* state, int nargs) {
    static auto monitored_add = withPerformanceMonitoring("mylib", "add", 
        [](State* s, int n) -> Value {
            // 原始函数实现
            LibUtils::ArgChecker checker(s, n);
            // ...
            return Value(result);
        });
    
    return monitored_add(state, nargs);
}
```

## 最佳实践

### 1. 错误处理

#### 统一错误处理

```cpp
class LibErrorHandler {
public:
    enum class ErrorType {
        ARGUMENT_ERROR,
        TYPE_ERROR,
        RANGE_ERROR,
        RUNTIME_ERROR
    };
    
    static void handleError(State* state, ErrorType type, 
                          const Str& message, int arg_index = -1) {
        Str formatted_message;
        
        switch (type) {
            case ErrorType::ARGUMENT_ERROR:
                formatted_message = formatArgError(arg_index, message);
                break;
            case ErrorType::TYPE_ERROR:
                formatted_message = formatTypeError(arg_index, message);
                break;
            case ErrorType::RANGE_ERROR:
                formatted_message = formatRangeError(arg_index, message);
                break;
            case ErrorType::RUNTIME_ERROR:
                formatted_message = message;
                break;
        }
        
        state->error(formatted_message);
    }
    
private:
    static Str formatArgError(int arg_index, const Str& message) {
        return "bad argument #" + std::to_string(arg_index) + " (" + message + ")";
    }
    
    static Str formatTypeError(int arg_index, const Str& message) {
        return "bad argument #" + std::to_string(arg_index) + " (" + message + ")";
    }
    
    static Str formatRangeError(int arg_index, const Str& message) {
        return "bad argument #" + std::to_string(arg_index) + " (" + message + ")";
    }
};
```

#### 异常安全的函数实现

```cpp
Value SafeLib::processData(State* state, int nargs) {
    try {
        LibUtils::ArgChecker checker(state, nargs);
        
        // 参数验证
        if (!checker.checkMinArgs(1)) {
            LibErrorHandler::handleError(state, 
                LibErrorHandler::ErrorType::ARGUMENT_ERROR,
                "expected at least 1 argument");
            return Value();
        }
        
        auto data = checker.getValue();
        if (!data || !data->isTable()) {
            LibErrorHandler::handleError(state,
                LibErrorHandler::ErrorType::TYPE_ERROR,
                "table expected", 1);
            return Value();
        }
        
        // 处理数据（可能抛出异常）
        auto result = processTableData(data->asTable().get());
        
        return Value(result);
        
    } catch (const std::bad_alloc&) {
        LibErrorHandler::handleError(state,
            LibErrorHandler::ErrorType::RUNTIME_ERROR,
            "out of memory");
        return Value();
    } catch (const std::exception& e) {
        LibErrorHandler::handleError(state,
            LibErrorHandler::ErrorType::RUNTIME_ERROR,
            e.what());
        return Value();
    }
}
```

### 2. 内存管理

#### RAII 资源管理

```cpp
class ResourceManager {
public:
    template<typename Resource, typename Deleter>
    class ScopedResource {
    public:
        ScopedResource(Resource* resource, Deleter deleter)
            : resource_(resource), deleter_(deleter) {}
        
        ~ScopedResource() {
            if (resource_) {
                deleter_(resource_);
            }
        }
        
        Resource* get() const { return resource_; }
        Resource* release() {
            Resource* temp = resource_;
            resource_ = nullptr;
            return temp;
        }
        
        // 禁用拷贝
        ScopedResource(const ScopedResource&) = delete;
        ScopedResource& operator=(const ScopedResource&) = delete;
        
        // 支持移动
        ScopedResource(ScopedResource&& other) noexcept
            : resource_(other.release()), deleter_(std::move(other.deleter_)) {}
        
    private:
        Resource* resource_;
        Deleter deleter_;
    };
    
    template<typename Resource, typename Deleter>
    static auto makeScopedResource(Resource* resource, Deleter deleter) {
        return ScopedResource<Resource, Deleter>(resource, deleter);
    }
};

// 使用示例
Value FileLib::readFile(State* state, int nargs) {
    LibUtils::ArgChecker checker(state, nargs);
    
    auto filename = checker.getString();
    if (!filename) {
        return Value();
    }
    
    // 使用 RAII 管理文件资源
    auto file = ResourceManager::makeScopedResource(
        std::fopen(filename->c_str(), "r"),
        [](FILE* f) { if (f) std::fclose(f); }
    );
    
    if (!file.get()) {
        LibErrorHandler::handleError(state,
            LibErrorHandler::ErrorType::RUNTIME_ERROR,
            "cannot open file: " + *filename);
        return Value();
    }
    
    // 读取文件内容
    // ...
    
    return Value(content);
}
```

### 3. 性能优化

#### 函数内联和模板优化

```cpp
template<typename T>
inline Value createNumberValue(T value) {
    static_assert(std::is_arithmetic_v<T>, "T must be arithmetic type");
    return Value(static_cast<LuaNumber>(value));
}

template<typename Container>
inline Value createArrayFromContainer(const Container& container) {
    auto table = GCRef<Table>(new Table());
    
    LuaInteger index = 1;
    for (const auto& item : container) {
        table->set(Value(index++), Value(item));
    }
    
    return Value(table);
}
```

#### 缓存优化

```cpp
class LibCache {
public:
    template<typename Key, typename Value>
    class LRUCache {
    public:
        LRUCache(size_t capacity) : capacity_(capacity) {}
        
        Opt<Value> get(const Key& key) {
            auto it = cache_.find(key);
            if (it != cache_.end()) {
                // 移动到最前面
                order_.splice(order_.begin(), order_, it->second.second);
                return it->second.first;
            }
            return std::nullopt;
        }
        
        void put(const Key& key, const Value& value) {
            auto it = cache_.find(key);
            if (it != cache_.end()) {
                // 更新现有项
                it->second.first = value;
                order_.splice(order_.begin(), order_, it->second.second);
            } else {
                // 添加新项
                if (cache_.size() >= capacity_) {
                    // 移除最旧的项
                    auto last = order_.back();
                    order_.pop_back();
                    cache_.erase(last);
                }
                
                order_.push_front(key);
                cache_[key] = {value, order_.begin()};
            }
        }
        
    private:
        size_t capacity_;
        std::list<Key> order_;
        std::unordered_map<Key, std::pair<Value, typename std::list<Key>::iterator>> cache_;
    };
    
    // 函数结果缓存
    static LRUCache<Str, Value> function_cache_;
    
    static Opt<Value> getCachedResult(const Str& function_name, const Vec<Value>& args) {
        Str key = function_name + "(" + serializeArgs(args) + ")";
        return function_cache_.get(key);
    }
    
    static void cacheResult(const Str& function_name, const Vec<Value>& args, const Value& result) {
        Str key = function_name + "(" + serializeArgs(args) + ")";
        function_cache_.put(key, result);
    }
    
private:
    static Str serializeArgs(const Vec<Value>& args) {
        Str result;
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) result += ",";
            result += args[i].toString();
        }
        return result;
    }
};

// 使用缓存的函数示例
Value MathLib::fibonacci(State* state, int nargs) {
    LibUtils::ArgChecker checker(state, nargs);
    
    auto n = checker.getNumber();
    if (!n || *n < 0) {
        return Value();
    }
    
    // 检查缓存
    Vec<Value> args = {Value(*n)};
    auto cached = LibCache::getCachedResult("fibonacci", args);
    if (cached) {
        return *cached;
    }
    
    // 计算结果
    LuaNumber result = calculateFibonacci(static_cast<int>(*n));
    Value result_value(result);
    
    // 缓存结果
    LibCache::cacheResult("fibonacci", args, result_value);
    
    return result_value;
}
```

### 4. 线程安全

#### 线程安全的库管理

```cpp
class ThreadSafeLibManager {
public:
    static ThreadSafeLibManager& getInstance() {
        static ThreadSafeLibManager instance;
        return instance;
    }
    
    void registerLibrary(const Str& name, std::function<UPtr<LibModule>()> factory) {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        libraries_[name] = factory;
    }
    
    bool loadLibrary(State* state, const Str& name) {
        // 读锁检查是否已加载
        {
            std::shared_lock<std::shared_mutex> read_lock(mutex_);
            auto loaded_it = loaded_modules_.find(name);
            if (loaded_it != loaded_modules_.end()) {
                return true; // 已加载
            }
        }
        
        // 写锁进行加载
        std::lock_guard<std::shared_mutex> write_lock(mutex_);
        
        // 双重检查
        auto loaded_it = loaded_modules_.find(name);
        if (loaded_it != loaded_modules_.end()) {
            return true;
        }
        
        auto it = libraries_.find(name);
        if (it == libraries_.end()) {
            return false;
        }
        
        auto module = it->second();
        if (module) {
            module->registerModule(state);
            loaded_modules_[name] = std::move(module);
            return true;
        }
        
        return false;
    }
    
    bool isLoaded(const Str& name) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return loaded_modules_.find(name) != loaded_modules_.end();
    }
    
private:
    mutable std::shared_mutex mutex_;
    HashMap<Str, std::function<UPtr<LibModule>()>> libraries_;
    HashMap<Str, UPtr<LibModule>> loaded_modules_;
};
```

#### 原子操作优化

```cpp
class AtomicCounter {
public:
    void increment() {
        count_.fetch_add(1, std::memory_order_relaxed);
    }
    
    void decrement() {
        count_.fetch_sub(1, std::memory_order_relaxed);
    }
    
    usize get() const {
        return count_.load(std::memory_order_relaxed);
    }
    
private:
    std::atomic<usize> count_{0};
};

class LibStatistics {
public:
    static void recordFunctionCall(const Str& lib_name, const Str& func_name) {
        Str key = lib_name + "." + func_name;
        call_counts_[key].increment();
    }
    
    static usize getFunctionCallCount(const Str& lib_name, const Str& func_name) {
        Str key = lib_name + "." + func_name;
        auto it = call_counts_.find(key);
        return (it != call_counts_.end()) ? it->second.get() : 0;
    }
    
private:
    static HashMap<Str, AtomicCounter> call_counts_;
};
```

## 调试和测试

### 1. 调试工具

#### 库调试器

```cpp
class LibDebugger {
public:
    enum class LogLevel {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };
    
    static void setLogLevel(LogLevel level) {
        log_level_ = level;
    }
    
    static void log(LogLevel level, const Str& lib_name, 
                   const Str& func_name, const Str& message) {
        if (level >= log_level_) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            
            std::cout << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
                     << "] [" << levelToString(level) << "] "
                     << lib_name << "." << func_name << ": " << message << std::endl;
        }
    }
    
    static void logFunctionEntry(const Str& lib_name, const Str& func_name, int nargs) {
        log(LogLevel::DEBUG, lib_name, func_name, 
            "Entry with " + std::to_string(nargs) + " arguments");
    }
    
    static void logFunctionExit(const Str& lib_name, const Str& func_name, const Value& result) {
        log(LogLevel::DEBUG, lib_name, func_name, 
            "Exit with result: " + result.toString());
    }
    
private:
    static LogLevel log_level_;
    
    static Str levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARNING";
            case LogLevel::ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }
};

// 调试宏
#define LIB_DEBUG_ENTRY(lib, func, nargs) \
    LibDebugger::logFunctionEntry(lib, func, nargs)

#define LIB_DEBUG_EXIT(lib, func, result) \
    LibDebugger::logFunctionExit(lib, func, result)

#define LIB_DEBUG_LOG(level, lib, func, msg) \
    LibDebugger::log(LibDebugger::LogLevel::level, lib, func, msg)
```

#### 函数调用跟踪

```cpp
class FunctionTracer {
public:
    struct CallInfo {
        Str lib_name;
        Str func_name;
        std::chrono::high_resolution_clock::time_point start_time;
        int nargs;
    };
    
    static void enterFunction(const Str& lib_name, const Str& func_name, int nargs) {
        CallInfo info{
            lib_name,
            func_name,
            std::chrono::high_resolution_clock::now(),
            nargs
        };
        
        call_stack_.push(info);
        
        // 打印缩进的调用信息
        Str indent(call_stack_.size() * 2, ' ');
        std::cout << indent << "-> " << lib_name << "." << func_name 
                 << "(" << nargs << " args)" << std::endl;
    }
    
    static void exitFunction(const Value& result) {
        if (call_stack_.empty()) {
            return;
        }
        
        auto info = call_stack_.top();
        call_stack_.pop();
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - info.start_time);
        
        Str indent(call_stack_.size() * 2, ' ');
        std::cout << indent << "<- " << info.lib_name << "." << info.func_name
                 << " (" << duration.count() << "μs) -> " 
                 << result.toString() << std::endl;
    }
    
    static void printCallStack() {
        std::cout << "=== Call Stack ===" << std::endl;
        auto temp_stack = call_stack_;
        Vec<CallInfo> stack_items;
        
        while (!temp_stack.empty()) {
            stack_items.push_back(temp_stack.top());
            temp_stack.pop();
        }
        
        for (auto it = stack_items.rbegin(); it != stack_items.rend(); ++it) {
            std::cout << "  " << it->lib_name << "." << it->func_name << std::endl;
        }
    }
    
private:
    static std::stack<CallInfo> call_stack_;
};

// RAII 跟踪器
class ScopedFunctionTracer {
public:
    ScopedFunctionTracer(const Str& lib_name, const Str& func_name, int nargs)
        : lib_name_(lib_name), func_name_(func_name) {
        FunctionTracer::enterFunction(lib_name, func_name, nargs);
    }
    
    ~ScopedFunctionTracer() {
        FunctionTracer::exitFunction(result_);
    }
    
    void setResult(const Value& result) {
        result_ = result;
    }
    
private:
    Str lib_name_;
    Str func_name_;
    Value result_;
};

#define TRACE_FUNCTION(lib, func, nargs) \
    ScopedFunctionTracer tracer(lib, func, nargs)

#define TRACE_RESULT(result) \
    tracer.setResult(result)
```

### 2. 单元测试框架

#### 测试基础设施

```cpp
class LibTestFramework {
public:
    struct TestResult {
        bool passed;
        Str message;
        std::chrono::microseconds duration;
    };
    
    class TestCase {
    public:
        TestCase(const Str& name) : name_(name) {}
        virtual ~TestCase() = default;
        
        virtual void setUp() {}
        virtual void tearDown() {}
        virtual void runTest() = 0;
        
        const Str& getName() const { return name_; }
        
    protected:
        void assertTrue(bool condition, const Str& message = "") {
            if (!condition) {
                throw TestFailure("Assertion failed: " + message);
            }
        }
        
        void assertEqual(const Value& expected, const Value& actual, const Str& message = "") {
            if (!expected.equals(actual)) {
                throw TestFailure("Expected " + expected.toString() + 
                                ", got " + actual.toString() + ". " + message);
            }
        }
        
        void assertNotNull(const Value& value, const Str& message = "") {
            if (value.isNil()) {
                throw TestFailure("Expected non-nil value. " + message);
            }
        }
        
    private:
        Str name_;
        
        class TestFailure : public std::exception {
        public:
            TestFailure(const Str& message) : message_(message) {}
            const char* what() const noexcept override { return message_.c_str(); }
        private:
            Str message_;
        };
    };
    
    static void addTest(UPtr<TestCase> test) {
        tests_.push_back(std::move(test));
    }
    
    static void runAllTests() {
        int passed = 0;
        int failed = 0;
        
        std::cout << "Running " << tests_.size() << " tests..." << std::endl;
        
        for (auto& test : tests_) {
            auto result = runSingleTest(*test);
            
            if (result.passed) {
                std::cout << "[PASS] " << test->getName() 
                         << " (" << result.duration.count() << "μs)" << std::endl;
                passed++;
            } else {
                std::cout << "[FAIL] " << test->getName() 
                         << ": " << result.message << std::endl;
                failed++;
            }
        }
        
        std::cout << "\nResults: " << passed << " passed, " 
                 << failed << " failed" << std::endl;
    }
    
private:
    static Vec<UPtr<TestCase>> tests_;
    
    static TestResult runSingleTest(TestCase& test) {
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            test.setUp();
            test.runTest();
            test.tearDown();
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            return {true, "", duration};
        } catch (const std::exception& e) {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            return {false, e.what(), duration};
        }
    }
};
```

#### 库特定测试

```cpp
class StringLibTest : public LibTestFramework::TestCase {
public:
    StringLibTest() : TestCase("StringLib") {}
    
    void setUp() override {
        state_ = std::make_unique<State>();
        LibManager::getInstance().loadLibrary(state_.get(), "string");
    }
    
    void tearDown() override {
        state_.reset();
    }
    
    void runTest() override {
        testLenFunction();
        testSubFunction();
        testUpperFunction();
        testLowerFunction();
    }
    
private:
    std::unique_ptr<State> state_;
    
    void testLenFunction() {
        // 模拟调用 string.len("hello")
        state_->push(Value("hello"));
        Value result = StringLib::len(state_.get(), 1);
        
        assertTrue(result.isNumber(), "len should return number");
        assertEqual(Value(5.0), result, "len of 'hello' should be 5");
    }
    
    void testSubFunction() {
        // 模拟调用 string.sub("hello", 2, 4)
        state_->push(Value("hello"));
        state_->push(Value(2.0));
        state_->push(Value(4.0));
        Value result = StringLib::sub(state_.get(), 3);
        
        assertTrue(result.isString(), "sub should return string");
        assertEqual(Value("ell"), result, "sub('hello', 2, 4) should be 'ell'");
    }
    
    void testUpperFunction() {
        state_->push(Value("hello"));
        Value result = StringLib::upper(state_.get(), 1);
        
        assertTrue(result.isString(), "upper should return string");
        assertEqual(Value("HELLO"), result, "upper('hello') should be 'HELLO'");
    }
    
    void testLowerFunction() {
        state_->push(Value("HELLO"));
        Value result = StringLib::lower(state_.get(), 1);
        
        assertTrue(result.isString(), "lower should return string");
        assertEqual(Value("hello"), result, "lower('HELLO') should be 'hello'");
    }
};

// 注册测试
void registerStringLibTests() {
    LibTestFramework::addTest(std::make_unique<StringLibTest>());
}
```

## 总结

Lua 标准库框架采用现代 C++ 设计模式，实现了高度模块化、可扩展、高性能的库管理系统。通过统一的 `LibModule` 接口、单例 `LibManager` 管理器、灵活的 `LibInit` 初始化系统和丰富的 `LibUtils` 工具库，为开发者提供了完整的库开发和管理解决方案。

### 核心优势

1. **模块化设计**: 每个库都是独立的模块，可以单独开发、测试和部署
2. **统一接口**: 所有库都实现相同的接口，确保一致性和可维护性
3. **延迟加载**: 库只在需要时才被加载，提高启动性能
4. **依赖管理**: 支持库间依赖关系的声明和自动解析
5. **性能监控**: 内置性能统计和监控功能
6. **线程安全**: 支持多线程环境下的安全访问
7. **调试支持**: 提供丰富的调试和测试工具
8. **扩展性**: 易于添加新的库模块和功能

### 开发流程

1. **设计阶段**: 定义库的功能和接口
2. **实现阶段**: 继承 `LibModule` 接口，实现库函数
3. **注册阶段**: 使用 `REGISTER_LIB` 宏注册库
4. **测试阶段**: 编写单元测试验证功能
5. **集成阶段**: 更新初始化选项和构建系统
6. **文档阶段**: 编写 API 文档和使用指南

### 未来发展

1. **动态库支持**: 支持运行时加载动态库
2. **热重载**: 支持库的热重载和更新
3. **分布式库**: 支持网络库和远程调用
4. **JIT 优化**: 与 JIT 编译器集成优化
5. **内存优化**: 进一步优化内存使用和垃圾回收
6. **安全增强**: 增强沙箱和安全机制

通过这个框架，开发者可以轻松地扩展 Lua 解释器的功能，同时保持代码的清晰性、可维护性和高性能。

// 特化版本用于常见类型
template<>
inline Value createArrayFromContainer<std::vector<LuaNumber>>(
    const std::vector<LuaNumber>& container) {
    auto table = GCRef<Table>(new Table());
    table->reserve(container.size()); // 预分配空间
    
    for (size_t i = 0; i < container.size(); ++i) {
        table->setArrayElement(i + 1, Value(container[i]));
    }
    
    return Value(table);
}
