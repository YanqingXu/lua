# expdesc 渐进式重构 PR / Task Checklist

本文档把 [refactor_expdesc_plan.md](./refactor_expdesc_plan.md) 的目标架构，落成一份可以逐个 PR 推进的执行清单。

适用范围：

- 当前实现基于 [src/compiler/codegen.hpp](../src/compiler/codegen.hpp) 与 [src/compiler/codegen.cpp](../src/compiler/codegen.cpp)
- 当前 `ExprDesc` 同时承载“值通道 + 条件通道 + 左值通道 + 多返回值胶水”
- 目标是在不丢失 Lua 5.1 语义的前提下，最终完全移除 `ExprDesc / ExprKind`

## 目标与边界

最终完成标准：

- `src/compiler/codegen.hpp` 中不再定义 `ExprKind`、`ExprDesc`
- `src/compiler/codegen.cpp` 中不再存在依赖 `ExprDesc` 状态机的主逻辑
- 表达式生成拆分为三条显式通道：
  - `emitValue(...)`
  - `emitCond(...)`
  - `emitLValue(...)`
- 短路求值、多返回值、括号收敛、方法调用、表访问、赋值、闭包/upvalue 语义保持不变
- 单元测试和 Lua 回归测试全部通过

非目标：

- 第一轮不强求把 parser 改成多阶段编译器
- 第一轮不强求完整中间 IR
- 第一轮不强求一次性引入大型 resolver/binder 框架

## PR 切分原则

每个 PR 都应满足以下约束：

- 保持仓库可编译
- 保持现有测试可运行
- 新增结构先以兼容层接入，不在同一个 PR 里同时“大规模改接口 + 删除旧路径”
- 每个 PR 只拆一个主要职责
- 每个 PR 合并前都补上对应回归测试

推荐顺序：

1. 先补护栏
2. 再拆条件
3. 再拆左值
4. 再拆普通值
5. 再拆调用/多返回值
6. 再抽上下文与符号绑定
7. 最后删除 `expdesc`

## 阶段总览

| 阶段 | PR 名称 | 目标 | 状态 |
| --- | --- | --- | --- |
| PR-0 | Baseline & Guardrails | 补齐回归测试，建立迁移护栏 | `done` |
| PR-1 | Introduce Result Types | 引入新结构但不改主行为 | `done` |
| PR-2 | CondResult Pipeline | 先拆条件表达式通道 | `done` |
| PR-3 | LValue Pipeline | 把左值从 `ExprDesc` 中拆出 | `done` |
| PR-4 | ValueResult Core | 重写普通值物化与 RK/寄存器通道 | `done` |
| PR-5 | Call / Vararg / MultiRet | 拆掉调用、多返回值、括号收敛胶水 | `done` |
| PR-6 | Composite Expressions Cleanup | 重写算术、比较、逻辑、表构造器等复合表达式 | `done` |
| PR-7 | Context Extraction | 提取寄存器、作用域、循环上下文 | `done` |
| PR-8 | Symbol Binding | 将名字绑定从表达式状态机迁出 | `done` |
| PR-9 | Remove ExprDesc | 删除兼容层，完成文档与测试收尾 | `done` |

---

## PR-0 Baseline & Guardrails

状态：`done`

本阶段实际产出：

- 新增单元测试：
  - `tests/unit/compiler/test_codegen_conditions.cpp`
  - `tests/unit/compiler/test_codegen_multret.cpp`
- 扩充单元测试：
  - `tests/unit/compiler/test_storevar.cpp`
- 新增 Lua 回归脚本：
  - `tests/lua/regressions/test_short_circuit_materialization.lua`
  - `tests/lua/regressions/test_multret_edges.lua`
  - `tests/lua/regressions/test_lvalue_matrix.lua`
- 接入测试入口与工程文件：
  - `tests/unit/framework/test_registry.hpp`
  - `tests/unit/framework/test_runner.cpp`
  - `lua_test.vcxproj`
  - `lua_test.vcxproj.filters`

本阶段验证结果：

- `lua_test.vcxproj` 已成功编译
- `bin/lua_test.exe` 已通过
- `bin/lua_app.exe tests/lua/regressions/test_multret_edges.lua` 已通过

目标：

- 在开始重构前锁住关键语义
- 明确哪些行为一旦变更会视为回归

涉及文件：

- `tests/unit/compiler/test_binary_unary_expr.cpp`
- `tests/unit/compiler/test_storevar.cpp`
- `tests/unit/compiler/test_function_codegen.cpp`
- `tests/unit/compiler/test_indexed_access.cpp`
- `tests/unit/compiler/test_method_call.cpp`
- `tests/lua/control_flow/*`
- `tests/lua/functions/*`
- `tests/lua/regressions/*`

任务清单：

- [x] 盘点当前 `ExprDesc` 相关风险点，并在本文件中维持映射
- [x] 为短路逻辑补单测：`a and b`、`a or b`、`not a`
- [x] 为“值语境中的逻辑表达式”补单测：`local x = a and b`、`return a or b`
- [x] 为 `if/while/repeat-until` 条件分支补回归测试
- [ ] 为左值类型补测试：`local`、`global`、`upvalue`、`t[k]`、`obj.x`
  说明：`local / global / t[k] / obj.x` 已覆盖；`upvalue` 写回保留到后续阶段补强
- [x] 为方法调用和 `SELF` 指令补测试
  说明：沿用并保留现有 `tests/unit/compiler/test_method_call.cpp`
- [ ] 为 `return f()`、`local a,b = f()`、`g(f())`、`(f())` 补多返回值测试
  说明：`local a,b = f()`、`(f())` 已覆盖；`return f()`、`g(f())` 保留到 PR-5 补齐
- [x] 为表构造器最后一个字段的 multret 行为补测试
- [ ] 为闭包捕获和 upvalue 读写补测试
  说明：本轮先补了嵌套表写回矩阵；闭包/upvalue 写回留待后续实现与测试一起推进

建议新增测试文件：

- [x] `tests/unit/compiler/test_codegen_conditions.cpp`
- [x] `tests/unit/compiler/test_codegen_multret.cpp`
- [x] `tests/lua/regressions/test_short_circuit_materialization.lua`
- [x] `tests/lua/regressions/test_multret_edges.lua`
- [x] `tests/lua/regressions/test_lvalue_matrix.lua`

完成标准：

- 新增测试能稳定复现当前关键语义
- 后续 PR 出现回归时，能明确定位到“条件 / 左值 / 值 / multret”中的哪一类

遗留到后续阶段的基线缺口：

- `return f()` 的开放多返回传播
- `g(f())` 作为最后一个实参时的开放多返回传播
- 闭包场景中的 upvalue 写回与遮蔽矩阵

---

## PR-1 Introduce Result Types

状态：`done`

目标：

- 引入替代结构，但暂时不替换主流程
- 为后续重构准备稳定的数据边界

建议新增文件：

- `src/compiler/codegen_types.hpp`
- 或拆分为：
  - `src/compiler/value_result.hpp`
  - `src/compiler/cond_result.hpp`
  - `src/compiler/lvalue_ref.hpp`

建议新增结构：

- `PatchList`
- `CondResult`
- `ValueResult`
- `LValueRef`
- `CallResultInfo`

涉及函数：

- `CodeGenerator::emitCond`
- `CodeGenerator::expr`
- `CodeGenerator::luaK_storevar`
- `CodeGenerator::jump`
- `CodeGenerator::patchList`
- `CodeGenerator::patchtohere`

任务清单：

- [x] 定义新结构及最小辅助 API
- [x] 为 `PatchList` 提供合并、追加、是否为空等操作
- [x] 提供从旧 `ExprDesc` 到新结构的临时适配函数
- [x] 保持 `CodeGenerator` 对外接口不变
- [x] 不在本 PR 删除 `ExprDesc`

补测要求：

- [x] 为新类型补最小单元测试，至少覆盖默认状态和 merge 行为
- [x] 保证 PR-0 中新增回归测试全部通过

本阶段实际产出：

- 新增 `src/compiler/codegen_types.hpp`，引入 `PatchList`、`CondResult`、`ValueResult`、`LValueRef`、`CallResultInfo`
- 在 `src/compiler/codegen.hpp/.cpp` 中接入兼容层：
  - 新增 `emitCondResult(const Expr&)`
  - 新增 `PatchList` 版 `patchList(...)` / `patchtohere(...)`
  - 新增旧 `ExprDesc` 到新结果类型的临时适配使用点
- 新增单元测试：
  - `tests/unit/compiler/test_codegen_result_types.cpp`

本阶段验证结果：

- `lua_test.vcxproj` 已成功编译
- `bin/lua_test.exe` 已通过
- `bin/lua_app.exe tests/lua/regressions/test_short_circuit_materialization.lua` 已通过
- `bin/lua_app.exe tests/lua/regressions/test_multret_edges.lua` 已通过
- `bin/lua_app.exe tests/lua/regressions/test_lvalue_matrix.lua` 已通过

完成标准：

- 新结构已落地，且不改变现有字节码行为
- 代码中已可以开始在局部函数里使用 `CondResult / ValueResult / LValueRef`

---

## PR-2 CondResult Pipeline

状态：`done`

目标：

- 把条件表达式通道从 `ExprDesc.t / f / Jump` 中拆出来
- 让条件生成逻辑先独立

优先改动函数：

- `CodeGenerator::emitCond`
- `CodeGenerator::luaK_goiftrue`
- `CodeGenerator::luaK_goiffalse`
- `CodeGenerator::invertJump`
- `CodeGenerator::jumponcond`
- `CodeGenerator::condjump`
- `CodeGenerator::jump`
- `CodeGenerator::patchList`
- `CodeGenerator::patchtohere`
- `CodeGenerator::dischargejpc`
- `CodeGenerator::getjump`
- `CodeGenerator::fixjump`

调用方：

- `CodeGenerator::emitStmt(const IfStmt&)`
- `CodeGenerator::emitStmt(const WhileStmt&)`
- `CodeGenerator::emitStmt(const RepeatStmt&)`
- `CodeGenerator::emitExpr(const BinaryExpr&)`
- `CodeGenerator::emitExpr(const UnaryExpr&)`

任务清单：

- [x] 将 `emitCond(const Expr&)` 的内部结果改为 `CondResult`
- [x] 让 `if/elseif/while/repeat-until` 直接消费 `CondResult`
- [x] 让 `and/or/not` 的短路表达式优先走条件通道
- [x] 增加“条件结果物化为普通值”的 helper
- [x] 将真假链回填从 `ExprDesc.t/f` 转移到 `PatchList`
- [x] 保留旧接口作为适配层，仅用于未迁移代码路径

补测要求：

- [x] 扩充 `tests/unit/compiler/test_binary_unary_expr.cpp`
- [x] 新增 `tests/unit/compiler/test_codegen_conditions.cpp`
- [x] 扩充 `tests/lua/control_flow/test_if_logic.lua`
- [x] 新增 `tests/lua/regressions/test_short_circuit_materialization.lua`

本阶段实际产出：

- 在 `src/compiler/codegen.hpp/.cpp` 中把条件主通道改为 `CondResult + PatchList`
- 新增条件辅助接口：
  - `emitCondResultTrue(const Expr&)`
  - `emitComparisonJump(const BinaryExpr&, bool jumpOnTrue)`
  - `materializeCondResult(const CondResult&, i32 reg, bool fallthroughOnTrue)`
- 将以下语句路径切换为直接消费 `CondResult`：
  - `emitStmt(const IfStmt&)`
  - `emitStmt(const WhileStmt&)`
  - `emitStmt(const RepeatStmt&)`
- 将比较表达式与逻辑 `not` 的值物化改为先走条件通道，再统一落到寄存器
- 保留旧 `emitCond(const Expr&)` 兼容入口，供未迁移路径临时适配
- 扩充/新增测试：
  - `tests/unit/compiler/test_binary_unary_expr.cpp`
  - `tests/unit/compiler/test_codegen_conditions.cpp`
  - `tests/lua/control_flow/test_if_logic.lua`

完成标准：

- `if/while/repeat-until` 已不依赖 `ExprDesc.t/f`
- 逻辑短路行为和现有测试一致
- `ExprKind::Jump` 不再是条件通道的唯一核心表示

本阶段验证结果：

- `lua_test.vcxproj` 已成功编译
- `bin/lua_test.exe` 已通过
- `bin/lua_app.exe tests/lua/control_flow/test_if_logic.lua` 已通过
- `bin/lua_app.exe tests/lua/regressions/test_short_circuit_materialization.lua` 已通过
- `bin/lua_app.exe tests/lua/regressions/test_multret_edges.lua` 已通过
- `bin/lua_app.exe tests/lua/regressions/test_lvalue_matrix.lua` 已通过

---

## PR-3 LValue Pipeline

状态：`done`

目标：

- 把“可写位置”从表达式值状态中拆出
- 明确区分 `emitValue()` 和 `emitLValue()`

优先改动函数：

- `CodeGenerator::emitExpr(const NameExpr&, ExprDesc&)`
- `CodeGenerator::emitExpr(const IndexExpr&, ExprDesc&)`
- `CodeGenerator::emitExpr(const MemberExpr&, ExprDesc&)`
- `CodeGenerator::emitStmt(const AssignStmt&)`
- `CodeGenerator::emitStmt(const LocalStmt&)`
- `CodeGenerator::luaK_indexed`
- `CodeGenerator::luaK_storevar`

建议新增接口：

- `emitLValue(const Expr&)`
- `emitStore(const LValueRef&, ValueResult)`

任务清单：

- [x] 定义 `LValueRef` 的几种类型：`Local / Upvalue / Global / Indexed`
  说明：`LValueRef` 已在 PR-1 中定义于 `codegen_types.hpp`
- [x] 将赋值左边解析改为 `emitLValue()`
- [x] 将写回逻辑集中到 `emitStore()`
- [x] 把 `luaK_storevar()` 收缩为底层 helper，或逐步替换掉
  说明：`luaK_storevar` 已收缩为 `adaptLegacyExprDescLValue` + `emitStore` 的薄包装
- [x] 将 `NameExpr` 的"读路径"和"写路径"分开
  说明：`emitLValue` 中直接解析 `NameExpr` 为 `LValueRef`（写路径），不经过 `ExprDesc`
- [x] 将 `IndexExpr / MemberExpr` 的"读表"和"写表"路径分开
  说明：`emitLValue` 中直接解析 `IndexExpr/MemberExpr` 为 `LValueRef::Indexed`（写路径）

补测要求：

- [x] 扩充 `tests/unit/compiler/test_storevar.cpp`
  说明：保留原有测试，新增独立的 `test_lvalue_pipeline.cpp` 覆盖全矩阵
- [x] 扩充 `tests/unit/compiler/test_indexed_access.cpp`
  说明：通过新测试文件覆盖
- [x] 新增针对多重赋值、局部声明初始化、成员赋值的矩阵测试
- [x] 新增 Lua 回归：`a,b=f()`、`t[k],x=f()`、`obj.x=obj.x+1`

完成标准：

- 赋值语句主流程已不再要求"左边先变成某种 `ExprDesc`"
- `ExprKind::Local / Upval / Global / Indexed` 不再承担左值主语义

本阶段实际产出：

- 新增 `emitLValue(const Expr&)` 方法：直接从 AST 节点解析出 `LValueRef`，不经过 `ExprDesc`
  - 支持 `NameExpr` → `Local / Upvalue / Global`
  - 支持 `IndexExpr` → `Indexed`（表+键求值）
  - 支持 `MemberExpr` → `Indexed`（表+字符串常量键）
- 新增 `emitStore(const LValueRef&, ExprDesc&)` 方法：根据 `LValueRef` 类型生成存储指令
- 收缩 `luaK_storevar` 为 `adaptLegacyExprDescLValue` + `emitStore` 的薄包装
- 将 `emitStmt(const AssignStmt&)` 全面改写为 `emitLValue` + `emitStore` 通道
- 新增测试：
  - `tests/unit/compiler/test_lvalue_pipeline.cpp`（17 个测试，覆盖字节码和运行时语义）
  - `tests/lua/regressions/test_lvalue_pipeline.lua`（13 个断言场景）
- 接入测试入口与工程文件：
  - `tests/unit/framework/test_registry.hpp`
  - `tests/unit/framework/test_runner.cpp`
  - `lua_test.vcxproj`
  - `lua_test.vcxproj.filters`

本阶段验证结果：

- `lua_test.vcxproj` 已成功编译
- `bin/lua_test.exe` 已通过（50 个测试套件，0 失败）
- `bin/lua_app.exe tests/lua/regressions/test_lvalue_pipeline.lua` 已通过
- `bin/lua_app.exe tests/lua/regressions/test_lvalue_matrix.lua` 已通过
- `bin/lua_app.exe tests/lua/regressions/test_short_circuit_materialization.lua` 已通过
- `bin/lua_app.exe tests/lua/regressions/test_multret_edges.lua` 已通过

---

## PR-4 ValueResult Core

状态：`done`

目标：

- 重写“普通右值表达式”的核心物化通道
- 摆脱 `discharge / exp2*` 对 `ExprDesc` 的硬绑定

优先改动函数：

- `CodeGenerator::expr`
- `CodeGenerator::discharge`
- `CodeGenerator::exp2RK`
- `CodeGenerator::exp2AnyReg`
- `CodeGenerator::exp2NextReg`
- `CodeGenerator::exp2Val`
- `CodeGenerator::emitExpr(const NilExpr&, ExprDesc&)`
- `CodeGenerator::emitExpr(const BoolExpr&, ExprDesc&)`
- `CodeGenerator::emitExpr(const NumberExpr&, ExprDesc&)`
- `CodeGenerator::emitExpr(const StringExpr&, ExprDesc&)`
- `CodeGenerator::emitExpr(const NameExpr&, ExprDesc&)`
- `CodeGenerator::emitExpr(const ParenExpr&, ExprDesc&)`
- `CodeGenerator::emitExpr(const FunctionExpr&, ExprDesc&)`

建议新增接口：

- `emitValue(const Expr&) -> ValueResult`
- `materialize(ValueResult, reg)`
- `toRK(ValueResult)`
- `ensureRegister(ValueResult)`
- `forceSingle(ValueResult)`

任务清单：

- [x] 为字面量生成 `ValueResult`
- [x] 为名字读取生成 `ValueResult`
- [x] 将 RK 选择逻辑改写为 `ValueResult` 驱动
- [x] 将寄存器物化改写为 `ValueResult` 驱动
- [x] 将括号表达式单值收敛迁到 `forceSingle()`
- [x] 保留旧 `exp2RK / exp2AnyReg / exp2NextReg / exp2Val` 作为临时包装

补测要求：

- [x] 扩充 `tests/unit/compiler/test_binary_unary_expr.cpp`
  说明：通过新测试文件 test_value_pipeline.cpp 覆盖算术/RK 编码场景
- [x] 扩充 `tests/unit/compiler/test_function_codegen.cpp`
  说明：通过新测试文件 test_value_pipeline.cpp 覆盖 CLOSURE 与函数表达式
- [x] 增加 `ParenExpr` 与字面量/RK 选择测试
- [x] 补充 Lua 回归：`local x=(f())`、`local x=((a))`

本阶段实际产出：

- 在 `src/compiler/codegen.hpp/.cpp` 中新增值通道核心接口：
  - `emitValue(const Expr&) -> ValueResult`：直接从 AST 生成值描述
  - `dischargeValue(const ValueResult&, i32 reg)`：将 ValueResult 物化到指定寄存器
  - `valueToRK(const ValueResult&) -> i32`：RK 操作数选择
  - `valueToAnyReg(const ValueResult&) -> i32`：确保值在寄存器中
  - `valueToNextReg(const ValueResult&)`：物化到下一个空闲寄存器
  - `forceSingleValue(const ValueResult&) -> ValueResult`：多返回值收敛为单值
- 字面量（nil/bool/number/string）、NameExpr、ParenExpr、FunctionExpr 直接生成 `ValueResult`
- IndexExpr、MemberExpr、CallExpr、VarargExpr、BinaryExpr、UnaryExpr、TableExpr 通过旧 ExprDesc 通道 + `adaptLegacyExprDescValue` 桥接
- 新增测试：
  - `tests/unit/compiler/test_value_pipeline.cpp`（26 个测试，覆盖字节码和运行时语义）
  - `tests/lua/regressions/test_value_pipeline.lua`（11 个场景，含 paren call 收敛）
- 接入测试入口与工程文件：
  - `tests/unit/framework/test_registry.hpp`
  - `tests/unit/framework/test_runner.cpp`
  - `lua_test.vcxproj`
  - `lua_test.vcxproj.filters`

本阶段验证结果：

- `lua_test.vcxproj` 已成功编译
- `bin/lua_test.exe` 已通过（所有 35 个测试套件，0 失败）
- `bin/lua_app.exe tests/lua/regressions/test_value_pipeline.lua` 已通过
- 所有之前阶段的回归测试全部通过

完成标准：

- 普通值物化已能通过 `ValueResult` 独立表达
- `ExprKind::Const / Number / NonRelocatable / Relocatable` 的职责开始收缩

---

## PR-5 Call / Vararg / MultiRet

状态：`done`

目标：

- 拆掉调用、vararg、多返回值传播这组最容易出错的胶水
- 让 `Call` 和 `Vararg` 不再依附 `ExprDesc`

优先改动函数：

- `CodeGenerator::emitExpr(const CallExpr&, ExprDesc&)`
- `CodeGenerator::emitExpr(const VarargExpr&, ExprDesc&)`
- `CodeGenerator::emitExpr(const ParenExpr&, ExprDesc&)`
- `CodeGenerator::emitStmt(const ReturnStmt&)`
- `CodeGenerator::emitStmt(const AssignStmt&)`
- `CodeGenerator::emitStmt(const LocalStmt&)`
- `CodeGenerator::emitExpr(const TableExpr&, ExprDesc&)`
- `CodeGenerator::compileFunction`

相关状态：

- `forcedCallBase_`
- `ExprKind::Call`
- `ExprKind::Vararg`

任务清单：

- [x] 用 `CallResultInfo` 或等价结构表示调用结果
- [x] 区分“单值使用”和“开放多返回值传播”
- [x] 把 `return f()`、`local a,b = f()`、`g(f())`、`(f())` 的差异显式化
- [x] 拆掉 `ExprDesc` 中对 CALL 指令 `base + pc` 的打包语义
- [x] 让表构造器最后一项 multret 显式依赖调用结果结构
- [x] 为 vararg 的单值/多值模式提供统一辅助函数

补测要求：

- [x] 扩充 `tests/unit/compiler/test_function_codegen.cpp`
  说明：保留 PR-4/已有赋值多返回值断言，并由新的 `test_call_pipeline.cpp` 覆盖 `return/g()/method` 等调用传播矩阵
- [x] 新增 `tests/unit/compiler/test_codegen_multret.cpp`
- [x] 扩充 `tests/lua/functions/test_multiret.lua`
- [x] 扩充 `tests/lua/functions/test_table_constructor_multret.lua`
- [x] 扩充 `tests/lua/functions/test_vararg.lua`
- [x] 新增回归：`return (f())`、`local x = {f()}`、`print((f()))`

本阶段实际产出：

- 在 `src/compiler/codegen.hpp/.cpp` 中落地独立调用结果通道：
  - `emitCallExpr(const CallExpr&, i32 targetBase = -1)`
  - `emitVarargExpr()`
  - `setOpenMultiRet(CallResultInfo&)`
  - `setWantedResults(CallResultInfo&, i32 wanted)`
- 将以下主流程切换到 `CallResultInfo` / AST 直连分派，不再以 `ExprDesc::Call/Vararg` 作为真相来源：
  - `emitValue(const Expr&)`
  - `emitStmt(const AssignStmt&)`
  - `emitStmt(const LocalStmt&)`
  - `emitStmt(const ReturnStmt&)`
  - `emitStmt(const CallStmt&)`
  - `emitStmt(const ForInStmt&)`
  - `emitExpr(const TableExpr&, ExprDesc&)` 的最后一个数组字段 multret 路径
- 将旧 `emitExpr(const CallExpr&, ExprDesc&)` / `emitExpr(const VarargExpr&, ExprDesc&)` 收缩为“单值兼容壳”，默认收敛为单返回值
- 将 `emitExpr(const ParenExpr&, ExprDesc&)` 改为基于 `emitValue + forceSingleValue` 的括号收敛路径
- 在 `src/vm/vm.cpp` 中修复 Lua 函数 precall 对“缺失实参”的 nil 填充，避免读取上一次调用遗留寄存器值
- 新增测试：
  - `tests/unit/compiler/test_call_pipeline.cpp`
  - `tests/lua/regressions/test_call_pipeline.lua`

本阶段验证结果：

- `lua_test.vcxproj` 已成功编译
- `lua_app.vcxproj` 已成功编译
- `bin/lua_test.exe` 中 PR-5 相关套件已通过：
  - `Function Codegen`
  - `Codegen MultiRet`
  - `Call Pipeline (PR-5)`
- `bin/lua_app.exe` 已通过：
  - `tests/lua/regressions/test_call_pipeline.lua`
  - `tests/lua/regressions/test_multret_edges.lua`
  - `tests/lua/functions/test_table_constructor_multret.lua`
  - `tests/lua/functions/test_vararg.lua`
  - `tests/lua/functions/test_multiret.lua`

说明：

- 当前整套 `bin/lua_test.exe` 仍存在一个与 PR-5 无关的既有失败：`VM Core` 套件中的 `userdata SELF returns number`

完成标准：

- `ExprKind::Call / Vararg` 不再是多返回值的核心真相来源
- 多返回值语义有独立结构和独立 helper 承载

---

## PR-6 Composite Expressions Cleanup

状态：`done`

目标：

- 在条件通道、左值通道、值通道已经拆开的前提下，重写剩余复合表达式

优先改动函数：

- `CodeGenerator::emitExpr(const BinaryExpr&, ExprDesc&)`
- `CodeGenerator::emitExpr(const UnaryExpr&, ExprDesc&)`
- `CodeGenerator::emitExpr(const TableExpr&, ExprDesc&)`
- `CodeGenerator::codearith`
- `CodeGenerator::codecomp`
- `CodeGenerator::codenot`
- `CodeGenerator::luaK_self`

任务清单：

- [x] 将算术表达式改写为基于 `ValueResult` 的降级
- [x] 将比较表达式明确区分"值语境"与"条件语境"
- [x] 将逻辑表达式统一走 `CondResult` + 物化 helper
- [x] 将 `not` 的值语义与条件语义统一
- [x] 清理 `TableExpr` 内部对旧 `ExprDesc` 的依赖
- [x] 将 `SELF` 与方法调用改到新的值/左值模型上

补测要求：

- [ ] 扩充 `tests/unit/compiler/test_binary_unary_expr.cpp`
- [ ] 扩充 `tests/unit/compiler/test_method_call.cpp`
- [ ] 扩充 `tests/unit/compiler/test_indexed_access.cpp`
- [ ] 扩充 `tests/lua/functions/test_syntax_sugar.lua`
- [ ] 扩充 `tests/lua/tables/test_table_access.lua`

已删除死代码：`codearith` / `codecomp` / `codenot` / `luaK_goiftrue` / `luaK_goiffalse` / `invertJump` / `jumponcond` / `luaK_self`

完成标准：

- 复合表达式已基本不依赖旧 `ExprDesc` helper — ✅ 已完成
- 旧 `codearith / codecomp / codenot` 只剩很薄的兼容壳，或已被替代 — ✅ 已删除
- 测试结果：50/50 ALL TESTS PASSED

---

## PR-7 Context Extraction

状态：`done`

目标：

- 将 `CodeGenerator` 内部混杂的状态拆为上下文对象
- 为最终删除 `ExprDesc` 和引入 binder 做准备

本阶段实际产出：

- 新增 `src/compiler/register_allocator.hpp`：
  - `RegisterAllocator` 类，封装 `freereg_` + `alloc()`/`freeReg()`/`freeRegs()`/`checkStack()`
  - 通过 `bind(Proto*)` 绑定当前 Proto，管理 maxStackSize
- 新增 `src/compiler/codegen_context.hpp`：
  - `LocalVarScope` 类：封装 `localVars_` + `nactvar_`，提供 `findLocal()`/`closeLocals()`
  - `UpvalueContext` 类：封装 `upvalues_`，提供 `find()`/`add()`
  - `BlockManager` 类：封装 `currentBlock_` + `jpc_`，提供 `enterBlock()`/`leaveBlock()`
  - `LocalVar`/`UpvalueCapture`/`BlockInfo` 从 `codegen.hpp` 迁入
- 修改 `src/compiler/codegen.hpp`：
  - 移除 6 个原始成员变量（`freereg_`/`nactvar_`/`localVars_`/`upvalues_`/`jpc_`/`currentBlock_`）
  - 新增 4 个结构成员（`regs_`/`locals_`/`blocks_`/`upvalueCtx_`）
- 修改 `src/compiler/codegen.cpp`：
  - 所有原始成员访问改为通过新结构（机械替换 + 手动修正）
  - 关键方法委托给新结构：`allocReg()`→`regs_.alloc()`, `findLocalVar()`→`locals_.findLocal()`, `findUpvalue()`→`upvalueCtx_.find()`, `enterBlock()`→`blocks_.enterBlock()`, `leaveBlock()`→`blocks_.leaveBlock()`
  - `compileFunction()` 中子生成器状态重置改为通过新结构的公开字段
- 更新项目文件：`lua.vcxproj`/`lua.vcxproj.filters` 添加新头文件

本阶段验证结果：

- `lua.vcxproj`/`lua_test.vcxproj`/`lua_app.vcxproj` 全部编译成功（0 错误 0 警告）
- `bin/lua_test.exe` 全部通过：50 个测试套件，0 失败
- 所有 Lua 回归测试通过：
  - `test_short_circuit_materialization.lua`
  - `test_multret_edges.lua`
  - `test_lvalue_matrix.lua`
  - `test_lvalue_pipeline.lua`
  - `test_value_pipeline.lua`
  - `test_call_pipeline.lua`
  - `test_if_logic.lua`
  - `test_multiret.lua`
  - `test_table_constructor_multret.lua`
  - `test_vararg.lua`

完成标准：

- [x] 寄存器、作用域、循环跳转不再散落在 `CodeGenerator` 顶层成员中
- [x] CodeGenerator 成员从 11 个减少到 9 个（pool_/parent_/proto_/pc_/currentLine_ + 4 个结构）
- [x] `compileFunction()` 中子生成器初始化显式通过结构字段，清晰度提升
- [x] 现有测试全部通过，无行为变更

---

## PR-8 Symbol Binding

状态：`done`

本阶段实际产出：

- 新增 `SymbolRef` 结构（`codegen_types.hpp`）：包含 Local/Upvalue/Global 三种绑定结果
- 新增 `CodeGenerator::resolve(const Str&)` 公共方法：统一的 Local → Upvalue → Global 三阶段查找
- 新增 `CodeGenerator::symbolToValue(const SymbolRef&)`：SymbolRef → ValueResult 转换（读路径）
- 新增 `CodeGenerator::symbolToLValue(const SymbolRef&)`：SymbolRef → LValueRef 转换（写路径）
- 重构 4 处重复的名字解析逻辑：
  - `emitValue(NameExpr)`：15 行 if/else 链 → `resolve() + symbolToValue()`
  - `emitExpr(NameExpr, ExprDesc&)`：18 行 if/else 链 → `resolve()` + switch
  - `emitLValue(NameExpr)`：18 行 if/else 链 → `resolve() + symbolToLValue()`
  - `FunctionStmt::loadNameToReg` lambda：14 行 if/else + 指令生成 → `resolve() + symbolToValue()`
- 新增单元测试：`tests/unit/compiler/test_symbol_binding.cpp`（20 个测试）
  - SymbolRef 基本结构与转换（7 测试）
  - 字节码级指令选择验证（4 测试）
  - 运行时语义验证（6 测试：局部/全局/遮蔽/upvalue 捕获/写回/链式）
  - FunctionStmt 表路径回归（3 测试）

本阶段验证结果：

- `lua_test.vcxproj` 成功编译（0 错误）
- `bin/lua_test.exe` 全部通过：51 个测试套件（含新增 Symbol Binding），0 失败
- 所有 Lua 回归测试通过

说明：

- `ResolvedSymbolMap` 和独立 `Resolver/Binder` 类的引入留给后续重构阶段
- 当前 `resolve()` 作为 CodeGenerator 公共方法，已实现名字绑定的单一真相来源

---

## PR-9 Remove ExprDesc

状态：`done`

本阶段实际产出：

- 将所有 16 处旧 ExprDesc 消费者迁移到原生通道：
  - `emitCallExpr`：func 表达式和实参从 `expr + exp2AnyReg` 迁移到 `emitValue + valueToAnyReg`
  - `emitStmt(AssignStmt/LocalStmt/ReturnStmt)`：值表达式从 `expr + discharge` 迁移到 `emitValue + dischargeValue`
  - `emitStmt(ForNumStmt)`：init/limit/step 从 `expr + exp2NextReg` 迁移到 `emitValue + valueToNextReg`
  - `emitStmt(CallStmt)`：fallback 路径从 `expr` 迁移到 `emitValue`
  - `emitLValue`：IndexExpr/MemberExpr 表/键求值从 `expr + luaK_dischargevars + exp2AnyReg` 迁移到 `emitValue + valueToAnyReg/valueToRK`
  - `emitStore`：签名从 `(LValueRef&, ExprDesc&)` 改为 `(LValueRef&, const ValueResult&)`
- 删除定义：
  - `ExprKind` 枚举（15 种）和 `ExprDesc` 结构体（`codegen_types.hpp`）
  - 13 个 `emitExpr(const XxxExpr&, ExprDesc&)` 重载
  - `expr()` / `discharge(ExprDesc&)` / `exp2RK` / `exp2AnyReg` / `exp2NextReg` / `exp2Val`
  - `luaK_dischargevars` / `luaK_indexed` / `luaK_storevar`
  - `adaptLegacyCondResult` + 6 个双向适配函数（`adaptLegacy*` / `valueResultToExprDesc`）
- 代码减少：
  - `codegen.hpp`：-32 行声明
  - `codegen_types.hpp`：-230 行（定义 + 适配器）
  - `codegen.cpp`：-416 行（旧实现）
  - 删除 `codegen.cpp.bak`
- 测试更新：
  - 删除 `test_codegen_result_types.cpp` 中的 "Legacy ExprDesc Adapters" 测试
  - 所有剩余测试保持通过

本阶段验证结果：

- `lua.vcxproj` / `lua_test.vcxproj` 全部成功编译
- `bin/lua_test.exe`：**51 个测试套件，0 失败**
- 所有 Lua 回归测试通过
- 代码库中 `ExprDesc`/`ExprKind` 引用：**0 处**
- 新人阅读 `codegen.cpp` 不再需要理解 Lua 5.1 风格 `expdesc` 状态机

---

## 风险最高的语义点

以下项目必须在每个阶段持续回归：

- 短路逻辑在“条件语境”和“值语境”之间切换
- `not` 对跳转链和普通值的双重语义
- `return f()` 与 `return (f())`
- `local a,b = f()` 与 `local a = f()`
- `g(f())` 只在最后一个参数位置传播 multret
- 表构造器最后一个数组字段的 multret 展开
- 方法调用 `obj:method()` 的 `SELF` 行为
- 循环中的 `break` 与局部变量作用域回收
- 闭包捕获的局部遮蔽与 upvalue 写回

## 合并门槛

每个 PR 合并前至少满足：

- [ ] 编译通过
- [ ] 该阶段新增测试全部通过
- [ ] 之前阶段的关键回归测试全部通过
- [ ] 未引入新的长期兼容层而没有后续删除计划
- [ ] 文档中的阶段状态已同步更新

## 建议的阶段状态跟踪

可以在提交 PR 时，把对应阶段改成以下状态之一：

- `planned`
- `in_progress`
- `blocked`
- `done`

建议在标题中沿用固定格式：

- `PR-0 Baseline & Guardrails`
- `PR-1 Introduce Result Types`
- `PR-2 CondResult Pipeline`
- `PR-3 LValue Pipeline`
- `PR-4 ValueResult Core`
- `PR-5 Call / Vararg / MultiRet`
- `PR-6 Composite Expressions Cleanup`
- `PR-7 Context Extraction`
- `PR-8 Symbol Binding`
- `PR-9 Remove ExprDesc`
