# Call Frame — 调用帧（函数调用上下文）

> 详细内容见 [05-vm-runtime/03-call-frame.md](../05-vm-runtime/03-call-frame.md)

## 关键点

1. **CallInfo** 记录每次函数调用的栈帧位置 (func, base, top)
2. **savedpc** 用于函数返回后恢复执行
3. **nresults** 控制期望的返回值数量
4. 调用 Lua 函数时创建新 CallInfo → goto reentry
5. 返回时弹出 CallInfo → 恢复调用者 savedpc
