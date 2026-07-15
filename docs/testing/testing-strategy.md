---
status: current
verified_against: tests/unit/; tests/unit/framework/; tests/lua/; tests/lua/official/; tests/compatibility/; tests/lua/regressions/; tests/unit/vm/test_vm_trace_debug.cpp; tools/run_quality_gate.ps1; tools/check_doc_drift.ps1; tools/check_lua51_official_sources.ps1; tools/run_lua51_official_strict.ps1; tools/test_quality_gate.ps1
last_checked: 2026-07-15
applies_to: 解释器测试分层、Golden 与回归证据
---

# 解释器测试分层、Golden 与回归证据

测试文档只解释验证模型，不复制命令清单或易漂移的用例数量。测试数量由运行器动态报告，文档漂移脚本只验证公开基线与真实执行一致。

## 测试金字塔

| 层次 | 目的 | 典型目录 |
|---|---|---|
| C++ unit | 数据结构、状态机、错误类型和边界 API | `tests/unit/` |
| Lua behavior | 语言可观察语义和跨模块组合 | `tests/lua/basic/`、`functions/`、`tables/`、`runtime/` |
| Official smoke | 受控改写、压力缩减和分阶段执行的快速回归 | `tests/unit/official/`、`tests/compatibility/lua51-official-smoke-deviations.json` |
| Official strict | SHA-256 锁定、临时副本中原样执行的 Lua 5.1 Release required gate；同时输出逐脚本 `stageProfile` | `tests/lua/official/`、`tools/run_lua51_official_strict.ps1` |
| Official TestC | 打开内部 `T` 模块后执行 `api.lua`，并在 SHA 锁定的 5.1.5 `luac` oracle 校正后执行 `code.lua` | `tests/unit/official/`、`lua51-official-sources.json`、`lua51-official-testc-xfails.json` |
| Differential | 同一探针在官方 Lua 5.1 与本解释器上的可观察行为对比 | `tests/lua/differential/`、`tools/run_lua51_differential.ps1` |
| Regression | 每个已修缺陷的最小稳定复现 | `tests/lua/regressions/` |
| Golden | 结构化且有意稳定的复杂输出 | `test_vm_trace_debug.cpp` 中的 trace golden cases |

## 单元与行为测试的分工

单元测试锁定内部不变量，例如寄存器释放、jump patch、open upvalue 唯一性和 GC phase 转移。Lua behavior test 锁定最终值、错误对象、副作用和库行为，不应引用 C++ 类名或内部 PC。

一个跨层修复通常需要两种证据：内部测试证明修复发生在正确机制，Lua regression 证明用户可观察行为不再回退。

## Golden 测试

Golden 适合结构复杂但本身应稳定的结果，例如反汇编或规范化 JSONL trace。使用条件：

- 输出已有明确 schema；
- 排除地址、unordered 顺序、计时和环境路径；
- diff 能定位语义字段，而不是整页格式噪声；
- 更新 golden 必须审阅变化原因，不能把“测试通过”当作批准。

普通错误文案、调试展示和对象地址通常不适合逐字 golden。优先断言错误类别、source/line、opcode、changed registers 等字段。

## Regression 设计

每个 regression 应最小化为一个主要语义点，并记录预期结果。命名描述行为而不是 issue 编号。若故障横跨 Compiler 与 VM，可以同时保存 Proto/trace 单元证据，但 Lua 脚本仍是最终合同。

高价值边界包括：

- lexer lookahead 与 parser error recovery；
- short-circuit、jump backpatch 和 scope close；
- CALL/RETURN/VARARG 的 fixed/open 数量；
- closure 在正常 return、break、tailcall 和异常路径关闭；
- metamethod 链、循环保护与错误对象；
- weak table/finalizer 跨多个 GC cycle；
- official Lua 脚本中的组合语义。

## 确定性与隔离

- 每个测试拥有独立 LuaState 或明确重置全局状态。
- 不依赖 unordered 遍历、地址、线程调度或宿主 locale。
- GC 测试显式触发阶段并观察语义，不断言脆弱的分配次数。
- trace/golden 对输出排序和对象 ID 做规范化。
- 测试失败必须返回非零状态，脚本不能只扫描“看起来成功”的文本。

Strict runner 对 stdout 的 SHA 锁定 banner 做实时计时，而不向官方脚本注入 hook 或改写源码。该 profile 用来区分真正的慢脚本与状态活锁；门禁仍以进程退出码和 `final OK` 所在的原样执行结果为准，计时本身不作为易抖动的绝对性能阈值。

## 覆盖闭环

opcode matrix 是一种结构覆盖合同：每个 opcode 必须有 CodeGen 生产证据和 VM handler 消费证据。它不能替代行为测试，但能发现“有 handler 从不生成”或“生成了却没有 handler”的静态断裂。

Documentation Drift Check 进一步验证核心技术页存在、全部 Markdown 有事实头部、`verified_against` 路径仍存在，并通过真实测试输出校验 README 基线。这样文档证据和代码证据使用同一质量门。

本地发布证据使用 `tools/run_quality_gate.ps1 -Strict`：环境依赖或预期测试产物缺失即失败。显式 skip 参数只用于调用者有意拆分门禁的场景，输出中必须保留对应 `[SKIP]`，不能把裁剪后的运行描述为完整质量门。
