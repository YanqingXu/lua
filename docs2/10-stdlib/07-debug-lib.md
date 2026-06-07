# Debug Library — 调试库

## 14/14 函数表面实现 (94% 完成)

| 函数 | 说明 | 状态 |
|------|------|------|
| `debug.getinfo([thread,] f [, what])` | 获取函数/调用信息 | ✅ |
| `debug.getlocal([thread,] level, local)` | 获取局部变量 | ✅ |
| `debug.setlocal([thread,] level, local, value)` | 设置局部变量 | ✅ |
| `debug.getupvalue(f, up)` | 获取 upvalue | ✅ |
| `debug.setupvalue(f, up, value)` | 设置 upvalue | ✅ |
| `debug.getfenv(o)` | 获取环境 | ✅ |
| `debug.setfenv(o, env)` | 设置环境 | ✅ |
| `debug.getregistry()` | 获取注册表 | ✅ |
| `debug.getmetatable(o)` | 获取元表 | ✅ |
| `debug.setmetatable(o, mt)` | 设置元表 | ✅ |
| `debug.traceback([thread,] [msg [, level]])` | 调用栈回溯 | ✅ |
| `debug.sethook([thread,] hook, mask [, count])` | 设置 hook | ✅ |
| `debug.gethook([thread])` | 获取 hook | ✅ |

## Hook 系统

```lua
-- Hook 类型
debug.sethook(function(event, line)
    print(event, line)
end, "crl")  -- c=call, r=return, l=line

-- Hook 在 VM 主循环中被调用
-- Count hook 在每条指令前检查
-- Line hook 在行号变化时触发
-- Call/Return hook 在函数调用/返回时触发
```
