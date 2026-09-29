---
status: current
verified_against: .clang-format; .clang-tidy; tools/run_quality_gate.ps1; tools/run_clang_tidy.py; tools/check_c_style_patterns.ps1; tools/check_doc_drift.ps1; .github/workflows/ci.yml; src/common/types.hpp; THIRD_PARTY_NOTICES.md; docs/runtime/execution-policy.md; docs/runtime/memory-contract.md
last_checked: 2026-09-29
applies_to: coding-rule scope, precedence, incremental adoption, and actual verification coverage for repository-owned code
---

# 编码规则入口

本目录是本项目自有代码的编码基线。新增和本次实际修改的代码按适用条款执行；历史代码按责任区逐批整改，进度见 [采用状态](adoption-status.md)。规则发布、一次检查通过或某个目录完成整改，都不表示全仓已合规。

## 阅读与适用范围

| 文档 | 负责内容 |
|---|---|
| [编码原则](coding_principles_rules.md) | 意图、职责、依赖、资源与失败保证 |
| [类型命名](type_naming_rules.md) | `Lua` 命名空间内的统一别名及边界例外 |
| [排版](formatting_rules.md) | 现有格式配置、显式花括号和修改范围 |
| [可读性](readability_rules.md) | 命名、职责注释、函数组织和条件表达 |
| [重构](refactoring_rules.md) | 行为保持、小步修改、复杂度评审与验证 |

### R-SCOPE-01 按所有权与语言应用规则

- **等级：MUST。**
- **范围：** 全部自有代码，包括核心、API 适配、应用、测试、示例、基准、工具及构建脚本；自有代码生成模板和生成源同样适用。
- **要求：** 新增和实际修改部分遵守适用规则；识别同一变更直接影响的调用方、模板及合同。通用设计、可读性和重构原则适用于各语言，C++ 排版与类型别名仅用于 C++。自有生成代码从生成源修改并重新生成。
- **例外：** `tests/lua/official/` 与 [第三方声明](../../THIRD_PARTY_NOTICES.md) 标明的上游材料保留来源、许可证及生成合同，不作本次规范整改；自有适配层仍适用。构建产物不作手工整改。纯排版触及文件不意味着必须顺带重构其全部历史代码。
- **验证：** 人工检查 diff、所有权和生成来源；核对受影响单元与 [采用状态](adoption-status.md)。格式工具按整份选中文件检查，不能把“仅改几行”当作其豁免。

## 等级与合同优先级

- **MUST**：满足范围和触发条件时必须遵守；仅可使用条款列出的例外。MUST 不等于已有自动检查。
- **SHOULD**：默认采用；偏离时在就近说明或变更说明中给出具体理由。
- **REVIEW**：调查与评审信号；指标或搜索命中不直接判错，也不自动授权扩大修改范围。

### R-SCOPE-02 通用风格不得改写运行时合同

- **等级：MUST。**
- **范围：** 编码、重命名、类型替换、格式化和重构。
- **要求：** 保持已有语义和兼容边界；发现规则与合同冲突时以明确的接口、所有权和运行时合同为约束，修正通用规则的适用解释。确需行为变化时单独说明并更新对应权威文档。
- **例外：** 经任务明确要求的行为变更可以修改合同，但不能仍宣称是纯排版或行为保持型重构。
- **验证：** 依照下列权威入口选择受影响检查；不在本目录复制第二套语义规范。

| 边界 | 权威入口 |
|---|---|
| Lua 语言与公开 C API、ABI | [Lua 5.1 兼容性](../compatibility/lua51/overview.md)、[C API 覆盖](../compatibility/lua-c-api-coverage.md)、[公开运行时 API](../runtime/public-runtime-api.md) |
| 服务与状态归属、重入和执行窗口 | [运行时服务](../runtime/services.md)、[执行治理](../runtime/execution-policy.md)、[VM Runtime](../vm/runtime/overview.md) |
| GC edge、root、allocator 与失败恢复 | [GC 概览](../gc/overview.md)、[GC 实现](../gc/implementation.md)、[内存合同](../runtime/memory-contract.md) |
| 编译器职责与控制流 | [代码生成职责](../compiler/codegen-responsibility-map.md)、[控制流 lowering](../compiler/control-flow/overview.md) |
| 验证分工 | [测试策略](../testing/testing-strategy.md)、[贡献说明](../../CONTRIBUTING.md) |

## 当前工具实际覆盖

| 检查入口 | 已覆盖内容 | 不能据此证明的内容 |
|---|---|---|
| [本地质量门](../../tools/run_quality_gate.ps1) 的 `FormatScope Changed/All` | 对 `src/`、`tests/`、`examples/`、`benchmarks/`、`lua_test/` 中 `.c/.cc/.cpp/.cxx/.h/.hpp` 按根 [格式配置](../../.clang-format) 检查；排除官方 Lua 目录。Changed 选中变更文件后检查文件整体；All 检查所列范围全部文件 | 不是所有自有文件、所有语言或仅变更行检查；不证明显式花括号、命名、注释和语义全部合规 |
| 本地质量门的 `clang-tidy smoke` | `Get-ClangTidyFiles` 现存白名单文件，独立参数下的有限 smoke；使用 `-std=c++23` | 不继承主目标完整编译参数，不是主构建验证、编译数据库全量分析或本次变更全覆盖 |
| [Linux CI](../../.github/workflows/ci.yml) 与 [run_clang_tidy.py](../../tools/run_clang_tidy.py) | 格式任务检查固定样本及事件选中文件整体；tidy 从编译数据库选取 `src/tests/benchmarks/examples` 的 C++ 翻译单元，运行 `HIGH_SIGNAL_CHECKS` 子集 | 并非执行 [.clang-tidy](../../.clang-tidy) 中所有检查，也不覆盖未进入数据库的目标与所有头文件组合 |
| [C-style 检查](../../tools/check_c_style_patterns.ps1) | 仅 `src/`、`tests/` 下 `.cpp/.hpp/.h` 的指定文本模式，按 Product/Tests/All 范围与 [位置基线](../../tools/c_style_allowlist.json) 对照 | 不证明所有 C 风格、所有权、C ABI 或短类型规则正确；基线记录不等于推荐写法 |
| [文档漂移检查](../../tools/check_doc_drift.ps1) | 文档事实头、引用及脚本定义的专项事实检查 | 不证明规则语义、注释质量或全部文档事实正确 |
| 变更自检与人工评审 | 类型等价、例外、命名、职责注释、依赖、控制体花括号及行为合同 | 需要实际阅读与针对性证据，不能用“质量门通过”替代 |

验证执行方式沿用 [贡献说明](../../CONTRIBUTING.md)。记录真实范围、结果与未运行项；不为清除报错自动扩大基线或放宽门禁。

## 参考来源与本地取舍

本组规则参考 [Hunter 的 server/rules](https://github.com/YanqingXu/Hunter/tree/ad1f7841f0fb3013898d099e6e8ed22eea658a6b/server/rules) 与 [Types.h](https://github.com/YanqingXu/Hunter/blob/ad1f7841f0fb3013898d099e6e8ed22eea658a6b/server/src/common/Types.h)，固定提交为 `ad1f7841f0fb3013898d099e6e8ed22eea658a6b`。

采用其意图表达、显式控制体、统一类型来源和小步重构思路；本地继续使用 4 空格、120 列与 Attach 花括号。注释以职责和调用合同为准，不要求全量中文或为简单函数添加重复说明；保留 `Cfg` 表示控制流图的既有术语，不强制机械缩写。Hunter 的游戏状态、线程、Android、存储和脚本宿主约束不迁入本项目。
