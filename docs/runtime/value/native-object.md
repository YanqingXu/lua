# Native Objects — Userdata & LightUserdata

## 1. 两种用户数据

| 类型 | 表示 | GC 管理 | 大小 | 用途 |
|------|------|---------|------|------|
| **LightUserdata** | `void*` | 否 | 1 个指针 | 轻量引用 |
| **Full Userdata** | `Userdata*` | 是 | 任意大小 | 包装 C++ 对象 |

## 2. Userdata 结构

```cpp
class Userdata : public GCObject {
    void* data_;        // 用户数据块 (8字节对齐)
    usize size_;        // 数据块大小
    Table* metatable_;  // 元表
    
    static Userdata* create(usize size);
    
    template<typename T>
    static Userdata* create(const T& value);
    
    void* getData();
    
    template<typename T>
    T* getTypedData();
    
    void setMetatable(Table* mt);
    bool hasMetatable() const;
};
```

## 3. 内存布局

```
[Userdata Header (GCObject + fields)]
[User Data Block (8-byte aligned)]
```

## 4. Userdata 创建

```cpp
// 原始分配
Userdata* ud = Userdata::create(64);  // 64 字节
void* raw = ud->getData();

// 类型化创建
struct Vec3 { float x, y, z; };
Userdata* ud = Userdata::create(Vec3{1, 2, 3});
Vec3* v = ud->getTypedData<Vec3>();
```

## 5. 元表支持

```lua
-- Lua 侧
local mt = { __index = { x = 10 } }
local ud = new_userdata()
setmetatable(ud, mt)
print(ud.x)  -- 10 (通过 __index)
```

## 6. GC 集成

```
Userdata 的 GC 生命周期:
  1. 创建时注册到 GC 链表
  2. 标记阶段: 标记其元表
  3. 清除阶段: 如果没有引用 → 回收
  4. Finalizer: 如果有 __gc 元方法 → 在回收前调用
  5. 释放数据块内存
```

## 7. LightUserdata

```cpp
// 只是一个 void* 指针，不受 GC 管理
int data = 42;
Value lud(static_cast<void*>(&data));

// 用途: 轻量级的 C 对象引用
// 警告: 需要自己管理生命周期
```
