# Allocation Path — 分配路径

## 1. 对象创建

```cpp
// 所有 GC 对象通过构造函数注册到 GC
GCObject::GCObject(GCObjectType type) {
    type_ = type;
    color_ = GCColor::White;
    
    // 注册到全局 GC 链表
    GlobalState::getInstance().getGC().registerObject(this);
}
```

## 2. GC 对象链表

```
GC 维护一个单向链表:
  head → Obj1 → Obj2 → Obj3 → ... → nullptr

registerObject(obj):
  obj->next = head;
  head = obj;

遍历所有对象:
  for (GCObject* obj = head; obj; obj = obj->next) { ... }
```

## 3. 内存分配路径

```
用户代码                内部实现
──────────────────────────────────────
{}                    → new Table() → GC.register()
"hello"               → StringPool::intern() → new GCString() → GC.register()
function() ... end     → CodeGen → new Proto() → CLOSURE → new Closure() → GC.register()
Userdata::create(64)  → new Userdata() → GC.register()
coroutine.create(f)   → new Thread() → GC.register()
```

## 4. 内存统计

```lua
-- collectgarbage("count") 返回:
-- 当前 GC 管理的总内存 (KB)

-- GC 自动触发:
-- 当分配内存超过 threshold 时触发
-- threshold = lastCount * pause / 100
```
