---
status: current
verified_against: src/; tests/; examples/; benchmarks/; lua_test/include/; tools/; cmake/; .github/workflows/; CMakeLists.txt; .clang-format; CONTRIBUTING.md; THIRD_PARTY_NOTICES.md; docs/runtime/services.md; docs/runtime/public-runtime-api.md; docs/knowledge/source-document-map.md
last_checked: 2026-09-29
applies_to: coding-rule adoption inventory, existing-code boundaries and verification evidence
---

# 编码规则存量采用清单

本页是[本项目编码规则](README.md)的存量登记：记录具体位置、规则适用性、尚需核查的契约和后续验证，不记录产品进度。规则面向全部自有代码，首轮采用不等于完成全仓审查，也不授权一次性重写所有历史代码。

规则建立之后，已按追加要求执行自有 C++ 的等价类型专项迁移，范围和保留原因见下方“自有 C++ 类型迁移”。该专项不代表命名、注释、控制体花括号和复杂度等其他历史问题全部完成。

## 登记口径

- 2026-09-29 首轮通过版本控制文件清单、文本搜索和下列位置的人工阅读建立范围与样本证据；没有逐个审查所有文件、函数或模板实例。
- **已完成**仅表示该行描述的变更与所列验证完成；**待验证**表示实现已调整但验证未收口；**待适配**表示已发现可处理的差异；**待核对**表示具体契约或适用性仍待判定；**已核查保留**表示所列实例有明确保留理由；**未审查**不能推断为合规。
- 目录已纳入范围不等于目录全部合规。表中列出的是实证实例；其余内容按后文范围表保持未审查，不用样本通过率代表整体状态。
- 注释按职责、所有权、有效期、线程、错误与副作用是否表达充分评审。保留现有语言，不以英文说明、文件没有模板头注释或简单函数没有逐个注释认定问题。工具指令、许可证和测试数据保持其语义。
- 裸标准类型是候选检索结果，须先核对别名是否真正等价、公开接口及依赖边界。自定义 allocator、deleter、hash、GC edge 和 C ABI 不能按文本替换。

## 批次与退出条件

| 批次 | 范围 | 退出条件 |
|---|---|---|
| B0 | 规则入口、来源登记、基础别名与质量门语言标准 | 链接和元数据有效；别名以 C++23 编译和类型等价检查验证；工具变更完成针对性检查，并披露未执行项 |
| B1 | `src/` 内部等价类型、命名及具体契约差异 | 按模块小步核查，保持 C API/ABI、GC、allocator、线程和运行时可观察行为；相关单元、兼容或性能检查有真实结果 |
| B2 | 自有测试、示例、benchmark、独立测试框架、工具和脚本 | 每次按独立消费边界或运行入口处理；保留测试信号、fixture 意图、工具输出和性能测量协议 |
| B3 | 需要行为分析的复杂度、模块依赖和剩余未审查项 | 先证明问题和收益，再确定改动；指标只是评审提示，不因行数、参数数、嵌套或 `switch` 自动重构 |

批次表示审查顺序和风险边界，不是自动执行授权或完成期限。后续改动按[源码—文档—测试责任表](../knowledge/source-document-map.md)选择验证，并更新本页相关行；不得以宽泛“已统一”替代证据。

## 已定位的事项

规则号以本目录相应主题为准；基础类型项见[类型命名规则](type_naming_rules.md)，排版项见[排版规则](formatting_rules.md)。位置同时给出稳定符号，避免行号随修改漂移后失去定位依据。

| 编号／规则 | 位置 | 现状与处理边界 | 风险 | 批次 | 验证与状态 |
|---|---|---|---|---|---|
| A01／R-TYPES-01/02/03 | `src/common/types.hpp`：`Arr`、`Span`、`Expect`、`Unexpect`、基础整数与大小别名 | 首轮补充四个等价别名、限定标准类型命名空间，并将文件头标准基线改为 C++23；不新增平行 `Types.h` | 公共内部头的编译兼容与模板默认参数 | B0 | 已完成：MSVC、GCC、Clang 临时探针验证独立包含、类型等价、固定/动态/const Span 与普通/void Expect 成功及错误路径；后续调用点迁移另见类型专项记录 |
| A02／R-TYPES-02、R-PRINCIPLES-07、R-REFACTOR-08 | `tools/run_quality_gate.ps1`：clang-tidy smoke 参数 | 首轮将 smoke 语言标准与项目 C++23 对齐；该入口不是所有翻译单元的完整静态分析 | 工具解析标准与实际构建不一致 | B0 | 已完成：参数捕获断言恰好一个 C++23 标准参数；现有质量门合同测试、失败退出码传递与真实 clang-tidy 解析通过 |
| A03／R-TYPES-01/05 | `src/compiler/parser/parser.hpp::Parser::parse`；`src/compiler/codegen/codegen.hpp::CodeGenerator::tryGenerate`；`src/core/table.hpp::setArrayRange`；`src/runtime/lua_allocator.hpp::inlineData_` | 后续专项已改为 `Expect`、`Span`、`Arr`，并迁移其他等价类型调用点与补齐直接 include | 错误类型、元素 const、extent、签名与包含依赖 | B1 | 类型拼写与语义审查已完成；构建与测试证据见类型专项记录 |
| A04／R-TYPES-03、R-PRINCIPLES-03 | `src/runtime/lua_allocator.hpp::LuaVector`、`LuaOwnedPtr`；`src/core/function.hpp::ConstantMap` | 这些容器或指针携带自定义 allocator、deleter、hash；保留专用语义，不能替换成默认 `Vec`、`UPtr`、`HashMap` | 丢失 allocator 计费、释放路径或键比较语义 | B1 | 已核查本次类型候选中的全部自定义参数实例，保留位置见类型专项；不据此宣称全部所有权设计已完成审查 |
| A05／R-READABILITY-01、R-REFACTOR-02 | `src/runtime/runtime_configuration.hpp::RuntimeConfiguration`；`src/runtime/runtime_services.hpp::EngineContext`；`src/bytecode/bytecode_printer.cpp::CfgBlock/CfgEdge` | 当前全称表达配置/上下文；`Cfg` 已用于控制流图，保留既有清晰词汇，不执行 `Config/Context` 全量缩写 | 新缩写与编译器术语冲突；字符串或外部引用遗漏 | B1 | 已核查保留所列名称；新增命名按局部与跨模块含义检查 |
| A06／R-TYPES-04、R-REFACTOR-01 | `src/lua.h`、`src/lauxlib.h`、`src/lualib.h`、`src/lua_runtime.h::lua_RuntimeConfig`；`tests/compatibility/public_api_c_compile.c` | 公开 C 头、C 类型与 ABI 名称是兼容合同；保留 `size_t`、`uint*_t` 等 C 拼写，不引入内部 C++ 别名 | 破坏纯 C 消费、符号名称或结构布局 | B1/B2 | 已核查保留边界；未来改动验证 `lua51_public_api_contract`、静态/共享 consumer 和 C API 差分 |
| A07／R-READABILITY-02/08/09 | `src/lua_runtime.h` 文件/API 说明；`src/runtime/runtime_services.hpp::RuntimeServices/EngineContext` | 已有英文 SDK 说明与中文所有权说明；简单访问器未逐个注释不构成缺陷，不要求翻译或模板化补注释 | 重复说明漂移，或改写时遗漏真实调用约束 | B1 | 已核查语言政策与所列说明；全部函数契约是否充分仍未审查 |
| A08／R-PRINCIPLES-03/06、R-REFACTOR-05 | `docs/runtime/services.md` 的 Owner-thread 合同；`docs/runtime/public-runtime-api.md` 跨线程取消；`src/runtime/execution_policy.hpp::ExecutionCancellationHandle` | services 页仍称 handle 不能超过 context 生命周期；公开页与弱引用实现说明关闭后的迟到请求安全无操作，存在需统一的生命周期表述 | 宿主误解取消句柄的有效期或释放责任 | B1 | 待核对：以实现及关闭后取消测试核对内部/公开句柄边界，更新原权威页；本轮不改运行时或另造合同 |
| A09／R-PRINCIPLES-05、R-REFACTOR-01/07 | `docs/runtime/public-runtime-api.md` 执行窗口；`tests/unit/vm/test_runtime_services.cpp::testExecutionPolicySurvivesCToLuaReentry` | Lua→C→Lua 重入共享预算；只允许 owner-thread 访问 State，独立取消句柄可跨线程请求；不导入 Hunter 禁同步重入的业务约束 | 破坏已测重入、协程预算或取消语义 | B1 | 已核查保留现有合同和测试入口；本轮重新执行包含重入共享预算的现有测试，结果见下方验证记录 |
| A10／R-READABILITY-05、R-REFACTOR-06 | `src/bytecode/bytecode_printer.cpp::hasCompanionJump`；`tests/lua/regressions/test_short_circuit_materialization.lua` | 下标访问受左侧长度判断短路保护；Lua 回归脚本直接断言 RHS 调用次数 | 为拆条件提前求值会越界或改变副作用次数 | B1/B2 | 已核查保留表达式；未来结构调整验证 bytecode 测试及该回归脚本 |
| A11／R-REFACTOR-03/04/06 | `src/vm/vm.cpp` opcode 分派及 `src/vm/vm_handlers/` | 集中的 opcode `switch` 调用独立 handler；复杂度数字仅触发职责评审，不自动拆分或引入继承 | 改变分派、重入、safepoint 顺序或热点成本 | B3 | 未审查整体复杂度；如提出重构，先给调用/分派行为基线与 benchmark 对照 |
| A12／R-TYPES-04、R-READABILITY-02/08/09 | `tests/unit/framework/test_framework.hpp`；`lua_test/include/test_framework/test_framework.hpp` | Lua 测试适配层及自有单元测试已迁移等价类型；项目无关基础框架保留标准类型，不引入解释器内部头。英文说明和简单方法无注释不列违规 | 框架独立性、skip 分类及测试失败信号 | B2 | 类型边界已核对，验证见类型专项；其余可读性和契约规则仍按原范围登记 |
| A13／R-TYPES-04、R-REFACTOR-01 | `examples/embedding.cpp`、`examples/production_worker.cpp`；`tests/packaging/consumer/main.c` | 示例/worker 及安装包 consumer 用公开头展示或验证独立消费；不为短类型引入 `src/common/types.hpp` | 示例无法使用安装后的 SDK，掩盖包缺依赖 | B2 | 已核查保留消费边界；未来调整验证 `example_embedding`、`cmake_package_consumer` 与对应 worker 测试 |
| A14／R-READABILITY-01、R-TYPES-01/04、R-REFACTOR-06 | `benchmarks/runtime_bench.cpp::Config/Report/parseArguments`；`benchmarks/debugger_bench.cpp` | 两个 benchmark 的内部等价类型已迁移；allocator 回调保留 C 签名，`Config` 含义清楚，不强制缩写；计时位置与输出内容保持不变 | 修改数据结构或计时位置会污染性能对比 | B2 | 类型专项核对通过；现有 benchmark 合同验证见下方，未据此作性能优化声明 |
| A15／R-READABILITY-02/09、R-REFACTOR-01 | `tests/fuzz/fuzz_parser.cpp::LLVMFuzzerTestOneInput`；`tests/soak/runtime_soak.cpp` | fuzzer 的入口签名来自外部合同，异常吞掉原因已有英文说明；soak 已有中文职责说明，不能由语言推断契约不足 | 模糊预期解析错误与真实崩溃；破坏长稳负载 | B2 | 已核查保留入口和语言；fuzz/soak 的全部不变量与故障处理未审查，后续按现有目标验证 |
| A16／R-READABILITY-02/08/09、R-REFACTOR-01 | `tests/lua/` 自有脚本；`examples/hello.lua` 等示例；`tests/compatibility/*.lua`、`tests/production/*.lua`、`tests/fuzz/corpus/` | 小型示例、故障 fixture、断言和输出属于各自测试意图；不机械插注释或改行号、字符串和语法错误样本 | 影响诊断行号、stdout/golden 或负例有效性 | B2 | 未审查全量脚本契约；处理具体样本时联查引用与回归/示例/worker 入口，故意非法输入保持用途 |
| A17／R-READABILITY-02/09、R-REFACTOR-08 | `tools/run_clang_tidy.py`、`tools/check_doc_drift.ps1`；`tests/production/verify_worker_json.py` | 已有英文文档串及自说明入口；注释语言保持，不由正则生成职责说明。文档漂移脚本包含动态执行测试逻辑，不能当作纯链接检查 | 改变命令参数、退出码、质量门的失败信号或误报已验证 | B2 | 待核对各工具实际契约；按邻接 `tools/test_*` 和脚本调用方选择验证，其余工具未全审 |
| A18／R-FORMATTING-01/05、R-REFACTOR-08 | `.clang-format`；`tools/run_quality_gate.ps1`；`README.md` 质量门说明 | 本地规则为 120 列、Attach、保留 include 顺序；README 已披露全量格式历史债，首轮未执行全仓重排 | 无关 diff 混入行为修改；格式检查范围被误认为语义规则覆盖 | B1/B2 | 待适配历史格式；按模块先取得格式器真实输出，再修具体文件并复查 diff；全仓格式状态未验证 |
| A19／R-READABILITY-02/09、R-REFACTOR-08 | `CMakeLists.txt`、`cmake/`（含模板）；`tests/cmake/`、`tests/packaging/verify_package.cmake`；`.github/workflows/` | 包含 CMake 函数、英文策略说明和内嵌脚本，均纳入自有脚本范围；生成结果从源模板维护，不对 JSON/XML 数据强塞注释 | 构建/发布合同或生成结果与源模板失配 | B2 | 未审查全部脚本契约；修改时选择现有 CMake/工具合同检查，不触发真实发布作验证 |
| A20／R-FORMATTING-02、R-REFACTOR-06 | `src/vm/vm.cpp`：`reentry` 中 `ci.func` 越界、函数类型和栈容量检查 | 所列历史 `if` 与栈扩容 `while` 仍使用无花括号的单语句体；不是本轮全仓补括号授权 | 错配控制体、重排异常或栈刷新顺序 | B1 | 待适配：在明确 VM 格式批次补齐所列控制体，核对 token/控制范围并选择 VM 定向测试；格式器不能代替此项人工检查 |

## 全仓范围登记与未覆盖项

以下按版本控制中的目录和文件类别覆盖范围；每行覆盖该范围内除已定位事项外的全部适用规则。类型拼写专项的审查范围另见下方；其他没有逐项证据的规则继续保持“未审查”。

| 范围与规则 | 已有证据／剩余工作 | 风险与批次 | 后续验证 | 状态 |
|---|---|---|---|---|
| `src/`：全部编码、类型、排版、可读性和重构规则 | A01–A11、A18、A20 仅覆盖所列实例；lexer/parser/codegen、VM、core、GC、stdlib、API、debugger、I/O、app 其余位置尚未逐个核查 | C API、GC、allocator 与 Lua 语义；B1/B3 | 按源码责任表选择模块测试、兼容、sanitizer 或热点测量 | 未审查剩余范围 |
| 自有 `tests/unit/`、`tests/fuzz/`、`tests/soak/`、`tests/compatibility/`、`tests/packaging/`：全部适用规则 | A06、A09、A10、A12、A13、A15、A16、A19 是样本；未逐个审查其余驱动、fixture 和 C/C++ 文件 | 测试信号与外部入口；B2/B3 | 对应测试目标、纯 C/安装消费、完整性与脚本引用检查 | 未审查剩余范围 |
| 自有 `tests/lua/`、`tests/production/`、`tests/cmake/`：语言适用的命名、可读性和行为保持规则 | A10、A16、A17、A19；排除下列第三方目录，保留故障脚本、行号与输出合同 | 负例与故障矩阵失真；B2/B3 | Lua 回归、worker、CMake 对应入口 | 未审查剩余范围 |
| `examples/`、`benchmarks/`：全部适用规则 | A13、A14、A16；独立 SDK 示例与依赖核心的 benchmark 分别核对 | SDK 边界与测量协议；B2/B3 | 示例测试、benchmark 合同及必要的性能对照 | 未审查剩余范围 |
| `lua_test/include/`：全部适用 C++ 规则 | A12；保留项目无关测试框架边界，未审查整个头文件 | 框架依赖与断言结果；B2/B3 | 框架运行与测试信号完整性检查 | 未审查剩余范围 |
| `tools/`、根构建入口、`cmake/`、`.github/workflows/` 及自有模板/内嵌脚本：语言适用规则 | A02、A17、A19；规则范围不由现有 C++ 格式器或静态分析扫描根目录反推 | 构建与治理证据失真；B2/B3 | 邻接工具回归、模板生成一致性和相应构建合同 | 未审查剩余范围 |

## 第三方、生成物与外部合同

来源和许可的唯一登记见 [THIRD_PARTY_NOTICES.md](../../THIRD_PARTY_NOTICES.md)。本轮核对的排除范围如下，不把未知来源文件自动归为第三方：

| 范围 | 来源事实与处理 | 验证／状态 |
|---|---|---|
| `tests/lua/official/**` | 官方材料由 `tests/compatibility/lua51-official-sources.json` 锁定，项目添加的空目录占位文件除外；不做命名、注释或格式统一。自有 `tests/unit/official/` harness 不在此排除范围内 | 已核查来源边界；后续升级按官方来源完整性检查与登记流程处理 |
| `tests/lua/alien_signals/**` | 通知记录其直接来源提交和 MIT；`example.lua` 是本仓适配入口，仍属于已声明第三方材料目录。首轮整目录排除机械规范迁移，必要适配需保留来源与修改说明 | 已核查来源边界；不能把排除当成该目录行为已验证 |
| 公开 Lua API 名称、常量和 REPL 版本标识 | 属于已记录的派生兼容接口；自有实现仍纳入规则，契约名称不改 | 已核查保留兼容边界 |
| 构建产物、下载依赖、外部生成代码 | 不以本地存在即推定自有或应整改；版本控制中的自有生成模板仍适用，生成结果由模板及现有工具更新 | 未逐项审查工作区非源码产物；不纳入本页源码合规结论 |

## 自有 C++ 类型迁移

2026-09-29 追加执行 B1/B2 的等价类型专项。相对规则建立后的源码快照，修改 249 个 C++ 文件：`src/` 171 个、自有测试 76 个、benchmark 2 个。连同基础类型头的首轮补齐，本次工作区共有 250 个 C++ 文件变更。未修改独立 SDK 示例、纯 C 文件、上游锁定材料或生成输出。

迁移覆盖 `Str/StrView/CharPtr`、固定宽度整数和大小类型、`f32/f64`、默认容器和智能指针、`Var/Opt/Func`、`Arr/Span/Expect/Unexpect`、`Atom/Mtx/ScopedLock` 的完全等价用法；保留已有领域类型。使用别名的迁移文件直接包含定义，命名空间外明确限定。为兼容 Clang 18，结果构造和数组推导位置使用已核对的显式错误类型、元素类型与长度，不引入新的推导或工厂抽象。

代码 token 对照排除了预期别名展开、显式模板实参和必要 include 后一致；字符串、字符值、宏条件、调用与控制流保持不变。修改文件按现有格式器处理，其中原有混合换行文件随格式器规范化；长 trace golden 字符串仅拆为相邻字面量，拼接内容逐项核对相同。

### 保留的类型写法与原因

以下是对可映射类型候选的逐项分类结果，不将目录整体当作豁免。没有对应别名的 `int`、`char`、`unsigned char`、`bool`、`void`、`long double`、SDK 句柄、流、chrono、类型特征及标准库函数继续使用准确的原类型或名称。

| 位置 | 保留原因 |
|---|---|
| `src/common/types.hpp` | 标准类型是别名定义的唯一来源，不能自我替换 |
| 公开 C 头、`src/api/lapi.cpp`/`lauxlib.cpp` 和 `src/lib/debuglib.cpp` 的公开定义，以及 reader/writer/allocator 回调、六个 `LLVMFuzzerTestOneInput` 入口 | 保留 C/SDK 合同签名；实现内部可等价替换的位置已迁移，`va_arg` 保留 ABI 提升后的准确类型 |
| `src/core/function.hpp::ConstantMap`、`src/core/string_pool.hpp::PoolMap`、`src/debugger/breakpoint_manager.cpp/.hpp` 的 `InstructionKeyHash` 映射 | 自定义 hash、比较器或 allocator，默认 `HashMap` 不等价 |
| `src/runtime/lua_allocator.hpp::LuaVector/LuaOwnedVector/LuaOwnedPtr` | 专用 allocator/deleter 定义，保留计费、生命周期与释放语义 |
| `src/core/thread.hpp`、`src/core/userdata.hpp`、`src/vm/state/lua_state.cpp/.hpp`、`src/lib/iolib.cpp`、`src/lib/oslib.cpp` | 分别保留 `LuaStateOwnerDeleter`、`UserdataBufferDeleter`、`SnapshotDeleter`、`EngineContextDeleter`、`FileCloser` 和 `std::free`；不能降为默认 `UPtr` |
| `examples/embedding.cpp`、`examples/production_worker.cpp`、`tests/compatibility/public_api_cpp_consumer.cpp`、`tests/compatibility/public_native_module_host.cpp`、`tests/packaging/consumer/` | 独立公开 SDK 消费，不引入 `common/types.hpp` 内部依赖 |
| `lua_test/include/test_framework/test_framework.hpp` | 项目无关基础框架；Lua 适配层 `tests/unit/framework/` 已迁移 |
| `test_codegen_result_types.cpp`、`test_codegen_state.cpp`、`test_parser_error_recovery.cpp`、`test_lib_catalog.cpp`、`test_runtime_services.cpp`、`test_vm_internal_boundaries.cpp` 中的精确类型断言 | 标准类型作为独立比较侧，继续验证领域返回类型的底层合同 |
| `src/debugger/remote_protocol_generated.hpp` | 声明由 `Debugger/tools/generate_protocol.py` 生成，该生成源未随当前仓库版本化；保留生成结果，不手工修改 schema 常量 |
| 注释、测试脚本/JSON 字面量及工具内嵌 fixture | 文本中的标准类型名称不是 C++ 类型声明，保持原有说明和测试数据 |

类型迁移后的工具适配仅用于保持原检查信号：`ValueResult` 的质量门断言使用现有 `Var` 拼写；C-style 扫描识别 `CharPtr` 的等价数组和游标声明，并用正反例回归验证没有漏检。C-style 基线仍为 1262 项，仅更新 1020 个行号和 5 个等价类型改写的文本指纹；测试信号基线仍为 5 项，仅随直接 include 更新行号。两份基线的规则、路径、理由和条目数均未改变，没有增加例外或新增类型自动检查器。

### 类型迁移后的验证

以下证据均来自类型调用点迁移后的源码；构建使用 C++23，MSVC 对应 `/std:c++latest`。

| 验证入口与环境 | 类型迁移后的实际结果 |
|---|---|
| Windows/MSVC 19.51，CMake Debug 构建与默认 CTest | 构建通过；46/46 CTest 通过；`lua_test` 选中 819 个测试，7127 项结果通过，0 失败/skip |
| Linux/GCC 14.2，CMake Debug 构建与默认 CTest | 构建通过；首次 45/46，仅测试信号基线的 5 个旧行号因新增直接 include 失配；核对原断言与哈希不变、修正行号后，该单项补跑 1/1 通过，46 项均取得通过证据 |
| Linux/Clang 18.1.3 与 libc++ 18，CMake Debug 构建与默认 CTest | 构建通过；46/46 CTest 通过；导出与 CI 一致的编译器及标准库环境，安装消费者通过 |
| 两个 Linux 环境的 `lua_test` | 均选中 819 个测试，7045 项结果通过、0 失败、1 项预期 skip（本机缺少葡萄牙语 LC_COLLATE locale），0 非预期 skip |
| Clang 18/libc++ 18，两个 benchmark | `lua_runtime_bench`、`lua_debugger_bench` 构建通过，现有 `runtime_benchmark_contract` 与 `debugger_benchmark_contract` 2/2 通过；未作迁移前后性能对比 |
| Clang 19.1.1 与 libstdc++ 14，六个 fuzz 目标 | 六个目标构建成功，复制现有 corpus 后各执行固定 seed 1 的 200 runs；全部退出码为 0，ASan/UBSan 无失败，原始 14 个 corpus 文件哈希不变；仅为短程 smoke |
| `tools/test_quality_gate.ps1` | 全部质量门合同通过，包括 C++23 参数捕获、失败退出码传递、别名写法仍受位置/文本基线约束的正反例 |
| `tools/run_quality_gate.ps1` 严格模式 | 使用本次 MSVC 构建，跳过重复 MSBuild；250 个变更 C++ 文件的 LLVM 18.1.8 格式检查、LLVM 22.1.3 真实 C++23 tidy smoke、来源/API/文档/测试信号与程序 SHA 检查通过；832 个注册测试、7140/7140 项结果通过，0 失败/skip |

初次严格质量门使用本机 LLVM 22 格式器时，三个文件因与 CI 的 LLVM 18 排版输出不同而失败；最终明确使用 LLVM 18.1.8 格式器及 LLVM 22.1.3 静态分析器，不改变格式配置、扫描范围或失败条件。迁移后的运行证据覆盖公开 C/C++ 消费、静态/共享与安装消费者、allocator 失败恢复、GC 生命周期和 OOM、Lua→C→Lua 重入预算，以及现有 soak smoke。

完整构建与运行日志保存在本地忽略目录 `build/type-alias-migration/validation/`，逐模块类型审查记录位于 `build/type-alias-migration/`。没有执行长时 Nightly、完整 Release/sanitizer 矩阵或性能对比；本次不修改工作流调度、CI runner 配置及 15 项 CI 要求，也不据这些结果宣称其他历史编码规则已全部整改。

## 规则建立阶段验证（调用点迁移前）

- 已完成：版本控制路径盘点、表中具体符号/声明/文档的人工对照、Hunter 固定提交 `LICENSE` 的来源核对，以及 37 条规则字段/编号、121 个相对链接和文档事实头检查。
- 独立类型头探针：MSVC 19.51.36256、GCC 14.2.0、Clang 18.1.3/libc++ 18 编译与运行通过。探针仅存在于忽略的构建目录，覆盖 A01 的类型和行为边界；CMake 请求 C++23，本机 MSVC 映射为 `/std:c++latest`，GCC/Clang 使用 `-std=c++23`。

| 验证入口与环境 | 规则建立阶段结果 |
|---|---|
| Windows/MSVC 19.51，CMake Debug 构建与默认 CTest 集合 | 构建通过；46/46 CTest 通过；其中 `lua_test` 选中 819 个测试，7127 项断言结果通过，0 失败/skip |
| Linux/GCC 14.2，CMake Debug 构建与默认 CTest 集合 | 构建通过；46/46 CTest 通过；`lua_test` 选中 819 个测试，7045 项结果通过、0 失败、1 项预期 skip（缺少葡萄牙语 LC_COLLATE locale），0 非预期 skip |
| Linux/Clang 18.1.3 与 libc++ 18，CMake Debug | 构建通过；首次 CTest 45/46，安装消费者因本轮验证未导出 CI 使用的 `CC/CXX/CXXFLAGS` 而误用 GCC/libstdc++；仅重建该消费者缓存并对齐环境后补跑 1/1 通过，46 项均取得通过证据。`lua_test` 7045 项结果通过、0 失败、1 项预期 locale skip，0 非预期 skip |
| `tools/test_quality_gate.ps1` | 参数捕获、错误退出码传递及现有质量门合同测试通过 |
| `tools/run_quality_gate.ps1` 严格模式，使用上述新 MSVC 测试程序 | 已先完成 CMake/MSVC 构建，此处跳过重复 MSBuild；Changed 范围格式检查、LLVM 22.1.3 真实 C++23 tidy smoke、现有边界/来源/API/文档检查和测试程序 SHA 检查通过；全量单测 832 registered、7140/7140 结果通过，0 失败/skip |
| 修改文件检查 | `types.hpp` 通过 clang-format 18.1.8 与 22.1.3；PowerShell 变更通过现有合同测试；Markdown 链接、事实头及 `git diff --check` 通过 |

该阶段已通过的运行时证据包括公开 C 编译与 C++ 消费、静态/共享库及安装消费者、allocator 失败与恢复、GC 生命周期与 OOM 传播、Lua→C→Lua 重入共享预算。规则建立阶段没有改变工作流调度、runner、15 项 CI 要求或现有白名单；后续迁移的位置基线更新见上方专项记录。

规则建立阶段未执行全仓规则语义审查、全仓格式修复、Release 与 sanitizer 矩阵、长时 Nightly 或性能对比；默认 CTest 通过不代表这些范围通过，也不代表全部自有代码已合规。
