# Lua 解释器架构设计文档

> **项目目标**: 使用现代C++标准(C++17/20/23)从零实现一个完整的Lua 5.1.5解释器
> 
> **参考资源**:
> - `lua_c_analysis`: Lua 5.1.5 C源码分析（主要参考）
> - `lua_with_cpp`: C++ Lua实现（次要参考）
> - `spec-kit`: 开发方法论和最佳实践

---

## 📋 目录

- [整体架构](#整体架构)
- [核心模块设计](#核心模块设计)
- [技术选型](#技术选型)
- [实施路线图](#实施路线图)
- [设计原则](#设计原则)

---

## 🏗️ 整体架构

### 系统分层架构

```
┌─────────────────────────────────────────────────────────┐
│                    应用层 (Application)                  │
│              Lua解释器主程序、REPL、调试器                │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                   标准库层 (Standard Library)            │
│        base, string, table, math, io, os, debug         │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                   API层 (C API Interface)                │
│           栈操作、类型转换、函数调用、错误处理            │
└─────────────────────────────────────────────────────────┘
                            ↓
┌──────────────────┬──────────────────┬───────────────────┐
│   编译器前端      │   虚拟机核心      │   运行时系统       │
│  (Compiler)      │   (VM Core)      │   (Runtime)       │
├──────────────────┼──────────────────┼───────────────────┤
│ • 词法分析器      │ • 指令执行引擎    │ • 垃圾回收器       │
│ • 语法分析器      │ • 栈管理         │ • 内存管理器       │
│ • 代码生成器      │ • 函数调用       │ • 字符串池         │
│ • 符号表管理      │ • 协程支持       │ • 对象系统         │
└──────────────────┴──────────────────┴───────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                  基础设施层 (Foundation)                 │
│        类型系统、异常处理、工具函数、平台抽象             │
└─────────────────────────────────────────────────────────┘
```

### 核心组件关系图

```
┌──────────────┐      使用      ┌──────────────┐
│   Compiler   │ ──────────────→ │  LuaState    │
│  (编译器)     │                │  (状态机)     │
└──────────────┘                └──────────────┘
       ↓                               ↓
    生成字节码                      管理执行状态
       ↓                               ↓
┌──────────────┐      执行      ┌──────────────┐
│   Function   │ ←────────────→ │  VMExecutor  │
│  (函数对象)   │                │  (VM执行器)   │
└──────────────┘                └──────────────┘
       ↓                               ↓
    包含常量表                      操作Value栈
       ↓                               ↓
┌──────────────┐      管理      ┌──────────────┐
│    Value     │ ←────────────→ │ GarbageGC    │
│  (值类型)     │                │ (垃圾回收)    │
└──────────────┘                └──────────────┘
       ↓                               ↓
  引用GC对象                        回收对象
       ↓                               ↓
┌──────────────────────────────────────────────┐
│         GCObject (GC对象基类)                 │
│  ├─ GCString  (字符串)                        │
│  ├─ Table     (表)                            │
│  ├─ Function  (函数)                          │
│  ├─ Userdata  (用户数据)                      │
│  └─ Thread    (协程)                          │
└──────────────────────────────────────────────┘
```

---

## 🔧 核心模块设计

### 1. 类型系统 (Type System)

#### 1.1 Value - 统一值表示

**设计目标**: 实现Lua的动态类型系统，对应C版本的`TValue`

**C++实现方案**:
```cpp
namespace Lua {
    // 值类型枚举
    enum class ValueType : uint8_t {
        Nil,
        Boolean,
        Number,
        String,
        Table,
        Function,
        Userdata,
        Thread,
        LightUserdata
    };
    
    // 使用std::variant实现Tagged Union
    class Value {
    private:
        using ValueVariant = std::variant<
            std::monostate,        // Nil
            bool,                  // Boolean
            double,                // Number
            GCRef<GCString>,       // String
            GCRef<Table>,          // Table
            GCRef<Function>,       // Function
            GCRef<Userdata>,       // Userdata
            GCRef<Thread>,         // Thread
            void*                  // LightUserdata
        >;
        
        ValueVariant data_;
        
    public:
        // 类型检查
        ValueType type() const noexcept;
        bool isNil() const noexcept;
        bool isBoolean() const noexcept;
        bool isNumber() const noexcept;
        // ... 其他类型检查
        
        // 值访问
        bool asBoolean() const;
        double asNumber() const;
        GCRef<GCString> asString() const;
        // ... 其他访问器
        
        // 构造函数
        Value() = default;  // Nil
        Value(bool b);
        Value(double n);
        Value(GCRef<GCString> s);
        // ... 其他构造函数
    };
}
```

**关键特性**:
- ✅ 使用`std::variant`替代C的union，提供类型安全
- ✅ 零开销抽象，性能与C版本相当
- ✅ 支持移动语义，减少不必要的拷贝
- ✅ 与GC系统无缝集成

#### 1.2 GCObject - 垃圾回收对象基类

**设计目标**: 实现所有可回收对象的统一基类，对应C版本的`GCObject`和`CommonHeader`

**C++实现方案**:
```cpp
namespace Lua {
    // GC对象类型
    enum class GCObjectType : uint8_t {
        String,
        Table,
        Function,
        Userdata,
        Thread,
        Proto,      // 函数原型
        Upvalue     // 上值
    };
    
    // GC颜色（三色标记）
    enum class GCColor : uint8_t {
        White0 = 0x01,  // 白色0
        White1 = 0x02,  // 白色1（双缓冲）
        Gray   = 0x04,  // 灰色
        Black  = 0x08   // 黑色
    };
    
    // GC对象基类
    class GCObject {
    private:
        GCObject* next_;        // GC链表指针
        GCObjectType type_;     // 对象类型
        uint8_t marked_;        // GC标记位
        
    protected:
        explicit GCObject(GCObjectType type);
        
    public:
        virtual ~GCObject() = default;
        
        // GC接口
        GCObjectType getType() const noexcept { return type_; }
        uint8_t getMarked() const noexcept { return marked_; }
        void setMarked(uint8_t mark) noexcept { marked_ = mark; }
        
        GCColor getColor() const noexcept;
        void setColor(GCColor color) noexcept;
        
        // 遍历引用（子类实现）
        virtual void markReferences(class GarbageCollector* gc) = 0;
        
        // 获取对象大小
        virtual size_t getSize() const = 0;
        
        // GC链表操作
        GCObject* getNext() const noexcept { return next_; }
        void setNext(GCObject* next) noexcept { next_ = next; }
    };
}
```

**关键特性**:
- ✅ 虚函数实现多态，替代C的类型标签分发
- ✅ RAII管理资源，析构函数自动清理
- ✅ 保持与Lua 5.1.5相同的内存布局和GC算法
- ✅ 支持三色标记清除算法

---

### 2. 字符串系统 (String System)

#### 2.1 GCString - 字符串对象

**设计目标**: 实现字符串驻留和哈希缓存，对应C版本的`TString`

**C++实现方案**:
```cpp
namespace Lua {
    class GCString : public GCObject {
    private:
        size_t hash_;       // 预计算的哈希值
        size_t length_;     // 字符串长度
        std::string data_;  // 字符串数据（使用SSO优化）
        
    public:
        // 构造函数（私有，通过StringPool创建）
        explicit GCString(std::string_view str);
        
        // 访问器
        const std::string& data() const noexcept { return data_; }
        size_t length() const noexcept { return length_; }
        size_t hash() const noexcept { return hash_; }
        
        // GCObject接口实现
        void markReferences(GarbageCollector* gc) override;
        size_t getSize() const override;
        
        // 比较操作（O(1)，通过指针比较）
        bool operator==(const GCString& other) const noexcept {
            return this == &other;  // 驻留字符串可以直接比较指针
        }
    };
    
    // 字符串池（单例）
    class StringPool {
    private:
        std::unordered_map<std::string_view, GCRef<GCString>> pool_;
        
    public:
        static StringPool& getInstance();
        
        // 获取或创建字符串（驻留）
        GCRef<GCString> intern(std::string_view str);
        
        // GC标记所有字符串
        void markAll(GarbageCollector* gc);
    };
}
```

**关键特性**:
- ✅ 字符串驻留：相同内容的字符串只存储一份
- ✅ 哈希缓存：创建时计算哈希，后续O(1)访问
- ✅ SSO优化：利用std::string的小字符串优化
- ✅ 指针比较：驻留字符串可以直接比较地址

---

### 3. 表系统 (Table System)

#### 3.1 Table - 混合数组/哈希表

**设计目标**: 实现Lua的表数据结构，对应C版本的`Table`

**C++实现方案**:
```cpp
namespace Lua {
    class Table : public GCObject {
    private:
        // 数组部分（连续整数键）
        std::vector<Value> array_;
        
        // 哈希部分（其他键）
        std::unordered_map<Value, Value, ValueHash, ValueEqual> hash_;
        
        // 元表
        GCRef<Table> metatable_;
        
        // 元方法缓存标志
        uint8_t flags_;
        
    public:
        Table();
        
        // 基本操作
        Value get(const Value& key) const;
        void set(const Value& key, const Value& value);
        size_t length() const;
        
        // 元表操作
        GCRef<Table> getMetatable() const { return metatable_; }
        void setMetatable(GCRef<Table> mt) { metatable_ = mt; }
        
        // 迭代器支持
        class Iterator {
            // ... 迭代器实现
        };
        Iterator begin();
        Iterator end();
        
        // GCObject接口实现
        void markReferences(GarbageCollector* gc) override;
        size_t getSize() const override;
    };
}
```

**关键特性**:
- ✅ 混合存储：数组部分用vector，哈希部分用unordered_map
- ✅ 自动优化：根据访问模式动态调整数组/哈希比例
- ✅ 元表支持：完整的元编程能力
- ✅ 迭代器：支持C++范围for循环

---

## 🎯 技术选型

### C++标准和特性

| 特性 | 版本 | 用途 |
|------|------|------|
| `std::variant` | C++17 | 实现Value的Tagged Union |
| `std::optional` | C++17 | 表示可选值 |
| `std::string_view` | C++17 | 高效字符串视图 |
| `if constexpr` | C++17 | 编译期条件判断 |
| 结构化绑定 | C++17 | 简化代码 |
| `std::span` | C++20 | 数组视图（可选） |
| Concepts | C++20 | 模板约束（可选） |
| Modules | C++20 | 模块化（可选） |

### 第三方库

**原则**: 最小化依赖，优先使用标准库

| 库 | 用途 | 必需性 |
|---|------|--------|
| 无 | 核心功能完全使用STL | - |
| Google Test | 单元测试 | 开发期 |
| Benchmark | 性能测试 | 开发期 |

---

## 📅 实施路线图

### 阶段1: 基础设施 (Week 1-2)

- [ ] 项目结构搭建
- [ ] 类型系统实现 (Value, ValueType)
- [ ] GC对象基类 (GCObject)
- [ ] 基础工具类 (异常、断言、日志)

### 阶段2: 对象系统 (Week 3-4)

- [ ] 字符串系统 (GCString, StringPool)
- [ ] 表系统 (Table, 数组/哈希混合)
- [ ] 函数对象 (Function, Proto)
- [ ] 用户数据 (Userdata)

### 阶段3: 垃圾回收 (Week 5-6)

- [ ] 三色标记算法
- [ ] 增量GC
- [ ] 写屏障
- [ ] 弱引用表

### 阶段4: 虚拟机核心 (Week 7-10)

- [ ] 状态管理 (LuaState, GlobalState)
- [ ] 栈管理
- [ ] 指令集定义
- [ ] 执行引擎

### 阶段5: 编译器 (Week 11-14)

- [ ] 词法分析器
- [ ] 语法分析器
- [ ] 代码生成器
- [ ] 优化器

### 阶段6: 标准库 (Week 15-16)

- [ ] 基础库 (base)
- [ ] 字符串库 (string)
- [ ] 表库 (table)
- [ ] 数学库 (math)
- [ ] I/O库 (io)

### 阶段7: 测试和优化 (Week 17-18)

- [ ] 单元测试
- [ ] 集成测试
- [ ] 性能测试
- [ ] 优化和调优

---

## 💡 设计原则

### 1. 现代C++优先

- ✅ 使用RAII管理资源
- ✅ 优先使用智能指针和容器
- ✅ 利用移动语义减少拷贝
- ✅ 使用constexpr和noexcept优化

### 2. 保持Lua语义

- ✅ 完全兼容Lua 5.1.5语法和语义
- ✅ 保持相同的GC行为
- ✅ 保持相同的API接口
- ✅ 保持相同的性能特征

### 3. 可测试性

- ✅ 模块化设计，便于单元测试
- ✅ 依赖注入，便于Mock
- ✅ 清晰的接口，便于验证
- ✅ 完善的测试覆盖

### 4. 可维护性

- ✅ 清晰的代码结构
- ✅ 完善的文档注释
- ✅ 一致的命名规范
- ✅ 合理的抽象层次

---

## 📚 参考资源映射

### lua_c_analysis → 现代C++实现

| C源文件 | 对应C++模块 | 说明 |
|---------|------------|------|
| `lobject.h/c` | `value.hpp/cpp`, `gc_object.hpp/cpp` | 对象系统 |
| `lstring.h/c` | `gc_string.hpp/cpp`, `string_pool.hpp/cpp` | 字符串系统 |
| `ltable.h/c` | `table.hpp/cpp` | 表系统 |
| `lgc.h/c` | `garbage_collector.hpp/cpp` | 垃圾回收 |
| `lstate.h/c` | `lua_state.hpp/cpp`, `global_state.hpp/cpp` | 状态管理 |
| `lvm.h/c` | `vm_executor.hpp/cpp` | 虚拟机 |
| `llex.h/c` | `lexer.hpp/cpp` | 词法分析 |
| `lparser.h/c` | `parser.hpp/cpp` | 语法分析 |
| `lcode.h/c` | `compiler.hpp/cpp` | 代码生成 |

---

**下一步**: 开始实现基础类型系统

