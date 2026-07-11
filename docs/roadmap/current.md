---
status: current
verified_against: docs/status/project-status.md; docs/roadmap/lua51-compatibility-next-stage.md; docs/roadmap/modern-cpp-teaching-audit-report.md; docs/guides/development.md; docs/knowledge/source-document-map.md; docs/vm/instruction-set.md; src/compiler/codegen/codegen_types.hpp; src/compiler/opcode.hpp; src/vm/vm_switch_dispatch.hpp; tests/unit/compiler/test_codegen_characterization.cpp; tests/unit/vm/opcode_coverage_matrix.md; tools/check_doc_drift.ps1; tools/check_c_style_patterns.ps1; tools/check_opcode_coverage_matrix.ps1; tools/test_quality_gate.ps1; tools/run_quality_gate.ps1
last_checked: 2026-07-11
applies_to: 当前仓库优化状态、续接检查清单与下一步任务
---

# Lua 仓库当前优化路线图

本文只记录可执行的当前事实和未完成任务。PR-1 至 PR-78 的详细完成史已归档到
`docs/archive/roadmap/optimization-history-through-pr78.md`。

## 下次续接检查清单

开始任何续接任务前运行：

```powershell
git status --short
powershell -NoProfile -ExecutionPolicy Bypass -File tools\check_doc_drift.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\test_quality_gate.ps1
```

`check_doc_drift.ps1` 会执行 `bin\lua_test.exe` 并动态解析测试汇总。如果测试可执行文件不存在，
先构建 `lua_test.vcxproj`。如果任务改变 C++ 行为，完成前还必须运行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
```

本机缺少 `clang-format` 或 `clang-tidy` 时，完整质量门会明确报告跳过对应步骤；MSBuild、
文档漂移和单元测试仍是 Windows 主路径的必要证据。

## 当前事实

<!-- live-facts:start -->

- 工作目标：最大化兼容 Lua 5.1.5，同时保持现代 C++23 教学示范价值。
- 声明边界：项目测试与官方 staged smoke 全绿不等于完整 Lua 5.1.5 等价。
- 最近一次完整绿跑：668 个 registered tests、3406 个 assertion results、0 failures。
- VM 使用 Lua 5.1 风格的 38 条 opcode；enum、metadata、switch handlers 和覆盖矩阵已有结构契约。
- `ValueResult` 已完成 variant-only 迁移；旧 mirror 字段、`legacyFields()`、drift probe、
  private-trial 宏和 mutable payload 写入口均已删除。
- Windows + Visual Studio/MSBuild + `.vcxproj` 是主要可复现路径；CMake/CTest 是 secondary 路径。
- `IncrementalGC` 仍是教学占位策略；官方 binary chunk、`testC`、完整 codegen parity 和生产路径
  singleton fallback 收口仍未完成。

<!-- live-facts:end -->

项目构建、测试和兼容性事实以 `docs/status/project-status.md` 为单一事实源；兼容性工作流以
`docs/roadmap/lua51-compatibility-next-stage.md` 为准。

## 本轮已收口风险

### P1：C-style 数量门（已收口）

`tools/check_c_style_patterns.ps1` 已使用 `tools/c_style_allowlist.json` 的
`rule/path/line/textHash/rationale` 位置基线。产品严格规则不再依赖 `AllowedCount`；同数量换位置的
回归由配置烟测锁住。测试源码中的手工所有权继续作为带显式基线的 advisory，允许后续递减。

### P1：opcode 测试引用漂移（已收口）

`tools/check_opcode_coverage_matrix.ps1` 除了验证 38 行、enum 顺序、metadata group 和
`mayInvokeMetamethod`，还会读取 `opcode_coverage_contract.json`，并通过
`bin\lua_test.exe --list` 验证 Positive / Boundary / Metamethod 精确测试 ID 与源码路径。

### P2：current 活动事实漂移（已收口）

`tools/check_doc_drift.ps1` 会自动发现 front matter 为 `status: current` 的文档，检查全部本地
`verified_against` 路径；`live-facts` 区块还验证动态测试摘要、已删除 ValueResult 兼容面和依赖
最后提交日期。历史完成记录已使用 `status: historical` 隔离。

## 下一步任务（按顺序）

1. **文档同步：活动事实收口。**
   - [x] 将 PR-78 以前的详细完成史移入 archive。
   - [x] 同步兼容路线、开发指南和 VM 指令文档中的活动基线与验证页眉。
2. **工具链优化：JSON 化 C-style 位置基线。**
   - [x] 使用 `rule/path/line/textHash/rationale` 记录允许位置。
   - [x] 提供受控 `-UpdateBaseline`，并用配置测试证明同数量换位置会失败。
   - [x] 测试目录先维持 advisory，但建立可递减的显式基线。
3. **工具链优化：opcode 可执行引用契约。**
   - [x] 增加机器可读 sidecar，以精确 `Suite::Test` ID 描述覆盖。
   - [x] 使用 `bin\lua_test.exe --list` 验证注册名，验证引用源码路径存在。
4. **工具链优化：文档漂移增强。**
   - [x] 自动发现 `status: current` 文档并检查全部 `verified_against` 路径。
   - [x] 对活动事实区校验测试摘要、已删除兼容符号和依赖提交日期，历史归档不参与活动事实断言。
5. **行为变更：L51-0406 codegen parity 窄切片。**
   - [x] 实现顶层、未捕获、后续不可读且不处于控制块中的 local nil dead-store elision。
   - [x] characterization 锁住 dead store、future read、captured local 和 loop 可观察边界；完整质量门作为本轮最终验收。

本轮完成后的下一行为工作仍是 L51-0406 动态 boolean/jump normalization；更大的兼容工作按
`docs/roadmap/lua51-compatibility-next-stage.md` 继续推进 Lua C API / `testC`、官方 binary chunk、
GC 调度和 runtime singleton fallback 收口。

## 教学价值验收

后续改动不以“使用更多新语法”为目标，而应让隐含约束成为可编译、可测试或可脚本验证的契约：

- C/GC 边界的例外必须有位置、哈希和理由，展示何时保留 C 风格代码是正确选择。
- opcode metadata、实现、测试注册和可读矩阵必须形成可追踪闭环。
- 文档必须区分语义正确、字节码形状一致、staged smoke 通过和完整 Lua 5.1.5 等价。

## 维护规则

每完成一个任务：

1. 更新本文的勾选项、当前风险和下一步任务。
2. 如仓库事实变化，同步 `docs/status/project-status.md`。
3. 行为变化同步对应架构/VM/编译器文档及回归测试。
4. 运行 `tools\check_doc_drift.ps1`；行为变化再运行 `tools\run_quality_gate.ps1`。
