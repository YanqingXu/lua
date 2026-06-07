# Object Reference Model — 对象引用模型

## 1. 这个模块解决什么问题？

GC 对象如何在运行时被引用和追踪。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/core/gc_object.hpp/cpp` | GCObject 基类 |

## 3. GCObject 结构

```cpp
class GCObject {
public:
    GCObjectType getType() const;     // String/Table/Function/Userdata/Thread/Proto/Upval
    GCColor getColor() const;         // White/Gray/Black
    void setColor(GCColor color);
    
    GCObject* getNext() const;        // GC 链表中的下一个对象
    void setNext(GCObject* next);
    
    bool isMarked() const;            // 是否已被标记
    void mark();                      // 标记为 Gray
    
    // 纯虚函数: 标记该对象引用的其他 GC 对象
    virtual void markChildren(GarbageCollector& gc) = 0;
    
    // 获取对象大小 (用于 GC 统计)
    virtual usize getSize() const = 0;
};
```

## 4. 三色标记

```
White (白色):
  - 初始状态
  - GC 标记阶段未访问到
  - 清除阶段会回收

Gray (灰色):
  - 已被标记，但子引用尚未扫描
  - 在 gray 列表中

Black (黑色):
  - 已标记且已扫描所有子引用
  - 不会被回收
```

## 5. GC 对象链表

```
所有 GC 对象组成单向链表:

GlobalState.gcObjects → Obj1 → Obj2 → Obj3 → ... → nullptr

新对象创建时被添加到链表头部。
GC 通过遍历链表找到所有对象。
```

## 6. 引用关系图示例

```
GlobalState (根)
  ├── Registry (Table)
  │     ├── key1 → String "hello"
  │     └── key2 → Function (main)
  │                  └── Proto
  │                       └── constants[3] → String "print"
  ├── MainThread (Thread)
  │     └── Stack
  │           ├── Table
  │           │     ├── array[0] → Number 42
  │           │     └── hash["key"] → Function
  │           └── Function
  └── Metatables[Number] → Table (元表)
```

## 7. markChildren 示例

```cpp
void Table::markChildren(GarbageCollector& gc) {
    // 标记数组部分的所有值
    for (auto& v : array_) {
        if (v.isCollectable()) gc.markObject(v);
    }
    
    // 标记哈希部分的所有键和值
    for (auto& [k, v] : hash_) {
        if (k.isCollectable()) gc.markObject(k);
        if (v.isCollectable()) gc.markObject(v);
    }
    
    // 标记元表
    if (metatable_) gc.markObject(metatable_);
}
```
