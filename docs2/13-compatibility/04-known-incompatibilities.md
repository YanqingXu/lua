# Known Incompatibilities — 已知不兼容

## 关键不兼容

| 项目 | 说明 | 优先级 |
|------|------|--------|
| **官方 binary chunk** | 不兼容，使用项目本地格式 | P1 |
| **testC helper (ltests.c)** | 未接入，api.lua/code.lua 走 skip 分支 | P1 |
| **IncrementalGC** | 教学占位，非增量式 | P2 |
| **错误消息文本** | 可能不完全一致 | P3 |
| **#t 未定义行为** | 有洞表的长度可能与官方不同 | P3 |
| **长字符串处理** | 所有字符串驻留，行为略有差异 | P3 |

## 标准库差异

| 函数 | 差异说明 |
|------|---------|
| `table.setn` | Lua 5.1 已废弃，未实现 |
| `collectgarbage("step")` | 分阶段推进，但非增量式 |
| `os.execute` | Windows 引号包装方式不同 |
| `package.loadlib` | 搜索路径实现细节不同 |
