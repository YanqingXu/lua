# Lua 垃圾回收系统详解

## 概述

本文档详细介绍了 Lua 解释器中垃圾回收(Garbage Collection, GC)系统的设计原理、架构实现以及与其他核心类型的集成方式。该垃圾回收系统采用现代化的三色标记清除算法，提供高效的内存管理和自动对象生命周期管理。

## 系统架构

### 整体设计原则

1. **模块化设计**: 每个子目录处理垃圾回收的特定方面
2. **性能优化**: 针对最小暂停时间和高吞吐量进行优化
3. **可配置性**: 为不同使用场景提供可调参数
4. **可扩展性**: 易于添加新算法和特性
5. **可维护性**: 清晰的关注点分离和良好的文档

### 目录结构

```
src/gc/
├── core/                    # 核心垃圾回收实现
│   ├── garbage_collector.hpp/cpp  # 主垃圾回收器类
│   ├── gc_object.hpp             # GC对象基类
│   ├── gc_ref.hpp/cpp            # GC引用包装器
│   ├── gc_string.hpp/cpp         # GC字符串实现
│   └── string_pool.hpp/cpp       # 字符串池(字符串驻留)
├── algorithms/              # 具体GC算法实现
│   ├── gc_marker.hpp/cpp         # 三色标记算法
│   └── gc_sweeper.hpp/cpp        # 清除算法
├── memory/                  # 内存管理工具
│   ├── allocator.hpp             # GC感知的内存分配器
│   ├── memory_pool.hpp           # 内存池管理
│   └── optimized_allocator.hpp   # 优化分配器
├── utils/                   # 工具类型和辅助函数
│   └── gc_types.hpp              # 核心类型定义和枚举
├── features/                # 高级GC特性(预留)
├── integration/             # VM组件集成(预留)
└── README.md               # 模块说明文档
```

## 核心组件详解

### 1. GCObject - 垃圾回收对象基类

`GCObject` 是所有需要被垃圾回收器管理的对象的基类，实现了三色标记清除算法的核心功能。

#### 关键特性

```cpp
class GCObject {
private:
    // GC标记信息打包到单个字节中
    // 位 0-1: 颜色 (White0, White1, Gray, Black)
    // 位 2: 固定对象标志 (永不回收)
    // 位 3: 已终结标志
    // 位 4: 弱引用标志
    // 位 5: 分离标志 (用于弱表)
    // 位 6-7: 保留位
    mutable Atom<u8> gcMark;
    
    GCObjectType objectType;     // 对象类型
    usize objectSize;            // 对象大小(字节)
    GCObject* nextObject;        // 分配链中的下一个对象
    GCObject* prevObject;        // 分配链中的上一个对象
    FinalizerState finalizerState; // 终结器状态
    u8 generation;               // 分代GC的代数
};
```

#### 核心方法

- **`markReferences(GarbageCollector* gc)`**: 标记此对象引用的所有对象(纯虚函数)
- **`finalize(State* state)`**: 对象销毁前的可选终结器
- **`getSize()`**: 获取对象大小用于内存统计
- **颜色管理**: `getColor()`, `setColor()`, `isWhite()`, `isGray()`, `isBlack()`

### 2. GarbageCollector - 主垃圾回收器

主垃圾回收器类实现了三色标记清除算法的协调逻辑。

```cpp
class GarbageCollector {
private:
    State* luaState;  // Lua状态引用
    
public:
    void markObject(GCObject* obj);     // 标记单个对象为可达
    void collectGarbage();              // 执行完整的垃圾回收周期
    bool shouldCollect() const;         // 检查是否应触发GC
};
```

### 3. GCMarker - 三色标记算法

实现三色标记算法的核心类，管理标记阶段的对象遍历。

#### 三色标记原理

- **白色(White)**: 可能是垃圾的对象(尚未访问)
- **灰色(Gray)**: 可达但其子对象尚未扫描的对象
- **黑色(Black)**: 可达且其子对象已扫描的对象

#### 标记过程

```cpp
class GCMarker {
private:
    Vec<GCObject*> grayStack;        // 灰色对象栈
    HashSet<GCObject*> graySet;      // 灰色对象集合(避免重复)
    usize markedObjectCount;         // 已标记对象计数
    usize maxGrayStackSize;          // 最大灰色栈大小
    
public:
    void markFromRoots(const Vec<GCObject*>& rootObjects, GCColor currentWhite);
    void markObject(GCObject* object, GCColor currentWhite);
    void processGrayObjects(GCColor currentWhite);
};
```

#### 标记算法流程

1. **初始化**: 重置统计信息，清空灰色栈
2. **标记根对象**: 将所有根对象标记为灰色
3. **标记字符串池**: 确保驻留字符串不被回收
4. **处理灰色对象**: 
   - 从灰色栈弹出对象
   - 标记为黑色(已处理)
   - 标记其所有子对象
   - 重复直到灰色栈为空

### 4. GCSweeper - 清除算法

实现标记清除算法的清除阶段，释放未标记的对象。

```cpp
class GCSweeper {
public:
    struct SweepStats {
        usize objectsSwept;      // 处理的对象总数
        usize objectsFreed;      // 释放的对象数
        usize bytesFreed;        // 释放的字节数
        usize objectsKept;       // 保留的对象数
        u64 sweepTimeUs;         // 清除时间(微秒)
        usize finalizersRun;     // 执行的终结器数量
    };
    
    GCObject* sweepAll(GCObject* objectList, GCColor white);
    bool sweepStep();  // 增量清除
};
```

#### 清除过程

1. **遍历对象链**: 检查每个分配的对象
2. **判断回收**: 如果对象是白色(未标记)，则回收
3. **执行终结器**: 对需要终结的对象执行清理
4. **更新链表**: 从分配链中移除已回收对象
5. **颜色翻转**: 为下一轮GC准备颜色

### 5. GCRef - 类型安全的GC引用

提供类型安全的GC对象引用，不干扰垃圾回收过程。

```cpp
template<typename T>
class GCRef {
private:
    T* ptr;
    
public:
    // 零开销的指针包装器
    T* operator->() const noexcept { return ptr; }
    T& operator*() const noexcept { return *ptr; }
    T* get() const noexcept { return ptr; }
    
    // 空指针安全检查
    explicit operator bool() const noexcept { return ptr != nullptr; }
    bool operator!() const noexcept { return ptr == nullptr; }
};
```

#### 关键特性

- **零开销**: 仅是类型化的指针包装器
- **类型安全**: 防止错误的类型转换
- **GC集成**: 与标记清除算法无缝配合
- **空指针安全**: 提供安全的空指针检查

## 内存管理系统

### 1. GCAllocator - GC感知的内存分配器

提供高效的GC对象内存分配，支持对象池和内存跟踪。

```cpp
class GCAllocator {
private:
    // 对象池(2的幂次大小)
    static constexpr usize NUM_POOLS = 16;
    std::array<UPtr<ObjectPool>, NUM_POOLS> objectPools;
    
    // 大对象分配(> MAX_POOL_SIZE)
    HashMap<void*, MemoryBlockHeader*> largeObjects;
    
    // 内存统计
    Atom<usize> totalAllocated;
    Atom<usize> totalFreed;
    Atom<usize> currentUsage;
    Atom<usize> gcThreshold;
    
public:
    template<typename T, typename... Args>
    T* allocateObject(GCObjectType type, Args&&... args);
    
    void deallocate(void* ptr);
    bool shouldTriggerGC() const;
};
```

### 2. 内存池系统

#### ObjectPool - 固定大小对象池

```cpp
class ObjectPool {
private:
    usize objectSize;            // 对象大小
    usize chunkSize;             // 内存块大小
    Vec<void*> chunks;           // 内存块列表
    MemoryBlockHeader* freeList; // 空闲对象链表
    
public:
    void* allocate(GCObjectType type);
    void deallocate(void* ptr);
    bool owns(void* ptr) const;
};
```

#### FixedSizePool - 固定大小内存池

```cpp
class FixedSizePool {
private:
    MemoryChunk* chunks;         // 内存块链表
    MemoryChunk* currentChunk;   // 当前分配块
    usize totalChunks;           // 总块数
    usize totalObjects;          // 总对象数
    
public:
    void* allocate();
    void deallocate(void* ptr);
    void shrink();               // 移除空块
};
```

### 3. 字符串池(String Interning)

字符串驻留系统，确保相同字符串共享内存位置。

```cpp
class StringPool {
private:
    HashSet<GCString*, GCStringHash, GCStringEqual> pool;
    mutable std::mutex poolMutex;
    
public:
    static StringPool& getInstance();  // 单例模式
    
    GCString* intern(const Str& str);  // 驻留字符串
    void remove(GCString* gcString);   // 移除字符串
    void markAll(GarbageCollector* gc); // 标记所有字符串
};
```

## 类型集成

### 1. Value 类集成

`Value` 类完全集成了GC系统，能够安全地引用GC对象。

```cpp
class Value {
public:
    bool isGCObject() const;           // 识别GC对象
    GCObject* asGCObject() const;      // 获取GC对象指针
    void markReferences(GarbageCollector* gc); // 标记GC引用
    
    // 使用GCRef安全引用GC对象
    GCRef<GCString> asString() const;
    GCRef<Table> asTable() const;
    GCRef<Function> asFunction() const;
};
```

### 2. Table 类集成

`Table` 类继承自 `GCObject`，实现完整的GC集成。

```cpp
class Table : public GCObject {
public:
    Table() : GCObject(GCObjectType::Table) {}
    
    void markReferences(GarbageCollector* gc) override {
        // 标记数组部分的所有Value
        for (const Value& val : arrayPart) {
            if (val.isGCObject()) {
                gc->markObject(val.asGCObject());
            }
        }
        
        // 标记哈希部分的所有键值对
        for (const auto& [key, value] : hashPart) {
            if (key.isGCObject()) {
                gc->markObject(key.asGCObject());
            }
            if (value.isGCObject()) {
                gc->markObject(value.asGCObject());
            }
        }
        
        // 标记元表引用
        if (metatable) {
            gc->markObject(metatable.get());
        }
    }
};

// 工厂函数
GCRef<Table> make_gc_table();
```

### 3. Function 类集成

`Function` 类继承自 `GCObject`，支持Lua函数和原生函数的GC管理。

```cpp
class Function : public GCObject {
public:
    Function(Type type) : GCObject(GCObjectType::Function), functionType(type) {}
    
    void markReferences(GarbageCollector* gc) override {
        // 标记常量表中的所有Value
        for (const Value& constant : constants) {
            if (constant.isGCObject()) {
                gc->markObject(constant.asGCObject());
            }
        }
        
        // 标记上值(upvalues)引用
        for (const auto& upvalue : upvalues) {
            if (upvalue && upvalue->isGCObject()) {
                gc->markObject(upvalue->asGCObject());
            }
        }
        
        // 标记函数原型引用
        if (prototype) {
            gc->markObject(prototype.get());
        }
    }
};

// 工厂函数
GCRef<Function> make_gc_function(Function::Type type);
```

### 4. State 类集成

`State` 类作为Lua状态机，也被纳入GC管理。

```cpp
class State : public GCObject {
public:
    State() : GCObject(GCObjectType::State) {}
    
    void markReferences(GarbageCollector* gc) override {
        // 标记栈中的所有Value
        for (const Value& val : stack) {
            if (val.isGCObject()) {
                gc->markObject(val.asGCObject());
            }
        }
        
        // 标记全局变量中的所有Value
        for (const auto& [name, value] : globals) {
            if (value.isGCObject()) {
                gc->markObject(value.asGCObject());
            }
        }
    }
};

// 工厂函数
GCRef<State> make_gc_state();
```

## GC配置和统计

### 1. GC配置参数

```cpp
struct GCConfig {
    // 内存阈值配置
    usize initialThreshold = 1024 * 1024;  // 初始GC阈值(1MB)
    usize maxThreshold = 64 * 1024 * 1024;  // 最大GC阈值(64MB)
    double growthFactor = 2.0;               // 阈值增长因子
    
    // 增量GC配置
    usize stepSize = 1024;                   // 每步处理的对象数
    u32 stepTimeMs = 5;                      // 每步最大时间(毫秒)
    double pauseMultiplier = 200.0;          // 暂停倍数(百分比)
    
    // 分代GC配置(可选)
    bool enableGenerational = false;         // 启用分代回收
    usize youngGenThreshold = 256 * 1024;   // 年轻代阈值
    u32 youngGenRatio = 20;                  // 年轻代回收比例
    
    // 调试和监控
    bool enableStats = true;                 // 启用统计
    bool enableLogging = false;              // 启用日志
    u32 logLevel = 1;                        // 日志级别(0-3)
};
```

### 2. GC统计信息

```cpp
struct GCStats {
    // 内存统计
    usize totalAllocated;        // 总分配内存
    usize totalFreed;            // 总释放内存
    usize currentUsage;          // 当前内存使用
    usize peakUsage;             // 峰值内存使用
    
    // 对象统计
    usize totalObjects;          // 总对象数
    usize liveObjects;           // 存活对象数
    usize collectedObjects;      // 已回收对象数
    
    // GC执行统计
    u64 gcCycles;                // GC周期数
    u64 totalGCTime;             // 总GC时间(微秒)
    u64 maxPauseTime;            // 最大暂停时间(微秒)
    u64 avgPauseTime;            // 平均暂停时间(微秒)
};
```

## 使用示例

### 1. 创建GC管理的对象

```cpp
#include "vm/state_factory.hpp"
#include "vm/table.hpp"
#include "vm/function.hpp"

// 创建GC管理的State对象
GCRef<State> state = make_gc_state();

// 创建GC管理的Table对象
GCRef<Table> table = make_gc_table();

// 创建GC管理的Function对象
Vec<Instruction> code;
Vec<Value> constants;
GCRef<Function> func = Function::createLua(
    std::make_shared<Vec<Instruction>>(code),
    constants, 0, 0, 0
);
```

### 2. 使用Value存储GC对象

```cpp
// Value自动使用GCRef来引用GC对象
Value stringValue("Hello World");  // 自动创建GCString
Value tableValue(table);           // 使用GCRef<Table>
Value functionValue(func);         // 使用GCRef<Function>

// 存储到State中
state->push(stringValue);
state->setGlobal("myTable", tableValue);
```

### 3. 执行垃圾回收

```cpp
#include "gc/core/garbage_collector.hpp"

// 创建垃圾回收器
GarbageCollector gc(state.get());

// 执行垃圾回收
gc.collectGarbage();
```

### 4. 配置GC参数

```cpp
// 创建自定义GC配置
GCConfig config;
config.initialThreshold = 2 * 1024 * 1024;  // 2MB初始阈值
config.enableGenerational = true;           // 启用分代GC
config.enableStats = true;                   // 启用统计

// 使用配置创建分配器
GCAllocator allocator(config);
```

## 技术特性

### 1. 三色标记清除算法

- **精确回收**: 所有核心类型正确实现标记阶段
- **循环引用处理**: 支持循环引用的检测和处理
- **增量回收**: 支持增量式垃圾回收，减少暂停时间

### 2. 类型安全

- **编译时检查**: 使用 `GCRef<T>` 提供编译时类型检查
- **防止错误**: 避免类型转换错误
- **空指针安全**: 安全的空指针检查

### 3. 性能优化

- **零开销引用**: GCRef是零开销的指针包装器
- **高效标记**: 优化的对象标记算法
- **内存统计**: 精确的内存使用追踪
- **对象池**: 减少分配开销和内存碎片

### 4. 内存管理

- **自动生命周期**: 自动对象生命周期管理
- **精确追踪**: 精确的内存使用追踪
- **分层分配**: 支持大对象和小对象的不同分配策略
- **字符串驻留**: 字符串池减少重复字符串的内存占用

### 5. 线程安全

- **原子操作**: 关键数据结构使用原子操作
- **互斥锁**: 字符串池和分配器使用互斥锁保护
- **无锁设计**: GCRef等核心组件采用无锁设计

## 调试和监控

### 1. 统计信息收集

```cpp
// 获取GC统计信息
GCStats stats = allocator.getStats();

std::cout << "总分配内存: " << stats.totalAllocated << " 字节\n";
std::cout << "当前使用: " << stats.currentUsage << " 字节\n";
std::cout << "GC周期数: " << stats.gcCycles << "\n";
std::cout << "平均暂停时间: " << stats.avgPauseTime << " 微秒\n";
```

### 2. 调试支持

```cpp
#ifdef DEBUG_GC
// 调试模式下的额外检查
class GCObject {
private:
    mutable Atom<u32> debugRefCount{0};  // 调试引用计数
    
public:
    void addDebugRef() const { debugRefCount++; }
    void removeDebugRef() const { debugRefCount--; }
    u32 getDebugRefCount() const { return debugRefCount; }
};
#endif
```

### 3. 日志记录

```cpp
// GC事件日志
if (config.enableLogging) {
    std::cout << "[GC] 开始标记阶段, 根对象数: " << rootObjects.size() << "\n";
    std::cout << "[GC] 标记完成, 已标记对象: " << markedObjectCount << "\n";
    std::cout << "[GC] 清除完成, 释放对象: " << stats.objectsFreed 
              << ", 释放内存: " << stats.bytesFreed << " 字节\n";
}
```

## 最佳实践

### 1. 对象设计

- **继承GCObject**: 所有需要GC管理的类都应继承 `GCObject`
- **实现markReferences**: 正确实现 `markReferences` 方法标记所有引用
- **使用GCRef**: 使用 `GCRef<T>` 而不是原始指针引用GC对象

### 2. 内存管理

- **避免循环引用**: 尽量避免强循环引用，考虑使用弱引用
- **及时释放**: 不再需要的对象应及时解除引用
- **合理配置**: 根据应用特点调整GC参数

### 3. 性能优化

- **批量操作**: 批量创建对象以减少GC压力
- **对象重用**: 重用对象而不是频繁创建销毁
- **监控统计**: 定期检查GC统计信息，优化性能瓶颈

### 4. 错误处理

- **空指针检查**: 始终检查GCRef是否为空
- **异常安全**: 确保异常情况下的内存安全
- **资源清理**: 在终结器中正确清理资源

## 未来发展

### 1. 计划特性

- **分代垃圾回收**: 完整的分代GC实现
- **并发垃圾回收**: 支持并发标记和清除
- **弱引用**: 完整的弱引用系统
- **终结器**: 更完善的对象终结机制

### 2. 性能改进

- **NUMA感知**: 针对NUMA架构的优化
- **缓存友好**: 改进数据结构的缓存局部性
- **向量化**: 利用SIMD指令优化标记过程

### 3. 工具支持

- **可视化工具**: GC行为可视化工具
- **性能分析**: 详细的性能分析工具
- **内存泄漏检测**: 自动内存泄漏检测

## 总结

Lua垃圾回收系统采用现代化的设计理念，实现了高效、安全、可扩展的内存管理。通过三色标记清除算法、类型安全的引用系统、优化的内存分配器和完善的集成机制，为Lua解释器提供了强大的内存管理能力。

### 核心优势

1. **高性能**: 优化的算法和数据结构，最小化GC暂停时间
2. **类型安全**: 编译时类型检查，防止内存错误
3. **易于集成**: 清晰的接口设计，易于与现有代码集成
4. **可配置**: 丰富的配置选项，适应不同应用场景
5. **可监控**: 详细的统计信息，便于性能调优

### 开发流程

1. **设计阶段**: 继承GCObject，实现markReferences
2. **实现阶段**: 使用GCRef引用，正确处理对象生命周期
3. **测试阶段**: 验证内存安全，检查性能指标
4. **优化阶段**: 根据统计信息调整配置，优化性能

该垃圾回收系统为Lua解释器提供了坚实的内存管理基础，支持复杂的对象关系和高性能的内存操作，是现代编程语言运行时系统的优秀实现。
