# Close Upvalue — 关闭上值

## 1. 何时关闭 Upvalue

```
以下几种情况需要关闭 Upvalue:

1. 函数返回 (RETURN)
   → 所有在该作用域内创建的 upvalue 需要关闭

2. break 跳出循环
   → 循环内创建的 upvalue 需要关闭

3. OP_CLOSE 指令
   → 显式关闭: close all upvalues >= R(A)
```

## 2. closeUpvalues(level)

```cpp
void LuaState::closeUpvalues(i32 level) {
    // 关闭所有 stackIndex >= level 的 open upvalue
    
    while (openUpvalues_ && openUpvalues_->stackIndex >= level) {
        Upvalue* uv = openUpvalues_;
        openUpvalues_ = uv->next;  // 从链表中移除
        
        uv->close();  // Open → Closed
    }
}

void Upvalue::close() {
    // 将栈上的值复制到 closedValue_
    closedValue_ = *v_;
    
    // 改变指针指向
    v_ = &closedValue_;  // 指向自己的存储
    
    // stackIndex 不再有效 (已关闭)
}
```

## 3. close() 的细节

```
关闭前 (Open):
  Stack: [...]  [x = 1]  [...]
                     ↑
  Upvalue: v_ ───────┘

关闭后 (Closed):
  Stack: [...]  [x = 1]  [...]  ← x 可能被覆盖
  
  Upvalue:
    closedValue_ = 1     ← x 的副本
    v_ ──→ &closedValue_ ← 指向自己的存储
```

## 4. 示例

```lua
function outer()
    local x = 1
    local f
    if true then
        local y = 2
        f = function() return x, y end
    end  -- OP_CLOSE 关闭 y (离开了 if 作用域)
    return f
end

local func = outer()
-- y 已被 close，但 x 还没有
-- f 可以正常使用 x 和 y

func()  -- 返回 1, 2 (两个值都在 he closed 状态)
```
