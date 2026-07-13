---
status: current
verified_against: examples/hello.lua; examples/control_flow.lua; examples/tables_and_methods.lua; examples/metamethods.lua; examples/embedding.cpp; CMakeLists.txt; bin/lua_app.exe
last_checked: 2026-07-13
applies_to: runnable Lua scripts and the public C API embedding example
---

# Examples

这些示例用于快速观察当前解释器行为。它们不是 Lua 5.1 完整兼容性声明；精确语义仍以测试套件为准。

先构建 `lua_app.vcxproj`，然后从仓库根目录运行：

```powershell
bin\lua_app.exe examples\hello.lua
bin\lua_app.exe examples\control_flow.lua
bin\lua_app.exe examples\tables_and_methods.lua
bin\lua_app.exe examples\metamethods.lua
```

## Files

| 文件 | 展示主题 |
|---|---|
| `hello.lua` | `print`、局部变量、字符串连接 |
| `control_flow.lua` | `while`、`if`、函数调用和返回值 |
| `tables_and_methods.lua` | 表字段、冒号方法调用、状态更新 |
| `metamethods.lua` | `setmetatable`、`__add`、元方法返回表 |
| `embedding.cpp` | 只通过 `lua.h` / `lauxlib.h` / `lualib.h` 注册 C++ 回调、加载脚本并读取返回值 |

## C++ embedding

`embedding.cpp` 是公开 C API 的真实宿主示例，不引用解释器内部 C++ 类型。使用 CMake 构建并运行：

```powershell
cmake -S . -B build\cmake
cmake --build build\cmake --config Debug --target lua_embedding_example
build\cmake\Debug\lua_embedding_example.exe
```

成功输出为 `embedding result: 42`。

## Related Reading

- `docs/index.md`
- `docs/glossary.md`
- `docs/compatibility/lua51/overview.md`
