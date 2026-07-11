# Array & Hash Part — 数组部分与哈希部分

## 1. 存储模型

`Table` 使用两个容器：

```cpp
Vec<Value> array_;
std::unordered_map<Value, Value, ValueHash, ValueEqual> hash_;
```

数组部分按 Lua 的 1-based 索引存放正整数键；其他键进入哈希部分。nil 和 NaN 不能作为写入键。

## 2. 写入路径

```text
Table::set(key, value)
  -> key == nil 或 NaN：抛出 RuntimeError
  -> value == nil：删除键
  -> isArrayIndex(key)：setArray(index, value)
  -> 其他键：hash_[key] = value
```

`setArray()` 在索引超出容量时直接扩展 `array_`，中间槽位填 nil。当前实现没有 Lua C 源码中的密度统计与数组/哈希重排算法，因此稀疏的大正整数索引可能扩大数组部分。

## 3. 读取与删除

- 正整数键先按数组索引读取，越界返回 nil。
- 其他键通过 `hash_.find()` 查询。
- 向任意键写 nil 等价于删除；数组槽位写回 nil，哈希条目直接 erase。
- 写入对象引用时，table 通过所属 GC 执行写屏障。

## 4. 长度与遍历

`length()` 在数组部分寻找 non-nil/nil 边界。有洞表的 `#t` 本来就是 Lua 5.1 未定义边界，结果不应被用于推断元素总数。

`next()` 先遍历数组中的非 nil 槽位，再遍历哈希容器；遍历顺序不是稳定 API。
