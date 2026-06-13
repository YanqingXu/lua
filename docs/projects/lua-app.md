---
status: current
verified_against: lua_app.vcxproj; CMakeLists.txt; src/main.cpp; src/repl.cpp; src/repl/; src/repl.hpp; src/app/app_options.cpp; src/app/app_options.hpp
last_checked: 2026-05-23
applies_to: lua_app interpreter executable
---

# lua_app

`lua_app.vcxproj` 构建解释器可执行文件。在 CMake 中，目标名为 `lua_app`。

## 源文件

- `src/app/app_options.cpp`
- `src/app/app_options.hpp`
- `src/main.cpp`
- `src/repl.cpp`
- `src/repl/repl_*.cpp`
- `src/repl.hpp`

## 运行模式

可执行文件支持：

- 版本模式
- 帮助模式
- 脚本模式
- REPL 模式
- 默认行为模式
- 可选 JSONL VM trace 输出

REPL 包含 `.help`、`.bytecode <expr|chunk>`、`.ast <expr|chunk>` 和 `.gc [stats|collect|strategy|help]` 元命令，用于在不离开交互会话的情况下检查 parser、字节码和活跃 `GCStrategy` 状态。Tab 补全覆盖元命令、全局名称和已加载库表字段（如 `string.sub`）；默认 prompt 显示行号，终端 REPL 错误会彩色显示，而重定向输出保持纯文本。

面向用户的行为参见 `docs/guides/repl-cli.md`。

## 运行时设置

`main.cpp` 创建 `LuaState`，加载标准库，可选安装 `JsonTraceSink`，然后运行脚本或进入 REPL。
