# Weak Table — 弱表

## 1. __mode 元方法

```lua
-- weak keys: 键是弱引用
local t = setmetatable({}, { __mode = "k" })

-- weak values: 值是弱引用
local t = setmetatable({}, { __mode = "v" })

-- 两者都是弱引用
local t = setmetatable({}, { __mode = "kv" })
```

## 2. 弱引用语义

```
普通引用: 只要 table 持有 key/value，GC 就不会回收
弱引用: 如果只有弱引用指向对象，GC 可以回收

弱键:
  key = {}  -- 只有 t 引用这个 key (且是弱引用)
  t[key] = "value"
  collectgarbage()
  -- key 被回收，对应的 entry 从 t 中移除

弱值:
  t[1] = {}  -- 只有 t 引用这个 value (且是弱引用)
  collectgarbage()
  -- value 被回收，t[1] 变为 nil
```

## 3. 弱表清理时机

```
GC 清除阶段:
  1. 检查 table 的 __mode
  2. 如果是弱键: 检查每个 key 的标记颜色
     → White → 移除该 entry
  3. 如果是弱值: 检查每个 value 的标记颜色
     → White → 移除该 entry
  4. 弱值在 finalizer 之前清理
  5. 弱键保留到 finalizer 之后 (保证 finalizer 能访问)
```
