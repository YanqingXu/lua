# 现代C++ Lua解释器

> **从零开始用C++17/20/23实现Lua 5.1.5解释器**

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![Tests](https://img.shields.io/badge/tests-74%2F74-brightgreen)]()
[![Coverage](https://img.shields.io/badge/coverage-100%25-brightgreen)]()
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue)]()
[![Platform](https://img.shields.io/badge/platform-Windows-blue)]()

---

## 🎯 项目概览

### 项目目标

本项目旨在**从零开始**使用现代C++（C++17/20/23）重新实现一个完整的**Lua 5.1.5解释器**。

**核心特点**：
- 📚 **参考实现**：基于`lua_c_analysis`中的Lua 5.1.5 C源码（带详细中文注释）
- 🔧 **技术栈**：C++17标准、MSVC编译器（Visual Studio 2026）、Windows平台
- ✨ **现代C++**：充分利用现代C++特性（`std::variant`、智能指针、类型别名、STL容器）
- 🎓 **教育价值**：清晰的架构、完善的测试、详细的文档，适合学习Lua实现原理

### 开发方法

1. **主要参考**：`lua_c_analysis/` - Lua 5.1.5 C源码 + 53篇中文技术文档
2. **次要参考**：`lua_with_cpp/` - 另一个C++ Lua实现（部分完成）
3. **方法论**：`spec-kit/` - Spec-Driven Development开发方法论

---

## 📊 当前进度

### 已完成模块（7个核心模块）

| 模块 | 文件 | 功能描述 | 测试数 | 状态 |
|------|------|---------|--------|------|
| **基础类型系统** | `src/common/types.hpp` | 类型别名定义（Vec、HashMap、usize等） | - | ✅ 完成 |
| **配置系统** | `src/common/config.hpp` | 编译配置和常量定义 | - | ✅ 完成 |
| **宏定义** | `src/common/macros.hpp` | 实用宏定义 | - | ✅ 完成 |
| **Value类** | `src/core/value.hpp/cpp` | Lua值的C++表示（使用std::variant） | 14 | ✅ 完成 |
| **GCObject基类** | `src/core/gc_object.hpp/cpp` | GC对象基类（三色标记） | 8 | ✅ 完成 |
| **GCString类** | `src/core/gc_string.hpp/cpp` | GC管理的字符串对象 | 10 | ✅ 完成 |
| **StringPool类** | `src/core/string_pool.hpp/cpp` | 字符串驻留池（单例模式） | 11 | ✅ 完成 |
| **Table类** | `src/core/table.hpp/cpp` | Lua表（数组+哈希混合存储） | 11 | ✅ 完成 |
| **Function类** | `src/core/function.hpp/cpp` | 函数对象（Proto + Closure） | 12 | ✅ 完成 |
| **GarbageCollector** | `src/gc/garbage_collector.hpp/cpp` | 垃圾回收器（标记-清除算法） | 8 | ✅ 完成 |

### 测试统计

```
总测试数：74个
通过率：  100% (74/74)
编译状态：Debug和Release版本均无警告
平台：    Windows + MSVC (Visual Studio 2026)
```

### 核心实现亮点

✅ **Value类**：使用`std::variant`实现类型安全的动态类型系统
✅ **GCObject**：三色标记（White/Gray/Black）支持增量GC
✅ **Table类**：混合存储（数组部分 + 哈希部分），自动优化
✅ **Function类**：支持C函数和Lua函数两种闭包类型
✅ **StringPool**：字符串驻留（interning），节省内存
✅ **GarbageCollector**：标记-清除算法，根对象管理

---

## 🏗️ 项目结构

### 目录结构

```
工作区根目录 (e:\Programming2\lua_in_cpp\)
│
├── lua/                          # 主项目目录 ⭐ 当前开发重点
│   ├── src/                      # 源代码
│   │   ├── common/              # 公共组件
│   │   │   ├── types.hpp        # 类型别名定义（Vec、HashMap、usize等）
│   │   │   ├── config.hpp       # 编译配置和常量
│   │   │   └── macros.hpp       # 实用宏定义
│   │   ├── core/                # 核心类型系统
│   │   │   ├── value.hpp/cpp    # Value类（Lua值表示）
│   │   │   ├── gc_object.hpp/cpp # GCObject基类
│   │   │   ├── gc_string.hpp/cpp # GCString类
│   │   │   ├── string_pool.hpp/cpp # StringPool类
│   │   │   ├── table.hpp/cpp    # Table类
│   │   │   └── function.hpp/cpp # Function类（Proto + Closure）
│   │   ├── gc/                  # 垃圾回收系统
│   │   │   └── garbage_collector.hpp/cpp # GC实现
│   │   ├── vm/                  # 虚拟机（待实现）
│   │   ├── compiler/            # 编译器（待实现）
│   │   ├── lib/                 # 标准库（待实现）
│   │   └── main.cpp             # 测试主程序（VS IDE手动编译用）
│   ├── docs/                    # 项目文档
│   │   ├── ARCHITECTURE.md      # 架构设计文档
│   │   ├── IMPLEMENTATION_PLAN.md # 7阶段18周开发计划
│   │   ├── DEVELOPMENT_GUIDE.md # 编码规范和类型系统使用指南
│   │   ├── PROJECT_OVERVIEW.md  # 项目总览
│   │   └── PROJECT_SUMMARY_CN.md # 中文项目总结
│   ├── build/                   # 构建输出目录（自动生成）
│   │   ├── debug/              # Debug版本输出
│   │   └── release/            # Release版本输出
│   ├── build_with_vcvars.bat   # MSVC构建脚本（主要构建方式）
│   ├── CMakeLists.txt          # CMake配置（备用）
│   ├── .gitignore              # Git忽略配置
│   └── README.md               # 本文件
│
├── lua_c_analysis/              # Lua 5.1.5 C源码（带中文注释）⭐ 主要参考
│   ├── src/                    # Lua C源码
│   │   ├── lobject.h/c         # 对象系统（TValue、Table、Closure等）
│   │   ├── lgc.h/c             # 垃圾回收
│   │   ├── lvm.h/c             # 虚拟机
│   │   ├── lparser.h/c         # 解析器
│   │   └── ...                 # 其他模块
│   └── docs/                   # 53篇技术文档
│       ├── object/             # 对象系统文档
│       ├── gc/                 # GC系统文档
│       ├── vm/                 # 虚拟机文档
│       └── ...
│
├── lua_with_cpp/                # 另一个C++ Lua实现（次要参考）
│   └── src/                    # C++实现代码
│
└── spec-kit/                    # 开发方法论工具包
    └── ...
```

### 关键文件说明

| 文件 | 用途 | 重要性 |
|------|------|--------|
| `build_with_vcvars.bat` | MSVC构建脚本，编译并运行测试 | ⭐⭐⭐ |
| `src/main.cpp` | 测试主程序，用于VS IDE手动编译 | ⭐⭐⭐ |
| `docs/ARCHITECTURE.md` | 架构设计，理解系统结构 | ⭐⭐⭐ |
| `docs/IMPLEMENTATION_PLAN.md` | 开发计划，了解下一步任务 | ⭐⭐⭐ |
| `docs/DEVELOPMENT_GUIDE.md` | 编码规范，类型系统使用指南 | ⭐⭐⭐ |
| `lua_c_analysis/src/` | Lua C源码参考 | ⭐⭐⭐ |

---



## 🚀 快速开始

### 环境要求

- **操作系统**：Windows 10/11
- **编译器**：Visual Studio 2026（MSVC）
- **C++标准**：C++17
- **构建工具**：MSVC命令行工具（vcvarsall.bat）

### 编译和测试

#### 方式1：使用构建脚本（推荐）

```powershell
# 进入项目目录
cd lua

# 编译Debug版本并运行测试
.\build_with_vcvars.bat debug

# 编译Release版本并运行测试
.\build_with_vcvars.bat release
```

**输出示例**：
```
[INFO] Lua C++ Interpreter Build Script
[INFO] Build type: Debug
[INFO] Compiling Value class...
[INFO] Compiling GCObject class...
...
[TEST 1] Testing Value class...
  [1] Nil value: PASS
  [2] Boolean value: PASS
  ...
[SUCCESS] All tests passed!
```

#### 方式2：Visual Studio IDE手动编译

1. 打开Visual Studio 2026
2. 打开文件：`lua/src/main.cpp`
3. 配置项目包含目录：`lua/src`
4. 添加所有`.cpp`文件到项目
5. 编译并运行

**注意**：`main.cpp`是临时测试文件，仅用于VS IDE手动编译测试。

### 构建输出

```
lua/build/
├── debug/
│   ├── test_build.exe      # Debug版本可执行文件
│   ├── *.obj               # 目标文件
│   └── *.pdb               # 调试符号
└── release/
    ├── test_build.exe      # Release版本可执行文件
    └── *.obj               # 目标文件
```

---

## 📅 开发路线图

### 当前状态：✅ 阶段3完成，准备进入阶段4

| 阶段 | 内容 | 状态 | 完成度 |
|------|------|------|--------|
| **阶段1** | 基础类型系统 | ✅ 完成 | 100% |
| **阶段2** | 字符串和表系统 | ✅ 完成 | 100% |
| **阶段3** | 垃圾回收系统 | ✅ 完成 | 100% |
| **阶段4** | 虚拟机核心 | ⏳ 待开始 | 0% |
| **阶段5** | 编译器 | ⏳ 待开始 | 0% |
| **阶段6** | 标准库 | ⏳ 待开始 | 0% |
| **阶段7** | 测试和优化 | ⏳ 待开始 | 0% |

### 里程碑

- [x] **M0**: 项目架构设计完成
- [x] **M1**: 基础类型系统实现（Value、GCObject）
- [x] **M2**: 字符串和表系统实现（GCString、StringPool、Table）
- [x] **M3**: 垃圾回收系统实现（GarbageCollector、Function）
- [ ] **M4**: 虚拟机核心实现（LuaState、GlobalState、Stack、CallInfo）
- [ ] **M5**: 编译器实现（Lexer、Parser、CodeGen）
- [ ] **M6**: 标准库实现（base、table、string、math等）
- [ ] **M7**: 1.0版本发布

---

## 🎯 下一步计划

根据`docs/IMPLEMENTATION_PLAN.md`，接下来有以下开发选项：

### 选项A：实现Userdata类（用户自定义数据）

**功能**：
- 用户自定义数据类型封装
- 元表支持
- GC集成
- 轻量级Userdata支持

**参考**：`lua_c_analysis/src/lobject.h` (Udata结构)

### 选项B：实现Thread类（协程支持）

**功能**：
- Lua协程（coroutine）支持
- 独立的执行栈
- 调用信息管理
- 协程状态管理

**参考**：`lua_c_analysis/src/lstate.h` (lua_State结构)

### 选项C：开始虚拟机核心（⭐ 推荐）

**功能**：
- 实现LuaState类（Lua状态管理）
- 实现GlobalState类（全局状态）
- 实现Stack类（栈管理）
- 实现CallInfo类（调用信息）

**原因**：
- 为后续字节码执行打好基础
- 是编译器和标准库的前置依赖
- 可以开始实现简单的Lua脚本执行

**参考**：`lua_c_analysis/src/lstate.h` 和 `lua_c_analysis/src/ldo.h`

### 选项D：完善Function类（添加Upvalue支持）

**功能**：
- 实现Upvalue类（上值）
- 实现UpvalueList（上值列表）
- 完善闭包的上值管理
- 支持闭包捕获外部变量

**参考**：`lua_c_analysis/src/lobject.h` (UpVal结构)

---

## 📚 重要文档索引

### 项目文档（lua/docs/）

| 文档 | 描述 | 用途 |
|------|------|------|
| **ARCHITECTURE.md** | 架构设计文档 | 理解系统整体架构和模块设计 |
| **IMPLEMENTATION_PLAN.md** | 开发计划 | 7阶段18周详细开发计划和任务分解 |
| **DEVELOPMENT_GUIDE.md** | 开发规范 | 编码规范、类型系统使用指南、质量标准 |
| **PROJECT_OVERVIEW.md** | 项目总览 | 项目背景、目标、技术选型 |
| **PROJECT_SUMMARY_CN.md** | 中文项目总结 | 项目进展总结（中文） |

### 参考资源

#### 1. lua_c_analysis（主要参考）⭐⭐⭐

**位置**：`../lua_c_analysis/`

**内容**：
- Lua 5.1.5 C源码（带详细中文注释）
- 53篇技术文档
- 核心算法详解

**关键文件**：
- `src/lobject.h/c` - 对象系统（TValue、Table、Closure等）
- `src/lgc.h/c` - 垃圾回收系统
- `src/lstate.h/c` - 状态管理（lua_State、global_State）
- `src/lvm.h/c` - 虚拟机执行
- `src/lparser.h/c` - 解析器
- `src/ldo.h/c` - 函数调用和栈管理

**使用方式**：
- 理解原始Lua的设计思路和算法
- 参考数据结构定义
- 学习性能优化技巧

#### 2. lua_with_cpp（次要参考）⭐⭐

**位置**：`../lua_with_cpp/`

**内容**：
- 另一个C++ Lua实现（部分完成）
- 现代C++实现模式
- GC系统和VM实现

**使用方式**：
- 参考C++实现方案
- 学习现代C++特性应用
- 避免已知的设计缺陷

#### 3. spec-kit（方法论指导）⭐

**位置**：`../spec-kit/`

**内容**：
- Spec-Driven Development方法论
- 开发流程模板
- 质量保证标准

---

## 💡 快速上手指南（给新AI会话）

### 2分钟快速理解项目

1. **这是什么项目？**
   - 从零开始用C++17实现Lua 5.1.5解释器
   - 参考`lua_c_analysis`中的Lua C源码（带中文注释）
   - 使用MSVC编译器，Windows平台

2. **已经完成了什么？**
   - ✅ 7个核心模块（Value、GCObject、GCString、StringPool、Table、Function、GarbageCollector）
   - ✅ 74个测试用例，100%通过率
   - ✅ Debug和Release版本均编译成功，无警告

3. **如何编译和测试？**
   ```powershell
   cd lua
   .\build_with_vcvars.bat debug    # 编译Debug版本并运行测试
   .\build_with_vcvars.bat release  # 编译Release版本并运行测试
   ```

4. **下一步做什么？**
   - **推荐**：开始虚拟机核心（LuaState、GlobalState、Stack、CallInfo）
   - 或者：实现Userdata类、Thread类、完善Function类（Upvalue）
   - 参考：`docs/IMPLEMENTATION_PLAN.md`

5. **在哪里找详细信息？**
   - 架构设计：`docs/ARCHITECTURE.md`
   - 开发计划：`docs/IMPLEMENTATION_PLAN.md`
   - 编码规范：`docs/DEVELOPMENT_GUIDE.md`
   - Lua C源码：`../lua_c_analysis/src/`

---

## 🔧 技术细节

### 类型系统使用规范

本项目使用类型别名（定义在`src/common/types.hpp`）以提高代码可读性和一致性：

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

**重要**：所有新代码必须使用类型别名，不得直接使用C++标准类型。详见`docs/DEVELOPMENT_GUIDE.md`。

### 核心设计模式

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

2. **GC系统**：三色标记（White/Gray/Black）+ 标记-清除算法
   - White：未标记（可回收）
   - Gray：已标记但未扫描子对象
   - Black：已标记且已扫描子对象

3. **Table类**：混合存储优化
   - 数组部分：`Vec<Value>`（连续存储，快速索引）
   - 哈希部分：`std::unordered_map<Value, Value>`（键值对存储）

4. **StringPool**：字符串驻留（Interning）
   - 单例模式
   - 相同内容的字符串只存储一份
   - 使用指针比较代替字符串比较

---

## 🧪 测试和质量保证

### 测试覆盖

| 模块 | 测试数 | 覆盖内容 |
|------|--------|---------|
| Value类 | 14 | 类型创建、类型检查、安全访问、Lua真值、相等性、toString |
| GCObject类 | 8 | 对象创建、类型检查、颜色标记、对象链、大小计算 |
| GCString类 | 10 | 字符串创建、长度、数据访问、哈希值、指针比较 |
| StringPool类 | 11 | 字符串驻留、指针相等性、池大小、查找、删除 |
| Table类 | 11 | 数组操作、哈希操作、长度计算、键存在性、元表 |
| Function类 | 12 | C函数、Lua函数、Proto、常量表、GC标记 |
| GarbageCollector | 8 | 对象注册、根对象、垃圾回收、统计信息 |

### 质量标准

- ✅ **测试通过率**：100% (74/74)
- ✅ **编译警告**：0个
- ✅ **内存泄漏**：无（手动管理，待添加智能指针）
- ✅ **代码规范**：遵循类型系统使用规范
- ✅ **文档完整性**：所有公共API都有注释

---

## 📊 技术栈和工具

### 核心技术

| 技术 | 版本 | 用途 |
|------|------|------|
| **C++** | C++17 | 编程语言 |
| **MSVC** | Visual Studio 2026 | 编译器 |
| **Windows** | 10/11 | 目标平台 |
| **Git** | Latest | 版本控制 |

### C++17特性使用

| 特性 | 应用场景 | 示例 |
|------|---------|------|
| `std::variant` | Value类动态类型 | `std::variant<std::monostate, bool, f64, ...>` |
| `std::string_view` | 字符串视图（类型别名StrView） | 函数参数传递 |
| 结构化绑定 | 简化代码 | `auto [key, val] : hash_` |
| `if constexpr` | 编译期条件 | 模板元编程 |
| 内联变量 | 单例模式 | `inline static StringPool instance` |

### 构建工具

- **主要构建方式**：`build_with_vcvars.bat`（MSVC命令行）
- **备用构建方式**：CMake（`CMakeLists.txt`）
- **IDE支持**：Visual Studio 2026

---

## 📈 项目统计

### 代码规模

```
源文件数：    20个
头文件：      10个 (.hpp)
实现文件：    10个 (.cpp)
总代码行数：  约3000行（不含注释和空行）
文档行数：    约2000行
测试用例：    74个
```

### 对象大小（Release版本）

| 类型 | 大小（字节） | 说明 |
|------|------------|------|
| Value | 16 | Lua值（std::variant） |
| GCObject | 24 | GC对象基类 |
| GCString | 87 | GC字符串（含数据） |
| Table | 152 | Lua表 |
| Proto | 96 | 函数原型 |
| Function | 48 | 函数闭包 |
| GarbageCollector | 72 | 垃圾回收器 |

---

## � 相关链接

### 项目仓库

- **GitHub**: [https://github.com/YanqingXu/lua](https://github.com/YanqingXu/lua)
- **本地路径**: `e:\Programming2\lua_in_cpp\lua`

### 参考资源

- **Lua官方网站**: [https://www.lua.org/](https://www.lua.org/)
- **Lua 5.1参考手册**: [https://www.lua.org/manual/5.1/](https://www.lua.org/manual/5.1/)
- **lua_c_analysis**: `../lua_c_analysis/`
- **lua_with_cpp**: `../lua_with_cpp/`

---

## 🙏 致谢

- **Lua团队**：创造了优秀的Lua语言
- **lua_c_analysis**：提供了详细的Lua 5.1.5源码分析和中文文档
- **lua_with_cpp**：提供了C++实现参考
- **spec-kit**：提供了Spec-Driven Development方法论指导

---

## � 开发日志

### 最近更新

- **2025-11-12**：实现Function类（Proto + Closure），74个测试全部通过
- **2025-11-12**：实现GarbageCollector类（标记-清除算法）
- **2025-11-12**：实现Table类（混合存储）
- **2025-11-12**：实现StringPool和GCString类
- **2025-11-12**：实现Value和GCObject基础类型系统
- **2025-11-12**：项目初始化，完成架构设计文档

### Git提交历史

```bash
# 查看最近的提交
git log --oneline -10

# 最近的提交示例：
# 9cbccbb Implement Function class: Proto and Closure support
# 6086b67 add function
# 165168b Add garbage collector implementation and update project files
# 90ef578 Implement GarbageCollector: tri-color mark-and-sweep algorithm
# ...
```

---

## 📄 许可证

本项目采用 **MIT 许可证**。

---

## 🚀 开始开发

### 对于新的AI会话

如果你是新的AI助手，请按以下步骤快速了解项目：

1. **阅读本README**（2分钟）- 了解项目概况
2. **查看`docs/ARCHITECTURE.md`**（5分钟）- 理解系统架构
3. **查看`docs/IMPLEMENTATION_PLAN.md`**（3分钟）- 了解开发计划
4. **查看`docs/DEVELOPMENT_GUIDE.md`**（5分钟）- 学习编码规范
5. **编译并运行测试**（1分钟）- 验证环境
   ```powershell
   cd lua
   .\build_with_vcvars.bat debug
   ```
6. **开始开发下一个模块** - 参考"下一步计划"部分

### 对于人类开发者

欢迎参与本项目的开发！请遵循以下步骤：

1. Fork本项目
2. 创建特性分支（`git checkout -b feature/AmazingFeature`）
3. 提交更改（`git commit -m 'Add some AmazingFeature'`）
4. 推送到分支（`git push origin feature/AmazingFeature`）
5. 开启Pull Request

---

**Happy Coding!** 🎉

