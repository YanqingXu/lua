# Table Structure — Table 结构

## 1. Table 类的核心属性

```cpp
class Table : public GCObject {
    Vec<Value> array_;         // 数组部分
    HashMap<Value, Value> hash_; // 哈希部分
    Table* metatable_;         // 元表
    usize arraySize_;          // 数组部分预分配大小
    
    // GC 相关
    bool weakKeys_;            // __mode 包含 "k"
    bool weakValues_;          // __mode 包含 "v"
};
```

## 2. 键的分类

```lua
t = {}
t[1] = "a"      -- 正整数键 → 数组部分
t[2] = "b"      -- 正整数键 → 数组部分
t["key"] = "c"  -- 字符串键 → 哈希部分
t[1000] = "d"   -- 大的正整数 → 哈希部分 (稀疏)
t[3.14] = "e"   -- 浮点键 → 哈希部分
t[{}]= "f"      -- table 键 → 哈希部分
```

## 3. 数组部分 vs 哈希部分

```
数组部分优势:
  - 连续内存，缓存友好
  - O(1) 访问
  - 支持长度运算 (#)

哈希部分优势:
  - 支持任意类型键
  - O(1) 平均查找
  - 动态扩容
```

## 4. 表长度的计算 (#t)

```
Lua 5.1 的表长度定义:
  找到最大的 n，使得 t[n] ~= nil 且 t[n+1] == nil

但 Lua 明确说这是"未定义行为"之一:
  有洞的表的长度是任意满足条件的边界

实现:
  在数组部分和哈希部分的边界进行二分搜索
```

## 5. Table 的内存占用

```
Table 对象:
  - GCObject header: ~16 bytes
  - array_: Vec 本身 ~24 bytes + 元素
  - hash_: HashMap 本身 ~56 bytes + 元素
  - metatable_: 8 bytes
  - 总计: ~104 bytes (不含元素)
```
