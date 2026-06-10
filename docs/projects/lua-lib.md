---
status: current
verified_against: lua.vcxproj; CMakeLists.txt; src/common/; src/core/; src/compiler/; src/vm/; src/gc/; src/lib/; src/runtime/runtime_services.hpp
last_checked: 2026-05-19
applies_to: lua.vcxproj / lua_core static library
---

# lua.lib / lua_core

`lua.vcxproj` 构建核心静态库，供解释器应用、字节码工具和测试运行器链接使用。在 CMake 中，对应目标为 `lua_core`。

## 职责

- Lua 值和对象模型
- 字符串驻留池
- table、function、upvalue、userdata、thread 对象
- 垃圾回收器
- 词法分析器、语法分析器、AST、字节码生成
- VM 执行辅助
- 标准库实现
- 运行时服务边界
- 调试 trace 序列化

## 不包含

核心库不包含以下内容：

- CLI 参数解析
- REPL 循环
- 字节码打印器可执行入口
- 测试运行器 main 函数

这些由其他项目负责。

## 主要文档

- `docs/architecture/overview.md`
- `docs/architecture/gc.md`
- `docs/architecture/runtime-services.md`
- `docs/compiler/bytecode-generation.md`
- `docs/vm/instruction-set.md`
- `docs/stdlib/overview.md`
