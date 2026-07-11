---
status: current
verified_against: docs/guides/test-runner.md; tests/unit/framework/test_runner.cpp; tools/run_quality_gate.ps1; tools/check_doc_drift.ps1; README.md
last_checked: 2026-07-11
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

测试总数、断言总数和最近绿跑只在根目录 `README.md` 维护。

## 3. 运行测试

```bash
# 编译
bin/build_test.bat

# 运行全部
bin/lua_test.exe

# 特定套件
bin/lua_test.exe --suite Lexer

# 调整内存限制
bin/lua_test.exe --max-memory-mb 256
bin/lua_test.exe --no-memory-limit
```
