---
status: current
verified_against: docs/status/project-status.md; docs/index.md; docs/guides/development.md; docs/vm/instruction-set.md; docs/knowledge/README.md; docs/ai/rag-knowledge-base.md; CMakeLists.txt; lua.slnx; lua.vcxproj; lua_app.vcxproj; lua_test.vcxproj; lua_bytecode.vcxproj
last_checked: 2026-06-13
applies_to: repository overview and contributor entry points
---

# 现代 C++ Lua 5.1.5 解释器

本项目是一个使用现代 C++ 实现的 Lua 5.1.5 解释器，覆盖从源码解析到字节码生成、虚拟机执行、垃圾回收和标准库加载的完整运行链路。它不仅关注 Lua 5.1.5 的兼容实现，也定位为展示 C++17/23 工程实践、可读架构拆分和解释器内部机制的教学示范项目。

项目面向希望研究 Lua 解释器内部机制、嵌入式语言运行时、现代 C++ 类型建模和虚拟机实现的开发者。工程持续以代码可读性、清晰边界和教学价值为核心质量目标，README 只保留稳定的用户入口和技术概览；构建状态、测试数字、兼容性差距和工程状态请以 [docs/status/project-status.md](docs/status/project-status.md) 为准。

[![C++](https://img.shields.io/badge/C%2B%2B-17%2F23-blue)]()[![Lua](https://img.shields.io/badge/Lua-5.1.5-blue)]()[![Platform](https://img.shields.io/badge/platform-Windows%20%2F%20MSVC-blue)]()[![License](https://img.shields.io/badge/license-MIT-green)]()

## 项目简介

解释器以 Lua 5.1.5 的运行时语义和指令模型为主要兼容目标，使用 C++17/23、MSVC 和 Visual Studio/MSBuild 组织工程。项目同时承担现代 C++ 教学标杆的角色：通过清晰的模块边界、自解释的数据结构和可追踪的文档链路，帮助读者理解 Lua 从源码到执行再到内存管理的完整机制。核心实现包括：

- 词法分析器、递归下降语法分析器和 AST 表示。
- AST 到 Lua 5.1 风格 `Proto` / 字节码的编译管线。
- 覆盖 Lua 5.1 指令集的寄存器式虚拟机。
- 基于现代 C++ 类型系统的运行时对象模型。
- 标准库、REPL、脚本执行入口和字节码查看工具。
- 面向 Lua 5.1 官方语义和复杂第三方 Lua 库的兼容性验证。

## 设计哲学

- **可读性优先**：实现更倾向于显式命名、清晰职责和可逐步阅读的代码路径，而不是把编译器、虚拟机和 GC 逻辑压缩到隐式技巧里。
- **边界清晰**：编译器前端、字节码生成、VM 执行、标准库和垃圾回收器分别拥有独立模块，读者可以按链路分段学习，也可以单独研究某个子系统。
- **类型表达语义**：编译管线使用 `ValueResult`、`CondResult`、`LValueRef`、`CallResultInfo` 等显式中间结果类型，让表达式值、条件跳转、左值引用和调用结果在类型层面可区分。
- **现代 C++ 建模**：`Value` 和编译中间结果使用 `std::variant` 表达受约束的动态状态，`std::expected` 用于 parser、codegen 和 VM 边界的错误返回，使控制流和失败路径更直接。
- **教学工具闭环**：源码实现、`docs/walkthroughs/` 引导文档、REPL 元命令和 `lua_bytecode` 工具共同构成“源码 + 文档 + 字节码工具”的学习路径。

## 核心特性

### 编译器前端

- Lexer 支持 Lua 5.1 语法词元、数字字面量、字符串字面量、注释和保留字扫描。
- Parser 将源码构造成 AST，并按语句、表达式、函数、表构造等边界拆分实现。
- CodeGenerator 将 AST lowering 到 `Proto`，包含常量表、局部变量、upvalue、跳转回填、寄存器分配和多返回值处理。
- 编译管线采用 `SymbolRef`、`ValueResult`、`CondResult`、`LValueRef`、`CallResultInfo` 等显式中间结果类型，降低表达式和语句 lowering 的耦合度。

### 虚拟机执行引擎

- 支持 Lua 5.1 风格的全量 38 条 VM 指令。
- 采用寄存器式执行模型，覆盖算术、比较、跳转、表访问、闭包、调用、返回、尾调用和泛型 `for` 等核心路径。
- `lua_app` 提供脚本执行和交互式 REPL。
- `lua_bytecode` 可查看编译后的 Proto、指令、常量表、子 Proto、side-by-side diff 和 Mermaid 控制流图。

### 现代 C++ 运行时模型

- `Value` 使用 `std::variant` 表示 Lua 的动态值类型，在 C++ 侧保持类型安全的访问边界。
- 核心运行时对象包括 `Table`、`Function`、`Proto`、`GCString`、`Userdata`、`Thread` 和 `Upvalue`。
- 项目统一使用 `src/common/types.hpp` 中的类型别名，如 `Vec<T>`、`HashMap<K, V>`、`Str`、`StrView`、`usize`、`i32`、`u32` 和 `f64`。
- `RuntimeServices` 和 `EngineContext` 为嵌入式运行时隔离、测试夹具和多上下文执行提供清晰边界。

### 内存管理与 GC

- GC 对象采用三色标记模型，并围绕字符串、表、函数、闭包上值、线程和 userdata 建立统一对象生命周期。
- `GCStrategy` 提供 mark-sweep 与增量垃圾回收（Incremental GC）策略边界，`collectgarbage` 控制路径覆盖收集、停止、恢复、分步推进和参数控制。
- 弱表、userdata `__gc` 终结器、open upvalue、运行时根集和写屏障路径均有专门实现和测试覆盖。

### 标准库与兼容性验证

- 标准库按 catalog 方式注册，覆盖 base、math、string、table、io、os、coroutine、debug 和 package 等 Lua 5.1 常用库。
- REPL 支持元命令、历史记录、增量解析、Tab 补全、字节码查看、AST 查看和 GC 信息查询。
- 兼容性验证包含 Lua 5.1 官方测试套件的 staged smoke、项目内 Lua 回归脚本和 C++ 单元测试。
- 项目包含复杂第三方 Lua 库 `alien-signals-in-lua` 的运行验证，用于检验闭包、元表、协程、模块加载、debug 反射和嵌套表操作等组合场景。

## 快速开始

### 环境要求

- Windows 10/11。
- Visual Studio / MSVC，支持 C++17/23。
- MSBuild。
- Git。
- 可选：CMake / CTest，用于辅助构建和烟测路径。

### 使用 Visual Studio

```powershell
git clone <repository-url>
cd lua
start lua.slnx
```

在 Visual Studio 中可以直接构建解决方案，或选择单独构建核心库、解释器入口、测试入口和字节码工具。

### 使用批处理脚本构建

仓库保留了面向 Windows / MSBuild 的批处理构建入口。所有脚本默认执行 `Rebuild`，并固定使用 `Debug|x64`：

| 脚本 | 对应项目 | 产物 |
|------|----------|------|
| `bin/build_lua.bat` | `lua.vcxproj` | `lua.lib`（核心静态库） |
| `bin/build_app.bat` | `lua_app.vcxproj` | `lua_app.exe` |
| `bin/build_test.bat` | `lua_test.vcxproj` | `lua_test.exe` |
| `bin/build_bytecode.bat` | `lua_bytecode.vcxproj` | `lua_bytecode.exe` |

示例：

```bat
cd lua
bin\build_lua.bat
bin\build_app.bat
bin\build_test.bat
bin\build_bytecode.bat
```

### 运行解释器

```powershell
bin\lua_app.exe examples\hello.lua
bin\lua_app.exe examples\control_flow.lua
bin\lua_app.exe examples\tables_and_methods.lua
bin\lua_app.exe examples\metamethods.lua
```

不传脚本时，`lua_app` 会进入 REPL：

```powershell
bin\lua_app.exe
```

### 运行测试

```powershell
bin\lua_test.exe
bin\lua_test.exe --list
bin\lua_test.exe --filter "Symbol Binding"
bin\lua_test.exe --report=junit
```

测试运行器会在输出中报告真实测试数量和断言结果。最近一次完整绿跑为 668 registered tests / 3404 assertion results / 0 failures。动态测试统计和质量门状态请查看 [docs/status/project-status.md](docs/status/project-status.md)。

### CMake / CTest 辅助路径

CMake 是辅助构建路径，不替代主要的 Visual Studio/MSBuild 工作流：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_cmake_smoke.ps1
```

如果 CMake 与 CTest 已在 PATH，也可以直接运行：

```powershell
cmake -S . -B build\cmake
cmake --build build\cmake --config Debug
ctest --test-dir build\cmake -C Debug --output-on-failure
```

## 项目结构

### 目录结构

```text
├── src/                           # 核心源码
│   ├── app/                       # 命令行参数解析与应用入口辅助逻辑
│   ├── bytecode/                  # 字节码工具入口与打印器
│   ├── common/                    # 基础类型、配置、宏
│   ├── compiler/                  # Lexer / Parser / AST / CodeGen
│   ├── core/                      # Value / Table / Function / String / Metatable 等核心对象
│   ├── gc/                        # 垃圾回收器与 GCStrategy 策略边界
│   ├── io/                        # 输入流、文件加载、动态缓冲区
│   ├── lib/                       # base / math / io / os / string / table / coroutine / debug / package 等标准库
│   ├── repl/                      # REPL completion / history / meta / signals / prompt / execution helpers
│   ├── runtime/                   # RuntimeServices 与 EngineContext
│   ├── vm/                        # GlobalState / LuaState / Stack / VM
│   ├── main.cpp                   # `lua_app` 入口
│   └── repl.cpp/.hpp              # REPL 公共入口与会话循环
├── tests/
│   ├── lua/                       # Lua 脚本级测试样例
│   │   └── official/              # Lua 5.1 官方测试套件 staged smoke 输入
│   └── unit/                      # C++ 单元测试
│       ├── app/                   # CLI / REPL 行为测试
│       ├── bytecode/              # 字节码工具测试
│       ├── compiler/              # 编译器相关测试
│       ├── core/                  # 核心对象测试
│       ├── framework/             # 测试框架自身
│       ├── gc/                    # GC 测试
│       ├── io/                    # I/O 测试
│       ├── metamethod/            # 元方法测试
│       ├── official/              # 官方 Lua 5.1 suite 入口测试
│       ├── stdlib/                # 标准库测试
│       └── vm/                    # VM / LuaState / RuntimeServices 测试
├── docs/                          # 项目文档
├── examples/                      # 可直接运行的 Lua 示例
├── tools/                         # 构建、质量门和文档漂移检查脚本
├── bin/                           # 编译批处理脚本与本地构建产物
├── lua.slnx                       # Visual Studio 解决方案
├── lua.vcxproj                    # 核心静态库项目
├── lua_app.vcxproj                # 解释器 / REPL 项目
├── lua_test.vcxproj               # 单元测试项目
├── lua_bytecode.vcxproj           # 字节码打印与对比工具项目
├── CMakeLists.txt                 # CMake 辅助构建入口
└── README.md
```

### 关键路径

| 路径 | 用途 |
|------|------|
| `src/` | 解释器产品源码 |
| `src/main.cpp` | `lua_app.exe` 的主入口 |
| `src/bytecode/bytecode_main.cpp` | `lua_bytecode.exe` 的主入口 |
| `src/bytecode/bytecode_printer.cpp` | 字节码、常量表、diff 和 CFG 输出层 |
| `tests/unit/` | C++ 单元测试 |
| `tests/lua/` | Lua 脚本级样例、回归测试和官方测试输入 |
| `docs/` | 架构、指南、兼容性和路线图文档 |
| `lua.slnx` | Visual Studio 解决方案入口 |

## 重要文档索引

README 是项目入口，不承载动态进度。深入理解学习路线、架构、开发规范、兼容性和路线图时，请按下表进入对应文档。

### 教学 walkthrough

`docs/walkthroughs/` 是项目教学价值的核心入口，建议配合源码和 `lua_bytecode` 一起阅读：

| 文档 | 学习目标 |
|------|----------|
| [docs/walkthroughs/hello-world.md](docs/walkthroughs/hello-world.md) | 从 `print("hello")` 追踪 Lexer、Parser、AST、CodeGen、字节码、VM dispatch 和标准库调用 |
| [docs/walkthroughs/closure-and-upvalue.md](docs/walkthroughs/closure-and-upvalue.md) | 理解闭包、upvalue 捕获、open/closed 生命周期和函数调用栈 |
| [docs/walkthroughs/gc-cycle.md](docs/walkthroughs/gc-cycle.md) | 观察 GC 根集、标记、扫描、清扫、弱表和终结器相关路径 |

推荐方式是先运行示例脚本，再用 `lua_bytecode` 查看 Proto 和指令，最后回到对应源码文件阅读实现。这样可以把 Lua 源码、字节码形状和 C++ 实现联系起来。

### 文档入口

| 文档 | 内容 |
|------|------|
| [docs/status/project-status.md](docs/status/project-status.md) | 构建入口、测试状态、兼容性边界和质量门事实源 |
| [docs/index.md](docs/index.md) | 新读者的推荐阅读顺序 |
| [docs/learning-roadmap.md](docs/learning-roadmap.md) | 面向新开发者的学习路线图，串联 walkthrough、教学脚本、`lua_app` / `lua_bytecode` 工具和源码阅读入口 |
| [docs/architecture/overview.md](docs/architecture/overview.md) | 架构分层、模块关系和执行链路总览 |
| [docs/architecture/gc.md](docs/architecture/gc.md) | GC 对象模型、根集、标记清除和策略边界 |
| [docs/architecture/runtime-services.md](docs/architecture/runtime-services.md) | RuntimeServices、EngineContext 和嵌入式上下文隔离 |
| [docs/guides/development.md](docs/guides/development.md) | 开发环境、编码约定、构建和测试流程 |
| [docs/guides/repl-cli.md](docs/guides/repl-cli.md) | `lua_app` 命令行和 REPL 使用说明 |
| [docs/guides/test-runner.md](docs/guides/test-runner.md) | `lua_test` 参数、过滤和报告输出 |
| [docs/guides/bytecode-tool.md](docs/guides/bytecode-tool.md) | `lua_bytecode` 用法、diff 和 CFG 输出 |
| [docs/compiler/bytecode-generation.md](docs/compiler/bytecode-generation.md) | AST 到 Proto 的字节码生成主线 |
| [docs/compiler/codegen-responsibility-map.md](docs/compiler/codegen-responsibility-map.md) | CodeGenerator 物理拆分和职责边界 |
| [docs/vm/instruction-set.md](docs/vm/instruction-set.md) | Lua 5.1 风格 VM 指令说明 |
| [docs/vm/trace-system.md](docs/vm/trace-system.md) | VM trace 和 trace diff 机制 |
| [docs/stdlib/overview.md](docs/stdlib/overview.md) | 标准库 catalog、注册方式和兼容性说明 |
| [docs/compatibility/lua51.md](docs/compatibility/lua51.md) | Lua 5.1 兼容性分章节记录 |
| [docs/compatibility/lua51-full-compatibility-audit.md](docs/compatibility/lua51-full-compatibility-audit.md) | 完整兼容性审计 |
| [docs/roadmap/lua51-compatibility-next-stage.md](docs/roadmap/lua51-compatibility-next-stage.md) | 后续兼容性工作入口 |
| [docs/roadmap/optimization_and_refactoring.md](docs/roadmap/optimization_and_refactoring.md) | 可读性、现代 C++ 应用和教学价值的工程质量路线 |
| [examples/README.md](examples/README.md) | 示例脚本运行说明 |

推荐阅读路径：

```text
README
  -> docs/status/project-status.md
  -> docs/index.md
  -> docs/learning-roadmap.md
  -> docs/architecture/overview.md
  -> docs/walkthroughs/hello-world.md
  -> docs/guides/development.md
  -> docs/compatibility/lua51.md
```

## 子项目说明

本仓库包含一个 Visual Studio 解决方案，用于组织核心库、运行入口、测试程序和字节码工具项目。

| 项目文件 | 输出类型 | 说明 |
|---------|---------|------|
| `lua.vcxproj` | 静态库（`lua.lib`） | 核心库，包含 Lexer、Parser、CodeGen、VM、GC、标准库和运行时对象模型 |
| `lua_app.vcxproj` | 可执行文件（`lua_app.exe`） | 解释器与 REPL 入口，支持脚本执行、交互模式、元命令、补全和 trace |
| `lua_test.vcxproj` | 可执行文件（`lua_test.exe`） | 单元测试运行器，覆盖 compiler、core、gc、stdlib、vm、app 等模块 |
| `lua_bytecode.vcxproj` | 可执行文件（`lua_bytecode.exe`） | 字节码工具入口，支持 Proto 输出、递归子 Proto、diff 和 Mermaid CFG |

使用建议：

- 开发解释器核心功能时，优先维护 `lua.vcxproj` 对应的库源码。
- 手动运行脚本或 REPL 时，使用 `lua_app.vcxproj` / `bin\lua_app.exe`。
- 验证模块行为和回归问题时，使用 `lua_test.vcxproj` / `bin\lua_test.exe`。
- 观察 Parser / CodeGen / Proto 输出时，使用 `lua_bytecode.vcxproj` / `bin\lua_bytecode.exe`。

## 技术约定

### 类型系统

项目统一使用 `src/common/types.hpp` 中定义的类型别名：

| C++ 标准类型 | 项目类型别名 | 用途 |
|--------------|--------------|------|
| `std::vector<T>` | `Vec<T>` | 动态数组 |
| `std::unordered_map<K, V>` | `HashMap<K, V>` | 哈希表 |
| `std::string` | `Str` | 字符串 |
| `std::string_view` | `StrView` | 字符串视图 |
| `size_t` | `usize` | 无符号大小类型 |
| `int32_t` | `i32` | 32 位有符号整数 |
| `uint32_t` | `u32` | 32 位无符号整数 |
| `int64_t` | `i64` | 64 位有符号整数 |
| `uint64_t` | `u64` | 64 位无符号整数 |
| `double` | `f64` | 64 位浮点数 |

新增代码应优先使用这些别名，以保持源码风格、可读性和跨模块一致性。类型别名不是单纯的缩写，而是让容器、字符串、整数宽度和 Lua 数值语义在函数签名中更容易识别。

### `Value` 表示

Lua 动态值在 C++ 中由 `std::variant` 建模：

```cpp
using ValueData = std::variant<
    std::monostate,
    bool,
    f64,
    void*,
    GCString*,
    Table*,
    Function*,
    Userdata*,
    Thread*
>;
```

### 现代 C++ 边界处理

项目有意识地使用现代 C++ 特性服务于代码清晰度：

- `std::variant` 用于 `Value` 和编译器中间结果，避免无约束字段组合，让状态空间在类型层面可见。
- `std::expected` 用于 Parser、CodeGenerator 和 VM 的边界返回，调用方可以直接区分成功值和结构化错误，而不是依赖散落的异常捕获。
- CRTP visitor 和 concepts 用于 AST 访问覆盖检查，使新增节点时的遗漏更早暴露在编译期。
- `RuntimeServices` / `EngineContext` 显式传递运行时依赖，降低全局单例对阅读、测试和嵌入式场景的干扰。

这些约定与 [docs/roadmap/optimization_and_refactoring.md](docs/roadmap/optimization_and_refactoring.md) 中“可读性、现代 C++ 应用、教学价值”的质量目标保持一致：代码应尽量让读者看见边界、看见数据流，也看见失败路径。

### 质量门

常用验证入口：

```powershell
bin\lua_test.exe
powershell -NoProfile -ExecutionPolicy Bypass -File tools\run_quality_gate.ps1
```

新增 C++ 源文件时，优先使用 `tools\add_source.ps1` 同步 CMake、`.vcxproj` 和 `.vcxproj.filters` 清单：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools\add_source.ps1 `
  -SourcePath src\gc\new_phase.cpp, src\gc\new_phase.hpp `
  -Target Core
```

更多规范见 [docs/guides/development.md](docs/guides/development.md)。

## 参考资源

- [Lua 官方网站](https://www.lua.org/)
- [Lua 5.1 Reference Manual](https://www.lua.org/manual/5.1/)

## 致谢

感谢 Lua 团队创造了简洁、高效、可嵌入的 Lua 语言，以及 Lua 社区长期积累的文档、测试和实现经验。

## 许可证

本项目采用 [MIT 许可证](LICENSE)。
