# 学习路线图

本文面向第一次阅读本仓库的新开发者，目标是用最短路径建立对“现代 C++ 实现的 Lua 5.1 解释器”的整体认识。建议把阅读过程拆成三条线并行推进：

- **源码**：从 `src/` 中的编译器、VM、GC 模块观察真实实现。
- **文档**：从 `docs/architecture/overview.md` 和 `docs/walkthroughs/` 建立概念模型。
- **工具**：用 `bin/lua_bytecode.exe` 和 `bin/lua_app.exe` 把 Lua 源码、字节码和运行结果对应起来。

## 快速上手路径

### 10 分钟：建立全局地图

1. 阅读 `docs/architecture/overview.md`，先理解项目分层：
   - `src/compiler/`：Lexer、Parser、AST、CodeGenerator。
   - `src/vm/`：LuaState、Stack、CallInfo、VM dispatch 与 handler。
   - `src/gc/`：GCObject、根集扫描、标记清除和策略边界。
   - `src/lib/`：base、string、table、io、os、coroutine、debug、package 等标准库。
2. 打开 `README.md` 的“设计哲学”和“核心特性”，确认本项目为何强调可读性、显式中间结果类型和现代 C++ 类型建模。
3. 运行一个最小脚本：

```powershell
bin\lua_app.exe examples\hello.lua
```

这一步只需要确认解释器入口能运行，并知道后续所有阅读都会围绕“源码如何变成输出”展开。

随后切到 `tests/lua/` 下的教学脚本。它们比 `examples/` 更短，适合第一次观察单个语法点：

```powershell
bin\lua_app.exe tests\lua\step01_basic.lua
bin\lua_app.exe tests\lua\step02_logic.lua
bin\lua_app.exe tests\lua\step03_table.lua
```

### 30 分钟：走完 hello-world 主线

1. 阅读 `docs/walkthroughs/hello-world.md`。这是最重要的第一篇端到端文档，覆盖：
   - Lua 源码进入 Lexer。
   - Parser 构建 AST。
   - CodeGenerator 生成 Proto 和字节码。
   - VM dispatch 执行指令。
   - `print` 进入 base library 并写出结果。
2. 用字节码工具查看 hello-world 和最小教学脚本：

```powershell
bin\lua_bytecode.exe examples\hello.lua full
bin\lua_bytecode.exe tests\lua\step01_basic.lua full
```

3. 回到源码中定位对应模块：
   - `src/compiler/lexer/lexer.cpp`
   - `src/compiler/parser/parser.cpp`
   - `src/compiler/codegen/codegen.cpp`
   - `src/compiler/codegen/expression_emitter.cpp`
   - `src/vm/vm.cpp`
   - `src/vm/vm_handlers.cpp`
   - `src/lib/baselib.cpp`

读源码时不要追求一次看完所有细节。先找到“入口函数”和“下一跳”，把主调用链连起来。

### 2 小时：建立源码到 VM 的可调试路径

1. 继续阅读：
   - `docs/compiler/bytecode-generation.md`
   - `docs/vm/instruction-set.md`
   - `docs/guides/bytecode-tool.md`
2. 对比三个教学脚本的字节码输出：

```powershell
bin\lua_bytecode.exe tests\lua\step01_basic.lua full
bin\lua_bytecode.exe tests\lua\step02_logic.lua full
bin\lua_bytecode.exe tests\lua\step03_table.lua full
```

3. 为包含分支和循环的脚本生成 CFG：

```powershell
bin\lua_bytecode.exe tests\lua\step02_logic.lua --cfg
bin\lua_bytecode.exe tests\lua\step02_logic.lua --cfg full
```

4. 打开 `src/compiler/opcode.hpp` 和 `src/vm/vm_handlers/`，把字节码指令和具体 handler 对应起来。
5. 阅读 `docs/walkthroughs/closure-and-upvalue.md` 和 `docs/walkthroughs/gc-cycle.md`，把函数生命周期和内存生命周期纳入同一张图。

## 端到端执行流追踪

推荐从 `examples/hello.lua` 开始：

```lua
local name = "Lua C++"
print("hello, " .. name)
```

### 1. 运行脚本

```powershell
bin\lua_app.exe examples\hello.lua
```

观察重点：

- 全局函数 `print` 如何被查找。
- 字符串拼接如何进入表达式求值。
- 语句执行结束后 chunk 如何返回。

### 2. 查看完整字节码

```powershell
bin\lua_bytecode.exe examples\hello.lua full
```

观察重点：

- `Proto` 头部信息：参数数量、vararg 标志、`maxStackSize`。
- `constants`：字符串常量和数字常量如何进入常量表。
- 指令列表：`LOADK`、`GETGLOBAL`、`MOVE`、`CONCAT`、`CALL`、`RETURN` 等如何构成执行序列。
- `full` 模式：当脚本包含嵌套函数时，如何递归输出子 Proto。

### 3. 生成控制流图

`hello.lua` 的控制流很直线。观察 CFG 时更推荐使用带分支和循环的脚本：

```powershell
bin\lua_bytecode.exe tests\lua\step02_logic.lua --cfg
bin\lua_bytecode.exe tests\lua\step02_logic.lua --cfg full
```

输出是 Mermaid `flowchart TD`。可以把它粘贴到支持 Mermaid 的 Markdown 查看器中，重点观察：

- 基本块如何按跳转边拆分。
- 条件测试如何形成 `test jump` 和 `test fallthrough`。
- `FORLOOP` / `FORPREP` 如何形成 loop back edge。
- `RETURN` 如何形成出口块。

### 4. 使用教学脚本建立映射

`tests/lua/step*.lua` 是专门给阅读源码用的低噪声输入。建议每次只打开一个脚本，先运行，再看字节码或 CFG，最后回到对应 C++ 入口。

| 脚本 | 观察命令 | 对应内部环节 | 源码阅读入口 |
|------|----------|--------------|--------------|
| `tests/lua/step01_basic.lua` | `bin\lua_app.exe tests\lua\step01_basic.lua`<br>`bin\lua_bytecode.exe tests\lua\step01_basic.lua full` | 演示 `Lexer` -> `Parser` -> `CodeGen` 的线性转换：局部变量、数字常量、二元表达式和一次 `print` 调用如何变成 `Proto`、常量表和直线指令序列。 | `Lexer::nextToken()` / `Lexer::scanToken()` in `src/compiler/lexer/lexer.cpp`；`Parser::parse()` / `Parser::Impl::parseBlock()` in `src/compiler/parser/parser.cpp`、`src/compiler/parser/parser_stmt.cpp`；`CodeGenerator::generate()` / `CodeGenerator::generateUnchecked()` in `src/compiler/codegen/codegen.cpp`；`ExpressionEmitter::emitValue()`、`StatementEmitter::emitStmt(const LocalStmt&)`。 |
| `tests/lua/step02_logic.lua` | `bin\lua_app.exe tests\lua\step02_logic.lua`<br>`bin\lua_bytecode.exe tests\lua\step02_logic.lua full`<br>`bin\lua_bytecode.exe tests\lua\step02_logic.lua --cfg` | 演示条件分支和数值 `for` 循环如何进入 CFG：`if i < 3 then` 形成测试边，`for i = 1, 3 do` 形成 `FORPREP` / `FORLOOP` 回边。 | `Parser::Impl::parseIfStmt()` / `parseForStmt()` in `src/compiler/parser/parser_stmt.cpp`；`StatementEmitter::emitStmt(const IfStmt&)` / `emitStmt(const ForNumStmt&)` in `src/compiler/codegen/statement_emitter.cpp`；`ExpressionEmitter::emitCondResult()`；`VM::executeProto()` 和 `runDispatchBackend()` in `src/vm/vm.cpp`；分支和循环 handler 位于 `src/vm/vm_handlers/vm_handlers_branch.cpp`、`src/vm/vm_handlers/vm_handlers_loop.cpp`。 |
| `tests/lua/step03_table.lua` | `bin\lua_app.exe tests\lua\step03_table.lua`<br>`bin\lua_bytecode.exe tests\lua\step03_table.lua full` | 演示表构造、字段读写和 `__index` 元方法：普通字段 `data.value` 走直接表访问，缺失字段 `data.missing` 进入 `VM::detail::gettable()` 的元方法路径。 | `Parser::Impl::parseTableConstructor()` in `src/compiler/parser/parser_table.cpp`；`ExpressionEmitter::emitValueTable()`、`emitValueMember()`、`emitStore()` in `src/compiler/codegen/expression_emitter.cpp`；`handleNewTable()` / `handleGetTable()` / `handleSetTable()` in `src/vm/vm_handlers/vm_handlers_table.cpp`；`VM::detail::gettable()` / `settable()` in `src/vm/vm_ops.cpp`；`setmetatable` 入口在 `src/lib/baselib.cpp`。 |

> 注：旧文档或脑图里可能会把字节码生成入口记成 `src/compiler/codegen.cpp`。当前仓库已拆到 `src/compiler/codegen/codegen.cpp`，阅读时以实际路径为准。

### 5. 用临时脚本做小实验

可以用临时文件构造更小的实验输入：

```powershell
$tmp = New-TemporaryFile
Set-Content -LiteralPath $tmp -Value 'for i = 1, 3 do print(i) end' -NoNewline
bin\lua_bytecode.exe $tmp full
bin\lua_bytecode.exe $tmp --cfg
bin\lua_app.exe $tmp
Remove-Item -LiteralPath $tmp
```

这种方式适合验证你对某条 VM 指令或某段 CodeGen 行为的理解。

## 源码映射指南

下面的表把 Lua 执行阶段、常见概念路径和当前仓库实际路径对应起来。若你来自其他 Lua 实现或早期文档，可能会习惯 `lexer.cpp`、`parser.cpp`、`virtual_machine.cpp` 这类单文件命名；当前仓库已经按职责拆分为更细的目录。

| Lua 执行阶段 | 概念入口 | 当前仓库路径 | 阅读重点 |
|--------------|----------|--------------|----------|
| 词法分析 | `src/compiler/lexer.cpp` | `src/compiler/lexer/lexer.cpp`, `src/compiler/lexer/lexer.hpp`, `src/compiler/parser/token.hpp` | 字符流如何变成 token；字符串、数字、注释和保留字如何扫描 |
| 语法分析 | `src/compiler/parser.cpp` | `src/compiler/parser/parser.cpp`, `src/compiler/parser/parser_stmt.cpp`, `src/compiler/parser/parser_expr.cpp`, `src/compiler/parser/parser_primary.cpp`, `src/compiler/parser/parser_func.cpp`, `src/compiler/parser/parser_table.cpp` | token 如何变成 AST；语句、表达式、函数和表构造如何分片解析 |
| AST 定义 | `src/compiler/ast.hpp` | `src/compiler/ast.hpp`, `src/compiler/ast.cpp`, `src/compiler/ast_visitor.hpp` | AST 节点类型、visitor 覆盖和教学型节点分派 |
| 字节码生成 | `src/compiler/codegen.cpp` | `src/compiler/codegen/codegen.cpp`, `src/compiler/codegen/expression_emitter.cpp`, `src/compiler/codegen/statement_emitter.cpp`, `src/compiler/codegen/codegen_ops.hpp` | AST 如何 lowering 到 Proto；`ValueResult` / `CondResult` / `LValueRef` 如何表达中间状态 |
| Proto / 函数对象 | `src/compiler/proto.hpp` | `src/core/function.hpp`, `src/core/function.cpp`, `src/compiler/codegen/bytecode_builder.hpp` | Proto 如何保存指令、常量、子函数、upvalue 和调试元数据 |
| 指令定义 | `src/vm/opcodes.hpp` | `src/compiler/opcode.hpp`, `src/compiler/opcode.cpp` | 38 条 Lua 5.1 风格指令的编码、解码和名称 |
| 虚拟机执行 | `src/vm/virtual_machine.cpp` | `src/vm/vm.cpp`, `src/vm/vm_loop.cpp`, `src/vm/vm_dispatch.hpp`, `src/vm/vm_switch_dispatch.hpp`, `src/vm/vm_handlers.cpp`, `src/vm/vm_handlers/` | VM 如何取指、分发、执行 handler，以及如何维护调用栈 |
| 运行时状态 | Lua state / stack | `src/vm/state/lua_state.hpp`, `src/vm/state/stack.hpp`, `src/vm/state/call_info.hpp`, `src/vm/state/global_state.hpp` | 栈、调用帧、全局状态和字符串池如何协作 |
| 标准库调用 | base / stdlib | `src/lib/baselib.cpp`, `src/lib/lib_catalog.cpp`, `src/lib/lib_manager.cpp` | `print`、`type`、`pcall`、`require` 等函数如何注册和调用 |
| 内存管理 | `src/gc/` | `src/gc/garbage_collector.cpp`, `src/gc/gc_mark.cpp`, `src/gc/gc_sweep.cpp`, `src/gc/gc_finalize.cpp`, `src/gc/gc_strategy.cpp` | 根集、标记、清扫、终结器、弱表和 GC 策略边界 |

## 实践建议

### 修改教学脚本验证 VM 行为

从 `tests/lua/step*.lua` 复制一个脚本做实验，或者直接新建临时文件。建议按以下顺序修改：

1. **表达式与常量**：修改 `tests/lua/step01_basic.lua`，加入数字、字符串拼接和局部变量。
2. **控制流**：修改 `tests/lua/step02_logic.lua`，观察 `if`、`while`、`for` 如何改变 CFG。
3. **表与元方法**：修改 `tests/lua/step03_table.lua`，观察 `GETTABLE`、`SETTABLE`、`NEWTABLE` 和 `__index` 慢路径。
4. **更完整的行为**：再切回 `examples/`，用 `examples/tables_and_methods.lua` 或 `examples/metamethods.lua` 观察更接近真实程序的组合场景。

每次修改后运行三步：

```powershell
$script = "tests\lua\step02_logic.lua"
bin\lua_app.exe $script
bin\lua_bytecode.exe $script full
bin\lua_bytecode.exe $script --cfg
```

如果脚本包含嵌套函数，使用 `--cfg full` 观察子 Proto 的 CFG。

### 深入专题

- 闭包与 upvalue：阅读 `docs/walkthroughs/closure-and-upvalue.md`，再查看 `src/core/upvalue.cpp`、`src/compiler/codegen/function_compiler.cpp` 和 `src/vm/vm_handlers/vm_handlers_closure.cpp`。
- GC 周期：阅读 `docs/walkthroughs/gc-cycle.md`，再查看 `src/gc/` 和 `src/core/gc_object.hpp`。
- 字节码工具：阅读 `docs/guides/bytecode-tool.md`，再查看 `src/bytecode/bytecode_main.cpp` 和 `src/bytecode/bytecode_printer.cpp`。
- REPL 辅助学习：运行 `bin\lua_app.exe`，使用 `.bytecode <expr|chunk>`、`.ast <expr|chunk>` 和 `.gc` 元命令快速检查理解。

## 工具链使用

### 确认测试入口可用

```powershell
bin\lua_test.exe --list
bin\lua_test.exe --filter "Runtime Services"
bin\lua_test.exe --filter "REPL Commands"
```

`lua_test.exe` 会输出真实注册测试和断言结果。阅读或实验前，先确认测试入口能启动；修改源码后，再运行相关 filter 缩小验证范围。

### 运行完整测试

```powershell
bin\lua_test.exe
```

完整测试适合在阶段性修改后运行。若你只是在阅读文档或运行示例脚本，可以先使用 `--list` 和小范围 `--filter`。

### 检查文档漂移

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\check_doc_drift.ps1
```

这个脚本用于检查文档中的关键事实是否与当前工程入口、测试统计和构建配置保持一致。修改 README、技术文档或项目文件后，建议运行一次。

### 推荐的日常学习循环

```text
读一段 walkthrough
  -> 找到对应源码入口
  -> 修改一个 tests/lua/step*.lua 教学脚本
  -> 用 lua_bytecode 查看字节码和 CFG
  -> 用 lua_app 运行脚本
  -> 用 lua_test filter 验证相关模块
```

这条循环能把“读懂文档”和“读懂执行结果”连接起来，是掌握本项目最快的方式。
