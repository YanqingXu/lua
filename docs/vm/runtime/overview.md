---
status: current
verified_against: src/vm/vm.cpp; src/vm/vm.hpp; src/vm/vm_entry.cpp; src/vm/vm_call.cpp; src/vm/vm_frame.cpp; src/vm/vm_loop.cpp; src/vm/state/lua_state.hpp; src/vm/state/lua_state.cpp; src/vm/state/call_info.hpp; src/vm/state/stack.hpp; src/vm/vm_handlers/; tests/unit/vm/; tests/unit/compiler/test_call_pipeline.cpp; tests/lua/functions/; src/compiler/opcode.hpp; src/vm/; tests/unit/vm/opcode_coverage_matrix.md
last_checked: 2026-07-11
applies_to: VM 主循环、寄存器窗口与调用协议
---

# VM 主循环、寄存器窗口与调用协议

VM 将 `Proto` 的寄存器字节码解释为对 `LuaState`、`CallInfo` 和运行时对象的状态变化。本页合并分发、栈布局、调用帧、Lua/native call、返回值和主循环，避免这些共享同一 ABI 的机制分散描述。

## 1. 核心状态

```text
LuaState
├── Stack              # 所有活动帧共享的 Value 存储
├── Vec<CallInfo>      # 调用帧数组
├── currentCI          # 当前帧索引
└── openUpvalues       # 指向活动栈槽的 upvalue 链

CallInfo
├── func               # 函数值的绝对栈索引
├── base               # R(0) 的绝对栈索引
├── top                # 当前帧可用窗口上界
├── savedpc            # 下一条指令地址；native frame 通常为空
├── nresults           # 调用者期望结果数，-1 为 multret
└── tailcalls          # 被折叠的尾调用数量
```

字节码的 `R(A)` 总是映射到 `stack[ci.base + A]`。指令不能保存 `Value*` 跨越可能的栈扩容；使用绝对索引或在扩容后重新取得引用，能避免 `std::vector` 重分配导致悬空。

## 2. 进入主循环

入口验证 Lua function 与 Proto，初始化根 frame，然后进入 `executeProtoImpl()`。每次循环：

1. 从当前 frame/Proto 重新取得 code；
2. 用 PC 取指并解码 opcode；
3. 构造包含 state、function、proto、pc 和 operand 的 handler context；
4. 调用 opcode handler；
5. handler 返回下一 PC 或通过 call/return 切换 frame；
6. 同步 `savedpc`，供错误位置、hook 和 traceback 使用。

dispatch 与 handler 必须对“谁推进 PC”有唯一约定。普通指令推进一条，比较/测试可能跳过下一条，JMP/loop 使用 `sBx`，CALL/RETURN 还可能替换当前 code 与 frame。

## 3. 寄存器与栈窗口

```text
低地址
[ caller values ... ]
[ function ][ arg0 ][ arg1 ][ local/temp registers ... ]
             ^ base                         ^ frame top
高地址
```

`Proto::maxStackSize` 决定 Lua frame 的最小窗口。LuaState 的物理 stack 可以扩容，但 frame 的 `func/base/top` 使用索引，因此扩容后仍有效。

几个不同的 top 不能混淆：

- LuaState absolute top：整个共享栈当前使用边界；
- `CallInfo::top`：帧可写窗口上界；
- opcode 的开放结果 top：CALL/VARARG/RETURN 在数量未知时使用的动态边界。

固定结果会按调用者期望截断或补 nil；开放结果让实际数量决定 top。

## 4. CALL

CALL 的寄存器布局：

```text
R(A)     callable
R(A+1)   arg 1
...
R(A+B-1) last fixed arg        # B == 0 时参数开放到当前 top
```

VM 先解析 callable。若不是函数，尝试 `__call` 并把原对象插入参数序列；仍不可调用则抛 `RuntimeError`。

### Lua function

创建新 `CallInfo`，`func` 指向 callable，`base` 指向第一个参数/寄存器槽，并按 `maxStackSize` 保证容量。缺失形参补 nil，多余参数在 vararg function 中形成可访问区。随后主循环切换到 callee Proto。

### Native function

native call 使用同一 LuaState 栈协议，但没有字节码 PC。C++ 函数返回一个整数表示压入的结果数量；VM 将这些值移动到 caller 指定结果区，再恢复 caller frame。native 函数不得保留可能因后续 push 扩容而失效的栈元素引用。

## 5. RETURN 与多返回值

RETURN 的 B 字段表达数量模式：固定结果从 `R(A)` 起连续取得，开放结果使用当前动态 top。返回处理需要：

1. 关闭 callee frame 的 open upvalue；
2. 计算实际返回区间；
3. 弹出 callee frame；
4. 按 caller 的 `nresults` 移动、截断或补 nil；
5. 恢复 caller 的 base/top/code/PC。

Lua 表达式列表只有最后一个调用或 vararg 能保持开放数量；前面的多结果表达式都收敛为单值。这一规则必须由 CodeGen 与 VM 共同实现。

## 6. TAILCALL

尾调用把当前函数的结果直接交给其调用者。实现可以复用当前 `CallInfo`，但必须先完成普通 return 同等的 upvalue 关闭和参数重排。`tailcalls` 记录被折叠的逻辑层数，使 debug library 能显示 tail-call 占位。

不能只把 `CALL + RETURN` 替换成跳转：callee 可能是 native function、可能触发 `__call`，也可能需要 vararg 与开放结果处理。

## 7. Closure、VARARG 与循环辅助

- `CLOSURE` 从子 Proto 创建 Function，并消费后续 MOVE/GETUPVAL 伪指令建立捕获关系。
- `CLOSE` 将指定寄存器及以上的 open upvalue 从栈槽迁移到自身存储。
- `VARARG` 按固定或开放数量把 frame 的额外实参复制到寄存器。
- `FORPREP/FORLOOP` 共享数值 for 的连续控制寄存器 ABI。
- `TFORLOOP` 通过常规 call pipeline 调用 iterator，再根据首结果决定回边。

这些路径会改变 frame、top 或 PC，因此集中在 `vm_frame.cpp`、`vm_loop.cpp`、call helper 和专用 handler，而不是塞入单个巨型 switch。

## 8. 错误、trace 与现代 C++ 边界

handler 内部使用 `RuntimeError` 做非局部退出；`tryExecuteProto()` 在 VM API 边界转换为 `std::expected<ExecResult, RuntimeError>`。保护调用再把 C++ 控制流映射回 Lua 错误对象。

trace sink 观察结构化事件，不拥有 LuaState、Function 或 Value 图。索引式 frame、`std::span` 式只读窗口和 RAII 管理的临时状态适合表达借用；GC 对象仍由可达性而非 `shared_ptr` 引用计数管理。

## 9. 验证不变量

- PC 总位于当前 Proto code 范围，`savedpc` 指向约定的下一条指令。
- `func <= base <= top <= stack capacity`，所有 `R(A)` 落在 frame 窗口。
- opcode coverage matrix 中每个指令同时有生产方和 handler。
- fixed/multret 在 CALL、RETURN、VARARG、native call 和 tailcall 上一致。
- frame 弹出前关闭 open upvalue，异常展开也遵守相同规则。
- Lua/native/metamethod call 最终共用一种结果归一化协议。

指令语义见 [VM 指令集](../instruction-set.md)，动态证据见 [Trace 系统](../trace-system.md)，closure 生命周期见 [Closure 与 Upvalue 全链路](../../runtime/functions/closure-upvalue-walkthrough.md)。
