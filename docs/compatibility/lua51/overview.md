---
status: current
verified_against: docs/compatibility/lua51/language-feature-matrix.md; docs/compatibility/lua51/behavior-differences.md; docs/compatibility/lua51/known-incompatibilities.md; docs/compatibility/lua51/compare-with-official-lua.md; tests/lua/official/
last_checked: 2026-07-11
applies_to: Lua 5.1 compatibility implementation boundaries
---

# Lua 5.1 兼容性实现

本模块描述项目实现与官方 Lua 5.1 在语言特性、运行时行为、标准库和底层策略上的对应关系。这里记录技术边界，不维护完成百分比、阶段优先级或项目计划。

## 文档结构

- [语言特性矩阵](language-feature-matrix.md)：语法与语义特性对应的测试和实现机制。
- [行为差异](behavior-differences.md)：number、table、字符串、错误、GC 和 binary chunk 差异。
- [已知不兼容](known-incompatibilities.md)：无法直接互操作或语义不完全相同的边界。
- [与官方实现对比](compare-with-official-lua.md)：C 与现代 C++ 实现策略对照。
- [兼容性测试方法](compatibility-test-suite.md)：官方脚本、Golden Test 和回归测试的证据模型。

兼容性结论必须由源码和测试共同支撑；未定义行为应描述允许范围，而不是依赖某一次运行结果。
