---
status: current
verified_against: src/bytecode/bytecode_main.cpp; src/bytecode/bytecode_printer.cpp; src/bytecode/bytecode_printer.hpp; lua_bytecode.vcxproj; CMakeLists.txt
last_checked: 2026-05-23
applies_to: lua_bytecode command-line tool status
---

# 字节码工具指南

`lua_bytecode.exe` 编译 Lua 脚本并打印生成的 `Proto` 字节码。

当前状态适用于字节码检查、轻量级字节码比较和控制流可视化：

- `src/bytecode/bytecode_main.cpp` 为 print / `--cfg` 模式读取一个脚本，或为 `--diff` 读取两个脚本，解析它们，生成 `Proto` 对象，并调用打印器 API。
- `src/bytecode/bytecode_printer.cpp` 打印 Proto 头、解码后的指令（decoded instructions）、常量引用（constant references）、常量表（constant table），以及在完整模式下递归打印子 proto（recursive child protos in full mode）。
- `printProtoBytecodeDiff()` 渲染两侧，忽略 source-path 元数据噪声，并仅在并排差异表（side-by-side）中打印发生变化的行（changed lines）。
- `printProtoBytecodeCfg()` 渲染 Mermaid `flowchart TD`，包含基本块（basic blocks）、跳转边、TEST / TFORLOOP companion-jump 边、FORLOOP 回边和返回出口等控制流边（control-flow edges）。

因此该工具适用于跨闭包的字节码检查、比较两个生成的字节码列表，以及将控制流图粘贴到支持 Mermaid 的文档中。

## 用法

```powershell
bin\lua_bytecode.exe examples\hello.lua
bin\lua_bytecode.exe examples\hello.lua full
bin\lua_bytecode.exe examples\control_flow.lua --cfg
bin\lua_bytecode.exe examples\control_flow.lua --cfg full
bin\lua_bytecode.exe examples\before.lua examples\after.lua --diff
bin\lua_bytecode.exe examples\before.lua examples\after.lua --diff full
```

`full` 启用递归子 Proto 输出。紧凑模式仅打印顶层 Proto；两种模式都会用引用的 `proto[index]` 摘要注解 `CLOSURE`。

`--diff` 在同一运行时服务中编译两个脚本并比较渲染后的字节码输出。Plain `--diff` 比较紧凑的顶层列表；`--diff full` 包含递归子 Proto 段。左右文件名显示为标签，而 `source:` 路径元数据在比较期间被忽略，因此不同文件中相同的字节码仍被报告为相同。

`--cfg` 编译一个脚本并直接输出 Mermaid 语法。Plain `--cfg` 打印顶层 Proto CFG；`--cfg full` 也为子 Proto 输出兄弟子图。该图每个基本块使用一个节点，并标注边如 `fallthrough`、`jump`、`test jump`、`test fallthrough`、`prepare`、`loop`、`exit` 和 `return`。

## 当前数据流

```text
脚本路径
  -> readWholeFile()
  -> Parser
  -> CodeGenerator
  -> Proto*
  -> printProtoBytecode(...)

左/右脚本路径
  -> readWholeFile()
  -> Parser
  -> CodeGenerator
  -> Proto*
  -> printProtoBytecodeDiff(...)

脚本路径 + --cfg
  -> readWholeFile()
  -> Parser
  -> CodeGenerator
  -> Proto*
  -> printProtoBytecodeCfg(...)
```

## 当前输出

当前打印器显示：

- 源码名称
- 参数数量和 vararg 标志
- `maxStackSize`
- 常量
- 行信息
- 每条指令及其解码后的 `A/B/C/Bx/sBx`
- `LOADK`、RK 操作数和跳转目标的常量注释
- 带子 proto 引用的 `CLOSURE` 注释
- 完整模式下的递归子 proto
- `--diff` 摘要字段：左标签、右标签、模式、状态和变更行
- `--diff` 变更的渲染字节码行的并排行
- `--cfg` Mermaid 输出，包含基本块和标注的控制流边
- `--cfg full` 递归子 Proto CFG 子图

## 待完成工作

一个完整的打印器还应添加：

- 局部变量调试名称

自然的实现依赖是用于指令解码的 `src/compiler/opcode.hpp` 和用于 `Proto` 访问器的 `src/core/function.hpp`。
