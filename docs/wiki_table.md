# Lua Table 和 Table 库设计文档

## 概述

本文档详细介绍了 Lua 解释器项目中 Table 数据结构和 Table 库的设计与实现。Table 是 Lua 中最重要的数据结构，既可以作为数组使用，也可以作为哈希表使用，是 Lua 语言的核心特性之一。

## Table 数据结构设计

### 核心架构

Table 类继承自 `GCObject`，支持垃圾回收机制。采用混合存储策略：

- **数组部分 (Array Part)**: 使用 `Vec<Value>` 存储连续的整数索引元素
- **哈希部分 (Hash Part)**: 使用 `Vec<void*>` 存储 Entry 指针，处理非连续索引

```cpp
class Table : public GCObject {
private:
    Vec<Value> array;        // 数组部分
    Vec<void*> entries;      // 哈希表部分
    Table* metatable;        // 元表
};
```

### Entry 结构

哈希部分的每个条目使用 Entry 结构存储：

```cpp
struct Table::Entry {
    Value key;    // 键
    Value value;  // 值
};
```

### 存储策略

#### 数组部分存储规则
- 键必须是正整数（>= 1）
- 键值连续时存储在数组部分
- 使用 1-based 索引（Lua 标准）
- 自动扩容以适应新元素

#### 哈希部分存储规则
- 非整数键或不连续的整数键
- 使用线性搜索查找（简化实现）
- 动态分配 Entry 对象

## 核心操作实现

### get 操作

```cpp
Value Table::get(const Value& key) {
    // 1. 检查是否为数组索引
    if (key.isNumber()) {
        LuaNumber n = key.asNumber();
        if (n == std::floor(n) && n >= 1 && n <= array.size()) {
            return array[static_cast<size_t>(n - 1)];
        }
    }
    
    // 2. 在哈希部分搜索
    int index = findEntry(key);
    if (index >= 0) {
        Entry* entry = static_cast<Entry*>(entries[index]);
        return entry->value;
    }
    
    // 3. 返回 nil
    return Value(nullptr);
}
```

### set 操作

```cpp
void Table::set(const Value& key, const Value& value) {
    // 1. 拒绝 nil 键
    if (key.isNil()) return;
    
    // 2. 尝试存储到数组部分
    if (key.isNumber()) {
        LuaNumber n = key.asNumber();
        if (n == std::floor(n) && n >= 1) {
            size_t index = static_cast<size_t>(n - 1);
            if (index >= array.size()) {
                if (value.isNil()) return;
                array.resize(index + 1, Value(nullptr));
            }
            array[index] = value;
            return;
        }
    }
    
    // 3. 处理哈希部分
    int index = findEntry(key);
    if (value.isNil()) {
        // 删除元素
        if (index >= 0) {
            delete static_cast<Entry*>(entries[index]);
            entries[index] = entries.back();
            entries.pop_back();
        }
    } else {
        // 更新或添加元素
        if (index >= 0) {
            static_cast<Entry*>(entries[index])->value = value;
        } else {
            entries.push_back(new Entry(key, value));
        }
    }
}
```

## 垃圾回收集成

### 引用标记

```cpp
void Table::markReferences(GarbageCollector* gc) {
    // 标记数组部分的所有值
    for (const auto& value : array) {
        if (value.isGCObject()) {
            gc->markObject(value.asGCObject());
        }
    }
    
    // 标记哈希部分的键值对
    for (const auto& entryPtr : entries) {
        Entry* entry = static_cast<Entry*>(entryPtr);
        if (entry->key.isGCObject()) {
            gc->markObject(entry->key.asGCObject());
        }
        if (entry->value.isGCObject()) {
            gc->markObject(entry->value.asGCObject());
        }
    }
    
    // 标记元表
    if (metatable != nullptr) {
        gc->markObject(metatable);
    }
}
```

### 内存计算

```cpp
usize Table::getAdditionalSize() const {
    usize arraySize = array.capacity() * sizeof(Value);
    usize entriesSize = entries.capacity() * sizeof(void*) + 
                       entries.size() * sizeof(Entry);
    return arraySize + entriesSize;
}
```

## Table 库函数

### 库结构

`TableLib` 类继承自 `LibModule`，提供标准的 Lua table 库函数：

```cpp
class TableLib : public LibModule {
public:
    // 核心函数
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

### 主要函数实现

#### table.insert

支持两种调用方式：
- `table.insert(table, value)` - 在末尾插入
- `table.insert(table, pos, value)` - 在指定位置插入

```cpp
Value TableLib::insert(State* state, int nargs) {
    if (nargs < 2 || nargs > 3) {
        throw std::runtime_error("table.insert: wrong number of arguments");
    }
    
    auto table = state->toTable(1);
    
    if (nargs == 2) {
        // 末尾插入
        Value value = state->get(2);
        int len = getTableLength(*table);
        table->set(Value(len + 1), value);
    } else {
        // 指定位置插入
        int pos = static_cast<int>(state->toNumber(2));
        Value value = state->get(3);
        int len = getTableLength(*table);
        
        // 元素右移
        for (int i = len; i >= pos; i--) {
            Value elem = table->get(Value(i));
            table->set(Value(i + 1), elem);
        }
        
        table->set(Value(pos), value);
    }
    
    return Value(); // nil
}
```

#### table.remove

支持两种调用方式：
- `table.remove(table)` - 移除最后一个元素
- `table.remove(table, pos)` - 移除指定位置元素

实现包括元素左移逻辑以保持数组连续性。

#### table.sort

使用快速排序算法：

```cpp
Value TableLib::sort(State* state, int nargs) {
    auto table = state->toTable(1);
    int len = getTableLength(*table);
    
    // 获取比较函数（可选）
    std::function<bool(const Value&, const Value&)> compare;
    if (nargs >= 2 && state->isFunction(2)) {
        auto compFunc = state->toFunction(2);
        compare = [state, compFunc](const Value& a, const Value& b) -> bool {
            Vec<Value> args = {a, b};
            Value result = state->call(Value(compFunc), args);
            return result.isTruthy();
        };
    } else {
        compare = defaultCompare;
    }
    
    quickSort(*table, 1, len, compare);
    return Value();
}
```

#### table.concat

将表中的字符串元素连接成一个字符串，支持分隔符和范围指定。

#### table.pack/unpack

- `pack`: 将参数打包成表
- `unpack`: 将表解包为参数列表

## 性能特性

### 优势

1. **混合存储**: 数组部分提供 O(1) 访问，哈希部分处理复杂键
2. **内存效率**: 数组部分紧凑存储，减少内存碎片
3. **GC 集成**: 完整的垃圾回收支持
4. **动态扩容**: 根据需要自动调整大小

### 局限性

1. **哈希性能**: 使用线性搜索，大表性能较差
2. **内存开销**: Entry 对象动态分配增加开销
3. **重哈希**: 当前实现不支持重哈希优化

## 设计决策

### 简化策略

1. **线性搜索**: 简化实现，适合小到中等规模的表
2. **动态分配**: Entry 对象独立分配，便于管理
3. **统一接口**: 数组和哈希部分使用相同的 get/set 接口

### 扩展性考虑

1. **元表支持**: 预留元表机制接口
2. **弱引用**: 提供弱引用清理接口
3. **模板化**: 哈希遍历使用模板，便于扩展

## 使用示例

### 基本操作

```lua
-- 创建表
local t = {}

-- 数组操作
t[1] = "first"
t[2] = "second"
print(#t)  -- 输出: 2

-- 哈希操作
t["key"] = "value"
t[3.14] = "pi"

-- 表库函数
table.insert(t, "third")
table.sort(t)
local str = table.concat(t, ", ")
```

### 高级用法

```lua
-- 自定义排序
table.sort(t, function(a, b) return a > b end)

-- 打包/解包
local packed = table.pack(1, 2, 3)
local a, b, c = table.unpack(packed)

-- 移动元素
table.move(t, 1, 3, 5)  -- 将 t[1:3] 移动到 t[5:7]
```

## 测试覆盖

项目包含完整的测试套件，覆盖：

1. **基本操作**: get/set/length
2. **边界条件**: 空表、大索引、nil 值
3. **库函数**: 所有 table.* 函数
4. **错误处理**: 参数验证、异常情况
5. **性能测试**: 大表操作、内存使用

## State 类与 Table 的关系

### 核心交互机制

`State` 类是 Lua 虚拟机的核心执行环境，它与 `Table` 之间存在密切的交互关系：

#### 1. 栈管理与 Table 操作

```cpp
class State : public GCObject {
private:
    Vec<Value> stack;           // 执行栈
    int top;                    // 栈顶指针
    std::unordered_map<Str, Value> globals;  // 全局变量表
};
```

- **栈存储**：Table 通过 `Value` 对象存储在执行栈中
- **类型检查**：`State::isTable(int idx)` 检查栈中指定位置是否为表
- **类型转换**：`State::toTable(int idx)` 将栈中的值转换为 Table 引用

#### 2. 垃圾回收集成

```cpp
void State::markReferences(GarbageCollector* gc) {
    // 标记栈中的所有值（包括 Table）
    for (int i = 0; i < top; i++) {
        stack[i].markReferences(gc);
    }
    
    // 标记全局变量（可能包含 Table）
    for (auto& pair : globals) {
        pair.second.markReferences(gc);
    }
}
```

- **引用标记**：State 负责标记所有可达的 Table 对象
- **生命周期管理**：通过 GC 系统自动管理 Table 的内存
- **弱引用支持**：配合 Table 的弱引用机制进行清理

#### 3. Value 系统的桥梁作用

```cpp
class Value {
private:
    using ValueVariant = std::variant<
        std::monostate,      // Nil
        LuaBoolean,          // Boolean
        LuaNumber,           // Number
        GCRef<GCString>,     // String
        GCRef<Table>,        // Table - 通过智能指针管理
        GCRef<Function>      // Function
    >;
};
```

- **统一接口**：Value 为 Table 提供统一的存储和访问接口
- **智能指针**：通过 `GCRef<Table>` 自动管理 Table 的引用计数
- **类型安全**：编译时确保类型安全的 Table 操作

#### 4. Table 库函数的执行环境

```cpp
Value TableLib::insert(State* state, int nargs) {
    // 从栈中获取 Table 参数
    if (!state->isTable(1)) {
        throw std::runtime_error("first argument must be a table");
    }
    auto table = state->toTable(1);
    
    // 执行 Table 操作
    // ...
}
```

- **参数传递**：通过 State 的栈机制传递 Table 参数
- **错误处理**：State 提供统一的异常处理机制
- **结果返回**：操作结果通过栈返回给调用者

#### 5. 全局变量与 Table

```cpp
void State::setGlobal(const Str& name, const Value& value);
Value State::getGlobal(const Str& name);
```

- **全局表存储**：Table 可以作为全局变量存储在 State 中
- **模块系统**：Table 库本身作为全局表 "table" 注册
- **命名空间**：支持将 Table 用作命名空间

### 设计优势

1. **解耦设计**：State 和 Table 通过 Value 系统解耦，便于维护
2. **内存安全**：通过 GC 系统确保 Table 的内存安全
3. **类型安全**：编译时类型检查减少运行时错误
4. **性能优化**：栈机制提供高效的参数传递
5. **扩展性**：统一的接口便于添加新的 Table 操作

### 交互流程示例

```cpp
// 1. 创建 Table
auto table = make_gc_table();
state->push(Value(table));

// 2. 设置 Table 元素
table->set(Value("key"), Value(42));

// 3. 调用 Table 库函数
state->push(Value("value"));
TableLib::insert(state, 2);  // 通过 State 栈传递参数

// 4. GC 自动管理生命周期
// Table 在不再被引用时自动回收
```

## 未来改进

1. **哈希优化**: 实现真正的哈希表，提高查找性能
2. **重哈希**: 动态调整哈希表大小
3. **弱引用**: 完整的弱引用支持
4. **元表**: 完整的元方法支持
5. **序列化**: 表的序列化和反序列化

## 总结

本项目的 Table 实现在简洁性和功能性之间取得了良好的平衡。虽然在某些性能方面有所妥协，但提供了完整的 Lua table 语义支持，并与垃圾回收系统良好集成。设计具有良好的扩展性，为未来的优化和功能增强奠定了坚实基础。