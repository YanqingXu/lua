# Regression Tests — 回归测试

## 1. 回归测试策略

```
每次修复 Bug 后:
  1. 添加最小复现用例
  2. 验证修复前失败
  3. 修复后通过
  4. 用例放入回归测试套件
  5. 以后每次修改都运行 → 防止回归
```

## 2. 回归测试目录

```
tests/lua/regressions/*.lua  — Lua 脚本级别
tests/unit/ 各模块的 test_*.cpp — C++ 单元测试级别
```

## 3. 自动化

```bash
# CI 流程
bin/build_test.bat
bin/lua_test.exe  -- 运行所有 C++ 测试

# 运行所有 Lua 回归脚本
for f in tests/lua/regressions/*.lua; do
    bin/lua_app.exe "$f" || exit 1
done

# 运行官方测试套件
bin/lua_app.exe tests/lua/official/all.lua
```
