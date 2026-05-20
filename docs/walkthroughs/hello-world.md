---
status: current
verified_against: bin/lua_bytecode.exe; bin/lua_app.exe; src/compiler/lexer.cpp; src/compiler/parser.cpp; src/compiler/parser_primary.cpp; src/compiler/codegen_expr.cpp; src/compiler/codegen_stmt.cpp; src/vm/vm.cpp; src/vm/vm_handlers.cpp; src/lib/baselib.cpp
last_checked: 2026-05-20
applies_to: end-to-end execution path for print("hello")
---

# Hello World End-to-End

这篇 walkthrough 追踪最小脚本：

```lua
print("hello")
```

它的路径是：源码字符进入 Lexer，Parser 生成调用表达式 AST，CodeGenerator 写入字节码，VM 通过默认 `SwitchDispatch` 执行 `GETGLOBAL` / `LOADK` / `CALL`，最后 `CALL` 进入基础库的 `luaB_print`，把字符串写到 `stdout`。

## 可复现命令

为了避免示例文件里的其它表达式干扰，可以用临时文件运行同一段源码：

```powershell
$tmp = New-TemporaryFile
Set-Content -LiteralPath $tmp -Value 'print("hello")' -NoNewline
.\bin\lua_bytecode.exe $tmp full
.\bin\lua_app.exe $tmp
Remove-Item -LiteralPath $tmp
```

`lua_app` 的最终输出只有一行：

```text
hello
```

`lua_bytecode` 会输出这段 Proto：

```text
Proto
  source: <temporary file>
  linedefined: 0
  lastlinedefined: 0
  numparams: 0
  is_vararg: true
  maxStackSize: 3
  upvalues (0): (none)
instructions (5)
0000 | line 1 | GETGLOBAL | A=0 Bx=0 ; K[0] = string "print"
0001 | line 1 | MOVE | A=1 B=0 C=0
0002 | line 1 | LOADK | A=2 Bx=1 ; K[1] = string "hello"
0003 | line 1 | CALL | A=1 B=2 C=1
0004 | line 0 | RETURN | A=0 B=1 C=0
constants (2)
  K[0] = string "print"
  K[1] = string "hello"
```

后面的解释都围绕这 5 条指令展开。

## 1. Lexer：字符变成 Token

入口在 `Lexer::nextToken()`，它负责处理 peek 缓存并调用 `scanToken()` 取下一个词法单元，见 `src/compiler/lexer.cpp:626` 和 `src/compiler/lexer.cpp:738`。

`print("hello")` 被拆成这些 token：

| 源码片段 | Token 含义 | 相关实现 |
|---|---|---|
| `print` | identifier / name | `Lexer::identifier()`，`src/compiler/lexer.cpp:337` |
| `(` | 单字符操作符 | `Lexer::scanToken()`，`src/compiler/lexer.cpp:738` |
| `"hello"` | string literal | `Lexer::string()`，`src/compiler/lexer.cpp:472` |
| `)` | 单字符操作符 | `Lexer::scanToken()`，`src/compiler/lexer.cpp:738` |
| EOF | 输入结束 | `Lexer::scanToken()`，`src/compiler/lexer.cpp:738` |

这里还没有“函数调用”的概念。Lexer 只负责告诉后续阶段：有一个名字、一个左括号、一个字符串、一个右括号。

## 2. Parser：Token 变成 AST

`Parser::parse()` 从 `src/compiler/parser.cpp:153` 开始循环读取语句。这个例子不是 `local`、`return` 或控制流语句，所以会进入表达式语句路径 `Parser::parseExprStmt()`，见 `src/compiler/parser_stmt.cpp:331`。

表达式内部的关键点在 `Parser::parsePrimaryExpr()` 和 `Parser::parsePostfixExpr()`：

- `print` 先被解析成 `NameExpr`，见 `src/compiler/parser_primary.cpp:99`。
- 后缀解析遇到 `(`，把前面的 `NameExpr` 作为被调用对象，构造 `CallExpr`，见 `src/compiler/parser_primary.cpp:107`。
- `"hello"` 被解析成 `StringExpr` 并放入 `CallExpr::args`，见 `src/compiler/parser_primary.cpp:54` 和 `src/compiler/parser_primary.cpp:114`。
- 语句层确认表达式确实是 `CallExpr`，把它包装成 `CallStmt`，见 `src/compiler/parser_stmt.cpp:369`。

简化后的 AST 形状是：

```text
Chunk
  CallStmt
    CallExpr
      func: NameExpr("print")
      args:
        StringExpr("hello")
```

AST 节点定义在 `src/compiler/ast.hpp`，其中 `StringExpr` 位于 `src/compiler/ast.hpp:88`，`CallExpr` 位于 `src/compiler/ast.hpp:158`。

## 3. CodeGen：AST 变成 Proto

`CodeGenerator::generate()` 创建当前 Proto，编译 chunk 中的语句，并在末尾补一条空返回，入口见 `src/compiler/codegen.cpp:40`。

对于语句级调用，`CodeGenerator::emitStmt(const CallStmt&)` 会复用 `emitCallExpr()`，然后把期望返回值改成 0 个，见 `src/compiler/codegen_stmt.cpp:338`。这就是字节码里 `CALL A=1 B=2 C=1` 的原因：Lua 5.1 指令格式中 `C=1` 表示调用语句丢弃返回值。

`emitCallExpr()` 的几个关键动作在 `src/compiler/codegen_expr.cpp:749` 开始：

1. `print` 是全局名，`materializeValue()` 会生成 `GETGLOBAL`，见 `src/compiler/codegen_expr.cpp:333`。
2. 调用需要函数和参数位于连续寄存器。因为 `print` 先落在 `R0`，而调用基址选择为 `R1`，所以生成 `MOVE R1 R0`，对应 `src/compiler/codegen_expr.cpp:797`。
3. `"hello"` 是字符串常量，`StringExpr` 的访问器在 `src/compiler/codegen_expr.cpp:209`，最终用 `LOADK` 放入参数寄存器，见 `src/compiler/codegen_expr.cpp:323`。
4. `CALL` 由 `codeABC(OpCode::CALL, base, bArg, 2)` 生成，随后语句层把 C 改为 1，见 `src/compiler/codegen_expr.cpp:868`。
5. chunk 末尾补 `RETURN A=0 B=1`，见 `src/compiler/codegen.cpp:50`。

寄存器视角如下：

```text
R0 = _ENV["print"]      -- GETGLOBAL
R1 = R0                 -- MOVE，让调用基址从 R1 开始
R2 = "hello"            -- LOADK
CALL R1, 1 arg, 0 ret   -- C=1 表示丢弃返回值
RETURN                  -- chunk 结束
```

## 4. VM Dispatch：逐条解释字节码

运行时入口最终会调用 `VM::executeProto(RuntimeServices&, LuaState*, Proto*, i32)`。这个函数先构造 `VMContext`，再选择 `RuntimeServices::dispatchStrategy` 或默认 `defaultDispatchStrategy()`，见 `src/vm/vm.cpp:84`。

默认策略是 `SwitchDispatch`。它恢复当前 `CallInfo`、`base`、`pc`，进入主循环，然后每条指令解码出 `op`、`A/B/C/Bx/sBx`，见 `src/vm/vm.cpp:97`。

这个例子经过的调度分支是：

| PC | 指令 | VM 行为 | 相关实现 |
|---|---|---|---|
| 0 | `GETGLOBAL R0 K0` | 从函数环境或全局表读取 `print`，放入 `R0`；当前由 handler 表处理 | `src/vm/vm.cpp:209`, `src/vm/vm_handlers.cpp:107` |
| 1 | `MOVE R1 R0` | 把函数值移动到调用基址；当前由 handler 表处理 | `src/vm/vm.cpp:189`, `src/vm/vm_handlers.cpp:59` |
| 2 | `LOADK R2 K1` | 把常量 `"hello"` 放入 `R2`；当前由 handler 表处理 | `src/vm/vm.cpp:190`, `src/vm/vm_handlers.cpp:77` |
| 3 | `CALL R1 2 1` | 调用 `R1` 中的函数，传入 1 个参数，不保留返回值 | `src/vm/vm.cpp:330` |
| 4 | `RETURN R0 1` | chunk 返回，结束最外层执行 | `src/vm/vm.cpp:429` |

其中 `GETGLOBAL` / `MOVE` / `LOADK` 等 opcode 已经有 `vm_handlers` 命令表入口，`GETGLOBAL` handler 内部通过 `VM::detail::gettable()` 读取全局表；表注册点见 `src/vm/vm_handlers.cpp:471`。`GETTABLE` / `SETTABLE` / `SELF` / `SETLIST` 等表操作、`ADD` / `SUB` / `MUL` / `DIV` / `MOD` / `POW` 算术操作、`UNM` / `NOT` / `LEN` / `CONCAT` 一元与连接操作，`JMP` / `EQ` / `LT` / `LE` / `TEST` / `TESTSET` 跳转与比较操作，`CLOSE` / `FORLOOP` / `FORPREP` / `TFORLOOP` upvalue 关闭与循环操作，以及 `CLOSURE` / `VARARG` 闭包与变参操作也已进入同一张 handler 表；闭包与变参注册点从 `src/vm/vm_handlers.cpp:504` 开始。`CALL` 则仍在 `SwitchDispatch` 的主 switch 中，通过 `VM::detail::precall()` 区分 Lua 函数和 C 函数。如果被调用对象是 C 函数，`precall()` 会直接调用 C++ 函数并完成返回值整理。

## 5. 标准库：`print` 写入 stdout

`print` 在基础库注册表中绑定到 `luaB_print`，注册位置是 `src/lib/baselib.cpp:1356`。基础库打开时会把这些 C 函数闭包安装到全局表，入口是 `openBaseLib()`，见 `src/lib/baselib.cpp:1397`。

`luaB_print()` 的实现从 `src/lib/baselib.cpp:35` 开始。它读取当前 Lua 栈上的参数数量，逐个把值转为字符串，然后：

- 多个参数之间写入 tab。
- 每个参数通过 `std::fputs(..., stdout)` 输出。
- 最后写入换行并 `std::fflush(stdout)`。
- 返回 `0`，表示 `print` 不产生 Lua 返回值。

所以 `CALL A=1 B=2 C=1` 的完整含义是：以 `R1` 中的 `print` 为函数，传入 `R2` 的 `"hello"`，调用后丢弃返回值。最终可见副作用不是栈上的值，而是标准输出中的 `hello\n`。

## 读完后的检查点

你可以用下面三件事确认自己已经串起整条链路：

1. 能解释为什么常量表里既有 `"print"` 又有 `"hello"`。
2. 能解释为什么调用前多出一条 `MOVE R1 R0`。
3. 能解释为什么 `CALL` 的 `C=1` 对应“0 个返回值”。
