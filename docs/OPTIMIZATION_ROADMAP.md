---
status: current
verified_against: docs/deep-research-report.md; docs/PROJECT_STATUS.md; docs/DEVELOPMENT_GUIDE.md; CMakeLists.txt; tools/run_cmake_smoke.ps1; tools/check_doc_drift.ps1; tools/run_quality_gate.ps1
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

如果本机没有安装 `clang-format` 或 `clang-tidy`，`run_quality_gate.ps1` 会报告跳过对应项。当前 Windows/MSBuild 构建和 `bin\lua_test.exe` 仍是本仓库的标准验证路径；CMake/CTest 作为 secondary 辅助路径由 `tools\run_cmake_smoke.ps1` 验证。

## 当前状态总览

| 优先级 | 领域 | 状态 | 说明 |
|---|---|---|---|
| 最高 | 事实对齐 | 已完成 | 当前构建、测试和编译器管线事实已经集中记录并加入漂移检查 |
| 最高 | 质量门禁 | 已完成 | 已有格式化/静态检查配置、本地门禁脚本和 CI 烟测工作流 |
| 高 | 可读性快修 | 已完成 | 共享文件读取、CLI 解析抽取和标准库表驱动注册已完成 |
| 高 | 测试 runner 报告与教学索引 | 已完成 | runner 已支持 `--list`、`--filter`、`--report=junit`，并新增 walkthrough 索引 |
| 中 | EngineContext / RuntimeServices | 已完成 | 已引入显式 RuntimeServices，并迁移入口层、CodeGenerator、Parser/VM 兼容重载 |
| 中 | 教学导航 | 已完成 | 已新增 START_HERE、术语表和 examples，并扩展 walkthrough 索引 |
| 低 | CMake + CTest | 已完成 | 已新增 secondary CMake/CTest 路径，不替代 VS/MSBuild 主路径 |
| 长期 | 拆分 CodeGenerator / VM / Parser | 进行中 | 8A-8C CodeGenerator 拆分、状态收口与发射边界收口已完成；下一步评估 VM dispatch 拆分 |

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
- CMake/CTest 曾在任务 1 标记为规划项；任务 7 后已作为 secondary 辅助路径落地。
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
- `bin\lua_test.exe` 运行 424 个注册测试 / 1725 个结果 / 0 失败。
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
- 默认 `bin\lua_test.exe` 运行 424 个注册测试 / 1725 个结果 / 0 失败。
- 过滤运行不会执行不匹配的测试套件。
- JUnit 报告包含 `<testsuites>` 根节点和对应 `<testsuite>` / `<testcase>` 条目。

### 5. EngineContext / RuntimeServices

完成日期：2026-05-19

创建的文件：

- `src/runtime/runtime_services.hpp`
- `tests/unit/vm/test_runtime_services.cpp`

更新的文件：

- `src/compiler/codegen.hpp`
- `src/compiler/codegen.cpp`
- `src/compiler/parser.hpp`
- `src/compiler/parser.cpp`
- `src/vm/lua_state.hpp`
- `src/vm/lua_state.cpp`
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
- `docs/PROJECT_STATUS.md`
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

- `docs/START_HERE.md`
- `docs/glossary.md`
- `examples/README.md`
- `examples/hello.lua`
- `examples/control_flow.lua`
- `examples/tables_and_methods.lua`
- `examples/metamethods.lua`

更新的文件：

- `docs/walkthroughs/index.md`
- `docs/PROJECT_STATUS.md`
- `docs/OPTIMIZATION_ROADMAP.md`
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
- `src/compiler/codegen.hpp/.cpp`
- `src/compiler/parser.hpp/.cpp`
- `src/vm/lua_state.hpp/.cpp`
- `src/vm/vm.hpp/.cpp`
- `src/core/metatable.hpp/.cpp`

## 已完成任务：教学导航

### 任务 6：教学导航

**目标：** 把现有文档和测试组织成清晰的学习路径。

已完成：

- [x] `docs/START_HERE.md`
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
- [x] 更新 `docs/PROJECT_STATUS.md` 和 `docs/DEVELOPMENT_GUIDE.md`，明确 CMake/CTest 是 secondary 辅助路径。
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

**目标：** 先降低 `src/compiler/codegen.cpp` 的单文件体积，不改变 `CodeGenerator` 的 public API、字节码语义或测试行为。

已完成：

- [x] `src/compiler/codegen.cpp` 保留构造、`generate()`、基础指令发射、寄存器、常量和局部变量管理。
- [x] 新增 `src/compiler/codegen_binding.cpp`，承载 upvalue 查找、`resolve()`、`symbolToValue()` 和 `symbolToLValue()`。
- [x] 新增 `src/compiler/codegen_expr.cpp`，承载值通道、条件通道、复合表达式、调用/vararg 和 LValue/store。
- [x] 新增 `src/compiler/codegen_jump.cpp`，承载跳转链、比较跳转和条件物化。
- [x] 新增 `src/compiler/codegen_stmt.cpp`，承载语句 lowering、函数编译、代码块管理和 debug metadata。
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

- [x] 新增 `src/compiler/codegen_state.hpp`，引入 `CodegenState` 作为 `CodeGenerator` 实现分片共享的状态边界。
- [x] 将 `services_`、`pool_`、`parent_`、`proto_`、`pc_`、`currentLine_`、`regs_`、`locals_`、`blocks_`、`upvalueCtx_` 收口到 `state_`。
- [x] 新增 `CodegenState::resetForProto()`，统一主函数和子函数编译的 Proto 初始化、寄存器绑定、source、vararg 和短生命周期状态清理。
- [x] 新增 `tests/unit/compiler/test_codegen_state.cpp`，锁定 `resetForProto()` 对临时状态和 Proto 初始字段的行为。
- [x] 将新增测试加入 `CMakeLists.txt`、`lua_test.vcxproj` 和 `lua_test.vcxproj.filters`，将新增头文件加入 `lua.vcxproj` 和 `lua.vcxproj.filters`。
- [x] 同步更新 `docs/PROJECT_STATUS.md`、README 测试徽章和文档漂移检查计数。

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

- [x] 新增 `src/compiler/bytecode_builder.hpp`，引入 `BytecodeBuilder` 作为当前 `Proto` 的发射写入边界。
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

## 下一步推荐任务：VM dispatch 拆分准备

建议从这里继续。

### 任务 8D：VM dispatch 拆分准备

**目标：** 在 CodeGenerator 边界已经完成 8A-8C 收口后，开始评估 VM 大文件拆分，优先把指令 dispatch、call/return、metamethod 路径按行为边界拆开，同时保持 `VM` public API 和现有执行语义不变。

建议顺序：

1. 先用 `rg` 梳理 `src/vm/vm.cpp` 中的 dispatch、call/return、metamethod、table/global/upvalue 操作分段。
2. 增加一组小的 VM 边界回归测试或复用现有 `VM Core` / `Function Call` / metamethod 测试作为拆分护栏。
3. 先做物理拆分，不改变执行循环和栈协议。
4. 拆分完成后再考虑更窄的 `CallFrame` / `DispatchContext` 边界。

共享文件读取、CLI 抽取、标准库 catalog 和 EngineContext 基础已经完成；深拆大型模块可以在教学导航和构建路径更稳定后启动。

## 维护规则

每完成一个优化任务后：

- 更新 `当前状态总览` 中对应行。
- 在 `已完成优化` 中追加简短记录，包括修改文件和验证命令。
- 如果仓库事实发生变化，同步更新 `docs/PROJECT_STATUS.md`。
- 运行 `tools\check_doc_drift.ps1`。
