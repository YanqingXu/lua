---
status: current
verified_against: tests/unit/; tests/unit/framework/; tests/quality/test_signal_allowlist.json; tests/lua/; tests/lua/official/; tests/compatibility/; tests/lua/regressions/; tests/unit/vm/test_vm_trace_debug.cpp; tools/run_quality_gate.ps1; tools/check_test_signal_integrity.ps1; tools/check_test_binary_sha.ps1; tools/check_doc_drift.ps1; tools/check_lua51_official_sources.ps1; tools/run_lua51_official_strict.ps1; tools/test_quality_gate.ps1
last_checked: 2026-07-26
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
| Differential | 同一 Lua/C probe 在官方 Lua 5.1 与本解释器上的可观察行为对比 | `tests/lua/differential/`、`lua51_c_api_differential_probe.c`、`tools/run_lua51_*_differential.ps1` |
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
- 显式 include/exclude filter 的交集若选中 0 个已注册测试，runner 必须以退出码 2 失败；定向
  coverage 或合同任务不能因测试命名漂移而空跑成功。

测试结果分为 passed、failed、expected skip 和 unexpected skip。expected skip 只能通过
inline helper `SKIP_EXPECTED` 显式登记测试名和环境理由；unexpected skip 是阻断结果，不能靠汇总时忽略。
发布前的完整结果必须同时报告两类 skip，当前本地 Release 基线为 0 expected skips /
0 unexpected skips。

`tools/check_test_signal_integrity.ps1` 扫描 `tests/unit/**/*.cpp`，拒绝已知的
`ASSERT_TRUE(..., true)` 和手写 `[SKIP]`/`skipped` 成功输出。仅编译期 `static_assert`
汇总探针可进入精确 allowlist；每项绑定规则、文件、行号、文本 SHA-256、理由和失效日期，
新增、重复、过期、删除或文本变化都会使门禁失败。对应 fixture contract 会主动制造每种负例，
防止检查器只在当前源码上“碰巧通过”。

Strict runner 对 stdout 的 SHA 锁定 banner 做实时计时，而不向官方脚本注入 hook 或改写源码。该 profile 用来区分真正的慢脚本与状态活锁；门禁仍以进程退出码和 `final OK` 所在的原样执行结果为准，计时本身不作为易抖动的绝对性能阈值。

## 覆盖闭环

opcode matrix 是一种结构覆盖合同：每个 opcode 必须有 CodeGen 生产证据和 VM handler 消费证据。它不能替代行为测试，但能发现“有 handler 从不生成”或“生成了却没有 handler”的静态断裂。

Documentation Drift Check 进一步验证核心技术页存在、全部 Markdown 有事实头部、`verified_against` 路径仍存在，并通过真实测试输出校验 README 基线。这样文档证据和代码证据使用同一质量门。

候选发布证据使用
`tools/run_quality_gate.ps1 -Strict -FormatScope Changed -FormatBase <revision>`：环境依赖或
预期测试产物缺失即失败，范围是 base 至 HEAD 的 committed 文件与 staged、working、untracked
文件并集。干净工作树上的 Changed scope 没有显式 merge-base 时会失败，不能把“没有未提交文件”
误当成“提交内容已经检查”。`All` 会覆盖 `src/tests/examples/benchmarks/lua_test` 下全部自有
C/C++，只排除由 source-integrity manifest 保护的 `tests/lua/official/**`；当前仍用于量化历史
格式债，不在 RC 冻结期通过全仓机械重排来伪造低风险变更。完整门禁还会比较
`lua_test --build-info` 的构建 SHA 与当前 `HEAD`，拒绝旧二进制。
SHA 缺失、`unknown`、格式错误、非零 `--build-info` 或与预期提交不一致均为失败，并由临时
fixture 覆盖正负合同。质量门本身的显式 skip 参数只用于调用者有意拆分步骤，输出中必须保留
对应 `[SKIP]`；它与测试级 `SKIP_EXPECTED` 是不同概念，不能把裁剪后的运行描述为完整质量门。
