---
status: current
verified_against: lua_bytecode.vcxproj; CMakeLists.txt; src/bytecode/bytecode_main.cpp; src/bytecode/bytecode_printer.cpp; src/bytecode/bytecode_printer.hpp
last_checked: 2026-05-23
applies_to: lua_bytecode executable
---

# lua_bytecode

`lua_bytecode.vcxproj` 构建字节码检查可执行文件。在 CMake 中，目标名为 `lua_bytecode`。

## 当前状态

该可执行文件可以解析和编译 Lua 源文件为 `Proto` 对象。`printProtoBytecode()` 打印 Proto 元数据、解码后的指令、常量引用、常量表，以及在传入可选的 `full` 参数时递归打印子 Proto 段。`--diff` 编译两个脚本并打印并排的字节码差异摘要。`--cfg` 编译单个脚本并生成 Mermaid `flowchart TD` 控制流图，包含基本块和标注的跳转/穿透/循环/返回边。

## 源文件

- `src/bytecode/bytecode_main.cpp`
- `src/bytecode/bytecode_printer.cpp`
- `src/bytecode/bytecode_printer.hpp`

## 文档

参见 `docs/guides/bytecode-tool.md`。
