# Roadmap — 路线图

## 当前状态 (2026-06)

```
✅ 编译器前端完整 (Lexer + Parser + CodeGen)
✅ VM 38 条指令全部实现
✅ 核心运行时对象 (Value/Table/Function/Upvalue/Userdata)
✅ GC 标记-清除 + 弱表 + finalizer
✅ 标准库主体完成 (9 个库)
✅ 668 个测试全绿
✅ Lua 5.1 官方测试套件 22/22 通过
```

## 近期 (P0/P1)

```
🟡 官方 testC helper 策略
   → 移植 ltests.c 或提供项目内 T 模块
   → 补齐 api.lua / code.lua 的完整路径

🟡 官方 binary chunk 兼容
   → 实现官方 Lua 5.1 binary chunk 格式
   → string.dump / loadstring 兼容
```

## 中期 (P2)

```
🟡 IncrementalGC 实现
   → 增量式标记-清除
   → 降低 GC 暂停时间

🟡 标准库精细边界对齐
   → debug 库 hook 精细边界
   → I/O 错误路径完善
```

## 长期愿景

```
🔮 热更新支持 (Hot Reload)
🔮 调试器 (Debugger) 完善
🔮 跨平台支持 (Linux/macOS)
🔮 性能优化 (JIT 研究)
```
