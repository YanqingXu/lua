---
status: current
verified_against: src/compiler/parser/parser_stmt.cpp; src/compiler/codegen/statement_emitter.cpp; src/compiler/codegen/jump_patcher.hpp; src/compiler/codegen/jump_patcher.cpp; src/compiler/codegen/scope_manager.hpp; src/vm/vm_handlers/vm_handlers_branch.cpp; src/vm/vm_handlers/vm_handlers_loop.cpp; tests/unit/compiler/test_codegen_conditions.cpp; tests/unit/compiler/test_jump_patcher.cpp; tests/lua/control_flow/; src/compiler/; tests/unit/compiler/; tests/lua/bytecode/
last_checked: 2026-07-11
applies_to: 控制流从 AST 到字节码的 lowering
---

# 控制流从 AST 到字节码的 lowering

Lua 控制流的核心不是语句种类，而是三种生成机制：条件值转跳转、未决边到目标 PC 的回填，以及退出作用域时的 upvalue 关闭。本页统一说明 `if`、循环、`break` 和 `return`，避免每种语句各自复制同一套跳转规则。

## 1. 通用模型

```text
AST statement
  → StatementEmitter
  → expression as CondResult / ValueResult
  → emit branch or loop opcode
  → record unresolved jump
  → finish target block
  → JumpPatcher resolves sBx
```

条件上下文优先生成 `CondResult`，其中保存 true/false 跳转链；需要物化为值时才写入布尔寄存器。这避免 `if a < b` 先产生临时 boolean 再测试。

所有 `sBx` 必须满足：

```text
target = instruction_after_jump + sBx
```

生成器记录的是指令索引，不保存指向 `Vec<Instruction>` 元素的指针，因此容器扩容不会让 patch 句柄悬空。

## 2. 条件分支

### `if / elseif / else`

每个条件的 false 链跳到下一个分支；执行完某个 then block 后，无条件跳到整个结构末尾：

```text
cond1 false ───────┐
then1              ▼
  JMP end        cond2 false ──→ else
                  then2
                    JMP end
end
```

短路 `and/or` 复用同样的链结构。生成器只在最终需要普通 Lua 值时合并并物化结果。

## 3. 循环

| 语句 | 条件位置 | 回边 | 退出目标 |
|---|---|---|---|
| `while` | 循环体之前 | body 末尾 → condition | condition false → end |
| `repeat until` | 循环体之后 | condition false → body start | condition true → end |
| numeric `for` | `FORPREP` 初始化 | `FORLOOP` 自增并回跳 | 比较失败后的下一条 |
| generic `for` | iterator call 前后 | `TFORLOOP` 有结果时回跳 | 首结果为 nil 时退出 |

### `repeat until` 的作用域

until 条件仍能访问 body 中声明的局部变量，因此 body scope 必须延续到条件生成之后。若条件为 false 并回跳，open upvalue 也不能提前关闭。

### Numeric for

数值循环使用连续寄存器窗口保存 index、limit、step 和用户变量。`FORPREP` 完成初始校正，`FORLOOP` 负责自增、边界比较、用户变量同步和回边。这个布局是 Compiler 与 VM 的共享 ABI，修改任何一侧必须同步 opcode 测试。

### Generic for

泛型循环保存 iterator、state、control 和用户变量。迭代调用可能是 Lua 或 native function，因此结果数量必须按 opcode 的 C 字段规范化；第一个结果更新 control，并决定是否继续。

## 4. `break`

`ScopeManager` 维护循环上下文栈。`break` 发射未决 `JMP` 并登记到最近一层循环；循环完成后统一 patch 到退出目标。

离开包含 captured local 的作用域时，跳转前必须发射或等价执行 `CLOSE`。因此 `break` 不是单纯修改 PC，它同时承担词法作用域清理。

循环外的 `break` 在 CodeGen 边界成为 `CodegenError`。Parser 只确认语法形状，生成器才拥有“当前是否在循环中”的结构化上下文。

## 5. `return` 与尾调用

return 表达式列表遵守 Lua 的末项展开规则：只有最后一个函数调用或 vararg 可以产生开放数量结果。

| 形式 | 生成策略 |
|---|---|
| `return` | `RETURN A, 1`，返回零个值 |
| `return x` | 固定单结果 |
| `return a, b` | 连续寄存器中的固定结果 |
| `return f()` | 调用结果开放并直接返回；满足条件时形成尾调用 |
| `return a, f()` | 前缀固定、末项开放 |

return 离开所有活动 scope，必须关闭相关 open upvalue。尾调用优化只能复用 frame，不能跳过该语义清理。

## 6. 关键不变量与测试

- 所有未决跳转在 Proto 完成前都已 patch，且目标位于 code 边界内。
- `break` 只绑定最近循环，嵌套循环的 patch 列表互不污染。
- repeat body 的局部变量在 until 条件生成时仍可绑定。
- for 控制寄存器窗口连续且不被临时表达式覆盖。
- 固定/开放返回模式在 CodeGen、CALL/RETURN handler 和 LuaState top 之间一致。
- 穿越 scope 边界的跳转不会留下指向失效栈槽的 open upvalue。

内部契约由 `test_codegen_conditions.cpp`、`test_jump_patcher.cpp` 与 statement emitter 测试覆盖；语言行为由 `tests/lua/control_flow/` 和 regressions 覆盖。
