# IO Runtime Trace Execution Plan

## Summary

1. 补通 `userdata -> metatable -> SELF/file method` 运行时链路。
2. 稳定 I/O 库核心可用路径，优先保障文件句柄读写和元表行为。
3. 补齐编译器调试元数据，让 `bytecode`/`trace` 输出真实 `source/line/local` 信息。

## Key Changes

- `LuaState::getMetatable(i32)` / `setMetatable(i32)` 扩展为支持 `table + userdata`。
- `VM::SELF` 改为统一走对象访问链路，不再限制对象必须为 `table`。
- `iolib` 文件句柄元表补齐 `__index`、`__tostring`、`__gc` 接线，优先支持 `file:*` 核心方法。
- `CodeGenerator` 为主 chunk 和子函数填充 `Proto::source`、`lineInfo`、`locvars`。
- `main.cpp`、`bytecode_main.cpp`、`repl.cpp`、`baselib.cpp` 动态编译入口统一透传源码名。

## Validation

- `lua_app.exe lua/tests/lua/stdlib/test_iolib_simple.lua`
- `lua_app.exe lua/tests/lua/stdlib/test_iolib.lua`
- `lua_app.exe lua/tests/lua/stdlib/test_iolib_core.lua`
- `lua_app.exe --trace <file> <script.lua>`
- `lua_bytecode.exe <script.lua>`
- `lua_test.exe`

## Scope Notes

- 本轮不引入 HIR、SSA、统一诊断框架，也不重组现有目录。
- `io.lines` / `file:lines` 仍保持第二阶段处理；当前目标是“失败清晰、核心路径可用”。
