---
status: current
verified_against: tests/unit/framework/test_runner.cpp; tools/run_quality_gate.ps1; tools/check_doc_drift.ps1
last_checked: 2026-06-13
applies_to: Chinese testing strategy overview
---

# Testing Strategy — 测试策略

## 1. 测试层次

```
测试金字塔:

        ╱ 集成测试 ╲         Lua 5.1 官方测试套件
       ╱ regression ╲        Lua 脚本回归测试
      ╱ Golden Tests ╲       vs 官方 Lua 输出对比
     ╱───────────────╲
    ╱  Unit Tests     ╲      C++ 单元测试
   ╱───────────────────╲
  ╱   Static Checks     ╲    编译器警告 /W4, static_assert
```

## 2. 测试框架

```
自定义轻量级测试框架 (零外部依赖):
  - TestSuite 类
  - ASSERT_TRUE / ASSERT_FALSE / ASSERT_EQ 宏
  - TestRegistry 单例 (自动注册)
  - 清晰的通过/失败报告
```

## 3. 证据设计

- Unit tests 锁定单个类型、算法和组件契约。
- Characterization tests 锁定重构前后的可观察语义与字节码形状。
- Golden tests 对比结构化输出，适合 trace、错误和字节码渲染。
- Lua regression scripts 复现跨 compiler、VM、runtime 的行为缺陷。
- 官方 Lua 5.1 脚本用于兼容性证据，但实现差异必须在 `docs/compatibility/lua51/` 中单独解释。

测试数量属于运行时统计，不写入技术文档；技术文档只描述测试层次、覆盖意图和证据位置。
