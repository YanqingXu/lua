---
status: current
verified_against: examples/hello.lua; examples/control_flow.lua; examples/tables_and_methods.lua; examples/metamethods.lua; bin/lua_app.exe
last_checked: 2026-05-19
applies_to: small runnable Lua scripts for the current interpreter
---

# Examples

这些示例用于快速观察当前解释器行为。它们不是 Lua 5.1 完整兼容性声明；精确语义仍以测试套件为准。

先构建 `lua_app.vcxproj`，然后从仓库根目录运行：

```powershell
bin\lua_app.exe examples\hello.lua
bin\lua_app.exe examples\control_flow.lua
bin\lua_app.exe examples\tables_and_methods.lua
bin\lua_app.exe examples\metamethods.lua
```

## Files

| 文件 | 展示主题 |
|---|---|
| `hello.lua` | `print`、局部变量、字符串连接 |
| `control_flow.lua` | `while`、`if`、函数调用和返回值 |
| `tables_and_methods.lua` | 表字段、冒号方法调用、状态更新 |
| `metamethods.lua` | `setmetatable`、`__add`、元方法返回表 |

## Related Reading

- `docs/index.md`
- `docs/glossary.md`
- `docs/walkthroughs/index.md`
