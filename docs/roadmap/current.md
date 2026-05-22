---
status: current
verified_against: docs/archive/research/deep-research-report.md; docs/status/project-status.md; docs/guides/development.md; docs/compiler/codegen-responsibility-map.md; CMakeLists.txt; lua.vcxproj; lua.vcxproj.filters; lua_test.vcxproj; lua_test.vcxproj.filters; src/compiler/parser/parser.cpp; src/compiler/parser/parser_stmt.cpp; src/compiler/parser/parser_expr.cpp; src/compiler/parser/parser_primary.cpp; src/compiler/parser/parser_func.cpp; src/compiler/parser/parser_table.cpp; src/compiler/codegen/codegen.hpp; src/compiler/codegen/codegen_expr.cpp; src/compiler/codegen/expression_emitter.hpp; src/compiler/codegen/expression_emitter.cpp; src/compiler/codegen/statement_emitter.hpp; src/compiler/codegen/statement_emitter.cpp; src/compiler/codegen/codegen_jump.cpp; src/compiler/codegen/codegen_stmt.cpp; src/compiler/codegen/jump_patcher.hpp; src/compiler/codegen/jump_patcher.cpp; src/compiler/codegen/scope_manager.hpp; src/compiler/codegen/scope_manager.cpp; src/vm/vm.cpp; src/vm/vm_internal.hpp; src/vm/vm_switch_dispatch.hpp; tests/unit/compiler/test_codegen_characterization.cpp; tests/unit/compiler/test_jump_patcher.cpp; tests/unit/compiler/test_scope_manager.cpp; tests/unit/compiler/test_expression_emitter.cpp; tests/unit/compiler/test_statement_emitter.cpp; tests/unit/framework/test_runner.cpp; tests/unit/vm/test_runtime_services.cpp; tests/unit/vm/test_vm_dispatch.cpp; tools/run_cmake_smoke.ps1; tools/check_doc_drift.ps1; tools/run_quality_gate.ps1
last_checked: 2026-05-22
applies_to: 仓库优化路线图与下次续接检查清单
---

# Lua 仓库优化路线图

> **给后续协作会话使用：** 继续做优化任务前，请先阅读本文档。每完成一个优化任务，都要更新这里的状态、已完成记录和下一步建议，避免下次重新阅读整份研究报告才能恢复上下文。

**目标：** 将深度研究报告中的诊断结果转化为可跟踪、可续接、可验证的优化路线，覆盖可读性、可扩展性、教学价值和工具链四个维度。

**总体策略：** 按层推进优化：先让仓库事实可信，再让质量检查自动化，然后清理低价值重复代码，接着引入显式运行时/编译器边界，最后再拆分大型编译器和 VM 模块。

**技术栈：** C++23-preview/MSVC 项目文件、PowerShell 脚本、GitHub Actions、自定义 C++ 测试运行器、Markdown 文档。

---

## 下次续接检查清单

继续本路线图时，先运行：

```powershell
git status --short
powershell -NoProfile -ExecutionPolicy Bypass -File tools\check_doc_drift.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\test_quality_gate.ps1
```

如果下一项任务会修改 C++ 行为，还要运行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
```

如果本机没有安装 `clang-format` 或 `clang-tidy`，`run_quality_gate.ps1` 会报告跳过对应项。当前 Windows/MSBuild 构建和 `bin\lua_test.exe` 仍是本仓库的标准验证路径；CMake/CTest 作为 secondary 辅助路径由 `tools\run_cmake_smoke.ps1` 验证。

## 当前状态总览

| 优先级 | 领域 | 状态 | 说明 |
|---|---|---|---|
| 最高 | 事实对齐 | 已完成 | 当前构建、测试和编译器管线事实已经集中记录并加入漂移检查 |
| 最高 | 质量门禁 | 已完成 | 已有格式化/静态检查配置、本地门禁脚本和 CI 烟测工作流 |
| 高 | 可读性快修 | 已完成 | 共享文件读取、CLI 解析抽取和标准库表驱动注册已完成 |
| 高 | 测试 runner 报告与教学索引 | 已完成 | runner 已支持 `--list`、`--filter`、`--report=junit`，并新增 walkthrough 索引 |
| 中 | EngineContext / RuntimeServices | 已完成 | 已引入显式 RuntimeServices，并迁移入口层、CodeGenerator、Parser/VM 兼容重载 |
| 中 | 教学导航 | 已完成 | 已新增 `docs/index.md`、术语表和 examples，并扩展 walkthrough 索引 |
| 低 | CMake + CTest | 已完成 | 已新增 secondary CMake/CTest 路径，不替代 VS/MSBuild 主路径 |
| 长期 | 拆分 CodeGenerator / VM / Parser | 进行中 | 8A-8C CodeGenerator 边界已完成，8D-8G VM 入口、dispatch 分类、ops/call/剩余 helper 与 trace/debug 边界已完成；8H Parser 函数组审计与行为锁定、8I Parser 物理拆分执行已完成；PR-39 已完成 Switch dispatch 每 opcode inline helper；PR-40 已完成 VM expected 异常映射 helper；PR-41 已完成 CodeGenerator 职责地图与 characterization 测试；PR-42 已完成 JumpPatcher 抽取；PR-43 已完成 ScopeManager 抽取；PR-44 已完成 ExpressionEmitter 抽取；PR-45 已完成 StatementEmitter 抽取 |

## 已完成优化

### 1. 事实对齐

完成日期：2026-05-18

创建或重组的文件：

- `docs/status/project-status.md`
- `docs/compiler/bytecode-generation.md`
- `docs/archive/history/exprdesc.md`
- `tools/check_doc_drift.ps1`

更新的文件：

- `README.md`
- `docs/guides/development.md`
- 核心文档已统一增加 `status`、`verified_against`、`last_checked`、`applies_to` 页眉。

完成效果：

- README 和开发指南都把 Windows/MSBuild/`.vcxproj` 描述为当前可复现路径。
- CMake/CTest 曾在任务 1 标记为规划项；任务 7 后已作为 secondary 辅助路径落地。
- 当前字节码生成文档改为说明 `AST -> SymbolRef / ValueResult / CondResult / LValueRef / CallResultInfo -> Proto`。
- 旧的 `ExprDesc / ExprKind` 说明已移动到 `docs/archive/history/exprdesc.md`。
- `tools/check_doc_drift.ps1` 会检查产品编译器源码中的 `ExprDesc` 漂移，以及开发文档里的当前构建路径漂移。

已使用的验证命令：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\check_doc_drift.ps1
rg "ExprDesc|ExprKind|expdesc" src/compiler
```

期望状态：

- 文档漂移检查通过。
- `rg "ExprDesc|ExprKind|expdesc" src/compiler` 没有匹配结果。

### 2. 质量门禁

完成日期：2026-05-18

创建的文件：

- `.clang-format`
- `.clang-tidy`
- `.github/workflows/ci.yml`
- `tools/run_quality_gate.ps1`
- `tools/test_quality_gate.ps1`

更新的文件：

- `docs/status/project-status.md`
- `docs/guides/development.md`

完成效果：

- 仓库已有统一格式化配置。
- 仓库已有一套保守起步的静态分析配置。
- GitHub Actions 使用 `windows-latest`、MSBuild、文档漂移检查、质量门禁烟测和 `bin\lua_test.exe`。
- 本地可以用一个 PowerShell 命令运行质量门禁。
- 本地格式化默认只检查变更过的源文件，避免在一个 PR 里强制全仓重排。

已使用的验证命令：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\test_quality_gate.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\check_doc_drift.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
```

期望状态：

- 质量门禁配置自检通过。
- 文档漂移检查通过。
- 本机有 MSBuild 和 `bin\lua_test.exe` 时，`run_quality_gate.ps1` 会构建 `lua_test.vcxproj`，并运行 512 个注册测试 / 2494 个结果 / 0 失败。

### 3A. 共享文件读取

完成日期：2026-05-19

创建的文件：

- `src/io/file_loader.hpp`
- `src/io/file_loader.cpp`
- `tests/unit/io/test_file_loader.cpp`

更新的文件：

- `src/main.cpp`
- `src/bytecode/bytecode_main.cpp`
- `tests/unit/framework/test_runner.cpp`
- `lua.vcxproj`
- `lua.vcxproj.filters`
- `lua_test.vcxproj`
- `lua_test.vcxproj.filters`

完成效果：

- `src/main.cpp` 和 `src/bytecode/bytecode_main.cpp` 不再各自维护整文件读取实现。
- 新增 `Lua::readWholeFile(const std::filesystem::path& path) -> Str`，统一二进制安全读取、打开失败和读取失败错误路径。
- 新增单元测试覆盖普通文本、空文件、包含空字节的二进制文件和文件不存在错误。

已使用的验证命令：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
MSBuild.exe lua_app.vcxproj /m /p:Configuration=Debug /p:Platform=x64
MSBuild.exe lua_bytecode.vcxproj /m /p:Configuration=Debug /p:Platform=x64
```

期望状态：

- 质量门禁通过。
- `bin\lua_test.exe` 运行 418 个注册测试 / 1640 个结果 / 0 失败。

### 3B. CLI 选项抽取

完成日期：2026-05-19

创建的文件：

- `src/app/app_options.hpp`
- `src/app/app_options.cpp`
- `tests/unit/app/test_app_options.cpp`

更新的文件：

- `src/main.cpp`
- `tests/unit/framework/test_runner.cpp`
- `lua_app.vcxproj`
- `lua_app.vcxproj.filters`
- `lua_test.vcxproj`
- `lua_test.vcxproj.filters`

完成效果：

- `src/main.cpp` 中的参数扫描逻辑已移动到 `Lua::parseArgs()`。
- `main()` 缩减为控制台编码设置、`parseArgs()`、`runApp()` 和异常兜底。
- `Lua::runApp(const AppOptions& opt)` 负责版本/帮助/脚本/REPL/默认行为分派。
- CLI 解析可以在不创建 `LuaState` 的情况下测试。

已使用的验证命令：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
MSBuild.exe lua_app.vcxproj /m /p:Configuration=Debug /p:Platform=x64
MSBuild.exe lua_bytecode.vcxproj /m /p:Configuration=Debug /p:Platform=x64
```

期望状态：

- 质量门禁通过。
- `bin\lua_test.exe` 运行 419 个注册测试 / 1658 个结果 / 0 失败。
- `lua_app.vcxproj` 和 `lua_bytecode.vcxproj` 构建通过。

### 3C. 标准库表驱动注册

完成日期：2026-05-19

创建的文件：

- `src/lib/lib_catalog.hpp`
- `src/lib/lib_catalog.cpp`
- `tests/unit/stdlib/test_lib_catalog.cpp`

更新的文件：

- `src/lib/lib_manager.cpp`
- `tests/unit/framework/test_runner.cpp`
- `lua.vcxproj`
- `lua.vcxproj.filters`
- `lua_test.vcxproj`
- `lua_test.vcxproj.filters`

完成效果：

- 标准库默认装配顺序集中到 `Lua::getStandardLibraryCatalog()` 返回的 `{id, name, open}` 表。
- `StandardLibrary::openAll()` 改为遍历 catalog，不再手写逐个标准库调用。
- `StandardLibrary::openBase()`、`openMath()` 等单库入口保留，但通过 catalog 查找执行，避免包装逻辑分叉。
- 新增测试同时覆盖 catalog 顺序、open 函数存在性、`openAll()` 注册全局函数/库表，以及 `package.loaded` 中的标准库缓存。

已使用的验证命令：

```powershell
bin\lua_test.exe
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
MSBuild.exe lua_app.vcxproj /m /p:Configuration=Debug /p:Platform=x64
MSBuild.exe lua_bytecode.vcxproj /m /p:Configuration=Debug /p:Platform=x64
```

期望状态：

- 质量门禁通过。
- `bin\lua_test.exe` 运行 424 个注册测试 / 1725 个结果 / 0 失败。
- `lua_app.vcxproj` 和 `lua_bytecode.vcxproj` 构建通过。

### 4. 测试 runner 报告与教学索引

完成日期：2026-05-19

创建的文件：

- `docs/walkthroughs/index.md`

更新的文件：

- `lua_test/include/test_framework/test_framework.hpp`
- `tests/unit/framework/test_runner.cpp`
- `docs/status/project-status.md`
- `docs/guides/development.md`
- `docs/roadmap/current.md`

完成效果：

- `bin\lua_test.exe --list` 可以列出所有已注册测试用例。
- `bin\lua_test.exe --filter <suite-or-name>` 可以按套件名、测试名或 `Suite::Name` 子串过滤执行，匹配大小写不敏感。
- `bin\lua_test.exe --report=junit` 会在当前目录生成 `lua_test_junit.xml`，供 CI 归档或后续 GitHub Actions 集成使用。
- `docs/walkthroughs/index.md` 将关键测试组织成编译器、VM、运行时和标准库的阅读路径。

已使用的验证命令：

```powershell
bin\lua_test.exe --list
bin\lua_test.exe --filter "Math Library"
bin\lua_test.exe --filter "Math Library" --report=junit
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
```

期望状态：

- 质量门禁通过。
- 默认 `bin\lua_test.exe` 运行 424 个注册测试 / 1725 个结果 / 0 失败。
- 过滤运行不会执行不匹配的测试套件。
- JUnit 报告包含 `<testsuites>` 根节点和对应 `<testsuite>` / `<testcase>` 条目。

### 5. EngineContext / RuntimeServices

完成日期：2026-05-19

创建的文件：

- `src/runtime/runtime_services.hpp`
- `tests/unit/vm/test_runtime_services.cpp`

更新的文件：

- `src/compiler/codegen/codegen.hpp`
- `src/compiler/codegen/codegen.cpp`
- `src/compiler/parser/parser.hpp`
- `src/compiler/parser/parser.cpp`
- `src/vm/state/lua_state.hpp`
- `src/vm/state/lua_state.cpp`
- `src/vm/vm.hpp`
- `src/vm/vm.cpp`
- `src/core/metatable.hpp`
- `src/core/metatable.cpp`
- `src/main.cpp`
- `src/repl.cpp`
- `src/bytecode/bytecode_main.cpp`
- `lua.vcxproj`
- `lua.vcxproj.filters`
- `lua_app.vcxproj`
- `lua_app.vcxproj.filters`
- `lua_bytecode.vcxproj`
- `lua_bytecode.vcxproj.filters`
- `lua_test.vcxproj`
- `lua_test.vcxproj.filters`
- `tests/unit/framework/test_runner.cpp`
- `docs/status/project-status.md`
- `tools/check_doc_drift.ps1`

完成效果：

- 新增 `RuntimeServices`，把 `GlobalState`、`StringPool` 和 `GarbageCollector` 收拢为显式运行时服务集合。
- `CodeGenerator` 新增 `CodeGenerator(RuntimeServices&)` 构造路径，`generate()` 和子函数编译改用 context 中的 GC；旧的 `StringPool*` 构造函数保留为兼容层。
- `Parser`、`LuaState` 和 `VM::{execute, executeProto, call}` 新增接收 context 的重载接口，旧接口继续走单例兼容层。
- `src/main.cpp`、`src/repl.cpp` 和 `src/bytecode/bytecode_main.cpp` 不再直接调用 `StringPool::getInstance()` 来驱动编译入口。
- VM 字符串连接路径和元方法调用路径开始通过显式服务或 `LuaState::getGlobalState()` 取运行时资源。
- 新增 `Runtime Services` 测试覆盖单例兼容层、context-aware 编译入口和 context-aware VM 执行入口。

已使用的验证命令：

```powershell
bin\lua_test.exe --filter "Runtime Services"
bin\lua_test.exe
MSBuild.exe lua_app.vcxproj /m /p:Configuration=Debug /p:Platform=x64
MSBuild.exe lua_bytecode.vcxproj /m /p:Configuration=Debug /p:Platform=x64
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
```

期望状态：

- 质量门禁通过。
- 默认 `bin\lua_test.exe` 运行 424 个注册测试 / 1725 个结果 / 0 失败。
- 入口层和优先迁移文件中不再直接出现 `StringPool::getInstance()` / `GlobalState::getInstance()` / `GarbageCollector::getInstance()` 的业务调用；兼容层保留在 `RuntimeServices::fromSingletons()` 等 API 内部。

### 6. 教学导航

完成日期：2026-05-19

创建的文件：

- `docs/index.md`
- `docs/glossary.md`
- `examples/README.md`
- `examples/hello.lua`
- `examples/control_flow.lua`
- `examples/tables_and_methods.lua`
- `examples/metamethods.lua`

更新的文件：

- `docs/walkthroughs/index.md`
- `docs/status/project-status.md`
- `docs/roadmap/current.md`
- `tools/check_doc_drift.ps1`

完成效果：

- 新增第一次进入仓库的阅读入口，按 10 分钟、30 分钟、2 小时和深入源码路线组织材料。
- 新增术语表，把 Lua 概念映射到当前仓库类型、文件和测试入口。
- 扩展 walkthrough 索引，加入 `Runtime Services`、topic shortcuts 和 examples 与测试的配对关系。
- 新增 `examples/` 小脚本，用于快速运行当前解释器支持的 print、控制流、表方法和元方法路径。
- 文档漂移检查会要求教学入口、术语表和示例索引持续存在，并检查关键引用。

已使用的验证命令：

```powershell
bin\lua_app.exe examples\hello.lua
bin\lua_app.exe examples\control_flow.lua
bin\lua_app.exe examples\tables_and_methods.lua
bin\lua_app.exe examples\metamethods.lua
powershell -NoProfile -ExecutionPolicy Bypass -File tools\check_doc_drift.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
```

期望状态：

- 教学导航文档都有事实页眉。
- `examples/README.md` 中列出的示例脚本可以用 `bin\lua_app.exe` 运行。
- 质量门禁通过，默认测试数量仍为 424 个注册测试 / 1725 个结果 / 0 失败。

## 已完成快修任务

这个阶段风险较低，主要删除重复逻辑、澄清入口职责，不改变 VM 或编译器语义；当前 3A、3B、3C 均已完成。

### 任务 3A：共享文件读取

**目标：** 删除应用入口和字节码工具入口中重复的整文件读取逻辑。

**涉及文件：**

- 创建：`src/io/file_loader.hpp`
- 创建：`src/io/file_loader.cpp`
- 修改：`src/main.cpp`
- 修改：`src/bytecode/bytecode_main.cpp`
- 修改：`lua.vcxproj`
- 修改：`lua.vcxproj.filters`

**实施步骤：**

- [x] 如果现有 IO 测试适合承载，先增加一个针对二进制安全整文件读取的单元测试或烟测。
- [x] 实现 `Lua::readWholeFile(const std::filesystem::path& path) -> Str`。
- [x] 替换 `src/main.cpp` 中的 `readFileContents()`。
- [x] 替换 `src/bytecode/bytecode_main.cpp` 中的 `readFile()`。
- [x] 将新文件加入 Visual Studio 项目和 filters 文件。
- [x] 运行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
```

**验收标准：**

- `main.cpp` 和 `bytecode_main.cpp` 不再各自维护整文件读取实现。
- 文件打开失败和读取失败走统一错误路径。
- 单元测试仍通过；新增测试后结果为 418 个注册测试 / 1640 个结果 / 0 失败。

### 任务 3B：CLI 选项抽取

**目标：** 将参数解析从 `main()` 中拆出，让 `main()` 只负责初始化和运行模式分派。

**涉及文件：**

- 创建：`src/app/app_options.hpp`
- 创建：`src/app/app_options.cpp`
- 修改：`src/main.cpp`
- 修改：`lua_app.vcxproj`
- 修改：`lua_app.vcxproj.filters`

**建议 API：**

```cpp
namespace Lua {

enum class RunMode {
    ShowVersion,
    ShowHelp,
    Repl,
    Script,
    DefaultBehavior
};

struct AppOptions {
    RunMode mode = RunMode::DefaultBehavior;
    const char* scriptFile = nullptr;
    const char* traceFile = nullptr;
    i32 scriptIndex = -1;
};

AppOptions parseArgs(int argc, char** argv);

} // namespace Lua
```

**实施步骤：**

- [x] 为 `-v`、`-h`、`-i`、`--trace <file>`、脚本模式和默认行为增加测试。
- [x] 将参数扫描逻辑移动到 `parseArgs()`。
- [x] 在解析稳定前，运行逻辑仍留在 `main.cpp`。
- [x] 运行 `tools\run_quality_gate.ps1`。

**验收标准：**

- `main()` 能读成“环境初始化 + 模式分派”。
- CLI 行为保持不变。
- CLI 解析可以在不创建 `LuaState` 的情况下测试。

### 任务 3C：标准库表驱动注册

**目标：** 把标准库开启顺序和模块列表变成显式数据结构，而不是重复包装函数。

**涉及文件：**

- 修改：`src/lib/lib_manager.hpp`
- 修改：`src/lib/lib_manager.cpp`
- 可选创建：`src/lib/lib_catalog.hpp`
- 可选创建：`src/lib/lib_catalog.cpp`
- 创建：`tests/unit/stdlib/test_lib_catalog.cpp`
- 修改：`lua.vcxproj`
- 修改：`lua.vcxproj.filters`
- 修改：`lua_test.vcxproj`
- 修改：`lua_test.vcxproj.filters`

**实施步骤：**

- [x] 增加一个测试或烟测断言，确认 `openAll()` 仍注册预期的库表和函数。
- [x] 定义 `{id, name, openFn}` catalog。
- [x] 将 `openAll()` 改为遍历 catalog。
- [x] 保留 `openBase()`、`openMath()` 等单库入口，供测试和按需加载使用。
- [x] 运行 `tools\run_quality_gate.ps1`。

**验收标准：**

- 标准库注册顺序能在一个数据结构里看清楚。
- 现有测试通过。
- 以后新增标准库时，只需增加一条 catalog 记录，不需要复制一组包装逻辑。

## 已完成任务：测试 runner 报告与教学索引

### 任务 4：测试 runner 报告与教学索引

**目标：** 让测试结果更适合 CI 消费，也更适合作为教学入口。

- [x] `--list`
- [x] `--filter <suite-or-name>`
- [x] `--report=junit`
- [x] `docs/walkthroughs/index.md`

先增强当前测试框架 API，不要急着迁移到新测试框架。

## 已完成任务：EngineContext / RuntimeServices

### 任务 5：EngineContext / RuntimeServices

**目标：** 在不重写运行时的前提下，开始降低单例压力。

已完成：

- [x] 引入 `RuntimeServices`。
- [x] 增加接收 context 的重载接口。
- [x] 当前单例访问先保留为兼容层，等调用点逐步迁移。

已迁移的第一批调用点：

- `src/main.cpp`
- `src/repl.cpp`
- `src/bytecode/bytecode_main.cpp`
- `src/compiler/codegen/codegen.hpp/.cpp`
- `src/compiler/parser/parser.hpp/.cpp`
- `src/vm/state/lua_state.hpp/.cpp`
- `src/vm/vm.hpp/.cpp`
- `src/core/metatable.hpp/.cpp`

## 已完成任务：教学导航

### 任务 6：教学导航

**目标：** 把现有文档和测试组织成清晰的学习路径。

已完成：

- [x] `docs/index.md`
- [x] `docs/glossary.md`
- [x] `docs/walkthroughs/index.md` 扩展
- [x] `examples/`

walkthrough 初始素材：

- `test_symbol_binding`
- `test_value_pipeline`
- `test_codegen_conditions`
- `test_lvalue_pipeline`
- `test_call_pipeline`
- `test_codegen_multret`
- 元方法和协程相关测试

## 已完成任务：CMake + CTest

### 任务 7：CMake + CTest

**目标：** 在不打断当前 Visual Studio 工作流的前提下，增加未来跨平台路径。

已完成：

- [x] 新增仓库根目录 `CMakeLists.txt`。
- [x] 通过 CMake 构建 `lua_core`、`lua_app`、`lua_bytecode` 和 `lua_test`。
- [x] 通过 CTest 注册 `lua_test` 和 `examples/` smoke。
- [x] 新增 `tools\run_cmake_smoke.ps1`，自动发现 PATH 或 Visual Studio 自带的 CMake/CTest。
- [x] 更新 `docs/status/project-status.md` 和 `docs/guides/development.md`，明确 CMake/CTest 是 secondary 辅助路径。
- [x] 更新 `tools\check_doc_drift.ps1`，防止 CMake/CTest 状态再次漂移。

已使用的验证命令：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_cmake_smoke.ps1
```

验收结果：

- CMake configure/build 通过。
- CTest 通过 5 个测试：`lua_test` 和 4 个 examples smoke。

## 已完成任务：CodeGenerator 模块拆分与状态收口

### 任务 8A：CodeGenerator 物理拆分

**目标：** 先降低 `src/compiler/codegen/codegen.cpp` 的单文件体积，不改变 `CodeGenerator` 的 public API、字节码语义或测试行为。

已完成：

- [x] `src/compiler/codegen/codegen.cpp` 保留构造、`generate()`、基础指令发射、寄存器、常量和局部变量管理。
- [x] 新增 `src/compiler/codegen/codegen_binding.cpp`，承载 upvalue 查找、`resolve()`、`symbolToValue()` 和 `symbolToLValue()`。
- [x] 新增 `src/compiler/codegen/codegen_expr.cpp`，承载值通道、条件通道、复合表达式、调用/vararg 和 LValue/store。
- [x] 新增 `src/compiler/codegen/codegen_jump.cpp`，承载跳转链、比较跳转和条件物化。
- [x] 新增 `src/compiler/codegen/codegen_stmt.cpp`，承载语句 lowering、函数编译、代码块管理和 debug metadata。
- [x] 将新文件加入 `lua.vcxproj`、`lua.vcxproj.filters` 和 `CMakeLists.txt`。
- [x] 在 `lua.vcxproj` 中补齐 `/utf-8`，与 CMake 的 MSVC 编译选项对齐，避免拆分后的 UTF-8 源文件产生 C4819 编码警告。

已使用的验证命令：

```powershell
MSBuild.exe lua_test.vcxproj /p:Configuration=Debug /p:Platform=x64 /m
bin\lua_test.exe --filter "Symbol Binding"
bin\lua_test.exe --filter "ValueResult Pipeline"
bin\lua_test.exe --filter "Call Pipeline"
bin\lua_test.exe --filter "Function Codegen"
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_cmake_smoke.ps1
```

验收结果：

- MSBuild x64 Debug 通过，0 警告 / 0 错误。
- CMake/CTest secondary 路径通过 5 个测试。
- 定向测试覆盖符号绑定、值通道、调用多返回值和函数生成路径。

### 任务 8B：CodeGenerator 状态收口

**目标：** 在 8A 的物理分片基础上，先把跨分片共享的可变状态集中到显式对象中，避免 `CodeGenerator` 顶层继续散落运行时服务、当前函数原型、PC、行号、寄存器、局部变量、block 和 upvalue 状态。

已完成：

- [x] 新增 `src/compiler/codegen/codegen_state.hpp`，引入 `CodegenState` 作为 `CodeGenerator` 实现分片共享的状态边界。
- [x] 将 `services_`、`pool_`、`parent_`、`proto_`、`pc_`、`currentLine_`、`regs_`、`locals_`、`blocks_`、`upvalueCtx_` 收口到 `state_`。
- [x] 新增 `CodegenState::resetForProto()`，统一主函数和子函数编译的 Proto 初始化、寄存器绑定、source、vararg 和短生命周期状态清理。
- [x] 新增 `tests/unit/compiler/test_codegen_state.cpp`，锁定 `resetForProto()` 对临时状态和 Proto 初始字段的行为。
- [x] 将新增测试加入 `CMakeLists.txt`、`lua_test.vcxproj` 和 `lua_test.vcxproj.filters`，将新增头文件加入 `lua.vcxproj` 和 `lua.vcxproj.filters`。
- [x] 同步更新 `docs/status/project-status.md`、README 测试徽章和文档漂移检查计数。

已使用的验证命令：

```powershell
MSBuild.exe lua_test.vcxproj /p:Configuration=Debug /p:Platform=x64 /m
bin\lua_test.exe --filter "Codegen State"
bin\lua_test.exe --filter "Symbol Binding"
bin\lua_test.exe --filter "ValueResult Pipeline"
bin\lua_test.exe --filter "Call Pipeline"
bin\lua_test.exe --filter "Function Codegen"
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
```

验收结果：

- 默认 `bin\lua_test.exe` 运行 425 个注册测试 / 1738 个结果 / 0 失败。
- 状态边界重构未改变 `CodeGenerator` public API、字节码语义或 CLI 行为。
- 8A 新增的实现分片现在通过 `state_` 共享上下文，后续可继续把发射逻辑收口到更窄的 builder API。

### 任务 8C：CodeGenerator 发射边界收口

**目标：** 在 `CodegenState` 已经集中可变状态后，进一步把字节码发射、常量写入、line info 和 Proto 写操作收口到更窄的 `BytecodeBuilder` / emission API，为后续拆 VM 和 Parser 前的编译器边界继续降耦。

启动前置条件：

- RuntimeServices 兼容边界已存在。
- 测试 runner 已有过滤和 JUnit 报告。
- CMake/CTest secondary 路径已可运行。
- 教学导航已能帮助新贡献者定位关键测试。

已完成：

- [x] 新增 `src/compiler/codegen/bytecode_builder.hpp`，引入 `BytecodeBuilder` 作为当前 `Proto` 的发射写入边界。
- [x] `BytecodeBuilder` 统一承载 `emitABC()`、`emitABx()`、`emitAsBx()`、line info、常量表、子 Proto、指令读取/替换和局部调试信息写入。
- [x] `CodegenState::resetForProto()` 绑定 `bytecode`，主函数和子函数编译共享同一套发射初始化路径。
- [x] `CodeGenerator` 的 `codeABC()`、`codeABx()`、`codeAsBx()`、常量 helper、跳转回填、call/vararg 调整、table 回填、函数闭包子 Proto 写入和 debug metadata 写入已改为经过 `state_.bytecode`。
- [x] 新增 `tests/unit/compiler/test_bytecode_builder.cpp`，覆盖指令发射 + line info、常量/子 Proto 写入和指令替换。
- [x] 将新增测试加入 `CMakeLists.txt`、`lua_test.vcxproj` 和 `lua_test.vcxproj.filters`，将新增头文件加入 `lua.vcxproj` 和 `lua.vcxproj.filters`。

已使用的验证命令：

```powershell
D:\VS2026\MSBuild\Current\Bin\MSBuild.exe lua_test.vcxproj /p:Configuration=Debug /p:Platform=x64 /m
bin\lua_test.exe --filter "Bytecode Builder"
bin\lua_test.exe --filter "Codegen State"
bin\lua_test.exe --filter "Function Codegen"
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
```

验收结果：

- 默认 `bin\lua_test.exe` 运行 428 个注册测试 / 1765 个结果 / 0 失败。
- CodeGenerator public API 保持不变；发射、常量和指令替换路径已通过 `BytecodeBuilder` 收口。
- `src/compiler` 中 CodeGenerator 实现文件不再直接调用 `Proto::addInstruction()`、`addLineInfo()`、`addConstant()`、`addProto()`、`getInstruction()`、`setInstruction()` 或 `getInstructionCount()`。

### 任务 8D：VM dispatch 拆分准备

**目标：** 在 CodeGenerator 边界已经完成 8A-8C 收口后，开始评估 VM 大文件拆分，优先把指令 dispatch、call/return、metamethod 路径按行为边界拆开，同时保持 `VM` public API 和现有执行语义不变。

已完成：

- [x] 梳理 `src/vm/vm.cpp` 中的 dispatch、call/return、metamethod、table/global/upvalue 操作分段。
- [x] 新增 `src/vm/vm_dispatch.hpp`，提供 opcode 分组和 metamethod 候选 opcode 分类，作为后续 dispatch 分片的稳定边界。
- [x] 新增 `src/vm/vm_internal.hpp`，为 VM 实现分片暴露最小内部桥接函数。
- [x] 新增 `src/vm/vm_entry.cpp`，将 `VM::call()` 和 `VM::execute()` 公共入口从主 dispatch 文件中拆出。
- [x] `src/vm/vm.cpp` 保留主字节码执行循环和现有内部操作 helper，未改动执行语义、栈协议或 `VM` public API。
- [x] 新增 `tests/unit/vm/test_vm_dispatch.cpp`，覆盖 opcode 分组、全 opcode 分类完整性和 metamethod 候选分类。
- [x] 将新增源码/头文件加入 `CMakeLists.txt`、`lua.vcxproj`、`lua.vcxproj.filters`、`lua_test.vcxproj` 和 `lua_test.vcxproj.filters`。

已使用的验证命令：

```powershell
D:\VS2026\MSBuild\Current\Bin\MSBuild.exe lua_test.vcxproj /p:Configuration=Debug /p:Platform=x64 /m
bin\lua_test.exe --filter "VM Dispatch"
bin\lua_test.exe --filter "VM Core"
bin\lua_test.exe --filter "Function Call"
bin\lua_test.exe --filter "Metamethod"
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
```

验收结果：

- 默认 `bin\lua_test.exe` 运行 431 个注册测试 / 1827 个结果 / 0 失败。
- `VM::call()` 和 `VM::execute()` 已从 `src/vm/vm.cpp` 拆到 `src/vm/vm_entry.cpp`。
- 主 dispatch 循环仍在 `src/vm/vm.cpp`，避免本阶段改动 pc、栈帧、yield 或 return 协议。

### 任务 8E：VM 操作分片

**目标：** 在 8D 已经拆出入口和 opcode 分类边界后，继续按行为把 VM 内部 helper 从 `src/vm/vm.cpp` 拆出，优先拆表/元方法、算术比较和 call/return 辅助逻辑，仍保持主 dispatch 循环与 public API 不变。

已完成：

- [x] 新增 `src/vm/vm_ops.cpp`，承载 `gettable()`、`settable()`、`arith()`、`equal()`、`lessThan()`、`lessEqual()`、`unaryMinus()`、`length()` 和 `concat()`。
- [x] 新增 `src/vm/vm_call.cpp`，承载 `precall()`、`postcall()` 和 `reuseCurrentFrameForTailCall()`。
- [x] 扩展 `src/vm/vm_internal.hpp`，将 8D 的桥接声明升级为 VM 实现分片共享的内部接口。
- [x] `src/vm/vm.cpp` 保留主 dispatch 循环、trace/debug hook 状态、`SETLIST`、`TFORLOOP`、`CLOSURE` 和 `VARARG` helper。
- [x] 新增 `tests/unit/vm/test_vm_internal_boundaries.cpp`，用编译期签名检查锁定内部 helper 边界。
- [x] 将新增源文件和测试加入 `CMakeLists.txt`、`lua.vcxproj`、`lua.vcxproj.filters`、`lua_test.vcxproj` 和 `lua_test.vcxproj.filters`。

已使用的验证命令：

```powershell
D:\VS2026\MSBuild\Current\Bin\MSBuild.exe lua_test.vcxproj /p:Configuration=Debug /p:Platform=x64 /m
bin\lua_test.exe --filter "VM Internal Boundaries"
bin\lua_test.exe --filter "VM Dispatch"
bin\lua_test.exe --filter "VM Core"
bin\lua_test.exe --filter "Function Call"
bin\lua_test.exe --filter "Call Pipeline"
bin\lua_test.exe --filter "Metamethod"
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
```

验收结果：

- 默认 `bin\lua_test.exe` 运行 433 个注册测试 / 1829 个结果 / 0 失败。
- `src/vm/vm.cpp` 的主 dispatch 循环未拆分，pc、栈帧、yield、return 协议保持原路径。
- VM 表/元方法、算术比较、concat 和 call/return helper 已通过 `VM::detail` 内部接口分片。

### 任务 8F：VM 剩余 helper 分片

**目标：** 在 8E 已经拆出 ops 和 call helper 后，继续评估 `src/vm/vm.cpp` 中剩余的 `SETLIST`、`TFORLOOP`、`CLOSURE`、`VARARG`、trace event 构造等 helper 是否适合拆到更窄的 `vm_table.cpp`、`vm_loop.cpp`、`vm_closure.cpp` 或 `vm_trace.cpp`，仍保持 `executeProto()` 主循环为唯一指令调度位置。

已完成：

- [x] 新增 `src/vm/vm_table.cpp`，承载 `setList()` / `SETLIST` 表数组批量写入 helper。
- [x] 新增 `src/vm/vm_frame.cpp`，承载 `closure()` 和 `vararg()`，保留闭包 upvalue 捕获与 vararg 栈顶协议。
- [x] 新增 `src/vm/vm_loop.cpp`，承载 `tforLoop()`，主 dispatch 循环仍持有 `pc` 并通过引用传入。
- [x] 扩展 `src/vm/vm_internal.hpp` 和 `tests/unit/vm/test_vm_internal_boundaries.cpp`，锁定剩余 VM helper 的内部签名边界。
- [x] `src/vm/vm.cpp` 保留 `executeProto()` 主循环、trace/debug hook 状态、计数/行 hook 和 call/return trace event 构造。
- [x] 将新增源码加入 `CMakeLists.txt`、`lua.vcxproj` 和 `lua.vcxproj.filters`。

已使用的验证命令：

```powershell
D:\VS2026\MSBuild\Current\Bin\MSBuild.exe lua_test.vcxproj /p:Configuration=Debug /p:Platform=x64 /m
bin\lua_test.exe --filter "VM Internal Boundaries"
bin\lua_test.exe --filter "VM Core"
bin\lua_test.exe --filter "Call Pipeline"
bin\lua_test.exe --filter "Symbol Binding"
bin\lua_test.exe --filter "Coroutine Lib"
bin\lua_test.exe --filter "String Library"
bin\lua_test.exe --filter "Metamethod"
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
```

验收结果：

- 默认 `bin\lua_test.exe` 运行 434 个注册测试 / 1830 个结果 / 0 失败。
- `SETLIST`、`TFORLOOP`、`CLOSURE`、`VARARG` helper 已从 `src/vm/vm.cpp` 拆出。
- `executeProto()` 仍是唯一主 dispatch 循环，未改变 pc、栈帧、yield 或 return 协议。

## 已完成任务：VM trace/debug 边界评估

### 任务 8G：VM trace/debug 边界评估

**目标：** 在 VM 行为 helper 已完成主要物理分片后，评估 `src/vm/vm.cpp` 中剩余的 trace/debug hook 状态和事件构造是否需要收口到更窄的内部 helper，或者是否应先转入 Parser 分片。该任务应优先输出边界设计和测试策略，避免直接搬动会影响事件顺序的代码。

已完成：

- [x] 梳理 `g_traceSink`、`g_traceSeq`、`g_dumpBytecode`、count/line hook、instruction/call/return trace event 的调用顺序。
- [x] 新增 `src/vm/vm_trace.cpp`，承载 trace sink 状态、debug hook 分发和 trace event 构造。
- [x] `src/vm/vm.cpp` 保留 `executeProto()` 主循环以及 count/line、instruction trace、call/return trace 的清晰触发点。
- [x] 扩展 `src/vm/vm_internal.hpp` 和 `tests/unit/vm/test_vm_internal_boundaries.cpp`，锁定 trace/debug helper 签名。
- [x] 新增 `tests/unit/vm/test_vm_trace_debug.cpp`，覆盖 instruction/call/return trace 顺序，以及 call/count/line/return debug hook 顺序。
- [x] 将新增源文件和测试加入 `CMakeLists.txt`、`lua.vcxproj`、`lua.vcxproj.filters`、`lua_test.vcxproj` 和 `lua_test.vcxproj.filters`。
- [x] 同步更新 `docs/status/project-status.md`、README 测试统计和文档漂移检查计数。

已使用的验证命令：

```powershell
D:\VS2026\MSBuild\Current\Bin\MSBuild.exe lua_test.vcxproj /p:Configuration=Debug /p:Platform=x64 /m
bin\lua_test.exe --filter "VM Trace Debug"
bin\lua_test.exe --filter "VM Internal Boundaries"
bin\lua_test.exe
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
```

验收结果：

- 默认 `bin\lua_test.exe` 运行 439 个注册测试 / 1862 个结果 / 0 失败。
- `src/vm/vm_trace.cpp` 已收口 trace/debug 状态与事件构造，`src/vm/vm.cpp` 仍是唯一主 dispatch 循环。
- 新增回归测试锁定 instruction/call/return trace 顺序，以及 call/count/line/return debug hook 顺序。

## 已完成任务：Parser 分片准备

### 任务 8H：Parser 物理拆分准备

**目标：** 在 CodeGenerator 和 VM 的主要边界已经收口后，开始降低 `src/compiler/parser/parser.cpp` 的单文件阅读负担。第一步不改变语法语义，只梳理 Parser 成员函数分组，并准备按表达式、语句、函数体/块、表构造器等主题拆分实现文件。

已完成：

- [x] 审计 `src/compiler/parser/parser.hpp` 和 `src/compiler/parser/parser.cpp`，按后续物理分片边界记录 Parser 成员函数分组。
- [x] 新增 `tests/unit/compiler/test_parser_boundaries.cpp`，集中锁定语句族、表达式优先级、函数/表构造器/后缀表达式和错误边界 AST 形状。
- [x] 将新增测试加入 `tests/unit/framework/test_runner.cpp`、`CMakeLists.txt`、`lua_test.vcxproj` 和 `lua_test.vcxproj.filters`。
- [x] 本阶段不移动 Parser 产品代码，不改变 public API、AST 结构或语法语义。

Parser 函数组审计结果：

| 建议分片 | 当前函数 | 说明 |
|---|---|---|
| `parser_core.cpp` 或保留在 `parser.cpp` | 构造函数、`current()`、`advance()`、`peek()`、`check()`、`match()`、`expect()`、`error()`、`reportError()`、`synchronize()`、`getTokenText()`、`makeErrorWithNear()`、`getTokenString()` | Token 管理、错误报告和恢复同步是所有分片共享的底层边界。 |
| `parser_block.cpp` / `parser_stmt.cpp` | `parse()`、`parseBlock()`、`parseStatement()` | chunk/block/stat dispatch 是语句层入口，后续可优先迁出。 |
| `parser_stmt.cpp` | `parseIfStmt()`、`parseWhileStmt()`、`parseDoStmt()`、`parseForStmt()`、`parseRepeatStmt()`、`parseLocalStmt()`、`parseReturnStmt()`、`parseBreakStmt()`、`parseExprStmt()` | 控制流、局部声明、return/break、赋值/调用语句；依赖表达式入口和块入口。 |
| `parser_func.cpp` | `parseFunctionStmt()`、`parseFunctionExpr()`、`parseParamList()` | 函数语句和函数表达式共享参数列表、vararg 和 body 解析规则。 |
| `parser_expr.cpp` | `parseExpression()`、`parseOrExpr()`、`parseAndExpr()`、`parseRelationalExpr()`、`parseConcatExpr()`、`parseAdditiveExpr()`、`parseMultiplicativeExpr()`、`parseUnaryExpr()`、`parsePowerExpr()` | 表达式优先级链；`..` 和 `^` 右结合规则应由新增 sentinels 保护。 |
| `parser_primary.cpp` | `parsePrimaryExpr()`、`parsePostfixExpr()` | literal/name/function/table/paren 基础表达式，以及 call/index/member/method/string-call/table-call 后缀链。 |
| `parser_table.cpp` | `parseTableConstructor()` | `[key]=value`、`name=value`、数组项、`,`/`;` 混合分隔符和尾随分隔符。 |
| `parser_list.cpp` 或并入表达式分片 | `parseExprList()` | 被语句、调用、return、table 字段等多处复用；拆分时要避免形成循环 include。 |
| 保留在 `parser.hpp` | `NodePool`、`RecursionGuard`、`makeExpr()`、`makeStmt()` | 模板和 RAII guard 当前适合留在头文件，后续只移动成员函数实现。 |

行为锁定测试：

- 现有 `Parser Recursion Depth` 覆盖嵌套括号、嵌套 if 和递归深度错误。
- 现有 `Parser Error Reporting` 覆盖基础语法错误、缺 `then`、未闭合括号和缺 `end`。
- 现有 syntax/call/value/lvalue/symbol/codegen 测试覆盖函数定义、表路径函数、函数调用、表构造和复杂表达式的下游 codegen/VM 行为。
- 新增 `Parser Boundary Sentinels` 直接检查 AST 形状，作为后续 Parser 物理拆分时的第一层回归测试。

已使用的验证命令：

```powershell
D:\VS2026\MSBuild\Current\Bin\MSBuild.exe lua_test.vcxproj /p:Configuration=Debug /p:Platform=x64 /m
bin\lua_test.exe --filter "Parser Boundary Sentinels"
bin\lua_test.exe --filter "Parser"
bin\lua_test.exe
powershell -NoProfile -ExecutionPolicy Bypass -File tools\check_doc_drift.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\test_quality_gate.ps1
```

验收结果：

- `Parser Boundary Sentinels` 运行 4 个注册测试 / 54 个结果 / 0 失败。
- `Parser` 过滤测试运行 17 个注册测试 / 97 个结果 / 0 失败。
- 默认 `bin\lua_test.exe` 运行 496 个注册测试 / 2423 个结果 / 0 失败。
- 新增测试只锁定当前 Parser 行为，没有修改 `src/compiler/parser/parser.*` 产品实现。

## 已完成任务：Parser 物理拆分执行

### 任务 8I：Parser 物理拆分执行

**目标：** 基于 8H 的函数分组和行为锁定测试，在不改变 public API、AST 结构和语法语义的前提下，把 `src/compiler/parser/parser.cpp` 分批拆成更窄的实现文件。

已完成：

- [x] `src/compiler/parser/parser.cpp` 保留构造函数、token 管理、错误报告/同步、`parse()` 入口，以及跨分片共享的 `tokenString()` / `errorWithNear()` 私有 helper。
- [x] 新增 `src/compiler/parser/parser_stmt.cpp`，承载 `parseBlock()`、`parseStatement()` 和控制流/local/return/break/赋值/调用语句解析。
- [x] 新增 `src/compiler/parser/parser_expr.cpp`，承载表达式优先级链和 `parseExprList()`。
- [x] 新增 `src/compiler/parser/parser_primary.cpp`，承载 primary 与 postfix 表达式解析。
- [x] 新增 `src/compiler/parser/parser_func.cpp`，承载函数语句、函数表达式和参数列表解析。
- [x] 新增 `src/compiler/parser/parser_table.cpp`，承载表构造器解析。
- [x] 更新 `CMakeLists.txt`、`lua.vcxproj`、`lua.vcxproj.filters`，让 CMake 与 VS 核心静态库都编译新分片。
- [x] 本阶段不改变 Parser public API、AST 结构或 Lua 语法语义。

已使用的验证命令：

```powershell
D:\VS2026\MSBuild\Current\Bin\MSBuild.exe lua_test.vcxproj /p:Configuration=Debug /p:Platform=x64 /m
bin\lua_test.exe --filter "Parser Boundary Sentinels"
bin\lua_test.exe --filter "Parser"
bin\lua_test.exe
powershell -NoProfile -ExecutionPolicy Bypass -File tools\check_doc_drift.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\test_quality_gate.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_cmake_smoke.ps1
```

验收结果：

- MSBuild `lua_test.vcxproj` 输出 0 警告 / 0 错误。
- `Parser Boundary Sentinels` 运行 4 个注册测试 / 54 个结果 / 0 失败。
- `Parser` 过滤测试运行 17 个注册测试 / 97 个结果 / 0 失败。
- 默认 `bin\lua_test.exe` 运行 496 个注册测试 / 2423 个结果 / 0 失败。
- `tools\check_doc_drift.ps1` 与 `tools\test_quality_gate.ps1` 通过。
- `tools\run_cmake_smoke.ps1` 通过，CTest 5/5。

## 已完成任务：VM Switch dispatch 路径重构

### PR-39 / 1.1.1：Switch dispatch 每 opcode 独立 inline 函数

**目标：** 按 `docs/roadmap/optimization_and_refactoring.md` 的 1.1.1 规划，让 `SwitchDispatch` 不再把 38 个 opcode 汇入同一个 `runCurrentHandler()` lambda，而是在 switch case 中逐条调用 opcode-specific inline helper，增强调试单步体验，并与 `TableDispatch` 的 handler table 路径形成清晰对比。

已完成：

- [x] 新增 `src/vm/vm_switch_dispatch.hpp`，为 38 条 opcode 提供 `execOp*()` inline entry point，并提供 `switchHandlerFor()` 用于测试和覆盖检查。
- [x] 更新 `src/vm/vm.cpp`，Switch dispatch 路径逐 case 调用 `VM::detail::execOp*()`，Table dispatch 仍直接走 `VM::runHandler()`。
- [x] 移除 `vm.cpp` 中 switch 路径专用的 `runCurrentHandler()` lambda。
- [x] 新增 `Switch Dispatch Helpers Cover Opcode Space` 测试，锁定 38 条 opcode 到 helper 的映射完整性。
- [x] 将新增头文件加入 `lua.vcxproj` 与 `lua.vcxproj.filters`。
- [x] 同步更新 README、`docs/status/project-status.md`、`docs/roadmap/optimization_and_refactoring.md` 和 `tools/check_doc_drift.ps1` 的测试计数。

已使用的验证命令：

```powershell
D:\VS2026\MSBuild\Current\Bin\MSBuild.exe lua_test.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /nologo /v:minimal
bin\lua_test.exe --filter "VM Dispatch"
bin\lua_test.exe
```

验收结果：

- MSBuild `lua_test.vcxproj` 通过。
- `VM Dispatch` 过滤测试运行 19 个注册测试 / 388 个结果 / 0 失败。
- 默认 `bin\lua_test.exe` 运行 496 个注册测试 / 2423 个结果 / 0 失败。

## 已完成任务：VM expected 异常映射边界收口

### PR-40 / 1.3.1：提取 `tryExecuteProto` 异常映射 helper

**目标：** 按 `docs/roadmap/optimization_and_refactoring.md` 的 1.3.1 规划，把 `VM::tryExecuteProto(RuntimeServices&, ...)` 中内联的异常到 `RuntimeError` 映射规则提取为可复用内部 helper，后续新增 `std::expected` 边界时不再复制 catch 链。

已完成：

- [x] 在 `src/vm/vm_internal.hpp` 新增 `VM::detail::mapExceptionToUnexpected()`。
- [x] 在 `src/vm/vm_internal.hpp` 新增模板 helper `VM::detail::captureRuntimeErrors<T>()`，统一处理成功返回、`RuntimeError` / `LuaError` / `std::exception` 到 `std::unexpected<RuntimeError>` 的映射，并保持 `std::bad_alloc` 继续抛出。
- [x] 更新 `src/vm/vm.cpp`，`tryExecuteProto(RuntimeServices&, ...)` 通过 `captureRuntimeErrors<ExecResult>()` 包裹 `executeProtoUnchecked()`。
- [x] 新增 `Runtime error capture helper maps expected boundary` 测试，覆盖成功值、三类异常映射和 `std::bad_alloc` 重抛。
- [x] 同步更新 README、`docs/status/project-status.md`、`docs/roadmap/optimization_and_refactoring.md` 和 `tools/check_doc_drift.ps1` 的测试计数。

已使用的验证命令：

```powershell
D:\VS2026\MSBuild\Current\Bin\MSBuild.exe lua_test.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /nologo /v:minimal
bin\lua_test.exe --filter "Runtime Services"
bin\lua_test.exe
```

验收结果：

- MSBuild `lua_test.vcxproj` 通过。
- `Runtime Services` 过滤测试运行 8 个注册测试 / 26 个结果 / 0 失败。
- 默认 `bin\lua_test.exe` 运行 496 个注册测试 / 2423 个结果 / 0 失败。

## 已完成任务：CodeGenerator 职责地图与 characterization 测试

### PR-41 / 2.5.1-a：CodeGenerator 职责地图 + characterization 测试

**目标：** 按 `docs/roadmap/optimization_and_refactoring.md` 的 2.5.1-a 规划，在不拆生产 `CodeGenerator` 的前提下，先记录职责地图并补 statement / jump characterization 测试，为 PR-42 `JumpPatcher` 和 PR-43 `ScopeManager` 提供行为锁。

已完成：

- [x] 新增 `docs/compiler/codegen-responsibility-map.md`，按 facade、指令写入、寄存器/常量、符号绑定、作用域、跳转、表达式、调用、左值、语句和函数编译记录当前职责。
- [x] 新增 `tests/unit/compiler/test_codegen_characterization.cpp`，覆盖 statement lowering runtime、structured statements pending jump、generic-for 字节码形状。
- [x] 将新增测试接入 `tests/unit/framework/test_runner.cpp`、`CMakeLists.txt`、`lua_test.vcxproj` 和 `lua_test.vcxproj.filters`。
- [x] 更新 `docs/compiler/bytecode-generation.md`、`docs/status/project-status.md`、README、`docs/roadmap/optimization_and_refactoring.md` 和 `tools/check_doc_drift.ps1`。

TDD 记录：

- 先只接入 runner，未加入项目清单时，MSBuild 链接失败，报 `registerCodegenCharacterizationTests` unresolved external。
- 补齐项目清单后，MSBuild 通过，新套件进入 `lua_test.exe`。

已使用的验证命令：

```powershell
D:\VS2026\MSBuild\Current\Bin\MSBuild.exe lua_test.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /nologo /v:minimal
bin\lua_test.exe --filter "Codegen Characterization"
bin\lua_test.exe --filter "Codegen Conditions"
bin\lua_test.exe
```

验收结果：

- MSBuild `lua_test.vcxproj` 通过。
- `Codegen Characterization` 过滤测试运行 3 个注册测试 / 20 个结果 / 0 失败。
- `Codegen Conditions` 过滤测试运行 4 个注册测试 / 12 个结果 / 0 失败。
- 默认 `bin\lua_test.exe` 运行 499 个注册测试 / 2443 个结果 / 0 失败。

## 已完成任务：CodeGenerator JumpPatcher 抽取

### PR-42 / 2.5.1-b：抽取 `JumpPatcher`

**目标：** 按 `docs/roadmap/optimization_and_refactoring.md` 的 2.5.1-b 规划，把 `CodeGenerator` 中最独立的跳转链表和回填职责移入 `JumpPatcher`，为 PR-43 `ScopeManager` 抽取降低控制流耦合。

已完成：

- [x] 新增 `src/compiler/codegen/jump_patcher.hpp` 和 `src/compiler/codegen/jump_patcher.cpp`，集中 `emitJump()`、`emitConditionalJump()`、`patchList()`、`patchToHere()`、`flushPendingJumps()`、`getJump()` 和 `fixJump()`。
- [x] 更新 `CodeGenerator`，保留 `jump()` / `patchList()` / `patchtohere()` / `fixjump()` 等薄包装，现有 statement / expression 调用面不变。
- [x] 新增 `tests/unit/compiler/test_jump_patcher.cpp`，直接覆盖 pending `jpc_` flush、旧式链表方向、`PatchList` 显式回填、`TESTSET + NO_REG -> TEST` 和过长跳转错误。
- [x] 将新增生产源文件加入 `CMakeLists.txt`、`lua.vcxproj`、`lua.vcxproj.filters`；将新增测试加入 `CMakeLists.txt`、`lua_test.vcxproj`、`lua_test.vcxproj.filters` 和 `test_runner.cpp`。
- [x] 同步更新 README、`docs/status/project-status.md`、`docs/compiler/bytecode-generation.md`、`docs/compiler/codegen-responsibility-map.md`、`docs/roadmap/optimization_and_refactoring.md` 和 `tools/check_doc_drift.ps1`。

TDD 记录：

- 先新增 `Jump Patcher` 测试并接入项目清单；MSBuild 失败于缺失 `compiler/codegen/jump_patcher.hpp`。
- 实现 `JumpPatcher` 后专项测试暴露一处测试假设错误：旧式链表方向应为“新 JMP 是 head，旧 pending list 接在后面”。修正测试后专项通过。

已使用的验证命令：

```powershell
D:\VS2026\MSBuild\Current\Bin\MSBuild.exe lua_test.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /nologo /v:minimal
bin\lua_test.exe --filter "Jump Patcher"
bin\lua_test.exe --filter "Codegen Characterization"
bin\lua_test.exe --filter "Codegen Conditions"
bin\lua_test.exe
```

验收结果：

- MSBuild `lua_test.vcxproj` 通过。
- `Jump Patcher` 过滤测试运行 5 个注册测试 / 16 个结果 / 0 失败。
- `Codegen Characterization` 过滤测试运行 3 个注册测试 / 20 个结果 / 0 失败。
- `Codegen Conditions` 过滤测试运行 4 个注册测试 / 12 个结果 / 0 失败。
- 默认 `bin\lua_test.exe` 运行 504 个注册测试 / 2459 个结果 / 0 失败。

## 已完成任务：CodeGenerator ScopeManager 抽取

### PR-43 / 2.5.1-c：抽取 `ScopeManager`

**目标：** 按 `docs/roadmap/optimization_and_refactoring.md` 的 2.5.1-c 规划，把 `CodeGenerator` 中 locals / blocks / upvalues 的作用域生命周期移入 `ScopeManager`，让 statement / expression 继续通过稳定包装调用，避免在同一 PR 中改变 lowering 行为。

已完成：

- [x] 新增 `src/compiler/codegen/scope_manager.hpp` 和 `src/compiler/codegen/scope_manager.cpp`，集中 `addLocalVar()`、`adjustLocalVars()`、`removeLocalVars()`、`enterBlock()`、`leaveBlock()`、`closeScopeUpvalues()`、`findUpvalue()`、`addUpvalue()` 和 `resolveUpvalue()`。
- [x] 更新 `CodeGenerator`，保留 `addLocalVar()` / `findLocalVar()` / `enterBlock()` / `leaveBlock()` / `resolveUpvalue()` 等薄包装；`compileFunction()`、`block()`、`break`、repeat-until 和 debug metadata 调用点改用 `scopes_`。
- [x] 新增 `tests/unit/compiler/test_scope_manager.cpp`，直接覆盖 local 生命周期与 `CLOSE` 发射、`RETURN` 后冗余 `CLOSE` 抑制、breaklist 进入 pending `jpc_`、upvalue 查找和去重。
- [x] 将新增生产源文件加入 `CMakeLists.txt`、`lua.vcxproj`、`lua.vcxproj.filters`；将新增测试加入 `CMakeLists.txt`、`lua_test.vcxproj`、`lua_test.vcxproj.filters` 和 `test_runner.cpp`。
- [x] 同步更新 README、`docs/status/project-status.md`、`docs/compiler/bytecode-generation.md`、`docs/compiler/codegen-responsibility-map.md`、`docs/roadmap/optimization_and_refactoring.md` 和 `tools/check_doc_drift.ps1`。

TDD 记录：

- 先新增 `Scope Manager` 测试并接入项目清单；MSBuild 失败于缺失 `compiler/codegen/scope_manager.hpp`。
- 实现 `ScopeManager` 后专项测试暴露一处测试假设错误：旧 `patchtohere()` 语义会先把 break jump 挂入 pending `jpc_`，需要在 flush 后才写入最终 offset。修正测试后专项通过。

已使用的验证命令：

```powershell
D:\VS2026\MSBuild\Current\Bin\MSBuild.exe lua_test.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /nologo /v:minimal
bin\lua_test.exe --filter "Scope Manager"
bin\lua_test.exe --filter "Codegen Characterization"
bin\lua_test.exe --filter "Symbol Binding"
bin\lua_test.exe --filter "Codegen Conditions"
bin\lua_test.exe
```

验收结果：

- MSBuild `lua_test.vcxproj` 通过。
- `Scope Manager` 过滤测试运行 4 个注册测试 / 21 个结果 / 0 失败。
- `Codegen Characterization` 过滤测试运行 3 个注册测试 / 20 个结果 / 0 失败。
- `Symbol Binding` 过滤测试运行 24 个注册测试 / 49 个结果 / 0 失败。
- `Codegen Conditions` 过滤测试运行 4 个注册测试 / 12 个结果 / 0 失败。
- 默认 `bin\lua_test.exe` 运行 508 个注册测试 / 2480 个结果 / 0 失败。

## 已完成任务：CodeGenerator ExpressionEmitter 抽取

### PR-44 / 2.5.1-d：抽取 `ExpressionEmitter`

**目标：** 按 `docs/roadmap/optimization_and_refactoring.md` 的 2.5.1-d 规划，把 `CodeGenerator` 中 `ValueResult` / `CondResult` / `CallResultInfo` / `LValueRef` 表达式通道移入 `ExpressionEmitter`，让 statement 层继续通过 `CodeGenerator` 的稳定包装调用。

已完成：

- [x] 新增 `src/compiler/codegen/expression_emitter.hpp` 和 `src/compiler/codegen/expression_emitter.cpp`，集中 `emitValue()`、`emitCondResult()`、`emitComparisonJump()`、`materializeCondResult()`、`emitCallExpr()`、`emitVarargExpr()`、`emitLValue()` 和 `emitStore()`。
- [x] 更新 `CodeGenerator`，移除自身 AST expression visitor 继承，保留 `emitValue()` / `emitCondResult()` / `emitCallExpr()` / `emitLValue()` 等薄包装。
- [x] 新增 `tests/unit/compiler/test_expression_emitter.cpp`，直接覆盖 `ExpressionEmitter` public boundary、返回类型契约和 immediate literal lowering。
- [x] 将新增生产源文件加入 `CMakeLists.txt`、`lua.vcxproj`、`lua.vcxproj.filters`；将新增测试加入 `CMakeLists.txt`、`lua_test.vcxproj`、`lua_test.vcxproj.filters` 和 `test_runner.cpp`。
- [x] 同步更新 README、`docs/status/project-status.md`、`docs/compiler/bytecode-generation.md`、`docs/compiler/codegen-responsibility-map.md`、`docs/roadmap/optimization_and_refactoring.md` 和 `tools/check_doc_drift.ps1`。

TDD 记录：

- 先新增 `Expression Emitter` 测试并接入项目清单；MSBuild 失败于缺失 `compiler/codegen/expression_emitter.hpp`。
- 实现迁移后构建暴露一处测试宏使用问题：`ASSERT_TRUE` 宏会把 `std::is_same_v<A, B>` 中的逗号当作参数分隔。将类型判断先保存为 `constexpr bool` 后恢复编译。
- 第二次构建暴露一处迁移遗漏：`emitComparisonJump()` / `materializeCondResult()` 已被 `CodeGenerator` 委托但尚未搬到 `ExpressionEmitter`。补齐后构建和表达式专项通过。

已使用的验证命令：

```powershell
D:\VS2026\MSBuild\Current\Bin\MSBuild.exe lua_test.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /nologo /v:minimal
bin\lua_test.exe --filter "Expression Emitter"
bin\lua_test.exe --filter "ValueResult Pipeline"
bin\lua_test.exe --filter "Codegen Conditions"
bin\lua_test.exe --filter "LValue Pipeline"
bin\lua_test.exe --filter "Call Pipeline"
bin\lua_test.exe --filter "Codegen MultiRet"
bin\lua_test.exe --filter "Symbol Binding"
bin\lua_test.exe --filter "Codegen Characterization"
bin\lua_test.exe
```

验收结果：

- MSBuild `lua_test.vcxproj` 通过。
- `Expression Emitter` 过滤测试运行 2 个注册测试 / 10 个结果 / 0 失败。
- `ValueResult Pipeline` 过滤测试运行 22 个注册测试 / 26 个结果 / 0 失败。
- `Codegen Conditions` 过滤测试运行 4 个注册测试 / 12 个结果 / 0 失败。
- `LValue Pipeline` 过滤测试运行 17 个注册测试 / 17 个结果 / 0 失败。
- `Call Pipeline` 过滤测试运行 18 个注册测试 / 21 个结果 / 0 失败。
- `Codegen MultiRet` 过滤测试运行 3 个注册测试 / 3 个结果 / 0 失败。
- `Symbol Binding` 过滤测试运行 24 个注册测试 / 49 个结果 / 0 失败。
- `Codegen Characterization` 过滤测试运行 3 个注册测试 / 20 个结果 / 0 失败。
- 默认 `bin\lua_test.exe` 运行 510 个注册测试 / 2490 个结果 / 0 失败。

## 已完成任务：CodeGenerator StatementEmitter 抽取

### PR-45 / 2.5.1-e：抽取 `StatementEmitter`

**目标：** 按 `docs/roadmap/optimization_and_refactoring.md` 的 2.5.1-e 规划，把 `CodeGenerator` 中 statement / block lowering 移入 `StatementEmitter`，让函数编译、closure upvalue 装配和 debug metadata 暂时继续由 facade 编排。

已完成：

- [x] 新增 `src/compiler/codegen/statement_emitter.hpp` 和 `src/compiler/codegen/statement_emitter.cpp`，集中 `statement()`、各 `emitStmt()`、`block()` 和语句级控制流 lowering。
- [x] 更新 `CodeGenerator`，新增 `StatementEmitter statements_`，并保留 statement / block 薄包装以稳定内部调用面。
- [x] 新增 `tests/unit/compiler/test_statement_emitter.cpp`，直接覆盖 `StatementEmitter` public boundary、`statement()` / `block()` void 契约和空语句 lowering。
- [x] 将新增生产源文件加入 `CMakeLists.txt`、`lua.vcxproj`、`lua.vcxproj.filters`；将新增测试加入 `CMakeLists.txt`、`lua_test.vcxproj`、`lua_test.vcxproj.filters` 和 `test_runner.cpp`。
- [x] 同步更新 README、`docs/status/project-status.md`、`docs/compiler/bytecode-generation.md`、`docs/compiler/codegen-responsibility-map.md`、`docs/roadmap/optimization_and_refactoring.md` 和 `tools/check_doc_drift.ps1`。

TDD 记录：

- 先新增 `Statement Emitter` 测试并接入项目清单；MSBuild 失败于缺失 `compiler/codegen/statement_emitter.hpp`。
- 实现迁移后，`CodeGenerator::emitStmt(...)` 与 `block()` 保留兼容包装，`StatementEmitter` 通过 `StmtVisitor<StatementEmitter, void>` 获得所有语句节点的编译期覆盖检查。
- 构建、专项和全量测试均通过。

已使用的验证命令：

```powershell
D:\VS2026\MSBuild\Current\Bin\MSBuild.exe lua_test.vcxproj /p:Configuration=Debug /p:Platform=x64 /m /nologo /v:minimal
bin\lua_test.exe --filter "Statement Emitter"
bin\lua_test.exe --filter "Codegen Characterization"
bin\lua_test.exe --filter "Scope Manager"
bin\lua_test.exe --filter "Expression Emitter"
bin\lua_test.exe --filter "Codegen Conditions"
bin\lua_test.exe --filter "ValueResult Pipeline"
bin\lua_test.exe --filter "Call Pipeline"
bin\lua_test.exe --filter "Symbol Binding"
bin\lua_test.exe --filter "LValue Pipeline"
bin\lua_test.exe
```

验收结果：

- MSBuild `lua_test.vcxproj` 通过。
- `Statement Emitter` 过滤测试运行 2 个注册测试 / 4 个结果 / 0 失败。
- `Codegen Characterization` 过滤测试运行 3 个注册测试 / 20 个结果 / 0 失败。
- `Scope Manager` 过滤测试运行 4 个注册测试 / 21 个结果 / 0 失败。
- `Expression Emitter` 过滤测试运行 2 个注册测试 / 10 个结果 / 0 失败。
- `Codegen Conditions` 过滤测试运行 4 个注册测试 / 12 个结果 / 0 失败。
- `ValueResult Pipeline` 过滤测试运行 22 个注册测试 / 26 个结果 / 0 失败。
- `Call Pipeline` 过滤测试运行 18 个注册测试 / 21 个结果 / 0 失败。
- `Symbol Binding` 过滤测试运行 24 个注册测试 / 49 个结果 / 0 失败。
- `LValue Pipeline` 过滤测试运行 17 个注册测试 / 17 个结果 / 0 失败。
- 默认 `bin\lua_test.exe` 运行 512 个注册测试 / 2494 个结果 / 0 失败。

## 维护规则

每完成一个优化任务后：

- 更新 `当前状态总览` 中对应行。
- 在 `已完成优化` 中追加简短记录，包括修改文件和验证命令。
- 如果仓库事实发生变化，同步更新 `docs/status/project-status.md`。
- 运行 `tools\check_doc_drift.ps1`。
