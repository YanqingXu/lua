# Compatibility Test Suite

## 1. Lua 5.1 官方测试

```
tests/lua/official/all.lua  → staged smoke 入口
tests/lua/official/*.lua    → 官方兼容性测试脚本

运行:
  bin/lua_app.exe tests/lua/official/literals.lua
  bin/lua_app.exe tests/lua/official/calls.lua
  ...
```

## 2. Golden Test 方法

```
Golden Test 思路:
  1. 同一个 Lua 脚本
  2. 分别用官方 Lua 5.1 和本项目执行
  3. 比较 stdout / stderr / exit code
  4. 如果一致 → 行为兼容

示例:
  lua5.1 tests/cases/closure.lua > expected.txt
  lua_app tests/cases/closure.lua > actual.txt
  diff expected.txt actual.txt
```

## 3. 回归测试

```
tests/lua/regressions/*.lua  — 修复过的 Bug 的回归用例
tests/lua/stdlib/*.lua       — 标准库专项测试

每次修改后运行确保不引入回归。
```
