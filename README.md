# 现代C++ Lua解释器

> **基于Lua 5.1.5，使用C++17/20/23标准从零实现**

---

## 📖 项目简介

本项目旨在使用现代C++标准重新实现一个完整的Lua 5.1.5解释器，在保持与原版Lua完全兼容的同时，充分利用现代C++的特性提升代码质量、可维护性和性能。

### 🎯 项目目标

- ✅ **完全兼容**: 与Lua 5.1.5语法和语义100%兼容
- ✅ **现代C++**: 使用C++17/20/23特性，RAII、智能指针、STL容器
- ✅ **高质量代码**: 清晰的架构、完善的测试、详细的文档
- ✅ **教育价值**: 作为学习Lua实现和现代C++的优秀范例

### 🌟 核心特性

- **类型系统**: 使用`std::variant`实现动态类型，类型安全且高效
- **垃圾回收**: 三色标记清除算法，增量GC，写屏障
- **虚拟机**: 基于寄存器的字节码执行引擎
- **编译器**: 递归下降解析器，单遍编译，代码优化
- **标准库**: 完整的Lua标准库实现

---

## 📚 文档导航

### 核心文档

| 文档 | 描述 |
|------|------|
| [架构设计](docs/ARCHITECTURE.md) | 系统整体架构和模块设计 |
| [实施计划](docs/IMPLEMENTATION_PLAN.md) | 详细的开发计划和里程碑 |
| [开发指南](docs/DEVELOPMENT_GUIDE.md) | 编码规范和开发流程 |

### 参考资源

本项目基于以下参考资源开发：

#### 1. lua_c_analysis (主要参考)

Lua 5.1.5 C源码的详细分析，包含：
- 完整的中文注释源码
- 53+篇技术文档
- 核心算法详解

**使用方式**:
- 理解原始设计思路和算法
- 参考数据结构定义
- 学习性能优化技巧

#### 2. lua_with_cpp (次要参考)

一个部分完成的C++ Lua实现，包含：
- 现代C++实现模式
- GC系统实现
- VM执行引擎

**使用方式**:
- 参考C++实现方案
- 学习现代C++特性应用
- 避免已知的设计缺陷

#### 3. spec-kit (方法论指导)

Spec-Driven Development方法论工具包，包含：
- 开发流程模板
- 质量保证标准
- 最佳实践指南

**使用方式**:
- 应用SDD开发方法论
- 使用文档模板
- 遵循质量标准

---

## 🏗️ 项目结构

```
lua/
├── src/                    # 源代码
│   ├── common/            # 公共定义和工具
│   ├── core/              # 核心类型系统
│   ├── gc/                # 垃圾回收系统
│   ├── vm/                # 虚拟机
│   ├── compiler/          # 编译器
│   └── lib/               # 标准库
├── tests/                 # 测试代码
│   ├── unit/             # 单元测试
│   ├── integration/      # 集成测试
│   └── performance/      # 性能测试
├── docs/                  # 文档
│   ├── ARCHITECTURE.md   # 架构设计
│   ├── IMPLEMENTATION_PLAN.md  # 实施计划
│   └── DEVELOPMENT_GUIDE.md    # 开发指南
├── examples/              # 示例代码
└── CMakeLists.txt        # 构建配置
```

---

## 🚀 快速开始

### 环境要求

- **编译器**: GCC 9+, Clang 10+, 或 MSVC 2019+
- **CMake**: 3.15+
- **C++标准**: C++17或更高

### 构建步骤

#### Windows (Visual Studio)

```powershell
# 生成Visual Studio项目
cmake -B build -G "Visual Studio 16 2019"

# 打开解决方案
start build/lua.sln
```

#### Linux / macOS

```bash
# 生成Makefile
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# 编译
cmake --build build

# 运行测试
cd build && ctest
```

---

## 📅 开发路线图

### 当前状态: 🔄 阶段1 - 基础设施

| 阶段 | 内容 | 状态 | 预计完成 |
|------|------|------|----------|
| **阶段1** | 基础类型系统 | 🔄 进行中 | Week 2 |
| **阶段2** | 字符串和表系统 | ⏳ 待开始 | Week 4 |
| **阶段3** | 垃圾回收系统 | ⏳ 待开始 | Week 6 |
| **阶段4** | 虚拟机核心 | ⏳ 待开始 | Week 10 |
| **阶段5** | 编译器 | ⏳ 待开始 | Week 14 |
| **阶段6** | 标准库 | ⏳ 待开始 | Week 16 |
| **阶段7** | 测试和优化 | ⏳ 待开始 | Week 18 |

### 里程碑

- [x] **M0**: 项目架构设计完成
- [ ] **M1**: 基础类型系统实现
- [ ] **M2**: 字符串和表系统实现
- [ ] **M3**: 垃圾回收系统实现
- [ ] **M4**: 虚拟机核心实现
- [ ] **M5**: 编译器实现
- [ ] **M6**: 标准库实现
- [ ] **M7**: 1.0版本发布

---

## 🎓 学习资源

### 推荐阅读顺序

1. **入门阶段**
   - 阅读 `docs/ARCHITECTURE.md` 了解整体架构
   - 阅读 `lua_c_analysis/README.md` 了解Lua 5.1.5
   - 阅读 `lua_c_analysis/docs/wiki.md` 了解核心概念

2. **深入阶段**
   - 阅读 `lua_c_analysis/docs/object/` 了解对象系统
   - 阅读 `lua_c_analysis/docs/gc/` 了解垃圾回收
   - 阅读 `lua_c_analysis/docs/vm/` 了解虚拟机

3. **实践阶段**
   - 阅读 `docs/DEVELOPMENT_GUIDE.md` 了解开发规范
   - 参考 `lua_with_cpp/src/` 的C++实现
   - 开始编写代码和测试

### 关键技术文档

| 主题 | 文档路径 |
|------|---------|
| TValue实现 | `lua_c_analysis/docs/object/tvalue_implementation.md` |
| 字符串驻留 | `lua_c_analysis/docs/object/string_interning.md` |
| 表结构 | `lua_c_analysis/docs/object/table_structure.md` |
| 三色标记 | `lua_c_analysis/docs/gc/tri_color_marking.md` |
| 增量GC | `lua_c_analysis/docs/gc/incremental_gc.md` |
| 虚拟机执行 | `lua_c_analysis/docs/vm/` |

---

## 🤝 贡献指南

### 开发流程

1. **需求分析**: 阅读参考资源，理解需求
2. **设计方案**: 编写设计文档，评审方案
3. **测试先行**: 编写测试用例
4. **实现功能**: 编写代码实现
5. **测试验证**: 运行测试，检查覆盖率
6. **代码审查**: 自我审查，静态分析

### 质量标准

- ✅ 代码覆盖率 > 80%
- ✅ 无内存泄漏
- ✅ 符合C++17标准
- ✅ 通过静态分析
- ✅ 性能测试通过

详见 [开发指南](docs/DEVELOPMENT_GUIDE.md)

---

## 📊 技术栈

### 核心技术

| 技术 | 版本 | 用途 |
|------|------|------|
| C++ | 17/20/23 | 编程语言 |
| CMake | 3.15+ | 构建系统 |
| Google Test | Latest | 单元测试 |

### C++特性使用

| 特性 | 版本 | 应用场景 |
|------|------|---------|
| `std::variant` | C++17 | Value类型实现 |
| `std::optional` | C++17 | 可选值表示 |
| `std::string_view` | C++17 | 字符串视图 |
| `if constexpr` | C++17 | 编译期条件 |
| 结构化绑定 | C++17 | 简化代码 |
| `std::span` | C++20 | 数组视图（可选） |
| Concepts | C++20 | 模板约束（可选） |

---

## 📄 许可证

本项目采用 MIT 许可证。详见 [LICENSE](LICENSE) 文件。

---

## 🙏 致谢

- **Lua团队**: 创造了优秀的Lua语言
- **lua_c_analysis**: 提供了详细的源码分析
- **lua_with_cpp**: 提供了C++实现参考
- **spec-kit**: 提供了开发方法论指导

---

## 📞 联系方式

- **项目主页**: [GitHub Repository]
- **问题反馈**: [GitHub Issues]
- **讨论区**: [GitHub Discussions]

---

**开始你的Lua解释器开发之旅！** 🚀

