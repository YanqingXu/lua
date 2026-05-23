---
status: current
verified_against: docs/status/project-status.md; lua.slnx; lua.vcxproj; lua_app.vcxproj; lua_test.vcxproj; lua_bytecode.vcxproj; CMakeLists.txt; tools/run_cmake_smoke.ps1
last_checked: 2026-05-23
applies_to: repository overview and current build workflows
---

# 现代C++ Lua解释器

> **从零开始用C++17/20/23实现Lua 5.1.5解释器**

[![Tests](https://img.shields.io/badge/tests-2735%2F2735-brightgreen)]()
[![Coverage](https://img.shields.io/badge/coverage-100%25-brightgreen)]()
[![C++](https://img.shields.io/badge/C%2B%2B-17%2F23-blue)]()
[![Platform](https://img.shields.io/badge/platform-Windows-blue)]()
[![Progress](https://img.shields.io/badge/progress-95%25-yellow)]()
[![Code](https://img.shields.io/badge/code-19k%20lines-blue)]()
[![Last Updated](https://img.shields.io/badge/updated-2026--05--23-blue)]()

---

## 🎯 项目概览

本项目是一个**使用现代 C++ 重新实现 Lua 5.1.5 解释器**的实验性工程，当前主要面向学习、验证和逐步补全实现。

> 当前构建、测试、工具链与编译器管线事实以 `docs/status/project-status.md` 为准。README 只保留概览信息，避免与工程文件和开发指南重复漂移。

### 项目目标

本项目旨在从零开始，用现代 C++ 重建 Lua 5.1.5 的核心执行链路，包括词法分析、语法分析、字节码生成、虚拟机执行、垃圾回收和标准库。

**核心特点**：
-  **技术路线明确**：当前主路径为 Windows + Visual Studio/MSBuild + `.vcxproj`；CMake/CTest 已作为 secondary 辅助路径落地
- ✨ **实现风格现代**：使用 `std::variant`、类型别名、STL 容器等现代 C++ 手段组织 Lua 运行时
- 🎓 **偏学习型工程**：强调结构可读、模块可追踪、便于理解 Lua 语言设计

---

## 📊 当前进度

### 总体状态

- **整体完成度**：约 95%
- **代码规模**：约 89 个源文件，约 19k 行有效代码
- **核心链路**：类型系统、编译器前端、字节码执行引擎已基本成型
- **主要短板**：部分标准库函数尚有缺失或不完整实现

### 当前判断

- ✅ **已较稳定的部分**：Value / Table / Function / Lexer / Parser / CodeGen / VM 主体
- ✅ **已补齐的初始化部分**：GlobalState 元方法、保留字、固定字符串初始化
- ✅ **GC / pcall / xpcall**：已全部通过测试，基础行为稳定
- ✅ **GC 最新进度**：`collectgarbage("collect")` 已恢复真实标记-清除流程；VM/标准库创建的主要 GCObject 已统一注册，Table/Function/Proto/Upvalue/Userdata/Thread 标记路径已补齐，根集会扫描全局状态、当前/主线程栈和 open upvalue；弱表 `__mode` 与 userdata `__gc` 终结器已接入；`GCStrategy` 已抽出 `MarkSweepGC` 与 incremental 教学占位策略
- ✅ **CodeGen 重构状态**：`ExprDesc` / `ExprKind` 已从产品代码移除，PR-C1~PR-C5 清理完成，寄存器指针访问已封装
- ✅ **闭包语义**：upvalue 捕获、写回、嵌套捕获链，以及作用域退出 / `break` 时的 `OP_CLOSE` 关闭路径已覆盖
- ✅ **尾调用优化（TCO）**：CodeGen 已对单值 `return f()` 发出 `TAILCALL`，VM 侧 Lua 尾调用会复用当前 `CallInfo`/栈帧
- ✅ **元方法最新进度**：`callTMWithResult/callTM` 已统一走 `VM::call`，Lua 函数形式的 `__add/__index/__newindex/__call` 等元方法已可用；基础类型元表查找已接入，string 类型已安装 `__index = string`
- ✅ **标准库状态**：math / os / io / string / table / coroutine / debug / package 库主体已实现（少量函数仍为 stub 或简化）
- ✅ **字节码工具状态**：`lua_bytecode` 已支持 compact / full 打印、side-by-side `--diff`，以及 `--cfg` Mermaid basic-block 控制流图输出

### 已完成模块（核心模块）

| 模块 | 文件 | 代码行数 | 功能描述 | 完成度 |
|------|------|----------|---------|--------|
| **基础类型系统** | `src/common/types.hpp` | 386 | 类型别名定义（Vec、HashMap、usize等） | ✅ 100% |
| **配置系统** | `src/common/config.hpp` | 378 | 编译配置和常量定义 | ✅ 100% |
| **宏定义** | `src/common/macros.hpp` | 377 | 实用宏定义 | ✅ 100% |
| **Value类** | `src/core/value.hpp/cpp` | 490 | Lua值的C++表示（std::variant） | ✅ 100% |
| **GCObject基类** | `src/core/gc_object.hpp/cpp` | 308 | GC对象基类（三色标记） | ✅ 100% |
| **GCString类** | `src/core/gc_string.hpp/cpp` | 251 | GC管理的字符串对象 | ✅ 100% |
| **StringPool类** | `src/core/string_pool.hpp/cpp` | 260 | 字符串驻留池（单例模式） | ✅ 100% |
| **Table类** | `src/core/table.hpp/cpp` | 683 | Lua表（数组+哈希混合存储），支持 GC 弱键/弱值清理辅助路径 | ✅ 97% |
| **Function类** | `src/core/function.hpp/cpp` | 1,096 | 函数对象（Proto + Closure + Upvalue） | ✅ 100% |
| **Upvalue类** | `src/core/upvalue.hpp/cpp` | 419 | 闭包上值管理（Open/Closed状态） | ✅ 100% |
| **Userdata类** | `src/core/userdata.hpp/cpp` | 282 | 用户数据（C++数据包装） | ✅ 100% |
| **Metatable元方法系统** | `src/core/metatable.hpp/cpp` | 697 | 17种元方法名称、table/userdata 与基础类型元表查找；C/Lua 函数元方法统一调用；`__gc/__mode` 名称供 GC 使用 | ✅ 96% |
| **GarbageCollector** | `src/gc/garbage_collector.hpp/cpp` + `src/gc/gc_strategy.hpp/cpp` | 808+ | 标记-清除、根集扫描、弱表清理、userdata `__gc` 两阶段终结、`GCStrategy` 策略边界与 `collectgarbage("strategy", ...)` 接入 | ✅ 98% |
| **GlobalState类** | `src/vm/state/global_state.hpp/cpp` | 261 | 全局状态管理（单例模式） | ✅ 95% |
| **Stack类** | `src/vm/state/stack.hpp/cpp` | 394 | 值栈管理（动态扩展） | ✅ 100% |
| **CallInfo类** | `src/vm/state/call_info.hpp` | 197 | 调用信息（函数调用上下文） | ✅ 100% |
| **LuaState类** | `src/vm/state/lua_state.hpp/cpp` | 1,095 | Lua状态（线程执行环境） | ✅ 95% |
| **Lexer词法分析器** | `src/compiler/parser/lexer.hpp/cpp` + `src/compiler/parser/token.hpp` | 1,167 | 词法分析（Token流生成） | ✅ 100% |
| **Parser语法分析器** | `src/compiler/parser/parser.hpp/cpp` + `src/compiler/parser/parser_*.cpp` + `src/compiler/ast.hpp/cpp` | 2,128 | 语法分析（AST生成，Parser 实现已按语句/表达式/函数/表构造分片） | ✅ 100% |
| **CodeGenerator字节码生成器** | `src/compiler/codegen/codegen.hpp/cpp` + `src/compiler/opcode.hpp/cpp` | 2,249 | 字节码生成（AST→Bytecode），单值 return call 已生成 TAILCALL | ✅ 95% |
| **VM字节码执行引擎** | `src/vm/vm.hpp/cpp` | 2,030 | 38条指令均有执行分支；TAILCALL 已复用栈帧，TFORLOOP 已支持 C/Lua 函数迭代器 | ✅ 95% |
| **I/O系统** | `src/io/*.hpp/cpp` | 670 | InputStream + DynamicBuffer | ✅ 100% |
| **基础库（Base Library）** | `src/lib/baselib.hpp/cpp` | 730+ | Lua 5.1 常用全局函数 + `_G` / `_VERSION`（newproxy 缺失，部分边界简化） | 🔄 93% |
| **数学库（Math Library）** | `src/lib/mathlib.hpp/cpp` | 681 | 28/28函数（含 sinh、cosh、tanh） | ✅ 100% |
| **I/O库（I/O Library）** | `src/lib/iolib.hpp/cpp` | 1,111 | 11/11函数 + 7/7文件方法主体实现；lines格式参数仍简化 | ✅ 95% |
| **字符串库（String Library）** | `src/lib/stringlib.hpp/cpp` | ~1,400 | 14/14函数（gsub 表/函数替换、dump、二进制安全入口已补齐） | ✅ 95% |
| **表库（Table Library）** | `src/lib/tablelib.hpp/cpp` | ~430 | Lua 5.1 核心函数 5/5 + 5.2 风格扩展；concat 边界仍需对齐 | ✅ 95% |
| **OS库（OS Library）** | `src/lib/oslib.hpp/cpp` | ~500 | 11/11函数主体实现；remove/rename 失败返回细节仍简化 | ✅ 95% |
| **协程库（Coroutine Library）** | `src/lib/coroutinelib.hpp/cpp` | ~210 | 6/6函数（create、resume、yield、status、running、wrap） | ✅ 100% |
| **Thread类** | `src/core/thread.hpp/cpp` | ~300 | 协程执行引擎，独立 LuaState + 栈转移 | ✅ 95% |
| **调试库（Debug Library）** | `src/lib/debuglib.hpp/cpp` | ~1,100 | 14/14函数表面实现（env/线程/栈层级边界仍简化） | 🔄 90% |
| **包/模块库（Package Library）** | `src/lib/packagelib.hpp/cpp` | ~830 | require、module、package.loaded/preload/loaders/path/cpath/loadlib/seeall；module 复合名/环境语义与 C loader/all-in-one loader 已补齐 | ✅ 98% |
| **库管理系统** | `src/lib/lib_manager.hpp/cpp` | ~130 | catalog 驱动的标准库注册和单库加载 | ✅ 100% |

### 测试统计（2026-05-23 更新）✅

```
测试框架：自定义轻量级测试框架（零外部依赖）
注册测试：544个
断言结果：2735个 ✅
通过率：  100% (2735/2735)
失败测试：0个
编译状态：Debug|x64 `/W4` 版本无警告，无链接冲突
平台：    Windows + MSVC (Visual Studio 2026)
```

补充验证：
- `bin/build_test.bat`：通过
- `bin/lua_test.exe`：544 个注册测试，2735 个结果，0 失败（0 failures）
- `bin/build_app.bat`：通过
- `tests/lua/regressions/*.lua`：全部通过
- `tests/lua/stdlib/test_collectgarbage*.lua` 与 `test_gcinfo*.lua`：全部通过，`collectgarbage("collect")` 可观察到内存下降

### 距离完整 Lua 5.1.5 仍缺失的功能

> **兼容性审计记录（2026-05-23）**：已对 README、本地 `src/lib/` 标准库实现、`src/vm/` 指令执行逻辑、GC/元方法相关核心代码进行核对。`bin/lua_test.exe` 当前为 544 个注册测试、2735 个结果、0 失败；这说明项目内测试覆盖的路径稳定，但不等价于 Lua 5.1.5 官方语义已经达到 95% 兼容。

> **最新补齐**：Lua 函数元方法调用链已打通，`callTMWithResult/callTM` 统一走 `VM::call`，C Closure 与 Lua Closure 共用同一调用入口；`getMetamethodByObject()` 已接入基础类型元表，string 类型已安装 `__index = string`；GC 已支持弱表 `__mode = "k"/"v"/"kv"` 清理、userdata `__gc` 两阶段终结和 `GCStrategy` 教学策略边界；尾调用优化已覆盖单值 `return f()` 的 `TAILCALL` 生成和 Lua 函数调用帧复用；泛型 `for` 已支持显式 iterator 三元组、函数/vararg 三值调整和 Lua 函数迭代器；string 库已补齐 `gsub` 表/函数替换、`string.dump` Proto/字节码序列化输出和含 `\0` 字符串的长度安全处理；`module()` 已补齐 `_PACKAGE`、复合模块名全局路径、调用方环境切换和 Lua option 函数调用语义；package 已补齐 `package.loadlib`、C loader 和 all-in-one C loader 动态加载边界。

#### 🔴 关键缺失（影响语义正确性）

当前没有单独列出的红色关键缺失；剩余高权重兼容边界集中在标准库和错误/调试语义。

#### 🟡 标准库函数缺失/不完整

| 缺失项 | 所属模块 | 说明 |
|--------|----------|------|
| **`error()` level 参数** | base 库 | level 已解析但不会在错误消息前添加源位置信息；非字符串错误对象也未完全按 Lua 语义保留 |
| **`pcall/xpcall` 错误处理** | base 库 / LuaState | `xpcall` 会传入错误处理器位置，但 `LuaState::pcall()` 中 `errfunc` 调用仍是 TODO |
| **`newproxy()`** | base 库 | 未实现（Lua 5.1 未文档化但存在的兼容函数） |
| **`loadfile()` / `dofile()` 无参 stdin** | base 库 | 无参数时应从标准输入读取；当前返回“不支持 stdin”错误 |
| **`debug.getfenv/setfenv` 栈层级/线程环境** | debug/base 库 | 函数对象路径已支持；栈层级、线程环境和 C 函数环境仍未完整兼容 |
| **`debug.getmetatable/setmetatable` 基础类型** | debug 库 / Metatable | VM/base 层基础类型元表已接入；debug 库路径仍只支持 table/userdata，不能完整操作 string/number/boolean 等基础类型元表 |
| **`io.lines/file:lines` 格式参数** | io 库 | `io.lines` 和 `file:lines` 对 Lua 5.1 的 read format 参数仍直接报 not supported |
| **`os.remove/os.rename` 失败返回值** | os 库 | 失败时只返回 `nil`，未返回错误消息和系统错误码 |
| **`table.concat` 边界** | table 库 | 当前会跳过 nil 并接受 boolean；Lua 5.1 要求数组元素为 string/number，非法值应报错 |

#### 🟠 语义简化但已部分可用

| 项目 | 当前状态 |
|------|----------|
| **`collectgarbage` 控制路径** | `"count"` 可用；`"collect"` 已通过当前 `GCStrategy` 触发完整标记-清除流程，并扫描全局状态、当前线程栈、主线程栈和 open upvalue；`"strategy"` 可查询/切换 `mark-sweep` 与 incremental 教学占位策略；`"stop"`、`"restart"`、`"step"`、`"setpause"`、`"setstepmul"` 仍是占位返回 |
| **表长度 `#t`** | `Table::length()` 返回数组部分最后一个非 nil 索引；对有洞数组未完全复现 Lua 5.1 边界定义 |
| **元方法覆盖面** | C 函数与 Lua 函数元方法、table/userdata 与基础类型查找路径已有测试；`__gc/__mode` 已由 GC 消费；debug 库基础类型元表操作仍是兼容边界 |
| **错误对象与 traceback** | 基础 traceback/debug hook 已有实现；错误位置 level、任意类型 error object、xpcall handler 仍不完整 |
| **Lua/动态 C 模块加载** | `require()` 可通过 `package.path` 加载 Lua 文件，也可通过 `package.cpath` 加载 C 模块；更细的系统 loader 搜索错误文本仍可继续对齐官方实现 |

#### ⚪ 低优先级/已废弃

| 缺失项 | 说明 |
|--------|------|
| `table.getn` / `table.setn` | Lua 5.1 已废弃，兼容性函数 |
| `table.foreachi` / `table.foreach` | Lua 5.1 已废弃，兼容性函数 |

#### 95% 完成度复核

当前“95%”更适合理解为**项目内功能和测试进度**，而不是严格的 Lua 5.1.5 官方兼容率。标准库主体、编译器前端、VM 主要指令路径、GC 基础/高级生命周期、Lua 函数元方法调用链、泛型 `for`、string 兼容性补强、`module()` 语义、package 动态 C 加载以及尾调用栈帧复用已经成型并全绿；但 debug/base 小边界仍是高权重语义项，若按官方规范和第三方 Lua 5.1 测试套衡量，实际兼容度应更保守。

### 接下来优先做什么

已完成：真正的尾调用优化（TCO）已覆盖单值 `return f()` 的字节码生成和 Lua 函数尾调用的栈帧复用，并新增 `TAILCALL` 字节码与深尾递归帧深度回归测试；泛型 `for` 已补齐 CodeGen iterator 表达式调整和 VM `TFORLOOP` Lua 函数迭代器调用路径；string 兼容性已补齐 `string.gsub` 表/函数替换、`string.dump` 和二进制安全字符串处理；`module()` 已补齐复合名、`_PACKAGE`、调用方环境和 option 函数语义；package 动态加载已补齐 `package.loadlib`、C loader 和 all-in-one loader。

1. **补错误与调试边界**：`error(level)` 位置信息、任意错误对象、`xpcall` errfunc、`getfenv/setfenv` 栈层级/线程环境、debug 库基础类型元表操作
2. **补小型标准库边界**：`io.lines/file:lines` 格式参数、`os.remove/os.rename` 失败返回值、`table.concat` 类型边界
3. **引入官方兼容测试**：在现有单元测试全绿基础上，逐步接入 Lua 5.1 官方/社区行为测试，避免只验证 happy path

### 核心实现亮点

✅ **Value类**：使用`std::variant`实现类型安全的动态类型系统
✅ **GCObject**：三色标记（White/Gray/Black）支持增量GC
✅ **Table类**：混合存储（数组部分 + 哈希部分），自动优化
✅ **Function类**：支持C函数和Lua函数两种闭包类型，集成Upvalue管理
✅ **Upvalue类**：闭包上值管理，支持Open/Closed状态转换，共享机制
✅ **Userdata类**：完整用户数据支持，8字节对齐，元表支持，GC集成
✅ **Metatable元方法系统**：已初始化17种元方法名称并支持 table/userdata 与基础类型元表查找；`callTMWithResult/callTM` 已统一走 `VM::call`，C/Lua 函数元方法均可用；string 类型已安装 `__index = string`；`__gc`、`__mode` 已接入 GC 语义
✅ **Lexer词法分析器**：完整Lua 5.1词法规则，支持所有关键字、运算符、字面量、注释
✅ **Parser语法分析器**：递归下降解析，完整AST生成，正确的运算符优先级和结合性；实现已按语句、表达式、函数和表构造边界分片
✅ **CodeGenerator字节码生成器**：AST→字节码转换，寄存器分配，常量表管理，跳转回填；`ExprDesc` / `ExprKind` 迁移和 PR-C 清理已完成
✅ **OpCode指令集**：完整Lua 5.1指令集（38条指令），iABC/iABx/iAsBx三种格式
✅ **VM字节码执行引擎**：38条指令均有执行分支，已覆盖 Upvalue、函数调用、尾调用栈帧复用、循环、闭包、SETLIST、TFORLOOP C/Lua 迭代器等主路径
✅ **基础库（Base Library）**：Lua 5.1 常用全局函数已覆盖，包含 `_G` / `_VERSION`、print、type、tostring、tonumber、error、assert、pcall、xpcall、pairs、ipairs、next、select、rawget、rawset、rawequal、loadstring、loadfile、dofile、collectgarbage、unpack、load、getfenv、setfenv 等
✅ **数学库（Math Library）**：28/28函数完整实现（abs, floor, ceil, sqrt, sin, cos, tan, sinh, cosh, tanh, log, exp, random 等），包括数学常量 math.pi 和 math.huge
✅ **I/O库（I/O Library）**：11/11函数 + 7/7文件方法主体实现（io.open, io.close, io.read, io.write, file:read, file:write 等），`io.lines/file:lines` 格式参数仍简化
✅ **协程库（Coroutine Library）**：6/6函数实现（coroutine.create、resume、yield、status、running、wrap），支持独立栈协程执行
✅ **调试库（Debug Library）**：14/14函数表面实现（debug.getinfo、getlocal、setlocal、getupvalue、setupvalue、traceback、sethook、gethook、getregistry、getmetatable、setmetatable、getfenv、setfenv、debug），支持运行时调试能力
✅ **包/模块库（Package Library）**：require、module 全局函数 + package.loaded/preload/loaders/path/cpath/config/loadlib/seeall，支持 preload、Lua 文件模块、动态 C 模块和 all-in-one C 模块加载
✅ **库管理系统**：模块化的标准库注册机制，支持 catalog 驱动的全量注册、单库加载和表函数注册
✅ **StringPool**：字符串驻留（interning），节省内存
✅ **GarbageCollector**：真实标记-清除流程、根集扫描、对象生命周期统一注册、弱表清理和 userdata `__gc` 两阶段终结，`GCStrategy` 已抽出 `MarkSweepGC` 与 incremental 教学占位策略，`collectgarbage("collect")` / `collectgarbage("strategy")` 已接入
✅ **GlobalState**：单例模式管理全局资源（字符串池、GC、注册表）
✅ **Stack**：动态值栈，自动扩展，O(1)压栈/弹栈操作
✅ **CallInfo**：轻量级调用上下文，支持函数调用链管理
✅ **LuaState**：完整的线程执行环境，整合栈、调用信息和Upvalue链表，扩展了30+个API方法支持基础库

### 虚拟机核心模块详解

#### GlobalState（全局状态）

**设计模式**：单例模式

**核心职责**：
- 管理所有线程共享的全局资源
- 字符串池（StringPool）的访问入口
- 垃圾回收器（GarbageCollector）的访问入口
- 注册表（Registry）管理：C代码专用的全局存储
- 元表管理：为基础类型（nil、boolean、number等）提供元表支持
- 主线程引用：维护主线程的指针

**关键特性**：
```cpp
GlobalState& gs = GlobalState::getInstance();  // 单例访问
StringPool& pool = gs.getStringPool();         // 字符串池
GarbageCollector& gc = gs.getGC();             // GC
Table* registry = gs.getRegistry();            // 注册表
gs.setMetatable(ValueType::Number, mt);        // 设置元表
```

**内存布局**：104字节（包含引用和指针数组）

#### Stack（值栈）

**设计模式**：动态数组

**核心职责**：
- 存储函数参数、局部变量和临时值
- 自动扩展：容量不足时自动翻倍
- 高效访问：O(1)压栈、弹栈、索引访问

**关键特性**：
```cpp
Stack stack;
stack.push(Value(42.0));           // 压栈
Value v = stack.pop();             // 弹栈
Value& top = stack.top();          // 访问栈顶
Value& val = stack.at(index);      // 索引访问
stack.ensureSpace(n);              // 确保有n个空闲槽位
```

**常量定义**：
- `MIN_STACK_SIZE = 20`：最小栈大小
- `INITIAL_STACK_SIZE = 40`：初始栈大小
- `EXTRA_STACK = 5`：额外保留空间

**内存布局**：40字节（Vec容器 + top指针）

#### CallInfo（调用信息）

**设计模式**：轻量级结构体

**核心职责**：
- 存储单次函数调用的上下文信息
- 管理栈帧布局（func、base、top）
- 记录返回值数量和尾调用计数

**栈帧布局**：
```
┌─────────────┐ ← top (栈顶)
│  局部变量3  │
│  局部变量2  │
│  局部变量1  │
├─────────────┤ ← base (栈基址)
│   参数2     │
│   参数1     │
│  函数对象   │ ← func
└─────────────┘
```

**关键字段**：
```cpp
CallInfo ci;
ci.func = 10;        // 函数对象在栈索引10
ci.base = 11;        // 参数从索引11开始
ci.top = 20;         // 栈顶在索引20
ci.nresults = 2;     // 期望2个返回值
ci.savedpc = ptr;    // 程序计数器（Lua函数）
ci.tailcalls = 0;    // 尾调用计数
```

**内存布局**：40字节（6个字段）

#### Upvalue（闭包上值）

**设计模式**：状态模式（Open/Closed状态）

**核心职责**：
- 管理闭包捕获的外部变量
- 支持Open状态（指向栈上变量）和Closed状态（独立存储）
- 实现多个闭包共享同一Upvalue的机制
- 参与GC标记和清除

**状态转换**：
```cpp
// Open状态：v_指向栈上的Value
Upvalue* uv = Upvalue::createOpen(&stackValue, stackIndex);
uv->isOpen();  // true
uv->getValue(); // 返回栈上的值

// 关闭操作：将栈上的值复制到closedValue_
uv->close();
uv->isClosed(); // true
uv->getValue(); // 返回closedValue_
```

**共享机制**：
```cpp
// LuaState管理open upvalue链表（按栈索引降序）
Upvalue* uv1 = L->findOrCreateUpvalue(5);  // 创建新upvalue
Upvalue* uv2 = L->findOrCreateUpvalue(5);  // 返回同一个upvalue
assert(uv1 == uv2);  // 共享同一个upvalue
```

**关闭时机**：
```cpp
// 函数返回时关闭所有栈层级 >= level 的upvalue
L->closeUpvalues(level);
```

**内存布局**：64字节（v_指针、stackIndex、closedValue、next指针）

**关键算法**：
- **findOrCreateUpvalue**：在降序链表中查找或创建upvalue
- **closeUpvalues**：批量关闭指定层级以上的upvalue
- **close()**：将Open状态转换为Closed状态

#### LuaState（Lua状态）

**设计模式**：RAII资源管理

**核心职责**：
- 管理单个Lua线程的完整执行状态
- 整合值栈（Stack）和调用栈（CallInfo数组）
- 管理Open Upvalue链表（按栈索引降序）
- 提供栈操作和Upvalue管理的便捷接口
- 管理全局表和线程状态

**关键特性**：
```cpp
LuaState* L = LuaState::newState();  // 创建新状态
L->pushNumber(42.0);                 // 压入数值
L->pushBoolean(true);                // 压入布尔值
L->pushString(str);                  // 压入字符串
Value v = L->pop();                  // 弹出值
Table* gt = L->getGlobalTable();     // 获取全局表
CallInfo& ci = L->getCurrentCallInfo(); // 当前调用信息

// Upvalue管理
Upvalue* uv = L->findOrCreateUpvalue(5);  // 查找或创建upvalue
L->closeUpvalues(10);                     // 关闭栈层级 >= 10 的upvalue
```

**状态枚举**：
```cpp
enum class ThreadStatus {
    OK = 0,         // 正常执行
    Yield = 1,      // 协程挂起
    ErrRun = 2,     // 运行时错误
    ErrSyntax = 3,  // 语法错误
    ErrMem = 4,     // 内存错误
    ErrErr = 5      // 错误处理函数错误
};
```

**初始化流程**：
1. 创建值栈（初始大小40）
2. 创建调用栈（初始大小8）
3. 创建全局表并注册为GC根对象
4. 初始化第一个CallInfo（虚拟主函数）
5. 如果是第一个LuaState，设置为主线程

**内存布局**：104字节（包含Stack、CallInfo数组、引用和指针）

#### Userdata（用户数据）

**设计模式**：GC管理的内存块

**核心职责**：
- 将C++数据结构包装成Lua对象
- 提供GC管理的内存块
- 支持元表实现自定义行为
- 保证内存对齐（8字节）

**关键特性**：
```cpp
// 创建完整用户数据
Userdata* ud = Userdata::createFull(64);  // 分配64字节

// 类型化创建
struct MyData { int id; double value; };
MyData data = {123, 3.14};
Userdata* ud2 = Userdata::create(data);

// 数据访问
void* rawData = ud->getData();
MyData* typedData = ud->getTypedData<MyData>();

// 元表支持
Table* mt = new Table();
ud->setMetatable(mt);
bool hasMt = ud->hasMetatable();
```

**内存布局**：
```
[Userdata对象头部][用户数据块（8字节对齐）]
```

**GC集成**：
- 自动标记元表
- 计算总内存大小（对象 + 数据）
- 析构时自动释放对齐内存

**平台兼容性**：
- Windows (MSVC): 使用`_aligned_malloc`/`_aligned_free`
- Linux/macOS: 使用`std::aligned_alloc`/`std::free`

#### Lexer（词法分析器）

**设计模式**：单遍扫描的LL(1)词法分析

**核心职责**：
- 将Lua源代码文本转换为Token流
- 识别所有Lua 5.1关键字、运算符和字面量
- 处理注释（单行和多行）
- 精确跟踪行号和列号

**关键特性**：
```cpp
// 创建词法分析器
Lexer lexer("local x = 42");

// 获取Token流
Token t1 = lexer.nextToken();  // local (关键字)
Token t2 = lexer.nextToken();  // x (标识符)
Token t3 = lexer.nextToken();  // = (运算符)
Token t4 = lexer.nextToken();  // 42 (数字)

// Token信息
std::cout << t4.lexeme;        // "42"
std::cout << t4.line;          // 行号
std::cout << t4.column;        // 列号
f64 value = std::get<f64>(t4.value);  // 42.0
```

**支持的Token类型**：
- **21个关键字**：and, break, do, else, elseif, end, false, for, function, if, in, local, nil, not, or, repeat, return, then, true, until, while
- **单字符运算符**：+ - * / % ^ # = < > ( ) { } [ ] ; : , .
- **多字符运算符**：.. ... == ~= <= >=
- **字面量**：数字（整数、浮点、科学计数法、十六进制）、字符串（单引号、双引号、长字符串）
- **标识符**：[a-zA-Z_][a-zA-Z0-9_]*

**注释处理**：
```lua
-- 单行注释
--[[ 多行注释 ]]
--[=[ 嵌套级别的长注释 ]=]
```

**字符串支持**：
```lua
"double quote"
'single quote'
[[long string]]
[=[long string with level]=]
"escape sequences: \n \t \\ \""
```

**错误处理**：
- 未闭合字符串检测
- 非法字符检测
- 详细的错误位置信息

#### Parser（语法分析器）

**设计模式**：递归下降解析（Recursive Descent Parsing）

**核心职责**：
- 将Token流转换为抽象语法树（AST）
- 实现Lua 5.1的完整语法规则
- 处理运算符优先级和结合性
- 提供详细的语法错误信息

**关键特性**：
```cpp
// 创建语法分析器
Parser parser("local x = 42");

// 解析生成AST
Chunk chunk = parser.parse();

// 访问AST节点
for (const auto& stmt : chunk.statements) {
    // 处理语句节点
}
```

**AST节点设计**：
```cpp
// 使用std::variant实现类型安全的多态
using ExprVariant = std::variant<
    NilExpr, BoolExpr, NumberExpr, StringExpr,
    NameExpr, BinaryExpr, UnaryExpr, TableExpr,
    CallExpr, IndexExpr, MemberExpr, FunctionExpr
>;

using StmtVariant = std::variant<
    EmptyStmt, AssignStmt, LocalStmt, CallStmt,
    IfStmt, WhileStmt, RepeatStmt, ForNumStmt,
    ForInStmt, FunctionStmt, ReturnStmt, BreakStmt, DoStmt
>;

struct Expr {
    ExprVariant variant;
    i32 getLine() const;
    i32 getColumn() const;
};

struct Stmt {
    StmtVariant variant;
    i32 getLine() const;
    i32 getColumn() const;
};
```

**运算符优先级表**：
```
优先级  运算符              结合性
------  -----------------  --------
1       or                 左结合
2       and                左结合
3       <, >, <=, >=, ==, ~=  左结合
4       ..                 右结合
5       +, -               左结合
6       *, /, %            左结合
7       not, -, #          右结合
8       ^                  右结合
```

**解析函数层次**：
```cpp
// 语句解析
StmtPtr parseStatement();
StmtPtr parseIfStmt();
StmtPtr parseWhileStmt();
StmtPtr parseForStmt();
// ... 其他语句

// 表达式解析（按优先级）
ExprPtr parseExpression();      // 入口
ExprPtr parseOrExpr();          // or
ExprPtr parseAndExpr();         // and
ExprPtr parseRelationalExpr();  // <, >, <=, >=, ==, ~=
ExprPtr parseConcatExpr();      // ..
ExprPtr parseAdditiveExpr();    // +, -
ExprPtr parseMultiplicativeExpr(); // *, /, %
ExprPtr parseUnaryExpr();       // not, -, #
ExprPtr parsePowerExpr();       // ^
ExprPtr parsePrimaryExpr();     // 字面量、标识符、括号表达式
```

**错误处理**：
```cpp
class ParseError : public std::runtime_error {
    i32 line_, column_;
public:
    ParseError(const Str& message, i32 line, i32 column);
    i32 getLine() const;
    i32 getColumn() const;
};
```

**内存管理**：
- 使用`std::unique_ptr`管理AST节点
- 自动内存释放，无需手动管理
- 移动语义优化性能

#### CodeGenerator（字节码生成器）

**设计模式**：AST访问者模式

**核心职责**：
- 将AST转换为Lua 5.1字节码
- 管理寄存器分配和释放
- 管理常量表（数字、字符串、布尔值、nil）
- 管理局部变量作用域
- 实现跳转指令回填

**关键特性**：
```cpp
// 创建代码生成器
StringPool* pool = &GlobalState::getInstance().getStringPool();
CodeGenerator codegen(pool);

// 生成字节码
Parser parser("return 42");
Chunk chunk = parser.parse();
Proto* proto = codegen.generate(chunk);

// 访问生成的字节码
const Vec<Instruction>& code = proto->getCode();
const Vec<Value>& constants = proto->getConstants();
usize maxStackSize = proto->getMaxStackSize();
```

**OpCode指令集**（38条Lua 5.1指令）：
```cpp
// 指令格式
iABC:  [6位OpCode][8位A][9位C][9位B]
iABx:  [6位OpCode][8位A][18位Bx]
iAsBx: [6位OpCode][8位A][18位sBx（有符号）]

// 主要指令类别
- 数据移动: MOVE, LOADK, LOADBOOL, LOADNIL
- 全局变量: GETGLOBAL, SETGLOBAL
- 表操作: GETTABLE, SETTABLE, NEWTABLE, SETLIST, SELF
- 算术运算: ADD, SUB, MUL, DIV, MOD, POW, UNM
- 逻辑运算: NOT, LEN, CONCAT
- 比较运算: EQ, LT, LE
- 跳转控制: JMP, TEST, TESTSET
- 函数调用: CALL, TAILCALL, RETURN
- 循环: FORLOOP, FORPREP, TFORLOOP
- 闭包: CLOSURE, GETUPVAL, SETUPVAL, CLOSE
- 可变参数: VARARG
```

**RK寻址模式**：
```cpp
// RK(x) = 如果x < 256则为寄存器R(x)，否则为常量K(x-256)
bool ISK(i32 x) { return x & BITRK; }  // BITRK = 0x100
i32 INDEXK(i32 x) { return x & ~BITRK; }
i32 RKASK(i32 x) { return x | BITRK; }
```

**寄存器分配**：
```cpp
// 简化版寄存器分配器
i32 allocReg();           // 分配新寄存器
void freeReg(i32 reg);    // 释放寄存器
i32 getTopReg();          // 获取当前栈顶寄存器
```

**常量表管理**：
```cpp
i32 addConstant(const Value& value);  // 添加常量，返回索引
// 自动去重：相同的常量只存储一次
```

**跳转回填**：
```cpp
i32 emitJump(OpCode op);              // 发射跳转指令，返回PC
void patchJump(i32 pc, i32 target);   // 回填跳转目标
```

#### VM（字节码执行引擎）

**设计模式**：指令解释器（Interpreter Pattern）

**核心职责**：
- 解释执行Lua 5.1字节码
- 管理虚拟机寄存器（基于栈的寄存器）
- 实现所有38条指令的执行逻辑
- 处理算术、逻辑、比较运算
- 实现跳转控制流

**关键特性**：
```cpp
// 创建虚拟机
LuaState* L = LuaState::newState();
VM vm(L);

// 执行字节码
Proto* proto = /* 从CodeGenerator获取 */;
vm.executeProto(proto);

// 获取执行结果
Stack& stack = L->getStack();
Value result = stack.top();
```

**执行循环**：
```cpp
void VM::executeProto(Proto* proto) {
    // 初始化
    currentProto_ = proto;
    pc_ = 0;

    // 确保栈空间
    usize requiredSize = proto->getMaxStackSize();
    while (stack.size() < requiredSize) {
        stack.push(Value());
    }

    // 主执行循环
    while (pc_ < code.size()) {
        Instruction inst = code[pc_++];
        OpCode op = GET_OPCODE(inst);

        switch (op) {
            case OpCode::MOVE: /* ... */ break;
            case OpCode::LOADK: /* ... */ break;
            // ... 其他38条指令
        }
    }
}
```

**寄存器访问**：
```cpp
Value& R(i32 index);      // 访问寄存器R(index)
Value RK(i32 rk);         // RK寻址：寄存器或常量
Value K(i32 index);       // 访问常量K(index)
```

**算术运算**：
```cpp
void arith(OpCode op, i32 a, i32 b, i32 c) {
    Value left = RK(b);
    Value right = RK(c);
    f64 result = /* 根据op计算 */;
    R(a) = Value(result);
}
```

**比较运算**：
```cpp
void compare(OpCode op, i32 a, i32 b, i32 c) {
    Value left = RK(b);
    Value right = RK(c);
    bool result = /* 根据op比较 */;
    if (result != (a != 0)) {
        pc_++;  // 跳过下一条指令
    }
}
```

**跳转控制**：
```cpp
void doJump(i32 offset) {
    pc_ += offset;  // 相对跳转
}
```

**已实现指令**（当前版本）：
- ✅ MOVE, LOADK, LOADBOOL, LOADNIL
- ✅ GETGLOBAL, SETGLOBAL
- ✅ GETTABLE, SETTABLE, NEWTABLE
- ✅ ADD, SUB, MUL, DIV, MOD, POW, UNM
- ✅ NOT, LEN, CONCAT
- ✅ JMP, EQ, LT, LE
- ✅ TEST, TESTSET
- ✅ RETURN
- ✅ GETUPVAL, SETUPVAL, CLOSE（Upvalue操作）
- ✅ CALL, TAILCALL, SELF（函数调用；TAILCALL 已复用 Lua 调用帧）
- ✅ FORLOOP, FORPREP, TFORLOOP（循环指令）
- ✅ CLOSURE, SETLIST, VARARG（闭包和表初始化）

**指令实现状态**：38条指令中，已实现38条（100%）
- 基础指令：完全实现
- 高级指令：TFORLOOP 已支持 C/Lua 函数迭代器；剩余高级边界主要在标准库和调试语义

**性能优化**：
- 使用switch-case指令分发（编译器优化为跳转表）
- 内联函数减少调用开销
- 直接栈访问避免间接寻址

#### BaseLib（基础库）

**文件**: `src/lib/baselib.hpp/cpp`

**核心功能**：
- 提供Lua脚本运行所需的核心函数
- 实现 Lua 5.1 常用基础全局函数
- 支持基本的类型操作、输出和错误处理
- 与VM和LuaState完全集成

**基础函数实现概览**：

1. **print(...)**
   - 打印任意数量的参数到标准输出
   - 参数间用制表符分隔，自动添加换行
   - 支持所有Lua类型的字符串转换

2. **type(v)**
   - 返回值的类型字符串
   - 支持的类型："nil", "boolean", "number", "string", "table", "function", "userdata", "thread"

3. **tostring(v)**
   - 将值转换为字符串表示
   - 支持所有基本类型
   - table/userdata 与基础类型元表路径已尝试调用 `__tostring`

4. **tonumber(e [, base])**
   - 将值转换为数字
   - 支持数字类型直接返回
   - 基础库路径已支持字符串和 2~36 进制整数转换；`LuaState::toNumber()` 底层 API 的字符串转换仍是简化路径

5. **error(message [, level])**
   - 抛出错误并终止执行
   - 支持自定义错误消息
   - 已解析 level 参数；错误消息位置信息仍为后续兼容项

6. **assert(v [, message])**
   - 断言检查，如果v为假值则抛出错误
   - 支持自定义错误消息
   - 断言成功时返回所有参数

7. **setmetatable(table, metatable)**
   - 设置表的元表
   - 只能为表类型设置元表
   - 元表必须是表或nil
   - 已检查 `__metatable` 保护字段

8. **getmetatable(object)**
   - 获取对象的元表
   - 如果没有元表返回nil
   - 已按 Lua 5.1 语义返回 `__metatable` 保护字段

其他已实现基础函数包括：`next`、`pairs`、`ipairs`、`rawget`、`rawset`、`rawequal`、`select`、`pcall`、`xpcall`、`loadstring`、`loadfile`、`dofile`、`gcinfo`、`getfenv`、`setfenv`、`collectgarbage`、`unpack`、`load`。`getfenv/setfenv` 当前支持函数对象路径；栈层级、线程环境和 C 函数环境仍为兼容边界。

**关键特性**：
```cpp
// 注册基础库函数
LuaState* L = LuaState::newState();
openBaseLib(L);  // 注册所有函数到全局环境

// 从Lua代码中调用
// print("Hello, Lua!")
// local t = type(42)  -- "number"
// local s = tostring(123)  -- "123"
```

**LuaState API扩展**（为支持基础库新增30+方法）：
- **栈操作**: getTop(), setTop(), pushValue(), at()
- **全局变量**: setGlobal(), getGlobal()
- **类型检查**: isNumber(), isString(), isTable(), isFunction(), isNil(), isBoolean(), type(), typeName()
- **类型转换**: toNumber(), toString(), toBoolean()
- **元表操作**: getMetatable(), setMetatable()
- **错误处理**: error(msg), error()

**已知限制**：
- `newproxy` 尚未实现
- `error()` 的 level 参数尚未用于生成源位置信息
- `collectgarbage("stop"/"restart"/"step"/"setpause"/"setstepmul")` 仍为简化占位返回；真正 incremental 写屏障和调度仍待实现

**测试覆盖**：Base Library 相关单元测试全部通过，并覆盖 `_G` 自引用和受保护元表行为

---

## 🏗️ 项目结构

### 目录结构

```
├── src/                           # 核心源码
│   ├── common/                    # 基础类型、配置、宏
│   ├── compiler/                  # Lexer / Parser / AST / CodeGen / Bytecode Printer
│   ├── core/                      # Value / Table / Function / String / Metatable 等核心对象
│   ├── gc/                        # 垃圾回收器与 GCStrategy 策略边界
│   ├── io/                        # 输入流、动态缓冲区
│   ├── lib/                       # base / math / io / os / string / table / coroutine / debug / package 等标准库
│   ├── vm/                        # GlobalState / LuaState / Stack / VM
│   ├── main.cpp                   # `lua_app` 入口
│   ├── repl.cpp/.hpp              # REPL 公共入口与会话循环
│   ├── repl/                      # REPL repl_* 子模块：completion / history / meta / signals / prompt / execution helpers
│   └── bytecode_main.cpp          # `lua_bytecode` 入口
├── tests/
│   ├── lua/                       # Lua 脚本级测试样例
│   └── unit/                      # C++ 单元测试
│       ├── compiler/              # 编译器相关测试
│       ├── core/                  # 核心对象测试
│       ├── framework/             # 测试框架自身
│       ├── gc/                    # GC 测试
│       ├── io/                    # I/O 测试
│       ├── metamethod/            # 元方法测试
│       ├── stdlib/                # 标准库测试
│       └── vm/                    # VM / LuaState 测试
├── docs/                          # 项目文档
├── bin/                           # 编译批处理脚本
├── lua.slnx                       # Visual Studio 解决方案
├── lua.vcxproj                    # 核心静态库项目
├── lua_app.vcxproj                # REPL / 临时执行入口项目
├── lua_test.vcxproj               # 单元测试项目
├── lua_bytecode.vcxproj           # 字节码打印与对比工具项目
├── .gitignore
└── README.md
```

### 关键文件说明

| 文件 | 用途 | 重要性 |
|------|------|--------|
| `src/` | 解释器核心实现目录，日常代码修改的主战场 | ⭐⭐⭐ |
| `src/main.cpp` | `lua_app.exe` 的入口文件 | ⭐⭐⭐ |
| `src/bytecode/bytecode_main.cpp` | `lua_bytecode.exe` 的入口文件 | ⭐⭐ |
| `src/bytecode/bytecode_printer.cpp` | 字节码打印工具的输出层；当前可打印 Proto 头信息、指令、常量表、diff 和 Mermaid CFG | ⭐ |
| `tests/unit/` | 单元测试目录，是验证 C++ 模块行为的第一入口 | ⭐⭐⭐ |
| `tests/lua/` | Lua 脚本级样例与回归测试输入，现已按语法/功能分类整理 | ⭐⭐ |
| `docs/architecture/overview.md` | 架构设计说明，适合先建立整体认识 | ⭐⭐⭐ |
| `docs/guides/development.md` | 开发规范与类型系统约定 | ⭐⭐⭐ |
| `lua.slnx` | 当前 Visual Studio 解决方案入口 | ⭐⭐⭐ |

### 目录说明补充

- `src/`、`tests/`、`docs/` 是最值得长期关注的三个目录

---



## 🚀 快速开始

### 环境要求

- **操作系统**：Windows 10/11
- **编译器**：Visual Studio 2026（MSVC）
- **C++标准**：C++17/23（Visual Studio 项目当前使用 C++23 preview）

### 建议阅读顺序

1. 先看本文档开头的“必读约定”
2. 再看 `docs/architecture/overview.md`
3. 最后进入 `src/` 和 `tests/unit/` 开始实际开发或排错

---

## 📚 重要文档索引

### 项目文档（lua/docs/）

| 文档 | 描述 | 用途 |
|------|------|------|
| `docs/index.md` | 新人入口 | 第一次打开仓库时的阅读顺序 |
| `docs/status/project-status.md` | 当前事实源 | 构建入口、测试状态、文档状态、CodeGen 管线事实 |
| `docs/architecture/overview.md` | 架构总览 | 理解当前源码分层和模块设计 |
| `docs/guides/development.md` | 开发规范 | 编码规范、类型系统使用指南、质量标准 |
| `docs/compiler/bytecode-generation.md` | 字节码生成说明 | 当前 `AST -> SymbolRef / ValueResult / CondResult / LValueRef / CallResultInfo -> Proto` 主线 |
| `docs/vm/instruction-set.md` | VM 指令集 | 38 条 Lua 5.1 风格指令的当前语义入口 |
| `docs/stdlib/overview.md` | 标准库总览 | 标准库 catalog、注册方式和兼容性缺口 |

## 💡 快速上手指南（给新AI会话）

### 2分钟快速理解项目

1. **这是什么项目？**
   - 一个用现代 C++ 重写 Lua 5.1.5 的项目

2. **已经完成了什么？**
   - 编译器前端、VM 主体、GC 基础/弱表/终结器路径、核心对象系统已经成型
   - 当前单元测试和 Lua 回归脚本均为全绿
   - 标准库仍有少量 Lua 5.1 兼容补完工作
   - 重点不再是“从零搭骨架”，而是“持续补功能、修边界、稳行为”

3. **下一步做什么？**
   - 先看最近在改的模块和回归脚本
   - 优先补 `error(level)` / `xpcall` / debug 环境边界，其次补 `io.lines` 格式参数、`os.remove/os.rename` 失败返回值和 `table.concat` 类型边界

4. **在哪里找详细信息？**
   - 当前事实源：`docs/status/project-status.md`
   - 架构设计：`docs/architecture/overview.md`
   - 编码规范：`docs/guides/development.md`

---

## 🔧 技术细节

### 类型系统使用规范

本项目统一使用 `src/common/types.hpp` 中定义的类型别名，以保持代码风格和语义一致：

| C++标准类型 | 项目类型别名 | 用途 |
|------------|------------|------|
| `std::vector<T>` | `Vec<T>` | 动态数组 |
| `std::unordered_map<K,V>` | `HashMap<K,V>` | 哈希表 |
| `std::string` | `Str` | 字符串 |
| `std::string_view` | `StrView` | 字符串视图 |
| `size_t` | `usize` | 无符号大小类型 |
| `int32_t` | `i32` | 32位有符号整数 |
| `uint32_t` | `u32` | 32位无符号整数 |
| `int64_t` | `i64` | 64位有符号整数 |
| `uint64_t` | `u64` | 64位无符号整数 |
| `double` | `f64` | 64位浮点数 |

**重要**：新增代码优先使用这些类型别名，而不是直接散落使用标准库原始类型。详细规范见 `docs/guides/development.md`。

### 核心实现约定

1. **Value类**：使用`std::variant`实现类型安全的动态类型
   ```cpp
   using ValueData = std::variant<
       std::monostate,  // Nil
       bool,            // Boolean
       f64,             // Number
       void*,           // LightUserdata
       GCString*,       // String
       Table*,          // Table
       Function*,       // Function
       Userdata*,       // Userdata
       Thread*          // Thread
   >;
   ```

2. **GC系统**：采用三色标记（White/Gray/Black）+ 标记-清除算法
   - White：未标记（可回收）
   - Gray：已标记但未扫描子对象
   - Black：已标记且已扫描子对象

3. **Table类**：采用数组部分 + 哈希部分的混合存储
   - 数组部分：`Vec<Value>`（连续存储，快速索引）
   - 哈希部分：`std::unordered_map<Value, Value>`（键值对存储）

4. **StringPool**：使用字符串驻留（Interning）
   - 单例模式
   - 相同内容的字符串只存储一份
   - 使用指针比较代替字符串比较

---

## 🧪 测试和质量保证

### 测试框架

本项目使用**自定义轻量级测试框架**。它的目标不是功能最全，而是足够轻、足够直接，便于在解释器这种底层项目里快速定位问题。

**测试框架特性**：
- ✅ 简单的断言宏（`ASSERT_TRUE`、`ASSERT_FALSE`、`ASSERT_EQ`）
- ✅ 测试套件组织（`TestSuite`类）
- ✅ 自动测试注册（`TestRegistry`单例）
- ✅ 清晰的测试报告（通过/失败统计）
- ✅ 零外部依赖（无需Google Test等第三方库）

**测试文件结构**：
```
tests/unit/
├── compiler/                   # Lexer / Parser / CodeGen / 调用管线 / 符号绑定
├── core/                       # Value / Table / Function / GCString
├── framework/                  # 测试框架和 test_runner
├── gc/                         # GC 与 Upvalue 基础测试
├── io/                         # DynamicBuffer 与 InputStream
├── metamethod/                 # 算术、比较、索引等元方法测试
├── stdlib/                     # base / string / table / os / coroutine / debug / package
└── vm/                         # VM Core、LuaState 初始化、函数调用
```

当前测试入口会输出真实注册测试数和断言结果数；最近一次验证为 544 个注册测试、2735 个结果、0 失败。

## 📊 技术栈和工具

### 核心技术

| 技术 | 版本 | 用途 |
|------|------|------|
| **C++** | C++17/23 | 主实现语言（项目文件当前使用 C++23 preview） |
| **MSVC** | Visual Studio 2026 | 编译器与 IDE 工具链 |
| **Windows** | 10/11 | 当前主要开发平台 |
| **Git** | Latest | 版本管理 |

### 现代 C++ 特性使用

| 特性 | 应用场景 | 示例 |
|------|---------|------|
| `std::variant` | Value类动态类型 | `std::variant<std::monostate, bool, f64, ...>` |
| `std::string_view` | 字符串视图（类型别名StrView） | 函数参数传递 |
| 结构化绑定 | 简化代码 | `auto [key, val] : hash_` |
| `if constexpr` | 编译期条件 | 模板元编程 |
| 内联变量 | 单例模式 | `inline static StringPool instance` |

## 🔗相关链接

### 项目仓库

- **GitHub**: [https://github.com/YanqingXu/lua](https://github.com/YanqingXu/lua)
- **本地路径**: `e:\Programming2\lua_in_cpp\lua`

### 参考资源

- **Lua官方网站**: [https://www.lua.org/](https://www.lua.org/)
- **Lua 5.1参考手册**: [https://www.lua.org/manual/5.1/](https://www.lua.org/manual/5.1/)

---

## 🙏 致谢

- **Lua团队**：创造了优秀的Lua语言

---

## 📄 许可证

本项目采用 **MIT 许可证**。

---

## 🏗️ 子项目说明（Visual Studio 解决方案）

本仓库包含一个 Visual Studio 解决方案，用于组织核心库、运行入口、测试程序和字节码工具项目。

### 解决方案与构建信息

- **解决方案文件**：`lua.slnx`
- **构建工具**：MSBuild
- **MSBuild路径**：`D:\VS2026\2026\MSBuild\Current\Bin\MSBuild.exe`
- **默认构建配置**：Debug
- **默认目标平台**：x64
- **默认输出目录**：`x64/Debug/`

### 编译批处理脚本（`bin/`）

所有脚本默认执行 `Rebuild`，并固定使用 `Debug|x64`：

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

### 四个子项目

| 项目文件 | 输出类型 | 说明 |
|---------|---------|------|
| `lua.vcxproj` | 静态库（`lua.lib`） | 核心库，包含 Lexer、Parser、CodeGen、VM、GC 等所有产品源码，供其他子项目链接使用 |
| `lua_app.vcxproj` | 可执行文件（`lua_app.exe`） | 解释器与 REPL 入口，支持脚本执行、默认 REPL、`.help` / `.bytecode` / `.ast` / `.gc` 元命令、Tab 补全、REPL 终端彩色错误、行号 prompt、`-v`/`-h`/`-i` 和 `--trace` |
| `lua_test.vcxproj` | 可执行文件（`lua_test.exe`） | 单元测试运行器，覆盖 compiler、core、gc、vm 等各模块的测试用例 |
| `lua_bytecode.vcxproj` | 可执行文件（`lua_bytecode.exe`） | 字节码工具入口，当前可走通源码到 `Proto` 的编译链路，输出基础字节码清单、递归子 Proto、side-by-side diff 和 `--cfg` Mermaid 控制流图 |

### 使用建议

- 开发核心功能时，优先修改并维护 `lua.vcxproj` 对应的库源码
- 需要手动运行解释器或 REPL 流程时，使用 `lua_app.vcxproj`
- 需要验证回归和模块正确性时，使用 `lua_test.vcxproj`
- 需要验证字节码工具入口、Parser/CodeGen 集成或后续打印器改造时，使用 `lua_bytecode.vcxproj`

### 一句话理解

> `lua.vcxproj` 是核心静态库；`lua_app.vcxproj` 是解释器/REPL 入口；`lua_test.vcxproj` 是测试运行器；`lua_bytecode.vcxproj` 是支持打印与对比的字节码工具入口。
