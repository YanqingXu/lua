# Third-Party Notices

本项目的自有代码使用根目录 [MIT License](LICENSE)。以下材料或派生接口保留其各自的 MIT 版权声明。

## Lua 5.1 official materials

- 范围：`tests/lua/official/**`（项目添加的空目录占位文件除外），以及为兼容 Lua 5.1.5 而采用的公开 API 名称、常量和 REPL 版本标识。
- 来源：Lua 5.1 官方测试套件 `https://www.lua.org/tests/lua5.1-tests.tar.gz`，发布于 2016-01-18，SHA-256 `49e4ca6561f82ea605908c5041ab5fad66ed9930fa0686675bd51b02767f18ad`。
- 参考实现：Lua 5.1.5 源码包 `https://www.lua.org/ftp/lua-5.1.5.tar.gz`，SHA-256 `2640fc56a795f29d28ef15e13c34a47e223960b0240e8cb0a82d9b0738695333`。
- 修改：仓库中的官方测试文件由 `tests/compatibility/lua51-official-sources.json` 逐文件锁定，不直接改写。测试 harness 会按已登记清单创建临时副本、缩减压力，或在内存中应用 Lua 5.1.5 oracle 校正；这些变更不会覆盖上游 fixture。
- 许可证：https://www.lua.org/license.html

Copyright (C) 1994-2012 Lua.org, PUC-Rio.

## Alien Signals Lua test material

- 范围：`tests/lua/alien_signals/**`。
- 直接来源：`https://github.com/YanqingXu/alien-signals-in-lua`，提交 `3996d71d0a0e6ca393d6c50fbd92d17654f14374`；来源项目声明兼容 `alien-signals` v3.2.1。
- 上游算法与 TypeScript 实现：`https://github.com/stackblitz/alien-signals`，版本 v3.2.1。
- 修改：`constants.lua`、`engine.lua`、`graph.lua`、`init.lua`、`primitives.lua`、`scheduler.lua`、`tracer.lua` 和 `bit.lua` 与上述直接来源提交的对应 blob 一致；本仓库的 `example.lua` 是用于解释器集成验证的适配入口。
- 许可证：两个来源项目均为 MIT。

Copyright (c) 2025 YanqingXu

Copyright (c) 2024-present Johnson Chu

## Hunter coding rules and type aliases

- 来源：[YanqingXu/Hunter](https://github.com/YanqingXu/Hunter/tree/ad1f7841f0fb3013898d099e6e8ed22eea658a6b)，固定提交 `ad1f7841f0fb3013898d099e6e8ed22eea658a6b`。
- 参考和改编范围：该提交的 `server/rules/README.md`、`server/rules/coding_principles_rules.md`、`server/rules/formatting_rules.md`、`server/rules/readability_rules.md`、`server/rules/refactoring_rules.md`、`server/rules/type_naming_rules.md`，以及 [`server/src/common/Types.h`](https://github.com/YanqingXu/Hunter/blob/ad1f7841f0fb3013898d099e6e8ed22eea658a6b/server/src/common/Types.h)。规则原文见[固定版本目录](https://github.com/YanqingXu/Hunter/tree/ad1f7841f0fb3013898d099e6e8ed22eea658a6b/server/rules)。
- 本项目范围：`docs/rules/` 中的规则适配与存量登记，以及 `src/common/types.hpp` 的相关类型别名增补。
- 修改：按主题重组并建立本项目规则编号，按 Lua 解释器的模块、GC、allocator、owner-thread、C API/ABI 与 SDK 消费边界改写；采用本项目 `.clang-format` 的 120 列和 Attach 花括号风格；注释以职责和调用契约为先并保留现有语言；不导入 Hunter 的全量中文要求、强制 `Cfg` 缩写、游戏业务规则或专属执行流程。
- 许可证：[该提交的 MIT License](https://github.com/YanqingXu/Hunter/blob/ad1f7841f0fb3013898d099e6e8ed22eea658a6b/LICENSE)。适用的许可条款全文列于下方。

Copyright (c) 2026 Hunter contributors

## MIT license terms for the materials above

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
