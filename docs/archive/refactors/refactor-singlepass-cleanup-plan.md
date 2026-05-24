---
status: historical
verified_against: docs/archive/refactors/refactor-expdesc-pr-checklist.md; src/compiler/codegen.cpp; src/compiler/codegen.hpp
last_checked: 2026-05-18
applies_to: completed single-pass cleanup plan
---

# 单遍编译策略残留清理 — 重构计划

> 本文档基于 [refactor-expdesc-plan.md](./refactor-expdesc-plan.md) 和 [refactor-expdesc-pr-checklist.md](./refactor-expdesc-pr-checklist.md) 的基础之上，
> 针对 PR-0 ~ PR-9 完成后仍残留在代码库中的单遍编译策略模式，制定系统性的分阶段清理方案。

---

## 一、背景

### 1.1 已完成的架构迁移

通过 PR-0 到 PR-9 的渐进式重构，本项目已完成以下核心转变：

| 维度 | 旧架构（Lua 5.1 风格） | 新架构（当前状态） |
|------|----------------------|-------------------|
| 编译策略 | 单遍：parse 的同时生成字节码 | **两遍**：parse → AST → codegen |
| 表达式描述 | `ExprDesc` / `ExprKind`（15 种状态） | `ValueResult` / `CondResult` / `LValueRef` / `CallResultInfo` |
| 代码生成入口 | 单一 `expr()` 函数 | 三条显式通道：`emitValue` / `emitCond` / `emitLValue` |
| 符号解析 | 分散在 4 处各自的 if/else 链 | 统一的 `resolve()` → `SymbolRef` |
| 上下文管理 | 8 个散落在 CodeGenerator 顶层的原始成员 | 4 个独立子系统结构（RegisterAllocator / LocalVarScope / BlockManager / UpvalueContext） |

### 1.2 为什么还需要进一步清理

虽然显式标记（`ExprDesc`、`expr()`、`forcedCallBase_`、适配函数）已全部删除，但深入到代码生成器的**执行逻辑**层面，仍有相当数量的单遍编译行为模式被保留了下来。这些模式不是"错了"，而是：

- **命名和注释**延续了 Lua 5.1 C 源码的习惯（如 `luaK_` 前缀）
- **寄存器管理**延续了直接操作裸 `int` 的方式（40+ 处 `regs_.freereg_ =`）
- **值物化机制**延续了"延迟描述 → 最后物化"的 `discharge` 模式

这份计划的目标是：**在不改变字节码语义的前提下，将这些遗留模式替换为与当前两遍架构一致的表达方式。**

---

## 二、残留清单

### 2.1 A 类：命名与注释残留（低风险，纯文本替换）

#### A-1：`luaK_concat` 函数名

- **位置**：[codegen.hpp:257](src/compiler/codegen.hpp#L257) / [codegen.cpp:1634](src/compiler/codegen.cpp#L1634)
- **现状**：函数名为 Lua 5.1 原始代码的 `luaK_concat`，"K" 代表 "Kernel"
- **问题**：项目中其他函数已不再使用 `luaK_` 前缀（如 `codearith`、`codecomp`、`codenot` 已在 PR-6 中删除）
- **建议**：重命名为 `concatJumpList`

#### A-2：`luaK_getlabel` 函数名

- **位置**：[codegen.hpp:262](src/compiler/codegen.hpp#L262) / [codegen.cpp:1680](src/compiler/codegen.cpp#L1680)
- **现状**：将 `pc_` 同步为当前指令数。函数体只有一行：`pc_ = static_cast<i32>(proto_->getInstructionCount());`
- **问题**：命名来自 Lua 5.1，语义不清晰（不是真的"获取标签"，而是同步 PC）
- **建议**：重命名为 `syncPC`，明确其语义

#### A-3：`dischargejpc` 函数名

- **位置**：[codegen.hpp:261](src/compiler/codegen.hpp#L261) / [codegen.cpp:215-221](src/compiler/codegen.cpp#L215)
- **现状**：在每条指令生成前将所有待处理跳转修补到当前位置
- **问题**：`discharge` 命名来自 Lua 5.1 的 `dischargejpc`，对现代 C++ 代码库不直观
- **建议**：重命名为 `flushPendingJumps`

#### A-4：`dischargeValue` 函数名

- **位置**：[codegen.hpp:172](src/compiler/codegen.hpp#L172) / [codegen.cpp:485-564](src/compiler/codegen.cpp#L485)
- **现状**：将 `ValueResult` 物化到指定寄存器（根据 Kind 发射对应指令）
- **问题**：`discharge` 是 Lua 5.1 术语，意为"将表达式从悬挂状态落盘到寄存器"
- **建议**：重命名为 `materializeValue`（更符合编译器领域的通用术语）

#### A-5：注释中的 "单遍代码生成"

- **位置**：[codegen.hpp:18](src/compiler/codegen.hpp#L18)
- **现状**：`* - 单遍代码生成`
- **问题**：架构已变为两遍（parse → AST → codegen），该描述具有误导性
- **建议**：改为 `* - 基于AST的字节码生成`

#### A-6：注释中引用已删除函数

- **位置**：[codegen.hpp:137-139](src/compiler/codegen.hpp#L137) — 引用 `luaK_goiftrue/luaK_goiffalse`
- **位置**：[codegen.hpp:190](src/compiler/codegen.hpp#L190) — 引用 `exp2Val`
- **位置**：[codegen.cpp:1984](src/compiler/codegen.cpp#L1984) — 引用 `exp2NextReg`
- **位置**：[codegen.cpp:1648](src/compiler/codegen.cpp#L1648) — 引用已删除的 `luaK_goiftrue/luaK_goiffalse/codenot`
- **问题**：这些函数已被删除，注释成为误导信息
- **建议**：更新为新函数名或删除过时信息

---

### 2.2 B 类：注释债务 — "P0修复" 标记（低风险，信息归档）

- **位置**：[codegen.cpp](src/compiler/codegen.cpp) 共 13 处

| 行号 | 引用来源 | 说明 |
|------|---------|------|
| 84 | `lcode.c:2886-2898 luaK_code` | `codeABC` 函数实现参考 |
| 95 | — | `codeABx` 前的 jpc 修补说明 |
| 105 | — | `codeAsBx` 前的 jpc 修补说明 |
| 166 | — | `addLocalVar` 后 freereg 递增说明 |
| 167 | — | `addLocalVar` 后 checkStack 说明 |
| 193 | `lcode.c:212-219 luaK_jump` | `jump()` 函数实现参考 |
| 203 | — | `patchList` 中 getjump 使用说明 |
| 204 | — | `patchList` 中 fixjump 使用说明 |
| 216 | `lcode.c:608-611 dischargejpc` | `dischargejpc` 函数实现参考 |
| 1335 | — | `emitCallExpr` 前 freereg 调整说明 |
| 1349 | — | `emitCallExpr` 后 freereg 恢复说明 |
| 1416 | — | `emitStmt(AssignStmt)` 后 freereg 恢复说明 |
| 1573 | `lparser.c:4712-4725 breakstat` | `emitStmt(BreakStmt)` 实现参考 |
| 1651 | `lcode.c:477-486 patchtestreg` | `condjump` 中 TESTSET→TEST 转换说明 |

- **问题**：这些是开发期用于追踪原始 Lua 5.1 代码对应关系的标记，对当前代码库的维护者已经没有参考价值。其中引用的是项目外部的分析仓库。
- **建议**：全部移除，将其中仍然有用的信息（如 TESTSET→TEST 转换的语义原因）改写为独立的、自解释的注释。

#### B-2：其他外部分析仓库引用

- **位置**：[codegen.cpp](src/compiler/codegen.cpp) 共 5 处（非 P0 标记）

| 行号 | 内容 |
|------|------|
| 1325 | `// 参考：<external-analysis>/src/lparser.c localstat() 函数` |
| 1489 | `// ifstat: <external-analysis>/src/lparser.c:5522-5542` |
| 1529 | `// 参考 <external-analysis>/src/lparser.c:4808-4823 whilestat实现` |
| 1591 | `// 参考 <external-analysis>/src/lparser.c:4853-4875 repeatstat实现` |
| 2031 | `// 参考 <external-analysis>/src/lcode.c中的forbody()和forlist()` |

- **建议**：与 B-1 一并处理。

---

### 2.3 C 类：寄存器裸操作封装（中风险，行为无变更）

- **位置**：[codegen.cpp](src/compiler/codegen.cpp) 共 **28 处** `regs_.freereg_ =` 直接赋值

完整清单：

```
行  58:  regs_.freereg_ = 0;                    // generate() 初始化
行 177:  regs_.freereg_ = locals_.nactvar_;      // addLocalVar 后
行 184:  regs_.freereg_ = locals_.nactvar_;      // removeLocalVars 后
行 844:  regs_.freereg_ = savedFreereg;          // emitLValue IndexExpr 恢复
行 876:  regs_.freereg_ = tableReg + 1;          // emitLValue IndexExpr 表求值后
行 902:  regs_.freereg_ = tableReg + 1;          // emitLValue MemberExpr 表求值后
行 908:  regs_.freereg_ = tableReg + 1;          // emitLValue MemberExpr 表求值后
行 999:  regs_.freereg_ = firstArgReg;           // emitCallExpr 初始化
行1043:  regs_.freereg_ = targetReg + 1;         // emitCallExpr 扩展特殊参数后
行1055:  regs_.freereg_ = ...                    // emitCallExpr 收尾
行1340:  regs_.freereg_ = base;                  // emitCallExpr 保存/恢复逻辑
行1351:  regs_.freereg_ = base;                  // emitCallExpr 保存/恢复逻辑
行1417:  regs_.freereg_ = savedFreereg;          // emitStmt(AssignStmt) 恢复
行1430:  regs_.freereg_ = base;                  // emitStmt(AssignStmt) 多赋值
行1441:  regs_.freereg_ = base + (nret - 1);     // emitStmt(AssignStmt) 多返回
行1462:  regs_.freereg_ = savedFreereg;          // emitStmt(LocalStmt) 恢复
行1473:  regs_.freereg_ = savedFreereg;          // emitStmt(LocalStmt) 恢复
行1484:  regs_.freereg_ = savedFreereg;          // emitStmt(ReturnStmt) 恢复
行1569:  regs_.freereg_ = locals_.nactvar_;      // emitStmt(BreakStmt)
行1810:  child.regs_.freereg_ = 0;               // compileFunction 子生成器初始化
行1943:  regs_.freereg_ = savedFreereg;          // FunctionStmt 表路径恢复
行1988:  regs_.freereg_ = base;                  // emitStmt(ForNumStmt)
行1996:  regs_.freereg_ = base + 4;              // emitStmt(ForNumStmt)
行2050:  regs_.freereg_ = base + 3;              // emitStmt(ForInStmt)
行2058:  regs_.freereg_ = base + 3;              // emitStmt(ForInStmt)
行2069:  regs_.freereg_ = base;                  // emitStmt(ForInStmt)
行2086:  regs_.freereg_ = base + 3 + nvars;      // emitStmt(ForInStmt)
```

- **问题**：`RegisterAllocator` 在 PR-7 中已创建，但 `freereg_` 字段设为 public 以兼容旧代码。当前所有寄存器操作都是直接写入该字段，而不是通过封装方法。这导致：
  - 无法在赋值时加入断言或日志
  - 无法追踪寄存器分配的调用链
  - 回退到 Lua 5.1 中 `FuncState.freereg` 裸 int 的操作习惯
- **建议**：为常见的赋值模式添加命名方法，如：
  - `reset(n)` → `freereg_ = n`
  - `resetToLocals()` → `freereg_ = locals_.nactvar_`
  - `restore(saved)` → `freereg_ = saved`
  - `alignAfter(base, offset)` → `freereg_ = base + offset`
  
  然后逐步将直接赋值替换为方法调用。

---

### 2.4 D 类：`ValueResult` 延迟物化模型审视（高风险，设计讨论）

- **位置**：[codegen_types.hpp:51-92](src/compiler/codegen_types.hpp#L51) 中 `ValueResult::Kind` 枚举

当前 `ValueResult` 有以下与单遍策略一脉相承的 Kind：

| Kind | 说明 | 对应旧 ExprDesc 状态 |
|------|------|---------------------|
| `PendingLoad` | 全局/upvalue/索引访问尚未发射 GET 指令 | `VGLOBAL` / `VUPVAL` / `VINDEXED` |
| `Relocatable` | 已发射指令但 A 寄存器待回填 | `VRELOCABLE` |
| `PendingJump` | 比较表达式通过跳转+bool 物化 | `VJMP` |
| `MultiRet` | 函数调用/vararg 的多返回值 | `VCALL` / `VVARARG` |

- **问题**：这四种 Kind 本质上复刻了旧 `ExprDesc` 的"先描述、延迟发射"策略——表达式先生成一个描述符，等到真正需要值的时候再调用 `dischargeValue` 把指令补全。

  在纯两遍架构中，理想情况是：**每个表达式直接发射完整的指令序列**，不经过延迟描述。例如：
  - `NameExpr("a")` 如果 `a` 是局部变量 → 直接返回 `ValueResult{Register, reg=slotA}`
  - `NameExpr("a")` 如果 `a` 是全局 → 立即发射 `GETGLOBAL tmp, "a"` → 返回 `ValueResult{Register, reg=tmp}`

  但当前实现中，`symbolToValue` 对于全局/upvalue 返回的是 `PendingLoad`，GETGLOBAL/GETUPVAL 指令被延迟到 `dischargeValue` 才发射。这是单遍编译"不确定是否需要这个值"的策略残留。

- **建议**：这个改造风险极高，不建议在本轮清理中进行。应当：
  1. 先完成 A/B/C 类清理
  2. 设计实验性 PR 来探索"立即发射"vs"延迟发射"对字节码质量的影响
  3. 在充分测试覆盖下逐步转换

  本计划中**仅记录这一发现**，实际执行范围限定在 A/B/C 三类。

---

### 2.5 E 类：`dischargejpc` 架构审视（中风险）

- **位置**：[codegen.cpp:83-111](src/compiler/codegen.cpp#L83) — `codeABC`/`codeABx`/`codeAsBx` 每条指令发射前调用

```
codeABC  → dischargejpc() → 发射指令
codeABx  → dischargejpc() → 发射指令
codeAsBx → dischargejpc() → 发射指令
```

- **机制**：`blocks_.jpc_` 保存了一个待处理跳转链表。每当发射一条新指令时，需要先将这些跳转的目标修补到当前指令位置，然后清空链表。

- **分析**：这个模式在 Lua 5.1 单遍编译中是必需的——因为在 parse 过程中就可能产生待修补的条件跳转，而下一条指令的位置在发射前才知道。但在两遍架构中，理论上可以：
  1. 在生成条件表达式时收集所有跳转
  2. 在块级别统一回填

  然而，由于当前的条件表达式生成逻辑（`emitCondResult`、`emitCondResultTrue`、`condjump`）深度依赖 `blocks_.jpc_` 链表机制，完全重构它需要重写整个条件代码生成管线。

- **建议**：本轮仅重命名（`dischargejpc` → `flushPendingJumps`），不改变机制。完整重构留给后续的大型架构迭代。

---

## 三、分阶段执行计划

### 阶段总览

| 阶段 | 名称 | 风险 | 改动量 | 依赖 |
|------|------|------|--------|------|
| PR-C1 | 命名规范化 | 低 | ~30 处 | 无 |
| PR-C2 | 注释债务清理 | 低 | ~18 处 | 无 |
| PR-C3 | 寄存器操作封装 | 中 | ~28 处 | PR-C1 |
| PR-C4 | 头文件文档更新 | 低 | ~5 处 | PR-C2 |
| PR-C5 | 最终审查与收尾 | 低 | 全局 | PR-C1~C4 |

---

### PR-C1：命名规范化

**目标**：消除所有 `luaK_` / `discharge` 命名的单遍编译残留。

**改动清单**：

#### 1.1 函数重命名

| 旧名称 | 新名称 | 影响范围 |
|--------|--------|---------|
| `luaK_concat(i32&, i32)` | `concatJumpList(i32&, i32)` | codegen.hpp(1) + codegen.cpp(5) |
| `luaK_getlabel()` | `syncPC()` | codegen.hpp(1) + codegen.cpp(1) |
| `dischargejpc()` | `flushPendingJumps()` | codegen.hpp(1) + codegen.cpp(8) |
| `dischargeValue(const ValueResult&, i32)` | `materializeValue(const ValueResult&, i32)` | codegen.hpp(1) + codegen.cpp(21) |

#### 1.2 参数名更新

在 `emitStore`、`emitCallExpr` 等函数中，如存在 `ExpDesc` 风格的参数名（如 `e` 表示表达式描述），统一使用 `val` / `result` / `expr`。

**涉及文件**：
- `src/compiler/codegen.hpp`
- `src/compiler/codegen.cpp`

**验证标准**：
- `grep -r "luaK_" src/` 返回 0 结果（除注释外）
- `grep -r "discharge" src/` 返回 0 结果（除注释外）
- 全量测试通过

**注意**：`luaK_concat` 被 `codegen.hpp:257` 声明为 private，同时被 `codegen.cpp:1587`（`emitStmt(BreakStmt)` 中通过 `luaK_concat(bl->breaklist, jump())` 调用。确保重命名后所有 5 处调用点都更新。

**状态**：`done` (2026-05-02)

**实际产出**：
- `luaK_concat` → `concatJumpList`：codegen.hpp(1) + codegen.cpp(5)
- `luaK_getlabel` → `syncPC`：codegen.hpp(1) + codegen.cpp(1)
- `dischargejpc` → `flushPendingJumps`：codegen.hpp(1) + codegen.cpp(4 调用 + 1 定义 + 1 注释)
- `dischargeValue` → `materializeValue`：codegen.hpp(1) + codegen.cpp(1 定义 + 17 调用)

**验证结果**：
- `grep -r "\bluaK_\b" src/` → 0 结果
- `grep -r "\bdischargeValue\b|\bdischargejpc\b" src/` → 0 结果
- `lua.vcxproj` / `lua_test.vcxproj` / `lua_app.vcxproj` 全部编译成功
- `bin/lua_test.exe`：50 个测试套件，0 失败
- 全部 10 个 Lua 回归测试通过

---

### PR-C2：注释债务清理

**目标**：移除 "P0修复" 及外部分析仓库相关开发期注释，替换为自解释注释或直接删除。

**改动清单**：

#### 2.1 移除 "⭐ P0修复" 标记注释（13 处）

| 行号 | 当前注释 | 处理方式 |
|------|---------|---------|
| 84 | `// ⭐ P0修复：参考 <external-analysis>/...luaK_code实现` | 删除 |
| 95 | `// ⭐ P0修复：在生成指令前修补待处理的跳转` | 保留语义，改为 `// 在生成指令前刷新待处理跳转` |
| 105 | 同上 | 同上 |
| 166 | `// ⭐ P0修复：添加局部变量后需要递增regs_.freereg_` | 删除（`regs_.alloc()` 已自解释） |
| 167 | `// ⭐ P0修复：确保maxStackSize >= regs_.freereg_` | 删除（`checkStack(0)` 已自解释） |
| 193 | `// ⭐ P0修复：参考 <external-analysis>/...luaK_jump实现` | 删除 |
| 203-204 | `// ⭐ P0修复：使用getjump...` / `// ⭐ P0修复：使用fixjump...` | 删除（函数名已自解释） |
| 216 | `// ⭐ P0修复：参考 <external-analysis>/...dischargejpc实现` | 删除 |
| 1335/1349 | `// ⭐ P0修复：在编译表达式之前/重新设置...` | 保留语义，改为描述寄存器保存/恢复逻辑 |
| 1416 | `// ⭐ P0修复：恢复regs_.freereg_` | 精简为 `// 恢复寄存器状态` |
| 1573 | `// ⭐ P0修复：参考 <external-analysis>/...breakstat实现` | 删除 |
| 1651 | `// ⭐ P0修复：参考 <external-analysis>/...patchtestreg实现` | **保留并改写**：TESTSET→TEST 转换是重要语义，改为 `// TESTSET(A=NO_REG) 转换为 TEST，避免无效寄存器引用` |

#### 2.2 移除其他外部分析仓库引用（5 处）

| 行号 | 处理方式 |
|------|---------|
| 1325 | 删除（`emitCallExpr` 逻辑已独立于原代码） |
| 1489 | 删除 |
| 1529 | 删除 |
| 1591 | 删除 |
| 2031 | 删除 |

**涉及文件**：
- `src/compiler/codegen.cpp`

**验证标准**：
- `grep -r "P0修复" src/compiler/codegen.cpp` 返回 0 结果
- 外部分析仓库名在 `src/compiler/codegen.cpp` 中返回 0 结果
- 全量测试通过

**状态**：`done` (2026-05-02)

**实际产出**：
- 移除/改写 13 处 "⭐ P0修复" 标记注释：
  - 4 处完全删除（行 166, 167, 193, 203, 204, 1573 等）
  - 5 处替换为简短的自解释注释
  - 1 处（TESTSET→TEST 转换）保留语义但移除标记
- 移除 5 处外部分析仓库参考注释
- 清理 1 处 `luaK_goiftrue` 残留注释引用
- 额外清理：修复 `codegen.cpp` 中 1 处已修改的行内注释（`flushPendingJumps` 已反映 PR-C1 重命名）

**验证结果**：
- `grep -r "P0修复" src/compiler/codegen.cpp` → 0 结果
- 外部分析仓库名在 `src/compiler/codegen.cpp` 中 → 0 结果
- `lua.vcxproj` / `lua_test.vcxproj` / `lua_app.vcxproj` 全部编译成功
- `bin/lua_test.exe`：50 个测试套件，0 失败
- 全部 10 个 Lua 回归测试通过

**注意**：`codegen.hpp:23-24` 文件头中的外部分析仓库参考注释保留，将在 PR-C4（头文件文档更新）中处理。

---

### PR-C3：寄存器操作封装

**目标**：将 28 处直接 `regs_.freereg_ =` 赋值替换为 `RegisterAllocator` 的语义化方法调用。

**改动清单**：

#### 3.1 新增 RegisterAllocator 方法

在 `src/compiler/register_allocator.hpp` 中新增加方法：

```cpp
/// 将 freereg_ 重置到指定值
void setFreeReg(i32 n) noexcept { freereg_ = n; }

/// 重置为局部变量数量的位置
void resetToLocals(i32 nactvar) noexcept { freereg_ = nactvar; }

/// 恢复到之前保存的值
void restore(i32 saved) noexcept { freereg_ = saved; }
```

设计原则：
- 不追求一步到位的完美抽象（如 RAII guard）
- 语义化命名，让调用点意图更清晰
- 保持 `freereg_` 为 public 以兼容渐进迁移

#### 3.2 替换调用点（28 处）

按语义分类替换：

| 原代码 | 替换为 | 出现次数 | 位置（行号） |
|--------|--------|---------|------------|
| `regs_.freereg_ = 0` | `regs_.setFreeReg(0)` | 2 | 58, 1810 |
| `regs_.freereg_ = locals_.nactvar_` | `regs_.resetToLocals(locals_.nactvar_)` | 3 | 177, 184, 1569 |
| `regs_.freereg_ = savedFreereg` | `regs_.restore(savedFreereg)` | 7 | 844, 1417, 1462, 1473, 1484, 1943 |
| `regs_.freereg_ = base` | `regs_.setFreeReg(base)` | 5 | 1340, 1351, 1430, 1988, 2069 |
| `regs_.freereg_ = base + N` | `regs_.setFreeReg(base + N)` | 8 | 1441, 1996, 2050, 2058, 2086 |
| `regs_.freereg_ = tableReg + 1` | `regs_.setFreeReg(tableReg + 1)` | 3 | 876, 902, 908 |
| `regs_.freereg_ = firstArgReg` | `regs_.setFreeReg(firstArgReg)` | 1 | 999 |
| `regs_.freereg_ = targetReg + 1` | `regs_.setFreeReg(targetReg + 1)` | 1 | 1043 |
| `regs_.freereg_ = ...` (条件表达式) | `regs_.setFreeReg(...)` | 1 | 1055 |

**涉及文件**：
- `src/compiler/register_allocator.hpp`
- `src/compiler/codegen.cpp`

**验证标准**：
- `grep -r "regs_\.freereg_\s*=" src/compiler/codegen.cpp` 返回 0 结果
- 全量测试通过

**状态**：`done` (2026-05-15)

**实际产出**：
- `RegisterAllocator` 新增 `current()` / `setFreeReg()` / `resetToLocals()` / `restore()` / `reserve()` / `ensureAtLeast()`。
- `freereg_` 改为 private，`CodeGenerator` 不再直接读写 `regs_.freereg_`。
- `BlockManager::leaveBlock()` 中的寄存器复位也改为调用 `resetToLocals()`。

**验证结果**：
- `rg "regs_\.freereg_" src/compiler/codegen.cpp src/compiler/codegen_context.hpp` → 0 结果
- `bin/build_test.bat` 成功，0 警告 0 错误

---

### PR-C4：头文件文档更新

**目标**：更新 `codegen.hpp` 中过时的架构描述。

**改动清单**：

| 行号 | 当前内容 | 改为 |
|------|---------|------|
| 18 | `* - 单遍代码生成` | `* - 基于AST的字节码生成` |
| 16-24 | 整体设计原则块 | 刷新为反映当前架构：AST 遍历、三通道表达式生成、结构化寄存器管理 |
| 137-139 | `luaK_goiftrue/luaK_goiffalse` 引用 | 改为 `emitCondResult/emitCondResultTrue` |
| 190 | `exp2Val 语义` | 改为 `括号单值收敛语义` |

**涉及文件**：
- `src/compiler/codegen.hpp`

**验证标准**：
- 头文件不再包含任何 `luaK_`、`exp2`、`单遍`、`discharge` 等遗留术语
- 全量编译通过

**状态**：`done` (2026-05-15)

**实际产出**：
- `codegen.hpp` 文件头改为描述 AST 字节码生成、三通道表达式模型和 `RegisterAllocator`。
- `emitCond()` 注释改为引用 `emitCondResult/emitCondResultTrue`。
- `forceSingleValue()` 注释改为“括号单值收敛语义”。

**验证结果**：
- `rg "luaK_|discharge|P0修复|外部分析仓库标记|单遍|exp2Val|exp2RK|exp2AnyReg|exp2NextReg|regs_\.freereg_" src/compiler/codegen.cpp src/compiler/codegen.hpp src/compiler/codegen_types.hpp src/compiler/register_allocator.hpp` → 0 结果

---

### PR-C5：最终审查与收尾

**目标**：全局 grep 扫描确认零残留，更新相关追踪文档。

**检查清单**：

- [x] `rg "luaK_|discharge|P0修复|外部分析仓库标记|单遍|exp2Val|exp2RK|exp2AnyReg|exp2NextReg|regs_\.freereg_" src/compiler/codegen.cpp src/compiler/codegen.hpp src/compiler/codegen_types.hpp src/compiler/register_allocator.hpp` → 0 结果
- [x] `rg "ExprDesc|ExprKind|expdesc" src/compiler` → 0 结果
- [x] 全量单元测试：`bin/lua_test.exe` 全部通过（402 个注册测试，1574 个结果，0 失败）
- [x] 全量 Lua 回归测试：所有 `tests/lua/regressions/*.lua` 通过
- [x] 更新 `docs/archive/refactors/refactor-expdesc-pr-checklist.md`：追加本文档的完成记录

**状态**：`done` (2026-05-15)

**注意**：`lexer.hpp` / `parser.hpp` / `opcode.hpp` 等文件仍保留对 Lua 5.1.5 C 实现的参考说明；它们不是 `codegen` 单遍清理残留，不在本轮 PR-C 范围内。

---

## 四、不改动的边界

以下模式**不在本轮清理范围内**，明确标注以供后续参考：

### 4.1 `ValueResult` 延迟物化（D 类）

`PendingLoad` / `Relocatable` / `PendingJump` / `MultiRet` 四种 Kind 构成了当前代码生成器的核心工作模型。将它们改为"立即发射"需要重新设计整个表达式代码生成管线，风险远超收益。

**判断**：这是**设计选择**而非"残留"。保留。

### 4.2 `BlockManager::jpc_` 跳转链表机制（E 类）

`blocks_.jpc_` 是条件代码生成的核心基础设施。虽然它源自单遍编译的 `FuncState.jpc`，但在当前架构中它是合理的"条件跳转待回填队列"。完全消除它需要引入基本块（Basic Block）和 CFG（Control Flow Graph），这是编译器中间表示（IR）级别的大型架构变更。

**判断**：当前机制足够清晰，重命名（`flushPendingJumps`）已足够表明语义。保留。

### 4.3 `emitCond` 旧签名 `i32 emitCond(const Expr&)`

[codegen.hpp:144](src/compiler/codegen.hpp#L144) — 返回 `i32`（falseList 的第一个元素）的旧版 `emitCond`：

- **当前用途**：仅在 `emitStmt(IfStmt)` 和 `emitStmt(WhileStmt)` 中用于判断条件是否为常假值（空语句体优化）
- **分析**：这是为兼容旧调用方保留的薄包装。由于需要检查其调用方是否真的依赖 `i32` 返回值，这个改动涉及到语句代码生成的结构调整
- **建议**：留给后续优化 PR，不在命名清理阶段处理

### 4.4 `getjump` / `fixjump` / `condjump`

这些函数虽然命名风格来自 Lua 5.1，但它们是跳转修补的基础工具函数，语义清晰。`getjump` 和 `fixjump` 是字节码层面的跳转偏移操作，不宜与 `discharge` / `luaK_` 等同看待。保留。

---

## 五、风险矩阵

| 阶段 | 字节码变更风险 | 编译失败风险 | 测试回归风险 | 缓解措施 |
|------|-------------|------------|------------|---------|
| PR-C1 命名规范化 | **零** — 纯文本替换 | 低（需确保引用全更新） | 低 | 全局 grep 确认无遗漏 |
| PR-C2 注释清理 | **零** — 仅注释 | 零 | 零 | 无 |
| PR-C3 寄存器封装 | **零** — 语义等价的 thin wrapper | 低 | 低 | 每种模式先替换 1 处验证，再批量替换 |
| PR-C4 文档更新 | **零** — 仅注释 | 零 | 零 | 无 |
| PR-C5 收尾 | 零 | 零 | 低 | 全局 grep 自动化验证 |

---

## 六、执行建议

### 推荐顺序

```
PR-C1 (命名规范化)
    ↓
PR-C2 (注释清理)    ← 可与 C1 并行
    ↓
PR-C4 (文档更新)    ← 可与 C3 并行
    ↓
PR-C3 (寄存器封装)  ← 需 C1 完成后进行
    ↓
PR-C5 (最终审查)
```

### 每个 PR 的提交前门禁

1. `lua.vcxproj` 编译通过（0 错误 0 警告）
2. `lua_test.vcxproj` 编译通过
3. `lua_app.vcxproj` 编译通过
4. `bin/lua_test.exe` 全部通过（51 个测试套件，0 失败）
5. 该阶段相关的 grep 残留扫描归零
6. 所有 Lua 回归测试通过

---

## 七、完成后预期状态

执行完 PR-C1 到 PR-C5 后，代码库将达到以下状态：

- **命名**：不再有 `luaK_`、`discharge` 等 Lua 5.1 C 源码前缀
- **注释**：不再有 "P0修复"、外部分析仓库名等开发期标记
- **寄存器管理**：所有 `freereg_` 操作通过语义化方法进行
- **文档**：`codegen.hpp` 描述准确反映当前两遍架构
- **行为**：字节码输出完全不变
- **测试**：所有 51 个套件 0 失败

届时可以说，本项目**不仅在结构上、更在代码表达上**彻底完成了从 Lua 5.1 单遍编译到现代 C++ 两遍编译的迁移。

---

> **关联文档**：
> - [refactor_expdesc_plan.md](./refactor_expdesc_plan.md) — 原始架构设计
> - [refactor_expdesc_pr_checklist.md](./refactor_expdesc_pr_checklist.md) — PR-0~PR-9 执行清单
