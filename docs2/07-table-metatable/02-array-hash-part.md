# Array & Hash Part — 数组部分与哈希部分

## 1. 混合存储策略

Lua 的 Table 使用数组 + 哈希的混合存储，自动选择最优存储方式。

## 2. 存储决策

```
写入 t[k] = v:

1. 如果 k 是正整数且在合理范围内 (1, 2, 3, ...):
   → 进入数组部分
   
2. 否则:
   → 进入哈希部分

注意: 不需要事先声明大小 (不像 C 数组)
```

## 3. 数组部分

```cpp
// 内部是 Vec<Value>
Vec<Value> array_;

// 访问:
Value& get(i32 idx) {
    if (idx >= 1 && idx <= array_.size()) {
        return array_[idx - 1];  // 1-based → 0-based
    }
    // 可能落在哈希部分
}

// 写入:
void set(i32 idx, Value v) {
    // 根据 key 特征和目标大小决定放在哪部分
}
```

## 4. 哈希部分

```cpp
// 内部是 HashMap<Value, Value>
HashMap<Value, Value> hash_;

// 键的类型: 任意 Value (除 nil)
// 值的类型: 任意 Value

// 支持:
//   string key: t["name"]
//   number key: t[3.14]
//   table key:  t[{}] (以 table 为键)
//   function key: t[func]
//   但不能以 nil 为键
```

## 5. 重新哈希 (Rehash)

```
当表增长或收缩到一定程度时:
  1. 统计所有键 (数组部分 + 哈希部分)
  2. 计算新的数组大小 (最优的正整数键范围)
  3. 重建数组部分
  4. 其余键放入哈希部分
  5. 保持 O(1) 访问

触发条件:
  - 数组部分利用率太低 (太多空洞)
  - 哈希部分太大 (太多正整数键应该放入数组)
```

## 6. 长度运算 #t 的细节

```
Lua 5.1 的 #t 定义:
  对于没有洞的表 (1, 2, ..., n 都非 nil):
    #t = n
    
  对于有洞的表 (如 {1, nil, 3}):
    结果是未定义的 (任意边界)
    
  实现: 二分搜索找到 nil 和非 nil 的边界
```
