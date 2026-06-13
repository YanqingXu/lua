---
status: current
verified_against: src/app/app_options.hpp; src/app/app_options.cpp; src/main.cpp; src/repl.hpp; src/repl.cpp; src/repl/; tests/unit/app/test_app_options.cpp; tests/unit/app/test_repl_commands.cpp
last_checked: 2026-05-23
applies_to: lua_app command-line and REPL behavior
---

# REPL 和 CLI 指南

`lua_app.exe` 是解释器可执行文件。它可以运行脚本、进入 REPL、打印版本/帮助，以及可选写入 VM trace。

## 命令行

```powershell
bin\lua_app.exe [options] [script [args]]
```

当前由 `AppOptions` 解析的选项：

| 选项 | 行为 |
|---|---|
| `-v` | 显示版本并退出 |
| `-h` | 显示用法并退出 |
| `-i` | 当未提供脚本时进入 REPL |
| `--trace <file>` | 将 VM 执行 trace 写入为 JSONL |
| `--trace-diff <file>` | 写入 VM 执行 trace，包含 `changedRegisters` 而非完整寄存器快照 |

第一个非选项参数被视为脚本路径。其后的参数保留为脚本参数。

优先级：

1. `-v`
2. `-h`
3. 脚本模式
4. 显式 REPL 模式
5. 默认行为

没有脚本且没有 `-i` 时，当前默认行为进入 REPL。

## 脚本模式

```powershell
bin\lua_app.exe examples\hello.lua
bin\lua_app.exe --trace bin\hello.jsonl examples\hello.lua
bin\lua_app.exe --trace-diff bin\hello-diff.jsonl examples\hello.lua
```

脚本模式通过 `readWholeFile()` 读取文件，解析它，生成 `Proto`，并通过 VM 执行。

## REPL 模式

```powershell
bin\lua_app.exe
bin\lua_app.exe -i
```

REPL 初始化：

- `_VERSION`
- `_PROMPT`
- `_PROMPT2`
- `exit()`

支持的 REPL 行为：

- 首行输入 `exit` 或 `quit` 退出
- `exit()` 通过注册的函数退出
- Ctrl+D / EOF 退出
- Ctrl+C 在支持时取消当前输入
- 默认 prompt 带行号，首行显示 `lua:1>`，续行显示 `lua:2>>`
- 自定义多行 prompt 原样使用 `_PROMPT2`
- 以 `<eof>` 结尾的 parser 错误保持缓冲区开放以继续输入；确定的语法错误立即报告
- `=expr` 被转换为 `return expr` 并打印返回值
- 普通输入作为语句解析，不自动打印表达式值
- Tab 补全覆盖元命令、全局变量和已加载库字段（如 `string.sub`）
- 彩色错误输出仅在终端 REPL 会话中启用；重定向输出保持纯文本

支持的元命令：

- `.help` 打印命令列表
- `.bytecode <expr|chunk>` 解析和编译输入，然后打印紧凑的 Proto 字节码
- `.ast <expr|chunk>` 解析输入并打印 AST 树视图
- `.gc [stats|collect|strategy|help]` 打印 GC 统计信息，通过活跃 `GCStrategy` 运行回收，或显示策略边界

`.bytecode` 和 `.ast` 首先将参数作为 chunk 尝试。如果失败且输入不是显式的 `=expr`，则将其作为 `return <expr>` 重试。`.ast` 输出将此回退标记为 `mode: expression`；普通 chunk 标记为 `mode: chunk`。

`.gc` 命令有意通过 `RuntimeServices.gc` 使用活跃回收器。它报告当前策略并列出可用的 `mark-sweep` 和增量式教学占位策略。

GC 策略也可从 Lua 查询或切换：

```lua
collectgarbage("strategy")
collectgarbage("strategy", "mark-sweep")
collectgarbage("strategy", "incremental")
```

增量策略当前委托给相同的标记-清除回收阶段，因此活跃对象语义保持等价，而未来的写屏障和调度工作仍保持显式。

Tab 补全刻意保持保守。它从当前行尾部补全，使用当前 `LuaState` 全局表获取全局名称，并遍历点分表路径获取已加载库字段。

彩色错误使用 ANSI 红色输出，通过 `ErrorColorMode::Auto`。在 Windows 上，REPL 在发射彩色错误前启用虚拟终端处理。

## Prompt 自定义

在 REPL 中：

```lua
_PROMPT = "lua> "
_PROMPT2 = "...> "
```

默认 `_PROMPT` / `_PROMPT2` 值选用带行号的 prompt。分配自定义 prompt 字符串则保持这些字符串不变。

## Trace 输出

`--trace <file>` 通过 `VM::setTraceSink()` 安装 `JsonTraceSink`。`--trace-diff <file>` 额外启用 VM diff 模式，使指令事件包含 `changedRegisters`。参见 `docs/vm/trace-system.md`。

## 相关测试

```powershell
bin\lua_test.exe --filter "AppOptions"
bin\lua_test.exe --filter "REPL Commands"
bin\lua_test.exe --filter "VM Trace Debug"
```
