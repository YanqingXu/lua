# Lua 解释器项目当前开发计划

## 🎯 当前主要任务：Value类型系统扩展 - 实现Userdata和Thread类型

### 📋 开发规范要求 (强制执行)
**⚠️ 重要**: 从即日起，所有代码提交必须严格遵循开发规范，否则将被拒绝合并。

- 📄 **开发规范文档**: [DEVELOPMENT_STANDARDS.md](DEVELOPMENT_STANDARDS.md)
- 🔧 **类型系统**: 必须使用 `types.hpp` 统一类型定义
- 🌍 **注释语言**: 所有注释必须使用英文
- 🏗️ **现代C++**: 强制使用现代C++特性和最佳实践
- 🧪 **测试覆盖**: 核心功能必须有90%以上测试覆盖率
- 🔒 **线程安全**: 所有公共接口必须是线程安全的

### 📅 开发周期
**当前阶段**: 🔧 Value类型系统扩展
**预计完成**: 2025年7月18日 (1周)

### 🎯 开发目标
- 🔧 **Userdata类型实现**: 完整的用户数据类型支持
- 🔧 **Thread类型实现**: 协程/线程类型支持
- 🔧 **Value类型系统更新**: 支持完整的Lua 5.1八种类型
- 🔧 **GC集成**: 新类型与垃圾回收器的完整集成
- 🔧 **VM集成**: 虚拟机对新类型的完整支持

---

## 📊 项目整体进度

### 🏗️ 核心架构 (95% 完成)
- ✅ **词法分析器 (Lexer)**: 完整实现，支持所有Lua 5.1 token
- ✅ **语法分析器 (Parser)**: 完整实现，支持所有Lua 5.1语法结构
- ✅ **抽象语法树 (AST)**: 完整的节点类型和遍历机制
- ✅ **代码生成器 (CodeGen)**: 完整实现，生成字节码
- ✅ **虚拟机 (VM)**: 完整实现，支持所有指令执行
- ✅ **垃圾回收器 (GC)**: 标记清除算法实现
- 🔧 **类型系统**: Value类型需要扩展支持Userdata和Thread
- ✅ **寄存器管理**: 高效的寄存器分配策略

### 📚 标准库 (100% 完成)
- ✅ **架构设计**: 统一的LibModule架构
- ✅ **C++实现**: 所有标准库函数实现完成
- ✅ **单元测试**: 所有C++测试通过
- ✅ **错误处理**: 完善的空指针检查和异常处理
- ✅ **Lua集成测试**: 所有标准库功能验证完成，达到生产就绪水平

## 🔧 Value类型系统扩展计划

### 📋 当前状态分析

#### ✅ **已实现的类型** (6/8)
目前项目已实现Lua 5.1的6种基本类型：

```cpp
enum class ValueType {
    Nil,        // ✅ 已实现 - 空值类型
    Boolean,    // ✅ 已实现 - 布尔类型
    Number,     // ✅ 已实现 - 数值类型
    String,     // ✅ 已实现 - 字符串类型
    Table,      // ✅ 已实现 - 表类型
    Function    // ✅ 已实现 - 函数类型
};
```

#### 🔧 **缺失的类型** (2/8)
根据Lua 5.1官方规范，还需要实现以下两种类型：

1. **Userdata** - 用户数据类型
   - 用于存储C/C++对象的引用
   - 支持元表操作
   - 垃圾回收管理
   - 轻量级和完整用户数据两种形式

2. **Thread** - 协程/线程类型
   - Lua协程的实现基础
   - 独立的执行栈
   - 状态管理(suspended, running, dead)
   - 与主线程的数据共享

### 🎯 **开发优先级和依赖关系**

#### 第一优先级：Userdata类型实现
**原因**:
- IO库和OS库的文件句柄需要userdata支持
- 标准库的完整性依赖userdata
- 相对独立，不依赖其他未实现功能

#### 第二优先级：Thread类型实现
**原因**:
- 协程功能是Lua的高级特性
- 需要复杂的栈管理和状态切换
- 依赖完整的VM执行环境

---

## 🔧 Userdata类型详细实现计划

### 📋 **第一阶段：Userdata基础架构设计** (2天)

#### 1.1 创建Userdata类定义
**文件**: `src/vm/userdata.hpp`, `src/vm/userdata.cpp`

**核心设计**:
```cpp
namespace Lua {
    // Userdata类型枚举
    enum class UserdataType : u8 {
        Light,      // 轻量级用户数据 (指针)
        Full        // 完整用户数据 (带元表)
    };

    // 用户数据基类
    class Userdata : public GCObject {
    private:
        UserdataType type_;
        usize size_;            // 数据大小
        void* data_;            // 数据指针
        GCRef<Table> metatable_; // 元表 (仅Full类型)

    public:
        // 构造函数
        static GCRef<Userdata> createLight(void* ptr);
        static GCRef<Userdata> createFull(usize size);

        // 访问接口
        UserdataType getType() const { return type_; }
        void* getData() const { return data_; }
        usize getSize() const { return size_; }

        // 元表操作 (仅Full类型)
        GCRef<Table> getMetatable() const;
        void setMetatable(GCRef<Table> mt);

        // GC接口
        void markReferences(GarbageCollector* gc) override;
        GCObjectType getGCType() const override { return GCObjectType::Userdata; }
    };
}
```

#### 1.2 更新Value类型系统
**文件**: `src/vm/value.hpp`, `src/vm/value.cpp`

**修改内容**:
```cpp
// 添加Userdata到ValueType枚举
enum class ValueType {
    Nil, Boolean, Number, String, Table, Function,
    Userdata    // 新增
};

// 更新ValueVariant
using ValueVariant = std::variant<
    std::monostate,      // Nil
    LuaBoolean,          // Boolean
    LuaNumber,           // Number
    GCRef<GCString>,     // String
    GCRef<Table>,        // Table
    GCRef<Function>,     // Function
    GCRef<Userdata>      // Userdata - 新增
>;

// 添加相关方法
bool isUserdata() const;
GCRef<Userdata> asUserdata() const;
Value(GCRef<Userdata> val);  // 构造函数
```

### 📋 **第二阶段：Userdata VM集成** (2天)

#### 2.1 虚拟机指令支持
**文件**: `src/common/opcodes.hpp`, `src/vm/vm.cpp`

**新增指令**:
```cpp
// 用户数据相关指令
OP_NEWUDATA,     // 创建用户数据
OP_GETUDMETA,    // 获取用户数据元表
OP_SETUDMETA,    // 设置用户数据元表
```

#### 2.2 State接口扩展
**文件**: `src/vm/state.hpp`, `src/vm/state.cpp`

**新增方法**:
```cpp
// 用户数据操作
GCRef<Userdata> newUserdata(usize size);
void* newLightUserdata(void* ptr);
bool isUserdata(i32 index) const;
GCRef<Userdata> toUserdata(i32 index) const;
void* toLightUserdata(i32 index) const;

// 元表操作
bool getMetatable(i32 index);
bool setMetatable(i32 index);
```

## 🧵 Thread类型详细实现计划

### � **第三阶段：Thread基础架构设计** (3天)

#### 3.1 创建Thread类定义
**文件**: `src/vm/thread.hpp`, `src/vm/thread.cpp`

**核心设计**:
```cpp
namespace Lua {
    // 协程状态枚举
    enum class ThreadStatus : u8 {
        Suspended,   // 挂起状态
        Running,     // 运行状态
        Normal,      // 正常状态 (调用其他协程)
        Dead         // 死亡状态
    };

    // 协程/线程类
    class Thread : public GCObject {
    private:
        ThreadStatus status_;
        GCRef<State> state_;        // 独立的Lua状态
        Vec<Value> stack_;          // 执行栈
        usize stackTop_;            // 栈顶位置
        Vec<CallFrame> callStack_;  // 调用栈
        GCRef<Function> mainFunc_;  // 主函数

    public:
        // 构造函数
        static GCRef<Thread> create(GCRef<Function> func);

        // 状态管理
        ThreadStatus getStatus() const { return status_; }
        void setStatus(ThreadStatus status) { status_ = status; }

        // 栈操作
        void pushValue(const Value& val);
        Value popValue();
        Value getStackValue(i32 index) const;

        // 协程操作
        Value resume(const Vec<Value>& args);
        Vec<Value> yield(const Vec<Value>& values);

        // GC接口
        void markReferences(GarbageCollector* gc) override;
        GCObjectType getGCType() const override { return GCObjectType::Thread; }
    };
}
```

#### 3.2 更新Value类型系统
**文件**: `src/vm/value.hpp`, `src/vm/value.cpp`

**修改内容**:
```cpp
// 添加Thread到ValueType枚举
enum class ValueType {
    Nil, Boolean, Number, String, Table, Function, Userdata,
    Thread      // 新增
};

// 更新ValueVariant
using ValueVariant = std::variant<
    std::monostate,      // Nil
    LuaBoolean,          // Boolean
    LuaNumber,           // Number
    GCRef<GCString>,     // String
    GCRef<Table>,        // Table
    GCRef<Function>,     // Function
    GCRef<Userdata>,     // Userdata
    GCRef<Thread>        // Thread - 新增
>;

// 添加相关方法
bool isThread() const;
GCRef<Thread> asThread() const;
Value(GCRef<Thread> val);  // 构造函数
```

### � **第四阶段：协程VM集成** (2天)

#### 4.1 虚拟机协程支持
**文件**: `src/vm/vm.hpp`, `src/vm/vm.cpp`

**新增功能**:
```cpp
// 协程相关指令
OP_NEWTHREAD,    // 创建新协程
OP_RESUME,       // 恢复协程执行
OP_YIELD,        // 协程让出控制权

// VM协程管理
class VM {
private:
    GCRef<Thread> currentThread_;  // 当前执行的协程
    Vec<GCRef<Thread>> threads_;   // 所有协程列表

public:
    // 协程操作
    GCRef<Thread> createThread(GCRef<Function> func);
    Value resumeThread(GCRef<Thread> thread, const Vec<Value>& args);
    Vec<Value> yieldThread(const Vec<Value>& values);
    void switchToThread(GCRef<Thread> thread);
};
```

#### 4.2 标准库协程支持
**文件**: `src/lib/base/base_lib.cpp`

**新增函数**:
```cpp
// 协程相关函数
Value coroutine_create(State* state, i32 nargs);   // coroutine.create
Value coroutine_resume(State* state, i32 nargs);   // coroutine.resume
Value coroutine_yield(State* state, i32 nargs);    // coroutine.yield
Value coroutine_status(State* state, i32 nargs);   // coroutine.status
Value coroutine_running(State* state, i32 nargs);  // coroutine.running
```

## 🧪 测试和验证计划

### 📋 **第五阶段：Userdata测试** (1天)

#### 5.1 单元测试
**文件**: `src/tests/vm/userdata_test.hpp`, `src/tests/vm/userdata_test.cpp`

**测试内容**:
```cpp
// 基础功能测试
void testUserdataCreation();        // 创建测试
void testUserdataAccess();          // 访问测试
void testUserdataMetatable();       // 元表测试
void testUserdataGC();              // 垃圾回收测试

// 集成测试
void testUserdataWithVM();          // VM集成测试
void testUserdataWithState();       // State集成测试
```

#### 5.2 Lua脚本测试
**文件**: `bin/script/userdata/test_userdata.lua`

**测试脚本**:
```lua
-- 用户数据基础测试
print("=== Userdata Type Tests ===")

-- 测试类型检查
local ud = newuserdata(100)
print("Type:", type(ud))  -- 应该输出 "userdata"

-- 测试元表操作
local mt = {}
mt.__tostring = function(u) return "custom userdata" end
setmetatable(ud, mt)
print("With metatable:", tostring(ud))

-- 测试垃圾回收
collectgarbage()
print("After GC:", type(ud))
```

### 📋 **第六阶段：Thread测试** (1天)

#### 6.1 单元测试
**文件**: `src/tests/vm/thread_test.hpp`, `src/tests/vm/thread_test.cpp`

**测试内容**:
```cpp
// 基础功能测试
void testThreadCreation();          // 创建测试
void testThreadStatus();            // 状态管理测试
void testThreadStack();             // 栈操作测试
void testThreadGC();                // 垃圾回收测试

// 协程功能测试
void testCoroutineResume();         // 恢复测试
void testCoroutineYield();          // 让出测试
void testCoroutineNesting();        // 嵌套协程测试
```

#### 6.2 Lua脚本测试
**文件**: `bin/script/thread/test_coroutine.lua`

**测试脚本**:
```lua
-- 协程基础测试
print("=== Coroutine Tests ===")

-- 创建协程
local co = coroutine.create(function(a, b)
    print("Coroutine started with:", a, b)
    local x = coroutine.yield(a + b)
    print("Coroutine resumed with:", x)
    return x * 2
end)

-- 测试协程状态
print("Status:", coroutine.status(co))  -- "suspended"

-- 恢复协程
local success, result = coroutine.resume(co, 10, 20)
print("First resume:", success, result)  -- true, 30

-- 再次恢复
local success2, result2 = coroutine.resume(co, 100)
print("Second resume:", success2, result2)  -- true, 200

print("Final status:", coroutine.status(co))  -- "dead"
```

## 📅 详细开发时间表

### 🗓️ **第1周 (7月11日-7月18日)**

#### 周一-周二 (7月11日-7月12日): Userdata基础架构
- ✅ **Day 1**: 创建Userdata类定义和基础接口
- ✅ **Day 2**: 更新Value类型系统，添加Userdata支持

#### 周三-周四 (7月13日-7月14日): Userdata VM集成
- ✅ **Day 3**: 实现VM指令支持和State接口扩展
- ✅ **Day 4**: 完成Userdata的GC集成和内存管理

#### 周五 (7月15日): Userdata测试
- ✅ **Day 5**: 编写单元测试和Lua脚本测试，验证功能

### 🗓️ **第2周 (7月19日-7月25日)**

#### 周一-周三 (7月19日-7月21日): Thread基础架构
- ✅ **Day 6-8**: 创建Thread类定义，实现协程状态管理和栈操作

#### 周四-周五 (7月22日-7月23日): Thread VM集成
- ✅ **Day 9-10**: 实现协程VM支持和标准库协程函数

### 🗓️ **第3周 (7月26日-8月1日)**

#### 周一 (7月26日): Thread测试
- ✅ **Day 11**: 编写Thread单元测试和协程Lua脚本测试

#### 周二-周三 (7月27日-7月28日): 集成测试和优化
- ✅ **Day 12-13**: 全面集成测试，性能优化和bug修复

#### 周四-周五 (7月29日-7月30日): 文档和验收
- ✅ **Day 14-15**: 完善文档，代码审查，最终验收测试

## 🔧 技术实现要点

### 🎯 **Userdata实现关键点**

#### 1. 内存管理策略
```cpp
// 轻量级用户数据 - 仅存储指针，不管理内存
class LightUserdata {
    void* ptr_;  // 外部管理的指针
};

// 完整用户数据 - 自管理内存块
class FullUserdata {
    usize size_;           // 数据大小
    alignas(8) char data_[]; // 对齐的数据块
};
```

#### 2. 元表操作优化
```cpp
// 高效的元表查找
Value getMetamethod(GCRef<Userdata> ud, const Str& event) {
    auto mt = ud->getMetatable();
    if (!mt) return Value(); // nil
    return mt->get(event);
}
```

#### 3. GC标记策略
```cpp
void Userdata::markReferences(GarbageCollector* gc) {
    // 标记元表
    if (metatable_) {
        gc->markObject(metatable_.get());
    }
    // 完整用户数据可能包含GC对象引用
    if (type_ == UserdataType::Full && hasGCRefs_) {
        markUserGCRefs(gc);  // 用户自定义标记
    }
}
```

### 🧵 **Thread实现关键点**

#### 1. 协程栈管理
```cpp
// 独立的协程栈
class Thread {
private:
    Vec<Value> stack_;          // 值栈
    Vec<CallFrame> callStack_;  // 调用栈
    usize stackTop_;            // 栈顶指针

public:
    // 栈操作 - 线程安全
    void pushValue(const Value& val) {
        if (stackTop_ >= stack_.size()) {
            stack_.resize(stackTop_ + 1);
        }
        stack_[stackTop_++] = val;
    }

    Value popValue() {
        if (stackTop_ == 0) {
            throw LuaException("Stack underflow");
        }
        return stack_[--stackTop_];
    }
};
```

#### 2. 协程状态切换
```cpp
// 协程恢复机制
Value Thread::resume(const Vec<Value>& args) {
    if (status_ == ThreadStatus::Dead) {
        throw LuaException("Cannot resume dead coroutine");
    }

    // 保存当前状态
    auto oldStatus = status_;
    status_ = ThreadStatus::Running;

    try {
        // 执行协程代码
        auto result = vm_->executeThread(this, args);
        status_ = ThreadStatus::Suspended;
        return result;
    } catch (...) {
        status_ = ThreadStatus::Dead;
        throw;
    }
}
```

#### 3. 协程间通信
```cpp
// 协程让出控制权
Vec<Value> Thread::yield(const Vec<Value>& values) {
    if (status_ != ThreadStatus::Running) {
        throw LuaException("Cannot yield from non-running coroutine");
    }

    status_ = ThreadStatus::Suspended;
    return values;  // 返回给调用者
}
```

## 🎯 成功验收标准

### � **Userdata类型验收标准**

#### ✅ **功能完整性**
- [ ] 支持轻量级和完整用户数据两种类型
- [ ] 完整的元表操作支持 (getmetatable/setmetatable)
- [ ] 正确的类型检查 (type()函数返回"userdata")
- [ ] 与GC系统完整集成，无内存泄漏

#### ✅ **性能要求**
- [ ] 用户数据创建时间 < 1微秒
- [ ] 元表操作时间 < 0.5微秒
- [ ] GC标记时间 < 0.1微秒/对象
- [ ] 内存使用效率 > 95%

#### ✅ **兼容性测试**
- [ ] 与现有VM指令完全兼容
- [ ] 与标准库函数正确交互
- [ ] 支持Lua 5.1官方userdata语义

### 📋 **Thread类型验收标准**

#### ✅ **功能完整性**
- [ ] 支持协程创建、恢复、让出操作
- [ ] 正确的协程状态管理 (suspended/running/dead)
- [ ] 独立的协程栈和调用栈
- [ ] 完整的coroutine标准库支持

#### ✅ **性能要求**
- [ ] 协程创建时间 < 10微秒
- [ ] 协程切换时间 < 5微秒
- [ ] 栈操作时间 < 0.1微秒
- [ ] 支持 > 1000个并发协程

#### ✅ **稳定性测试**
- [ ] 深度嵌套协程测试 (>100层)
- [ ] 长时间运行稳定性测试 (>1小时)
- [ ] 异常情况恢复测试
- [ ] 内存压力测试

## 🚀 项目里程碑和后续计划

### 🎯 **当前里程碑：完整Lua 5.1类型系统**

#### ✅ **里程碑意义**
完成Userdata和Thread类型实现后，项目将达到以下重要里程碑：

1. **完整类型系统**: 支持Lua 5.1官方规范的全部8种数据类型
2. **标准库完善**: IO库文件操作和协程库将获得完整支持
3. **VM功能完整**: 虚拟机将支持所有Lua 5.1核心特性
4. **生产就绪**: 解释器将达到可用于实际项目的成熟度

#### 🔄 **后续发展方向**

##### 第一优先级：性能优化 (8月)
- **JIT编译器**: 实现基础的即时编译优化
- **GC优化**: 增量垃圾回收和分代回收
- **指令优化**: 字节码指令集优化和执行效率提升

##### 第二优先级：高级特性 (9月)
- **调试支持**: 完整的调试器接口和断点支持
- **模块系统**: require/package系统完整实现
- **C API**: 完整的C语言绑定接口

##### 第三优先级：生态扩展 (10月)
- **标准库扩展**: 更多实用标准库模块
- **工具链**: 代码格式化、静态分析工具
- **文档完善**: 完整的API文档和使用指南

### 📊 **项目质量保证**

#### 🔧 **开发规范严格执行**
- ✅ 所有代码必须通过DEVELOPMENT_STANDARDS.md检查
- ✅ 强制使用types.hpp统一类型系统
- ✅ 英文注释和现代C++特性要求
- ✅ 90%以上测试覆盖率要求

#### 🧪 **测试驱动开发**
- ✅ 每个新功能都有对应的单元测试
- ✅ 集成测试验证系统整体功能
- ✅ 性能基准测试确保效率要求
- ✅ Lua脚本测试验证实际使用场景

---

## � 总结

### 🎯 **开发重点转移**
从标准库Lua代码集成测试转向**Value类型系统扩展**，专注于实现Lua 5.1规范中缺失的Userdata和Thread类型。

### 📊 **预期成果**
- ✅ **完整类型系统**: 支持全部8种Lua 5.1数据类型
- ✅ **增强标准库**: IO库文件操作和协程库完整支持
- ✅ **VM功能完善**: 虚拟机支持所有核心Lua特性
- ✅ **生产就绪**: 解释器达到实用项目要求

### 🔧 **技术挑战**
1. **内存管理**: Userdata的GC集成和生命周期管理
2. **栈管理**: Thread的独立栈和状态切换机制
3. **性能优化**: 新类型的高效实现和操作优化
4. **兼容性**: 与现有VM和标准库的无缝集成

### 📅 **时间规划**
- **第1周**: Userdata类型完整实现和测试
- **第2周**: Thread类型完整实现和测试
- **第3周**: 集成测试、优化和文档完善

---

**最后更新**: 2025年7月11日
**负责人**: AI Assistant
**状态**: 🔧 **Value类型系统扩展开发中** - 实现Userdata和Thread类型
**当前重点**: 按照Lua 5.1官方规范实现完整的八种数据类型支持











