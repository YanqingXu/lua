# Open Upvalue — 开放上值

## 1. Open Upvalue 状态

```
Open Upvalue = 外部变量仍在栈上，upvalue 直接指向栈位置

特征:
  v_ → stack[stackIndex]  (直接指针)
  isOpen() == true
  closedValue_ 未使用
```

## 2. 创建 Open Upvalue

```cpp
// 在 LuaState::findOrCreateUpvalue(i32 stackIndex) 中:
Upvalue* LuaState::findOrCreateUpvalue(i32 stackIndex) {
    // 1. 在 open upvalue 链表中查找 (降序)
    Upvalue* prev = nullptr;
    Upvalue* uv = openUpvalues_;
    
    while (uv && uv->stackIndex > stackIndex) {
        prev = uv;
        uv = uv->next;
    }
    
    // 2. 找到了 → 返回 (共享)
    if (uv && uv->stackIndex == stackIndex) {
        return uv;
    }
    
    // 3. 没找到 → 创建新的
    Upvalue* newUV = new Upvalue();
    newUV->v_ = &stack[stackIndex];  // 直接指向栈
    newUV->stackIndex_ = stackIndex;
    newUV->next = uv;
    if (prev) prev->next = newUV;
    else openUpvalues_ = newUV;
    
    return newUV;
}
```

## 3. 共享机制

```
多个闭包捕获同一个栈变量 → 共享同一个 Upvalue:

local x = 1
local f1 = function() return x end  -- 创建 Upvalue → 指向栈[x]
local f2 = function() x = x + 1 end -- 查找 Upvalue → 返回同一个!

f1 的 upvalues[0] == f2 的 upvalues[0]  // true

因此: f2 修改 x, f1 会看到变化
```

## 4. Open Upvalue 链表

```
LuaState 维护一个按 stackIndex 降序排列的链表:

openUpvalues_ → uv(stackIdx=5) → uv(stackIdx=3) → uv(stackIdx=1) → null

降序保证:
  - 查找: O(n) 但通常很短
  - 插入: O(n) 按正确位置插入
  - 关闭: 从头部开始，关闭所有 stackIdx >= level 的
```
