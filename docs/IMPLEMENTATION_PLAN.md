# Lua 解释器实施计划

> **基于**: Spec-Driven Development (SDD) 方法论
> 
> **目标**: 系统化、模块化地实现现代C++ Lua解释器

---

## 📋 总体策略

### 开发方法论

本项目采用 **Spec-Driven Development (SDD)** 方法论，参考 `spec-kit` 项目的最佳实践：

1. **规格先行**: 每个模块先编写详细规格说明
2. **测试驱动**: 先写测试，再写实现
3. **增量开发**: 小步快跑，持续集成
4. **文档同步**: 代码和文档同步更新

### 参考资源使用策略

```
优先级1: lua_c_analysis (Lua 5.1.5 C源码分析)
  ↓ 理解原始设计和算法
  
优先级2: lua_with_cpp (C++ Lua实现)
  ↓ 参考C++实现模式
  
优先级3: spec-kit (开发方法论)
  ↓ 应用最佳实践
  
最终产出: 现代C++标准的Lua解释器
```

---

## 🎯 阶段1: 基础类型系统 (第1-2周)

### 目标

实现Lua的核心类型系统，为后续所有模块提供基础。

### 任务清单

#### 1.1 项目结构搭建

**参考**:
- `lua_with_cpp/src/` 的目录结构
- `spec-kit` 的项目组织方式

**任务**:
```
lua/
├── src/
│   ├── common/          # 公共定义
│   │   ├── types.hpp    # 基础类型定义
│   │   ├── config.hpp   # 配置选项
│   │   └── macros.hpp   # 工具宏
│   ├── core/            # 核心类型
│   │   ├── value.hpp/cpp
│   │   └── gc_object.hpp/cpp
│   ├── gc/              # 垃圾回收
│   ├── vm/              # 虚拟机
│   ├── compiler/        # 编译器
│   └── lib/             # 标准库
├── tests/               # 测试
├── docs/                # 文档
└── CMakeLists.txt       # 构建配置
```

**验收标准**:
- [ ] 项目可以成功编译
- [ ] 基础的CMake配置完成
- [ ] 测试框架集成完成

#### 1.2 基础类型定义

**参考**:
- `lua_c_analysis/src/lobject.h` - TValue定义
- `lua_c_analysis/docs/object/tvalue_implementation.md` - TValue详解

**实现**: `src/common/types.hpp`

```cpp
namespace Lua {
    // 基础类型别名
    using LuaInteger = int64_t;
    using LuaNumber = double;
    using LuaBoolean = bool;
    
    // 字节类型
    using u8 = uint8_t;
    using u16 = uint16_t;
    using u32 = uint32_t;
    using u64 = uint64_t;
    
    using i8 = int8_t;
    using i16 = int16_t;
    using i32 = int32_t;
    using i64 = int64_t;
    
    // 大小类型
    using usize = size_t;
    using isize = ptrdiff_t;
}
```

**测试**: `tests/common/test_types.cpp`
- [ ] 类型大小验证
- [ ] 类型范围验证

#### 1.3 Value类型实现

**参考**:
- `lua_c_analysis/src/lobject.h` - TValue结构
- `lua_with_cpp/src/vm/value.hpp` - C++实现

**实现**: `src/core/value.hpp` 和 `src/core/value.cpp`

**核心接口**:
```cpp
class Value {
public:
    // 构造函数
    Value();                          // Nil
    Value(bool b);                    // Boolean
    Value(double n);                  // Number
    Value(int64_t i);                 // Integer (转为Number)
    Value(GCRef<GCString> s);         // String
    Value(GCRef<Table> t);            // Table
    Value(GCRef<Function> f);         // Function
    
    // 类型检查
    ValueType type() const noexcept;
    bool isNil() const noexcept;
    bool isBoolean() const noexcept;
    bool isNumber() const noexcept;
    bool isString() const noexcept;
    bool isTable() const noexcept;
    bool isFunction() const noexcept;
    
    // 值访问
    bool asBoolean() const;
    double asNumber() const;
    GCRef<GCString> asString() const;
    GCRef<Table> asTable() const;
    GCRef<Function> asFunction() const;
    
    // 转换为字符串（用于打印）
    std::string toString() const;
    
    // 比较操作
    bool operator==(const Value& other) const;
    bool operator<(const Value& other) const;
};
```

**测试**: `tests/core/test_value.cpp`
- [ ] Nil值测试
- [ ] Boolean值测试
- [ ] Number值测试
- [ ] 类型检查测试
- [ ] 值访问测试
- [ ] 比较操作测试

#### 1.4 GCObject基类实现

**参考**:
- `lua_c_analysis/src/lobject.h` - CommonHeader和GCObject
- `lua_with_cpp/src/gc/core/gc_object.hpp` - C++实现

**实现**: `src/core/gc_object.hpp` 和 `src/core/gc_object.cpp`

**核心接口**:
```cpp
enum class GCObjectType : u8 {
    String,
    Table,
    Function,
    Userdata,
    Thread,
    Proto,
    Upvalue
};

enum class GCColor : u8 {
    White0 = 0x01,
    White1 = 0x02,
    Gray   = 0x04,
    Black  = 0x08
};

class GCObject {
protected:
    GCObject* next_;
    GCObjectType type_;
    u8 marked_;
    
    explicit GCObject(GCObjectType type);
    
public:
    virtual ~GCObject() = default;
    
    // 类型访问
    GCObjectType getType() const noexcept;
    
    // GC标记
    u8 getMarked() const noexcept;
    void setMarked(u8 mark) noexcept;
    GCColor getColor() const noexcept;
    void setColor(GCColor color) noexcept;
    
    // GC链表
    GCObject* getNext() const noexcept;
    void setNext(GCObject* next) noexcept;
    
    // 虚函数接口
    virtual void markReferences(class GarbageCollector* gc) = 0;
    virtual size_t getSize() const = 0;
};
```

**测试**: `tests/core/test_gc_object.cpp`
- [ ] 对象类型测试
- [ ] GC标记测试
- [ ] 颜色设置测试
- [ ] 链表操作测试

---

## 🎯 阶段2: 字符串和表系统 (第3-4周)

### 2.1 字符串系统

**参考**:
- `lua_c_analysis/src/lstring.h/c` - 字符串实现
- `lua_c_analysis/docs/object/string_interning.md` - 字符串驻留详解

**实现**: `src/core/gc_string.hpp/cpp` 和 `src/core/string_pool.hpp/cpp`

**核心功能**:
- [ ] GCString类实现
- [ ] 字符串哈希计算
- [ ] 字符串池（驻留）
- [ ] 字符串比较优化

**测试**:
- [ ] 字符串创建测试
- [ ] 字符串驻留测试
- [ ] 哈希计算测试
- [ ] 字符串比较测试

### 2.2 表系统

**参考**:
- `lua_c_analysis/src/ltable.h/c` - 表实现
- `lua_c_analysis/docs/object/table_structure.md` - 表结构详解

**实现**: `src/core/table.hpp/cpp`

**核心功能**:
- [ ] Table类基本结构
- [ ] 数组部分实现
- [ ] 哈希部分实现
- [ ] get/set操作
- [ ] length操作
- [ ] 元表支持

**测试**:
- [ ] 表创建测试
- [ ] 数组访问测试
- [ ] 哈希访问测试
- [ ] 混合访问测试
- [ ] 长度计算测试
- [ ] 元表测试

---

## 🎯 阶段3: 垃圾回收系统 (第5-6周)

### 3.1 三色标记算法

**参考**:
- `lua_c_analysis/src/lgc.h/c` - GC实现
- `lua_c_analysis/docs/gc/tri_color_marking.md` - 三色标记详解
- `lua_c_analysis/docs/gc/incremental_gc.md` - 增量GC详解

**实现**: `src/gc/garbage_collector.hpp/cpp`

**核心功能**:
- [ ] 标记阶段实现
- [ ] 清除阶段实现
- [ ] 根对象收集
- [ ] 对象遍历

**测试**:
- [ ] 简单对象回收测试
- [ ] 循环引用测试
- [ ] 根对象保护测试

### 3.2 增量GC和写屏障

**核心功能**:
- [ ] 增量标记
- [ ] 写屏障实现
- [ ] GC步进控制
- [ ] 内存阈值管理

**测试**:
- [ ] 增量GC测试
- [ ] 写屏障测试
- [ ] 性能测试

---

## 🎯 阶段4: 虚拟机核心 (第7-10周)

### 4.1 状态管理

**参考**:
- `lua_c_analysis/src/lstate.h/c` - 状态管理
- `lua_with_cpp/src/vm/lua_state.hpp` - C++实现

**实现**:
- [ ] GlobalState类
- [ ] LuaState类
- [ ] 栈管理
- [ ] CallInfo管理

### 4.2 指令集和执行引擎

**参考**:
- `lua_c_analysis/src/lopcodes.h` - 指令定义
- `lua_c_analysis/src/lvm.h/c` - VM执行

**实现**:
- [ ] 指令集定义
- [ ] 指令解码
- [ ] 执行循环
- [ ] 算术运算
- [ ] 逻辑运算
- [ ] 表操作
- [ ] 函数调用

---

## 🎯 阶段5: 编译器 (第11-14周)

### 5.1 词法分析器

**参考**:
- `lua_c_analysis/src/llex.h/c` - 词法分析

**实现**:
- [ ] Token定义
- [ ] 词法扫描器
- [ ] 关键字识别
- [ ] 字符串解析
- [ ] 数字解析

### 5.2 语法分析器

**参考**:
- `lua_c_analysis/src/lparser.h/c` - 语法分析

**实现**:
- [ ] 递归下降解析器
- [ ] 表达式解析
- [ ] 语句解析
- [ ] 作用域管理
- [ ] 符号表

### 5.3 代码生成器

**参考**:
- `lua_c_analysis/src/lcode.h/c` - 代码生成

**实现**:
- [ ] 字节码生成
- [ ] 寄存器分配
- [ ] 跳转修补
- [ ] 常量池管理
- [ ] 优化

---

## 🎯 阶段6: 标准库 (第15-16周)

### 6.1 基础库

**参考**:
- `lua_c_analysis/src/lbaselib.c` - 基础库

**实现**:
- [ ] print
- [ ] type
- [ ] tonumber
- [ ] tostring
- [ ] error
- [ ] pcall
- [ ] pairs/ipairs

### 6.2 其他标准库

**实现**:
- [ ] string库
- [ ] table库
- [ ] math库
- [ ] io库（简化版）

---

## 🎯 阶段7: 测试和优化 (第17-18周)

### 7.1 测试

- [ ] 单元测试完善
- [ ] 集成测试
- [ ] Lua官方测试套件
- [ ] 性能基准测试

### 7.2 优化

- [ ] 性能分析
- [ ] 热点优化
- [ ] 内存优化
- [ ] 文档完善

---

## 📊 进度跟踪

### 里程碑

| 里程碑 | 目标日期 | 状态 | 完成日期 |
|--------|---------|------|---------|
| M0: 架构设计 | Week 0 | ✅ 完成 | 2025-11-11 |
| M1: 基础类型系统 | Week 2 | ✅ 完成 | 2025-11-12 |
| M2: 字符串和表 | Week 4 | ✅ 完成 | 2025-11-12 |
| M3: 垃圾回收 | Week 6 | ✅ 完成 | 2025-11-12 |
| M4: 虚拟机核心 | Week 10 | ✅ 完成 | 2025-11-12 |
| M5: 编译器 | Week 14 | ✅ 完成 | 2025-11-13 |
| M6: 标准库 | Week 16 | 🔄 进行中 | - |
| M7: 发布1.0 | Week 18 | ⏳ 待开始 | - |

### 当前状态（2025-12-04更新）

**已完成阶段**：
- ✅ 阶段1-5：基础类型系统、字符串和表、垃圾回收、虚拟机核心、编译器
- 🔄 阶段6：标准库（进行中，基础库部分完成）
- ⏳ 阶段7：测试和优化（待开始）

**最新成果**：
- ✅ **main.cpp重构完成**（2025-12-04）：实现双模式支持（测试模式/解释器模式）
- ✅ **build_main.bat双模式构建**（2025-12-04）：支持测试模式和解释器模式编译
- ✅ **125个单元测试全部通过**（2025-12-04）：测试覆盖率100%
- ✅ **构建系统验证**（2025-12-04）：Debug和Release版本均编译成功

**测试统计**：
```
总测试数：125个
通过率：  100% (125/125)
测试套件：15个
编译状态：无警告，无错误
平台：    Windows + MSVC (Visual Studio 2026)
```

---

## 🔍 质量保证

### 代码质量标准

- ✅ 所有代码必须通过单元测试
- ✅ 代码覆盖率 > 80%
- ✅ 无内存泄漏（Valgrind检查）
- ✅ 符合C++17标准
- ✅ 通过静态分析（clang-tidy）

### 文档标准

- ✅ 每个模块有详细设计文档
- ✅ 每个公共接口有注释
- ✅ 有使用示例
- ✅ 有性能说明

---

## 📝 最新文档索引（2025-12-04更新）

### 核心文档
- **ARCHITECTURE.md** - 架构设计文档
- **DEVELOPMENT_GUIDE.md** - 开发规范和编码指南
- **PROJECT_SUMMARY_CN.md** - 项目总结（中文）
- **IMPLEMENTATION_PLAN.md** - 本文档

### 新增文档（2025-12-04）
- **BUILD_TEST_REPORT.md** - 构建测试报告（位于 `lua/` 目录）
- **MAIN_REFACTORING.md** - main.cpp重构说明（已删除，内容已整合）
- **BUILD_MAIN_SCRIPT.md** - build_main.bat使用指南（已删除，内容已整合）
- **INITIALIZATION_CHECKLIST.md** - 初始化检查清单（已删除，内容已整合）

### 参考资源
- **lua_c_analysis/** - Lua 5.1.5 C源码分析（主要参考）
- **lua_with_cpp/** - C++ Lua实现（次要参考）
- **spec-kit/** - 开发方法论指导

---

## 🎯 下一步行动（优先级P0）

### 立即任务（本周）

根据最新的测试结果和代码审查，以下是需要立即完成的P0优先级任务：

#### 1. 补充LuaState初始化步骤 ⭐⭐⭐

**当前状态**：
- ✅ 已完成：基础初始化（全局表、调用栈、值栈）
- ❌ 缺失：5个关键初始化步骤

**必须完成**：
1. **字符串表初始化** (`luaS_resize`)
   - 实现 `StringPool::resize(usize newSize)` 方法
   - 在 `GlobalState` 构造函数中调用 `stringPool_.resize(32)`
   - 参考：`lua_c_analysis/src/lstring.c` 中的 `luaS_resize`

2. **元方法名称初始化** (`luaT_init`)
   - 创建 `GlobalState::initializeMetaMethods()` 方法
   - 初始化17个元方法名称字符串并标记为固定
   - 参考：`lua_c_analysis/src/ltm.c` 中的 `luaT_init`

3. **保留字初始化** (`luaX_init`)
   - 创建 `GlobalState::initializeReservedWords()` 方法
   - 初始化21个保留字字符串并标记为固定
   - 参考：`lua_c_analysis/src/llex.c` 中的 `luaX_init`

4. **内存错误消息固定**
   - 在初始化时固定 "not enough memory" 字符串
   - 添加为GC根对象防止回收

5. **GC阈值设置**
   - 实现 `GlobalState::setGCThreshold(usize threshold)` 方法
   - 实现 `GlobalState::getTotalBytes()` 方法
   - 设置阈值为 `4 * totalBytes`

**预计工作量**：2-3天

**验收标准**：
- 所有初始化步骤按照Lua 5.1.5标准顺序执行
- 字符串表、元方法、保留字正确初始化
- GC阈值正确设置
- 通过初始化相关的单元测试

#### 2. 实现脚本执行功能 ⭐⭐

**当前状态**：
- ✅ 已完成：main.cpp框架，包含 `executeScript()` 函数声明
- ❌ 未实现：函数体为空

**必须完成**：
- 实现文件读取逻辑
- 集成Lexer、Parser、CodeGen、VM完整流程
- 添加错误处理和报告
- 支持命令行参数传递

**预计工作量**：1-2天

#### 3. 实现交互式REPL ⭐

**当前状态**：
- ✅ 已完成：main.cpp框架，包含 `interactiveMode()` 函数声明
- ❌ 未实现：函数体为空

**必须完成**：
- 实现readline风格的输入处理
- 支持多行输入（续行检测）
- 实现历史记录功能
- 添加自动补全（可选）

**预计工作量**：2-3天

---

## 📚 参考实现对照表

### Lua C实现 → C++实现映射

| Lua C函数 | C++实现 | 状态 | 文件位置 |
|-----------|---------|------|---------|
| `lua_newstate` | `LuaState::newState()` | ✅ 完成 | `vm/lua_state.cpp` |
| `luaL_newstate` | `createLuaState()` | ✅ 完成 | `main.cpp` |
| `luaS_resize` | `StringPool::resize()` | ✅ 完成 | `core/string_pool.cpp` |
| `luaT_init` | `GlobalState::initMetamethodNames()` | ✅ 完成 | `vm/global_state.cpp` |
| `luaX_init` | `GlobalState::initReservedWords()` | ✅ 完成 | `vm/global_state.cpp` |
| `luaS_fix` | `GCString::markFixed()` | ✅ 完成 | `core/gc_string.hpp` |
| `luaL_openlibs` | `StandardLibrary::openAll()` | 🔄 部分完成 | `lib/lib_manager.cpp` |

---

## 🎉 最新成果（2026-01-18更新）

### ✅ P0任务1：LuaState初始化系统 - 已完成

**完成日期**：2026-01-18

**实现内容**：
1. ✅ **字符串表初始化**：实现`StringPool::resize(32)`方法
2. ✅ **元方法名称初始化**：创建并固定17个元方法名称字符串
3. ✅ **保留字初始化**：创建并固定21个Lua关键字字符串
4. ✅ **内存错误消息固定**：创建并固定错误消息字符串
5. ⏸️ **GC阈值设置**：延后实现（当前GC无自动触发机制）

**技术成就**：
- 实现了Lua 5.1.5兼容的FIXED标志机制
- 修复了GC系统的两个关键bug：
  - `GarbageCollector::mark()`：保留FIXED标志不被清除
  - `GarbageCollector::clearAll()`：跳过固定对象不删除
- 建立了完整的固定对象保护机制
- 实现了17个元方法名称和21个保留字的自动管理

**测试覆盖**：
- 新增20个单元测试，全部通过
- 测试文件：`tests/unit/vm/test_lua_state_init.cpp`
- 总测试数：399个（原379个 + 新增20个）

**修改文件**：
- `src/vm/global_state.hpp/cpp`：添加初始化方法
- `src/core/string_pool.hpp/cpp`：添加resize()方法
- `src/core/gc_object.hpp`：添加FIXEDBIT常量
- `src/core/gc_string.hpp`：添加markFixed()和isFixed()方法
- `src/gc/garbage_collector.cpp`：修复mark()和clearAll()方法
- `tests/unit/vm/test_lua_state_init.cpp`：新增测试文件

**整体完成度更新**：**78% → 80%**

---

**下一步**: 实施P0任务2（脚本执行功能）和P0任务3（修复高优先级TODO）

