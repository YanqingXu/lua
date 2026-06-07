# Bytecode Disassembler

> 详见 [04-bytecode-compiler/10-disassembler.md](../04-bytecode-compiler/10-disassembler.md)

## 快速命令

```bash
# 查看字节码
lua_bytecode script.lua

# 完整输出
lua_bytecode --format full script.lua

# 对比两个文件
lua_bytecode --diff a.lua b.lua

# Mermaid 控制流图
lua_bytecode --cfg script.lua

# REPL 中
> .bytecode  -- 查看当前会话的字节码
```
