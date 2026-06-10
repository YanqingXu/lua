---
status: current
verified_against: lua_test.vcxproj; CMakeLists.txt; tests/unit/framework/test_runner.cpp; tests/unit/
last_checked: 2026-05-19
applies_to: lua_test unit test executable
---

# lua_test

`lua_test.vcxproj` 构建自定义 C++ 单元测试可执行文件。在 CMake 中，目标名为 `lua_test`。

## 职责

- 注册所有单元测试
- 运行完整或筛选后的测试集
- 报告通过/失败计数
- 可选输出 JUnit XML

最新记录的项目状态追踪在 `docs/status/project-status.md`。

## 测试领域

- app 选项
- 编译器
- 核心运行时对象
- GC
- I/O 辅助
- 元方法
- 标准库
- VM

参见 `docs/guides/test-runner.md`。
