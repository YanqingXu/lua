# Value & Object System Overview — 值对象系统概览

## 1. 这个模块解决什么问题？

回答：**Lua 运行时值是怎么表示的？** 这是整个运行时系统的基础。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/core/value.hpp/cpp` | Value 类 (9 种类型的 variant) |
| `src/core/gc_object.hpp/cpp` | GCObject 基类 (三色标记) |
| `src/core/gc_string.hpp/cpp` | GCString (驻留字符串) |
| `src/core/string_pool.hpp/cpp` | StringPool (字符串驻留池) |
| `src/core/userdata.hpp/cpp` | Userdata (C 数据包装) |
| `src/core/thread.hpp/cpp` | Thread (协程) |

## 3. 类型系统全景

```
Value (std::variant)
├── Nil (std::monostate)           × 非 GC
├── Boolean (bool)                 × 非 GC
├── LightUserdata (void*)          × 非 GC
├── Number (double)                × 非 GC
├── String (GCString*)             ○ GC 对象
├── Table (Table*)                 ○ GC 对象
├── Function (Function*)           ○ GC 对象
├── Userdata (Userdata*)           ○ GC 对象
└── Thread (Thread*)               ○ GC 对象
```

## 4. GC 对象继承链

```
GCObject (基类)
├── GCString    — 字符串
├── Table       — 表
├── Function    — 函数 (包装 Closure)
├── Userdata    — 用户数据
├── Thread      — 协程
├── Proto       — 函数原型 (内部)
└── Upval       — 上值 (内部)
```

## 5. Value 特性

- **值语义**: 拷贝 Value 是浅拷贝（GC 对象指针拷贝）
- **类型安全**: `std::variant` 自动管理类型标签
- **内存布局**: 约 16 字节（variant 最大成员 + tag）
- **Trivially copyable**: 适合在栈上频繁创建销毁
