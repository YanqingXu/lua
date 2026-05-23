---
status: current
verified_against: bin/lua_bytecode.exe; bin/lua_app.exe; src/compiler/parser/parser_func.cpp; src/compiler/parser/parser_stmt.cpp; src/compiler/codegen/codegen_binding.cpp; src/compiler/codegen/expression_emitter.cpp; src/compiler/codegen/statement_emitter.cpp; src/compiler/codegen/codegen_stmt.cpp; src/compiler/codegen/scope_manager.cpp; src/vm/vm_frame.cpp; src/vm/vm_handlers/vm_handlers_closure.cpp; src/vm/vm_handlers/vm_handlers_global_upvalue.cpp; src/vm/vm_handlers/vm_handlers_call.cpp; src/vm/state/lua_state.cpp; src/core/upvalue.cpp
last_checked: 2026-05-23
applies_to: closure creation, upvalue capture, and open-to-closed upvalue lifetime
---

# Closure And Upvalue End-to-End

这篇 walkthrough 追踪一个计数器闭包：

```lua
local function makeCounter()
    local count = 0
    return function(step)
        count = count + step
        return count
    end
end

local c = makeCounter()
print(c(2), c(3))
```

它的关键行为是：`makeCounter()` 返回内部函数后，外层局部变量 `count` 仍然活着；两次调用 `c` 共享同一个 upvalue，所以输出是 `2` 和 `5`。

## 可复现命令

```powershell
$tmp = Join-Path $env:TEMP 'closure_upvalue_walkthrough.lua'
@'
local function makeCounter()
    local count = 0
    return function(step)
        count = count + step
        return count
    end
end

local c = makeCounter()
print(c(2), c(3))
'@ | Set-Content -LiteralPath $tmp -NoNewline -Encoding UTF8
.\bin\lua_bytecode.exe $tmp full
.\bin\lua_app.exe $tmp
```

`lua_app` 输出：

```text
2	5
```

PR-55 后，`lua_bytecode full` 会先打印顶层 Proto，再递归打印 child protos。下面摘录了关键行：外层函数、返回的内部闭包，以及内部闭包捕获的 `count` upvalue 都可以直接从工具输出里看到。

```text
Proto
  source: <temporary file>
  linedefined: 0
  lastlinedefined: 0
  numparams: 0
  is_vararg: true (flags=2)
  maxStackSize: 7
  upvalues (0): (none)
instructions (14)
0000 | line 1 | CLOSURE | A=0 Bx=0 ; proto[0] = <temporary file>:1
0001 | line 9 | MOVE | A=1 B=0 C=0
0002 | line 9 | CALL | A=1 B=1 C=2
0003 | line 10 | GETGLOBAL | A=2 Bx=0 ; K[0] = string "print"
0004 | line 10 | MOVE | A=3 B=2 C=0
0005 | line 10 | MOVE | A=4 B=1 C=0
0006 | line 10 | LOADK | A=5 Bx=1 ; K[1] = number 2
0007 | line 10 | CALL | A=4 B=2 C=2
0008 | line 10 | MOVE | A=5 B=1 C=0
0009 | line 10 | LOADK | A=6 Bx=2 ; K[2] = number 3
0010 | line 10 | CALL | A=5 B=2 C=0
0011 | line 10 | CALL | A=3 B=0 C=1
0012 | line 0 | CLOSE | A=0 B=0 C=0
0013 | line 0 | RETURN | A=0 B=1 C=0
constants (3)
  K[0] = string "print"
  K[1] = number 2
  K[2] = number 3
child protos (1)
  proto[0] <temporary file>:1
    Proto
      source: <temporary file>
      linedefined: 1
      ...
      numparams: 0
      upvalues (0): (none)
    instructions (5)
    0001 | line 3 | CLOSURE | A=1 Bx=0 ; proto[0] = <temporary file>:3
    0002 | line 3 | MOVE | A=0 B=0 C=0
    0003 | line 3 | RETURN | A=1 B=2 C=0
    child protos (1)
      proto[0] <temporary file>:3
        Proto
          source: <temporary file>
          ...
          numparams: 1
          upvalues (1): count
        instructions (6)
        0000 | line 4 | GETUPVAL | A=1 B=0 C=0
        0001 | line 4 | ADD | A=1 B=1 C=0
        0002 | line 4 | SETUPVAL | A=1 B=0 C=0
        0003 | line 5 | GETUPVAL | A=1 B=0 C=0
        0004 | line 5 | RETURN | A=1 B=2 C=0
```

读这段输出时先抓住三件事：

1. 顶层 `CLOSURE R0 proto[0]` 创建 `makeCounter`。
2. `CALL R1` 调用 `makeCounter`，返回的内部闭包进入局部变量 `c`。
3. `c(2)` 和 `c(3)` 是对同一个 closure 的两次调用；最后一个参数调用用 `C=0` 保持开放多返回值，再交给 `print`。

## 1. Parser：函数语法变成 AST

`local function makeCounter() ... end` 在 `Parser::parseLocalStmt()` 里走 local-function 分支，见 `src/compiler/parser/parser_stmt.cpp:235`。内部匿名函数 `function(step) ... end` 是表达式，入口是 `Parser::parseFunctionExpr()`，见 `src/compiler/parser/parser_func.cpp:83`。

简化后的 AST 形状是：

```text
Chunk
  FunctionStmt(isLocal=true, name="makeCounter")
    body:
      LocalStmt(name="count", value=0)
      ReturnStmt
        FunctionExpr(params=["step"])
          AssignStmt
            lhs: NameExpr("count")
            rhs: BinaryExpr(NameExpr("count") + NameExpr("step"))
          ReturnStmt
            NameExpr("count")
  LocalStmt(name="c", value=CallExpr(NameExpr("makeCounter")))
  CallStmt
    CallExpr(NameExpr("print"), [CallExpr(NameExpr("c"), 2), CallExpr(NameExpr("c"), 3)])
```

AST 本身还不决定 `count` 是 local 还是 upvalue。这个判定发生在 CodeGen 的名字绑定阶段。

## 2. CodeGen：名字绑定到 upvalue

`CodeGenerator::resolve()` 的查找顺序是 local -> upvalue -> global，见 `src/compiler/codegen/codegen_binding.cpp:26`。当内部函数体里遇到 `count` 时，它不是内部函数的参数或局部变量，于是 `resolveUpvalue()` 会问父 `CodeGenerator` 是否有同名 local。

这条路径在 `ScopeManager::resolveUpvalue()` 中很清楚，见 `src/compiler/codegen/scope_manager.cpp:71`：

```text
inner function asks for "count"
  parent has local "count" at register R0
  -> record UpvalueCapture{name="count", inStack=true, index=0}
```

如果捕获的是祖父函数的 upvalue，而不是直接父函数的 local，同一个函数会走 `parent->resolveUpvalue(name)`，并记录 `inStack=false`。这正好对应 Lua 5.1 的两类 closure pseudo instruction：

| 捕获来源 | `UpvalueCapture` | 发出的伪指令 |
|---|---|---|
| 父函数栈上的 local | `inStack=true, index=<local reg>` | `MOVE 0 <local reg>` |
| 父函数已有 upvalue | `inStack=false, index=<upvalue slot>` | `GETUPVAL 0 <slot>` |

这些伪指令由 `CodeGenerator::emitClosureUpvalues()` 发出，见 `src/compiler/codegen/codegen_stmt.cpp:125`。它们紧跟在 `CLOSURE` 后面，不是普通运行指令，而是给 VM 创建 closure 时消费的捕获描述。

## 3. CodeGen：读写 upvalue

内部函数里有两种对 `count` 的访问：

```lua
count = count + step
return count
```

读 `count` 时，`SymbolRef::Kind::Upvalue` 会变成 `ValueResult::AccessKind::Upvalue`，见 `src/compiler/codegen/codegen_binding.cpp:57`。真正需要把值放进寄存器时，`ExpressionEmitter::materializeValue()` 发出 `GETUPVAL`，见 `src/compiler/codegen/expression_emitter.cpp:381`。

写 `count` 时，赋值左侧会变成 `LValueRef::Kind::Upvalue`，见 `src/compiler/codegen/codegen_binding.cpp:81`。`ExpressionEmitter::storeVar()` 最终发出 `SETUPVAL`，见 `src/compiler/codegen/expression_emitter.cpp:1052`。

所以内部闭包的核心形状可以理解为：

```text
-- 参数 step 在 R0，捕获的 count 是 upvalue[0]
GETUPVAL temp, 0
ADD      temp, temp, R0
SETUPVAL temp, 0
GETUPVAL result, 0
RETURN   result, 2
```

这段核心形状现在可以直接在 `lua_bytecode full` 的递归 child proto 输出中看到：内部函数的 `upvalues (1): count` 对应捕获，`GETUPVAL` / `SETUPVAL` 对应读写捕获值。

## 4. CodeGen：函数变成子 Proto

函数编译由 `CodeGenerator::compileFunction()` 负责，见 `src/compiler/codegen/codegen_stmt.cpp:78`。它会创建一个子 `CodeGenerator`，把 `child.state_.parent` 指向当前函数，然后：

1. 为子函数创建新的 `Proto`。
2. 把形参注册为子函数 local。
3. 编译子函数 body。
4. 收集 `child.scopes_.upvalues()`，写入子 Proto 的 upvalue 名称和数量。
5. 把捕获描述返回给外层，让外层在 `CLOSURE` 后发出伪指令。

在这个例子里有两个函数边界：

| 函数 | 捕获 |
|---|---|
| `makeCounter` | 不捕获外层变量 |
| `function(step)` | 捕获 `makeCounter` 的 local `count` |

顶层字节码里只直接看到第一个边界：

```text
0000 | line 1 | CLOSURE | A=0 Bx=0
```

第二个边界存在于 `makeCounter` 的子 Proto 中：它会创建内部 closure，并在 `CLOSURE` 后附带 `MOVE 0 0` 这样的捕获伪指令，表示“从父函数 R0 捕获 `count`”。

## 5. VM：`CLOSURE` 消费伪指令

VM 的 `CLOSURE` handler 在 `src/vm/vm_handlers/vm_handlers_closure.cpp:12`，它委托给 `VM::detail::closure()`。真正的捕获逻辑在 `src/vm/vm_frame.cpp:45`：

```text
for each child upvalue:
  read next pseudo instruction
  if MOVE:
      closure->addUpvalue(L->findOrCreateUpvalue(parent base + B))
  if GETUPVAL:
      closure->addUpvalue(parent closure upvalue[B])
```

这就是为什么 CodeGen 发出的 `MOVE 0 0` 不是普通赋值。它告诉 VM：新 closure 的第 0 个 upvalue 要指向父函数当前栈帧里的 `R0`，也就是 `count`。

`LuaState::findOrCreateUpvalue()` 会复用同一栈槽已经存在的 open upvalue，见 `src/vm/state/lua_state.cpp:156`。这保证了多个闭包捕获同一个 local 时，它们共享同一个 `Upvalue` 对象，而不是各自复制一份值。

## 6. Open Upvalue：闭包仍指向栈

`Upvalue::createOpen()` 创建的是 open upvalue，见 `src/core/upvalue.cpp:19`。open 状态下，`Upvalue::getValue()` 和 `setValue()` 通过 `stackIndex_` 访问所属栈上的值，见 `src/core/upvalue.cpp:63` 和 `src/core/upvalue.cpp:83`。

在 `makeCounter` 正在执行时，`count` 还在 `makeCounter` 的调用帧里：

```text
makeCounter frame
  R0 = count

returned inner closure
  upvalue[0] -> R0  (open)
```

此时读写 `count` 不复制值，而是通过 upvalue 访问那个栈槽。对 `count` 的更新必须能被下一次调用看见，这正是共享 `Upvalue` 对象的意义。

## 7. Closed Upvalue：函数返回后保存值

`makeCounter` 返回内部 closure 时，父函数栈帧即将消失。VM 必须在栈槽失效前把 open upvalue 关闭。

返回指令 handler 在 `src/vm/vm_handlers/vm_handlers_call.cpp:101`，其中 `state->closeUpvalues(ci.base)` 位于 `src/vm/vm_handlers/vm_handlers_call.cpp:114`。`LuaState::closeUpvalues()` 会关闭所有栈索引大于等于该层级的 open upvalue，见 `src/vm/state/lua_state.cpp:191`。

关闭动作在 `Upvalue::close()` 中完成，见 `src/core/upvalue.cpp:95`：

```text
closedValue_ = stack[stackIndex_]
isOpen_ = false
```

关闭后结构变成：

```text
makeCounter frame gone

c closure
  upvalue[0] -> closedValue_ = 0
```

随后 `c(2)` 调用内部函数，`GETUPVAL` 读到 `0`，`SETUPVAL` 写回 `2`。第二次 `c(3)` 读到同一个 closed upvalue 中的 `2`，写回 `5`。`GETUPVAL` 和 `SETUPVAL` 的 VM handler 分别在 `src/vm/vm_handlers/vm_handlers_global_upvalue.cpp:53` 和 `src/vm/vm_handlers/vm_handlers_global_upvalue.cpp:68`。

## 8. 顶层 `CLOSE` 为什么出现

顶层字节码末尾有：

```text
0012 | line 0 | CLOSE | A=0 B=0 C=0
0013 | line 0 | RETURN | A=0 B=1 C=0
```

这个 `CLOSE` 来自作用域离开时的保守收口：`ScopeManager::removeLocalVars()` 会先调用 `closeScopeUpvalues()`，见 `src/compiler/codegen/scope_manager.cpp:35`；如果当前作用域还有 active locals，它会发出 `CLOSE`，见 `src/compiler/codegen/scope_manager.cpp:43`。运行时 `CLOSE` handler 会调用 `LuaState::closeUpvalues(ci.base + A)`，见 `src/vm/vm_handlers/vm_handlers_loop.cpp:13`。

在这个具体脚本里，真正必须关闭的是 `makeCounter` 返回时的 `count`；顶层 `CLOSE A=0` 是安全的收尾指令，用同一机制处理可能存在的 open upvalue。

## 读完后的检查点

1. 能解释为什么内部函数里的 `count` 不是 global，而是 upvalue。
2. 能解释 `CLOSURE` 后面的 `MOVE` / `GETUPVAL` 为什么是捕获描述。
3. 能解释 open upvalue 为什么需要在函数返回时变成 closed upvalue。
4. 能解释为什么两次调用 `c` 后结果是 `2` 和 `5`，而不是 `2` 和 `3`。
