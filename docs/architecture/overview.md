---
status: current
verified_against: docs/architecture/patterns.md; CMakeLists.txt; cmake/LuaCppConfig.cmake.in; src/common/; src/core/; src/compiler/; src/vm/; src/gc/; src/lib/; src/runtime/runtime_services.hpp; src/runtime/; tests/packaging/; tests/lua/regressions/; tests/lua/integration/
last_checked: 2026-07-22
applies_to: current high-level architecture and source layout
---

# 架构总览

本项目是一个用现代 C++ 实现的 Lua 5.1.5 风格解释器。当前代码库按以下清晰的流水线组织：

```text
Lua 源码
  -> Lexer / Parser（词法/语法分析）
  -> AST（抽象语法树）
  -> CodeGenerator（代码生成器）
  -> Proto 字节码
  -> VM 执行
```

实现刻意采用学习友好型设计：大多数 Lua 概念都有直接的 C++ 对应物，编译器/VM 边界通过 `Proto`、`OpCode` 和基于寄存器的指令模型清晰可见。

## 源码分层

| 层 | 主要文件 | 当前职责 |
|---|---|---|
| Application（应用层） | `src/main.cpp`, `src/repl.cpp`, `src/repl/*`, `src/app/app_options.*` | CLI、脚本模式、REPL 模式、trace 参数解析、REPL 字节码/AST/GC 元命令、Tab 补全、行号 prompt、终端彩色 REPL 错误 |
| Bytecode tool（字节码工具） | `src/bytecode/bytecode_main.cpp`, `src/bytecode/bytecode_printer.*` | 将脚本编译为 `Proto`；打印解码指令、常量、完整模式下的递归子 Proto、并排字节码差异和 Mermaid CFG 图 |
| Runtime services（运行时服务） | `src/runtime/runtime_services.hpp` | 封装 `GlobalState`、`StringPool`、`GarbageCollector`、活跃 `GCStrategy` 和可选 VM 分发策略的轻量显式束 |
| Compiler frontend（编译器前端） | `src/compiler/lexer/lexer.*`, `src/compiler/parser/parser*.cpp`, `src/compiler/ast.*` | 将源码分词并构建 AST |
| Compiler shared model（编译器共享模型） | `src/compiler/ast.*`, `src/compiler/ast_visitor.hpp`, `src/compiler/opcode.*`, `src/compiler/register_allocator.hpp` | Parser、CodeGen、测试和 VM 共享的 AST 和字节码定义 |
| Code generation（代码生成） | `src/compiler/codegen/codegen*.cpp`, `src/compiler/codegen/function_compiler.*`, `src/compiler/codegen/codegen_types.hpp`, `src/compiler/codegen/codegen_context.hpp`, `src/compiler/codegen/codegen_ops.hpp`, `src/compiler/codegen/bytecode_builder.hpp` | 将 AST 降低为 `Proto` 字节码 |
| Core objects（核心对象） | `src/core/value.*`, `table.*`, `function.*`, `upvalue.*`, `userdata.*`, `thread.*` | Lua 值和 GC 对象的 C++ 表示 |
| VM（虚拟机） | `src/vm/vm*.cpp`, `src/vm/state/lua_state.*`, `src/vm/state/stack.*`, `src/vm/state/call_info.hpp` | 执行字节码，管理调用、栈、hook、trace 和协程 yield |
| GC（垃圾回收） | `src/gc/garbage_collector.*`, `src/gc/gc_strategy.*`, `src/core/gc_object.*` | 策略驱动的标记-清除回收、弱表、userdata 终结器、增量式教学占位实现 |
| Standard library（标准库） | `src/lib/*.cpp`, `lib_catalog.*`, `lib_manager.*`, `lib_registry.*` | 将 Lua 标准库注册到 `LuaState` |
| Debug trace（调试追踪） | `src/debug/*.hpp`, `src/debug/*.cpp`, `src/vm/vm_trace.cpp` | JSONL VM 执行追踪和值序列化 |

## 核心运行时模型

`Value` 是统一的 Lua 值容器，使用 `std::variant` 覆盖 nil、boolean、light userdata、number、`GCString*`、`Table*`、`Function*`、`Userdata*` 和 `Thread*`。

`GCObject` 是所有可回收对象的基类。当前可回收类型包括：

- `GCString`
- `Table`
- `Proto`
- `Function`
- `Upvalue`
- `Userdata`
- `Thread`

每个子类实现 `mark(GarbageCollector&)` 和 `getSize()`。回收器持有全局对象链表并执行标记-清除回收。

## Global 和 Thread 状态

`GlobalState` 仍以单例为后盾，但新入口点应优先在可用时传递 `RuntimeServices`。它拥有共享运行时服务：

- 字符串驻留池
- 垃圾回收器
- 注册表
- 固定字符串和元方法名称
- 基础类型元表
- 主线程和当前运行线程指针

`LuaState` 表示单个执行状态。它拥有值栈、调用栈、全局表、debug hook 状态、open upvalue 和 yield 簿记。`Thread` 包装独立的 `LuaState` 用于协程执行。VM 状态类型位于 `src/vm/state/` 下；执行、分发、辅助和处理文件仍位于 `src/vm/` 和 `src/vm/vm_handlers/` 下。

## 编译器形态

当前编译器基于 AST 而非 Lua 5.1 原始的单遍 parser/codegen 流水线。`ast.hpp/.cpp` 位于 `src/compiler/`，因为它们是 parser 的产出并被代码生成/测试消费。`opcode.hpp/.cpp` 也位于 `src/compiler/`，因为它们定义了代码生成、字节码打印和 VM 共享的字节码契约。

```text
Parser -> Chunk AST -> CodeGenerator -> Proto
```

`CodeGenerator` 仍是编排类，但其实现已拆分：

- `name_binder.cpp`：名字解析为 `SymbolRef` 以及绑定转换为 value/lvalue 通道
- `codegen_binding.cpp`：`CodeGenerator` 上稳定的公开绑定包装器
- `codegen_ops.hpp`：共享的低层指令发射、参数回填和行号/寄存器保护
- `function_compiler.cpp`：子 `Proto` 编译、closure upvalue 和调试元数据
- `expression_emitter.cpp`：value、condition、lvalue、call、vararg、table 表达式降低
- `statement_emitter.cpp`：语句、循环、返回、块和语句级控制流
- `jump_patcher.cpp`：跳转链表和回填
- `codegen_stmt.cpp`：函数级辅助函数的兼容转发
- `codegen.cpp`：构造函数、顶层生成、字节码发射包装器

表达式流水线使用 `SymbolRef`、`ValueResult`、`CondResult`、`LValueRef` 和 `CallResultInfo`，分别表达绑定结果、右值、条件跳转、左值和多返回值调用。

## VM 形态

VM 执行 Lua 5.1 风格的寄存器字节码。`src/compiler/opcode.hpp` 定义了 38 条操作码和指令编码。`src/vm/vm.cpp` 包含主分发循环，辅助函数分布在专注的文件中：

- `vm_entry.cpp`：`VM::execute()` 和 `VM::call()`
- `vm_call.cpp`：调用前、调用后、尾调用帧复用
- `vm_ops.cpp`：算术、比较、表/元方法辅助函数
- `vm_table.cpp`：表初始化辅助函数
- `vm_frame.cpp`：闭包和 vararg 辅助函数
- `vm_loop.cpp`：泛型 for 循环辅助函数
- `vm_trace.cpp`：trace 和 debug hook 分发

## 构建目标

Visual Studio 解决方案包含四个活跃项目：

- `lua.vcxproj`：核心静态库
- `lua_app.vcxproj`：解释器/REPL 可执行文件
- `lua_test.vcxproj`：单元测试可执行文件
- `lua_bytecode.vcxproj`：字节码检查可执行文件

CMake/CTest 构建 `lua_core`、`lua_public_api_shared`、`lua_app`、`lua_test` 和 `lua_bytecode`，并安装带版本合同的 `LuaCpp::Lua` / `LuaCpp::Shared` 导出目标、公开 C 头文件与 PackageConfig。`cmake_package_consumer` 使用全新纯 C 源码 consumer 验证安装后的静态及共享消费路径；因静态库由 C++ 实现，consumer 工程同时启用 C++ linker language。

## 阅读地图

- 技术百科入口：`docs/index.md`
- 编译器详情：`docs/compiler/bytecode-generation.md`
- 架构模式：`docs/architecture/patterns.md`
- VM 操作码：`docs/vm/instruction-set.md`
- GC 详情：`docs/gc/implementation.md`
- 运行时服务：`docs/runtime/services.md`
- 标准库总览：`docs/stdlib/overview.md`
