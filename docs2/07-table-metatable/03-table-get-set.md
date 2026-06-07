# Table Get / Set — 表的读写操作

## 1. t[k] 的执行流程 (GETTABLE)

```
1. 检查 t 是否为 table
   → 如果不是: 检查 metatable 的 __index

2. 检查 key 是否为 nil
   → 如果是 nil: error "table index is nil"

3. 直接查表
   → 如果是正整数且在数组范围内: 查 array_[k-1]
   → 否则: 查 hash_[k]
   → 找到了 → 返回

4. 如果找不到，检查 metatable
   → 如果有 __index:
     → __index 是 function: 调用 function(t, k)
     → __index 是 table:   递归查找 __index[k]
   → 如果没有元表: 返回 nil
```

## 2. t[k] = v 的执行流程 (SETTABLE)

```
1. 检查 t 是否为 table
   → 如果不是: 检查 metatable 的 __newindex

2. 检查 key 是否为 nil
   → 如果是 nil: error "table index is nil"

3. 如果 key 已存在
   → 直接设置值
   → 不触发 __newindex

4. 如果 key 不存在
   → 检查 metatable 的 __newindex
     → __newindex 是 function: 调用 function(t, k, v)
     → __newindex 是 table:   设置 __newindex[k] = v
   → 没有元表: 直接插入新键值对

5. 写屏障 (GC)
   → 如果值是新分配的 GC 对象 → 标记为 Gray
   → (保守写屏障)
```

## 3. rawget / rawset

```lua
-- rawget: 不触发 __index
rawget(t, "key")  -- 直接查表，不触发元方法

-- rawset: 不触发 __newindex
rawset(t, "key", value)  -- 直接设置，不触发元方法
```

## 4. 测试用例

```lua
-- 基本读写
local t = {}
t[1] = "a"        -- SETTABLE: 插入到数组部分
t["1"] = "b"      -- SETTABLE: 插入到哈希部分（"1" ≠ 1）
print(t[1], t["1"])  -- GETTABLE: "a", "b"

-- 元表 __index
local mt = { __index = { x = 100 } }
local t = setmetatable({}, mt)
print(t.x)  -- 100 (通过 __index)

-- 元表 __newindex
local proxy = {}
local data = {}
local mt = {
    __newindex = function(t, k, v)
        data[k] = v  -- 重定向到 data
    end
}
setmetatable(proxy, mt)
proxy.x = 42
print(data.x)  -- 42 (实际存在 data 中)
```
