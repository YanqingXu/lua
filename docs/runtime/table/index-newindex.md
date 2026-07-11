# __index / __newindex — 索引元方法

## 1. __index 元方法

### 当读取不存在的键时触发

```lua
local defaults = { x = 0, y = 0 }
local t = setmetatable({}, { __index = defaults })

print(t.x)  -- 0 (来自 defaults)
t.x = 10     -- 在 t 中创建 (不触发 __index)
print(t.x)  -- 10 (t 自己的值)
```

### __index 的两种形式

```
1. Table 形式: __index = anotherTable
   → 自动在 anotherTable 中查找
   
2. Function 形式: __index = function(t, k)
   → 调用 function 获取值
   
示例:
  mt.__index = defaults        -- Table 形式
  mt.__index = function(t, k)  -- Function 形式
      return "default"
  end
```

### VM 侧实现

```cpp
// GETTABLE 指令中:
Value result = table.get(key);

if (result.isNil() && hasMetamethod("__index")) {
    Value indexMethod = metatable.get("__index");
    if (indexMethod.isTable()) {
        result = indexMethod.asTable()->get(key);  // 递归查找
    } else if (indexMethod.isFunction()) {
        result = callFunction(indexMethod, table, key);  // 函数调用
    }
}
```

## 2. __newindex 元方法

### 当写入不存在的键时触发

```lua
local data = {}
local proxy = setmetatable({}, {
    __newindex = function(t, k, v)
        data[k] = v  -- 重定向写入
    end
})

proxy.x = 42  -- data.x = 42 (不写入 proxy)
print(proxy.x)  -- nil (proxy 中没有 x)
print(data.x)   -- 42
```

### __newindex 的两种形式

```
1. Table 形式: __newindex = anotherTable
   → 写入 anotherTable 而不是当前 table
   
2. Function 形式: __newindex = function(t, k, v)
   → 调用 function 处理写入
```

### 关键: 只在 key 不存在时触发

```
如果 key 已经在 table 中:
  t[k] = v → 直接写入，不触发 __newindex

只有 key 不存在时:
  t[k] = v → 触发 __newindex
```
