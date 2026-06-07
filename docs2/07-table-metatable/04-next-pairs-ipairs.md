# next / pairs / ipairs — 表遍历

## 1. next() 函数

```
next(table [, index])

返回: key, value (表中的下一对)
如果 index 为 nil: 返回第一对
如果没有更多元素: 返回 nil

遍历顺序: 未定义 (取决于内部实现)
```

## 2. pairs() 遍历

```lua
-- pairs 遍历所有键值对 (顺序不保证)
for k, v in pairs(t) do
    print(k, v)
end

-- 等价于:
for k, v in next, t, nil do
    print(k, v)
end
```

## 3. ipairs() 遍历

```lua
-- ipairs 只遍历数组部分 (正整数键 1, 2, 3, ..., 直到遇到 nil)
for i, v in ipairs(t) do
    print(i, v)
end

-- ipairs 保证顺序: 1, 2, 3, ...

-- 遇到 nil 就停止
local t = {1, 2, nil, 4}
for i, v in ipairs(t) do
    print(v)  -- 1, 2 (nil 处停止)
end
```

## 4. 遍历中的修改

```lua
-- 可以安全地修改当前元素的值
for k, v in pairs(t) do
    t[k] = v * 2  -- 安全
end

-- 可以安全地删除当前元素?
-- Lua 5.1: 部分安全 (取决于实现)
-- 官方 Lua: 可以删除当前 key
for k, v in pairs(t) do
    t[k] = nil  -- 删除当前 key (next 函数会从上次位置继续)
end

-- 添加新键 → 行为未定义
for k, v in pairs(t) do
    t[new_key] = value  -- 不安全! 可能被遍历到也可能不会
end
```

## 5. table.foreach / table.foreachi

```lua
-- Lua 5.1 兼容函数 (5.2 已移除)
table.foreach(t, function(k, v) print(k, v) end)
table.foreachi(t, function(i, v) print(i, v) end)
```
