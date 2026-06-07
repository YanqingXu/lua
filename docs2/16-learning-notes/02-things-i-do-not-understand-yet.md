# Things I Do Not Understand Yet — 我还不理解的问题

> 这个文件不是暴露弱点，而是学习地图。记录我当前还不完全理解的部分，以后回来补。

## 2026-06-07

### VM / 指令相关
- [ ] OP_CALL 的 B/C 参数还没有完全吃透（特别是多返回值场景）
- [ ] CALL 和 TAILCALL 的栈帧复用细节
- [ ] FORPREP 为什么要先做 `R(A) -= R(A+2)` 的减法
- [ ] SETLIST 的扩展块编码 (C=0 时后续指令的处理)

### Upvalue 相关
- [ ] open upvalue close 的精确时机（break 跳出多层嵌套时）
- [ ] vararg 在嵌套函数里的处理
- [ ] 协程 yield 时 upvalue 的状态

### GC 相关
- [ ] markChildren 的完整调用顺序
- [ ] finalizer 的两阶段执行细节
- [ ] 写屏障的所有入口点

### 标准库
- [ ] string.gsub 的 table/function 替换的完整语义
- [ ] debug hook 的 count/call/return/line 四种模式的区别
- [ ] package.loadlib 的 Windows DLL 加载细节

### 其他
- [ ] EngineContext 与 Singleton GlobalState 的关系
- [ ] RuntimeServices 的完整依赖注入机制
