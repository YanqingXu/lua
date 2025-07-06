# Lua 解释器项目文档索引

本文档记录了从各个模块迁移到 `docs/` 目录的所有技术文档。

## 🎯 核心开发文档

### 📋 开发规范与计划
- **[开发规范文档](../DEVELOPMENT_STANDARDS.md)** ⭐ **强制要求**
  - 位置：项目根目录
  - 内容：代码规范、类型系统、注释要求、现代C++规范、测试标准
  - 状态：**强制执行 - 所有代码必须遵循**

- **[当前开发计划](../current_develop_plan.md)** 
  - 位置：项目根目录
  - 内容：标准库重构进度、开发里程碑、下一步计划
  - 状态：实时更新

### 🛠️ 开发工具与配置
- **[开发工具说明](../tools/README.md)**
  - 位置：tools/README.md
  - 内容：代码规范检查脚本、构建配置、质量控制工具使用指南
  
- **[CMake 质量配置](../tools/cmake_standards.cmake)**
  - 位置：tools/cmake_standards.cmake
  - 内容：编译器设置、静态分析集成、自动化检查目标
  
- **[代码格式配置](../.clang-format)**
  - 位置：项目根目录
  - 内容：基于 Google Style Guide 的格式化规则
  
- **[静态分析配置](../.clang-tidy)**
  - 位置：项目根目录
  - 内容：全面的代码质量检查规则

### 🏗️ 架构设计文档
- **[标准库框架设计](standard_library_framework_design.md)**
  - 内容：标准库整体架构、模块化设计、接口规范
  
- **[完整重构方案](comprehensive_standard_library_refactoring_plan.md)**
  - 内容：详细的重构计划、分层架构、实现策略
  
- **[核心组件实现计划](core_framework_implementation_plan.md)**
  - 内容：LibraryManager、LibContext、LibFuncRegistry详细设计

## 📁 文档结构

### 🗑️ 垃圾回收 (GC) 模块
位置：`docs/gc/`

- **`gc_overview.md`** - GC 模块总览和架构说明
  - 原路径：`src/gc/README.md`
  - 内容：GC 系统整体设计、核心组件介绍

- **`gc_integration_guide.md`** - GC 集成指南
  - 原路径：`src/gc/GC_INTEGRATION_README.md`
  - 内容：如何将 GC 系统集成到 VM 中

- **`string_pool_implementation.md`** - 字符串池实现详解
  - 原路径：`src/gc/core/STRING_POOL_README.md`
  - 内容：字符串池的实现原理和优化策略

- **`allocator_optimization.md`** - 内存分配器优化
  - 原路径：`src/gc/memory/ALLOCATOR_OPTIMIZATION.md`
  - 内容：内存分配器的优化技术和性能分析

### 🧪 测试系统
位置：`docs/tests/`

- **`testing_guide.md`** - 测试系统总览和使用指南
  - 原路径：`src/tests/README.md`
  - 内容：测试框架使用方法、测试编写规范

- **`error_handling_tests.md`** - 错误处理测试
  - 原路径：`src/tests/ERROR_HANDLING_TESTS_README.md`
  - 内容：错误处理机制的测试策略和案例

- **`closure_testing.md`** - 闭包功能测试
  - 原路径：`src/tests/vm/closure/README.md`
  - 内容：闭包实现的测试方法和验证策略

- **`closure_boundary_test_implementation.md`** - 闭包边界测试实现
  - 原路径：`src/tests/vm/closure/BOUNDARY_TEST_IMPLEMENTATION_SUMMARY.md`
  - 内容：闭包边界条件测试的具体实现

- **`closure_boundary_test_guide.md`** - 闭包边界测试指南
  - 原路径：`src/tests/vm/closure/closure_boundary_test_README.md`
  - 内容：闭包边界测试的使用指南和最佳实践

### 📚 库系统
位置：`docs/lib/`

- **`library_architecture.md`** - 库系统架构设计
  - 原路径：`src/lib/README.md`
  - 内容：标准库系统的架构设计和实现说明

## 🚀 标准库重构文档 (2025年6月29日新增)

### 核心重构文档
- **`comprehensive_standard_library_refactoring_plan.md`** - 标准库统一重构方案 ⭐ **最新**
  - 内容：完整的标准库架构重构计划、分阶段实施方案、技术路线图
  - 包含：架构分层设计、核心组件规范、实施时间线、成功标准

- **`core_framework_implementation_plan.md`** - 核心框架组件实现计划 ⭐ **最新**
  - 内容：LibraryManager、LibContext、LibFuncRegistry 等核心组件的详细实现计划
  - 包含：实现要点、测试策略、性能目标、错误处理

### 现有架构文档
- **`base_lib_architecture_comparison.md`** - BaseLib 架构对比分析
  - 内容：新旧 BaseLib 架构的详细对比、重构方案设计

- **`standard_library_framework_design.md`** - 标准库框架设计方案
  - 内容：统一标准库框架的完整设计、核心组件规范

### 开发指南
- **`modern_library_framework_guide.md`** - 现代库框架开发指南
  - 内容：基于现代C++的标准库框架设计指南

- **`modern_cpp_lua_design_evaluation.md`** - 现代C++ Lua设计评估
  - 内容：现代C++在Lua解释器开发中的应用评估

### 编译和集成文档
- **`library_framework_compilation_report.md`** - 库框架编译报告
  - 内容：现代C++标准库框架的编译测试结果

- **`framework_replacement_report.md`** - 框架替换报告
  - 内容：老框架向新框架迁移的详细记录

### 🔧 测试框架
位置：`docs/test_framework/`

- **`test_framework_guide.md`** - 测试框架完整指南
  - 原路径：`src/test_framework/README.md`
  - 内容：测试框架的设计理念、API 参考、使用示例

- **`naming_check_tool.md`** - 命名检查工具说明
  - 原路径：`src/test_framework/tools/NAMING_CHECK_README.md`
  - 内容：代码命名规范检查工具的使用方法

## 📋 迁移记录

### 迁移日期
2025年7月6日

### 迁移原因
1. **统一文档管理** - 将分散在各个模块中的文档集中到 `docs/` 目录
2. **改善文档结构** - 按功能模块组织文档，便于查找和维护
3. **标准化命名** - 将原有的大写、下划线命名改为更易读的小写连字符命名
4. **清理源码目录** - 减少源码目录中的非代码文件，保持目录整洁

### 命名规范
- 使用小写字母和连字符分隔（kebab-case）
- 文件名应该描述文档的主要内容
- 避免使用 README、ALL_CAPS 等不规范的命名

### 目录结构说明
```
docs/
├── gc/                     # 垃圾回收相关文档
├── tests/                  # 测试相关文档  
├── lib/                    # 库系统文档
├── test_framework/         # 测试框架文档
├── [原有项目级文档]        # 项目整体设计和分析文档
└── documentation_index.md  # 本索引文件
```

## 🔗 相关链接

- [项目进度报告](project_progress_report.md)
- [基础库架构对比](base_lib_architecture_comparison.md)
- [Wiki 文档系列](wiki.md)

---

**注意：** 如果您需要查找特定的技术文档，建议先查看本索引文件确定文档位置，或直接在 `docs/` 目录下按模块查找。
