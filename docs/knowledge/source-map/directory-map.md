---
status: current
verified_against: src/; tests/unit/; tests/lua/; CMakeLists.txt; lua.vcxproj; lua_app.vcxproj; lua_test.vcxproj; lua_bytecode.vcxproj
last_checked: 2026-07-11
applies_to: stable source directory ownership map
---

# 源码目录责任地图

本页只维护稳定的模块责任，不复制每个文件名。具体改动入口与测试闭环见 [源码、文档与测试映射](../source-document-map.md)。

## 顶层

| 路径 | 责任 |
|---|---|
| `src/` | 解释器、运行时、库和应用生产代码 |
| `tests/unit/` | C++ 内部契约与数据结构测试 |
| `tests/lua/` | Lua 行为、official 与 regression scripts |
| `docs/` | 唯一技术百科根目录 |
| `examples/` | 可运行 Lua 示例，不承担规范事实 |
| `tools/` | 构建与质量门自动化，使用方式由脚本自身维护 |
| `CMakeLists.txt`、`*.vcxproj` | CMake 与 Visual Studio 构建图 |

## `src/` 模块

| 目录 | 拥有的数据/控制流 | 允许依赖的方向 |
|---|---|---|
| `common/` | 类型别名、错误层次、数值转换、低层公共设施 | 不依赖解释器上层 |
| `compiler/lexer/` | 字符 cursor、token 扫描、lookahead、词法位置 | common、parser token 类型 |
| `compiler/parser/` | token stream、递归下降、AST 构造、错误恢复 | lexer、AST、common |
| `compiler/codegen/` | binding、result types、寄存器、jump、Proto 生成 | AST、opcode、core Proto 接口 |
| `core/` | Value、Table、Function/Proto、Upvalue、Userdata、Thread、GCObject | common；通过窄接口接入 GC/VM |
| `runtime/` | `RuntimeServices` 等显式服务集合 | core、GC、VM 策略接口 |
| `vm/state/` | LuaState、GlobalState、Stack、CallInfo | core、runtime services |
| `vm/vm_handlers/` | opcode 的执行语义 | VM context、core/runtime |
| `vm/` | dispatch、call/frame/loop、metamethod helper、trace hook | state、handlers、core、runtime |
| `gc/` | collector、strategy、mark/sweep/weak/finalize、barrier | GCObject 接口、GlobalState roots |
| `lib/` | Lua 5.1 标准库 native functions 与注册 | LuaState API、runtime services |
| `api/` | 对外 Lua C API 适配 | LuaState、core、lib |
| `debug/` | 结构化 trace event/sink 与 Value 序列化 | 只读借用 VM/core 状态 |
| `io/` | 输入流、文件加载、动态缓冲 | common；不拥有 parser 语义 |
| `bytecode/` | Proto 反汇编与 CFG 展示工具 | compiler/core 的只读产物 |
| `repl/` | 交互输入、history、completion、元命令 | app/compiler/VM 公共入口 |
| `app/`、`main.cpp` | 参数适配、脚本/REPL 应用入口 | 组合下层模块，不被下层依赖 |

## 依赖主干

```text
Application / REPL / Tools
          ↓
Compiler ─────────→ Core Proto
   ↓                   ↓
Opcode ABI ───────→ VM / LuaState
                         ↓
                 Core Runtime Objects
                    ↙           ↘
                  GC          Standard Library/API
```

`core` 与 `vm` 的协作不可完全画成无环层级，但新增依赖应通过窄接口或 `RuntimeServices`，避免隐藏 singleton 和跨模块全局状态。

## 构建产物边界

| 项目 | 责任 |
|---|---|
| `lua` | 核心静态库 |
| `lua_app` | 脚本与 REPL 应用 |
| `lua_test` | 统一测试运行器 |
| `lua_bytecode` | Proto/bytecode 观察工具 |

构建文件决定物理编译单元是否进入所有构建系统；新增源码时 CMake 与 Visual Studio 项目必须同步，这属于源码图一致性而非文档进度。
