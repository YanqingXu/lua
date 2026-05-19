---
status: current
verified_against: docs/deep-research-report.md; docs/PROJECT_STATUS.md; docs/DEVELOPMENT_GUIDE.md; tools/check_doc_drift.ps1; tools/run_quality_gate.ps1
last_checked: 2026-05-19
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

如果本机没有安装 `clang-format` 或 `clang-tidy`，`run_quality_gate.ps1` 会报告跳过对应项。当前 Windows/MSBuild 构建和 `bin\lua_test.exe` 仍是本仓库的标准验证路径。

## 当前状态总览

| 优先级 | 领域 | 状态 | 说明 |
|---|---|---|---|
| 最高 | 事实对齐 | 已完成 | 当前构建、测试和编译器管线事实已经集中记录并加入漂移检查 |
| 最高 | 质量门禁 | 已完成 | 已有格式化/静态检查配置、本地门禁脚本和 CI 烟测工作流 |
| 高 | 可读性快修 | 已完成 | 共享文件读取、CLI 解析抽取和标准库表驱动注册已完成 |
| 高 | 测试 runner 报告与教学索引 | 已完成 | runner 已支持 `--list`、`--filter`、`--report=junit`，并新增 walkthrough 索引 |
| 中 | EngineContext / RuntimeServices | 待开始 | 下一步建议阻断入口层直接访问单例运行时服务 |
| 中 | 教学导航 | 待开始 | 新增 START_HERE、术语表、walkthroughs、examples |
| 低 | CMake + CTest | 待开始 | 等 MSBuild 事实和测试报告稳定后再补跨平台路径 |
| 长期 | 拆分 CodeGenerator / VM / Parser | 待开始 | 等边界、测试和入口快修完成后再启动 |

## 已完成优化

### 1. 事实对齐

完成日期：2026-05-18

创建或重组的文件：

- `docs/PROJECT_STATUS.md`
- `docs/BYTECODE_GENERATION.md`
- `docs/history/exprdesc.md`
- `tools/check_doc_drift.ps1`

更新的文件：

- `README.md`
- `docs/DEVELOPMENT_GUIDE.md`
- 核心文档已统一增加 `status`、`verified_against`、`last_checked`、`applies_to` 页眉。

完成效果：

- README 和开发指南都把 Windows/MSBuild/`.vcxproj` 描述为当前可复现路径。
- CMake/CTest 已标记为规划项，而不是当前构建入口。
- 当前字节码生成文档改为说明 `AST -> SymbolRef / ValueResult / CondResult / LValueRef / CallResultInfo -> Proto`。
- 旧的 `ExprDesc / ExprKind` 说明已移动到 `docs/history/exprdesc.md`。
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

- `docs/PROJECT_STATUS.md`
- `docs/DEVELOPMENT_GUIDE.md`

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
- 本机有 MSBuild 和 `bin\lua_test.exe` 时，`run_quality_gate.ps1` 会构建 `lua_test.vcxproj`，并运行 414 个注册测试 / 1634 个结果 / 0 失败。

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
- `bin\lua_test.exe` 运行 421 个注册测试 / 1717 个结果 / 0 失败。
- `lua_app.vcxproj` 和 `lua_bytecode.vcxproj` 构建通过。

### 4. 测试 runner 报告与教学索引

完成日期：2026-05-19

创建的文件：

- `docs/walkthroughs/index.md`

更新的文件：

- `lua_test/include/test_framework/test_framework.hpp`
- `tests/unit/framework/test_runner.cpp`
- `docs/PROJECT_STATUS.md`
- `docs/DEVELOPMENT_GUIDE.md`
- `docs/OPTIMIZATION_ROADMAP.md`

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
- 默认 `bin\lua_test.exe` 仍运行 421 个注册测试 / 1717 个结果 / 0 失败。
- 过滤运行不会执行不匹配的测试套件。
- JUnit 报告包含 `<testsuites>` 根节点和对应 `<testsuite>` / `<testcase>` 条目。

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

## 下一步推荐任务：EngineContext / RuntimeServices

建议从这里继续。

### 任务 5：EngineContext / RuntimeServices

**目标：** 在不重写运行时的前提下，开始降低单例压力。

建议第一步：

- 引入 `RuntimeServices` 或 `EngineContext`。
- 增加接收 context 的重载接口。
- 当前单例访问先保留为兼容层，等调用点逐步迁移。

优先迁移的调用点：

- `src/main.cpp`
- `src/repl.cpp`
- `src/bytecode/bytecode_main.cpp`
- `src/compiler/codegen.hpp/.cpp`

### 任务 6：教学导航

**目标：** 把现有文档和测试组织成清晰的学习路径。

建议创建：

- `docs/START_HERE.md`
- `docs/glossary.md`
- `docs/walkthroughs/`
- `examples/`

walkthrough 初始素材：

- `test_symbol_binding`
- `test_value_pipeline`
- `test_codegen_conditions`
- `test_lvalue_pipeline`
- `test_call_pipeline`
- `test_codegen_multret`
- 元方法和协程相关测试

### 任务 7：CMake + CTest

**目标：** 在不打断当前 Visual Studio 工作流的前提下，增加未来跨平台路径。

启动前置条件：

- MSBuild CI 已稳定。
- 项目文件事实仍由文档维护。
- 测试 runner 已有机器可读输出。

### 任务 8：深拆大型模块

**目标：** 在显式边界已经建立后，拆分超大模块。

建议顺序：

1. `CodeGenerator`：binder、表达式 lowering、语句 lowering、bytecode builder、function compiler。
2. `VM`：dispatch、call/return、table ops、算术/元方法慢路径、trace。
3. `Parser`：token 工具、语句解析、表达式解析、表/函数解析、错误恢复。

在共享文件读取、CLI 抽取、标准库 catalog 和 EngineContext 基础完成前，不建议启动这一阶段。

## 维护规则

每完成一个优化任务后：

- 更新 `当前状态总览` 中对应行。
- 在 `已完成优化` 中追加简短记录，包括修改文件和验证命令。
- 如果仓库事实发生变化，同步更新 `docs/PROJECT_STATUS.md`。
- 运行 `tools\check_doc_drift.ps1`。
