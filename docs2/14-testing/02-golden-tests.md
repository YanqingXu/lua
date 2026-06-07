# Golden Tests

## 1. 什么是 Golden Test

```
Golden Test 的思路:
  1. 同一个 Lua 脚本
  2. 分别用官方 Lua 5.1 和本项目执行
  3. 比较 stdout / stderr / exit code
  4. 如果一致 → 行为兼容
```

## 2. 执行方式

```bash
# 手动对比
lua5.1 test_case.lua > expected.txt
lua_app test_case.lua > actual.txt
diff expected.txt actual.txt

# 自动化脚本
tools/compare_with_lua51.ps1 test_case.lua
```

## 3. Golden Test 目录

```
tests/lua/official/  — 官方 Lua 5.1 测试脚本 (Golden 数据源)
tests/lua/regressions/ — 本项目修复过的 Bug 回归测试
```

## 4. 官方测试覆盖

```
22 个官方测试脚本全部通过:
  literals, calls, attrib, locals, constructs, vararg,
  strings, math, nextvar, errors, sort, pm, closure,
  gc, db, api, events, big, verybig, files, code, main
```
