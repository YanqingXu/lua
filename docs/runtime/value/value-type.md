# Value Type — Value 类型详解

## 1. 支持的 Lua 类型

| Lua 类型 | C++ 表示 | variant 索引 | 是否 GC |
|----------|---------|-------------|---------|
| nil | `std::monostate` | 0 | 否 |
| boolean | `bool` | 1 | 否 |
| lightuserdata | `void*` | 2 | 否 |
| number | `LuaNumber` (double) | 3 | 否 |
| string | `GCString*` | 4 | 是 |
| table | `Table*` | 5 | 是 |
| function | `Function*` | 6 | 是 |
| userdata | `Userdata*` | 7 | 是 |
| thread | `Thread*` | 8 | 是 |

## 2. Value 的变体表示

```cpp
using ValueVariant = std::variant<
    std::monostate,     // Nil
    bool,               // Boolean
    void*,              // LightUserdata
    LuaNumber,          // Number (double)
    GCString*,          // String
    Table*,             // Table
    Function*,          // Function
    Userdata*,          // Userdata
    Thread*             // Thread
>;
```

## 3. Value 是值语义还是引用语义？

```
Value 本身是值语义:
  Value a(42.0);
  Value b = a;           // 拷贝 (浅拷贝)
  a == b;                // true (variant 内容相同)

但对于 GC 对象 (Table*, Function*, etc.):
  Value a(table);
  Value b = a;           // 两个 Value 指向同一个 Table
  a.asTable() == b.asTable(); // true (指针相同)

即: Value 是浅拷贝，GC 对象的引用语义体现在指针共享。
```

## 4. 拷贝 Value 时发生什么？

```
拷贝 (Value& operator=):
  value_ = other.value_;  // variant 拷贝

对于 Non-GC 类型 (nil, bool, number):
  → 值拷贝，完全独立

对于 GC 类型 (string, table, function, userdata, thread):
  → 指针拷贝，两个 Value 指向同一个对象
  → 不增加引用计数 (GC 通过标记-清除管理)
```

## 5. Lua 真值语义

```cpp
// 只有 nil 和 false 是假值
bool isFalse() const {
    return isNil() || (isBoolean() && !asBoolean());
}

// 0, 空字符串 "" 都是真值!
Value(0.0).isTrue();    // true
Value("").isTrue();     // true
```

## 6. 相等性比较

```cpp
bool operator==(const Value& other) const {
    if (value_.index() != other.value_.index())
        return false;
    return value_ == other.value_;  // variant 默认比较
}

// GC 对象: 指针相等 = 引用相等
// 数字: 值相等
// 字符串: 因为是 GCString* 指针比较，但 StringPool 保证同一字符串同一指针
```
