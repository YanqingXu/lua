# Lua 5.1 Compatibility Goal — 兼容性目标

## 1. 兼容性策略

```
默认以 Lua 5.1.5 strict 行为作为兼容决策基准。
项目扩展不得替代官方兼容声明。

当前项目仍不声明完整 Lua 5.1.5 等价。
```

## 2. 兼容范围

| 维度 | 目标 | 当前状态 |
|------|------|---------|
| **语法** | 完整 Lua 5.1 语法 | ✅ 基本完成 |
| **语义** | 核心语义一致 | ✅ 主要路径通过 |
| **标准库** | 函数行为一致 | ✅ 主体完成，少量边界待对齐 |
| **C API** | API 兼容 | 🔄 主要接口实现，testC 未接入 |
| **Binary Chunk** | 二进制兼容 | ❌ 使用项目本地格式 |
| **GC 行为** | 回收行为一致 | ✅ 标记-清除主路径一致 |
| **错误消息** | 消息文本一致 | 🔄 主要一致，细节可能有差异 |

## 3. 官方测试套件

```
Lua 5.1 官方测试套件已以 staged smoke 形式接入:
  tests/lua/official/ 下的 .lua 文件

当前 skip 表为 0 (所有官方测试子脚本均通过)：
  ✅ literals.lua    ✅ calls.lua      ✅ attrib.lua
  ✅ locals.lua      ✅ constructs.lua ✅ vararg.lua
  ✅ strings.lua     ✅ math.lua       ✅ nextvar.lua
  ✅ errors.lua      ✅ sort.lua       ✅ pm.lua
  ✅ closure.lua     ✅ gc.lua         ✅ db.lua
  ✅ api.lua         ✅ events.lua     ✅ big.lua
  ✅ verybig.lua     ✅ files.lua      ✅ code.lua
  ✅ main.lua
```

## 4. 参考资源

- Lua 5.1 参考手册: https://www.lua.org/manual/5.1/
- 完整兼容性审计: `docs/compatibility/lua51-full-compatibility-audit.md`
- 下一步计划: `docs/roadmap/lua51-compatibility-next-stage.md`
