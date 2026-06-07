# String Interning — 字符串驻留

## 1. 这个模块解决什么问题？

相同内容的字符串在内存中只存一份（驻留），节省内存并加速比较。

## 2. 核心文件

| 文件 | 作用 |
|------|------|
| `src/core/string_pool.hpp/cpp` | StringPool 单例 |
| `src/core/gc_string.hpp/cpp` | GCString 类 |

## 3. StringPool 设计

```cpp
class StringPool {
public:
    static StringPool& getInstance();  // 单例
    
    // 驻留字符串: 如果已存在返回已有对象，否则创建新对象
    GCString* intern(StrView str);
    
    // 已知长度的驻留
    GCString* intern(const char* data, usize len);
    
    // 总数
    usize size() const;
    
private:
    HashMap<Str, GCString*> pool_;   // 短字符串哈希表
    // 长字符串也走哈希表 (Lua 5.1 实际也这样做)
};
```

## 4. GCString 结构

```cpp
class GCString : public GCObject {
    Str data_;          // 字符串内容 (std::string)
    usize hash_;        // 预计算的哈希值
    
    usize getLength() const;
    const char* c_str() const;
    usize getHash() const;
};
```

## 5. 驻留的好处

```
1. 内存节省:
   "hello" 出现了 1000 次 → 只存 1 份

2. 加速比较:
   a == b  → 指针比较 而非 strcmp
   因为同一个字符串只有一个 GCString*
   
3. 哈希加速:
   哈希值预计算，用作 HashMap 的 key 不需要重复计算
```

## 6. 驻留时机

```
字符串在以下时机驻留:
  - Lexer 解析字符串字面量时
  - 字符串拼接 (CONCAT) 结果
  - C API: lua_pushstring()
  - 标准库创建字符串时

注意: number → string 转换 (tostring) 也走 StringPool
```

## 7. 短字符串 vs 长字符串

```
Lua 5.1 区分短字符串 (≤40 chars) 和长字符串 (>40 chars):
  - 短字符串: 驻留 (HashMap 管理)
  - 长字符串: 不一定驻留 (减少哈希开销)

本项目简化处理: 所有字符串都尝试驻留
```
