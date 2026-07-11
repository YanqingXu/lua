---
status: current
verified_against: docs/guides/development.md; CMakeLists.txt; lua.slnx; lua.vcxproj; lua_app.vcxproj; lua_test.vcxproj; lua_bytecode.vcxproj; README.md
last_checked: 2026-06-13
applies_to: Chinese build and run guide
---

# Build and Run — 构建与运行

## 1. 快速构建

```bash
# VS 构建 (推荐)
bin/build_lua.bat       # 核心库
bin/build_app.bat       # 解释器
bin/build_test.bat      # 测试
bin/build_bytecode.bat  # 字节码工具

# CMake 构建
cmake -B build
cmake --build build --target lua_core
```

## 2. 运行

```bash
# 执行脚本
bin/lua_app.exe hello.lua

# REPL 交互式
bin/lua_app.exe

# 运行测试
bin/lua_test.exe

# 查看字节码
bin/lua_bytecode.exe hello.lua
bin/lua_bytecode.exe --format full hello.lua
bin/lua_bytecode.exe --diff a.lua b.lua

# 开启 Trace
bin/lua_app.exe --trace script.lua
```

## 3. REPL 元命令

```
> .help      — 显示帮助
> .bytecode  — 查看当前编译的字节码
> .ast       — 查看当前编译的 AST
> .gc        — 查看 GC 状态
```

## 4. 环境要求

- **OS**: Windows 10/11
- **编译器**: Visual Studio 2026 (MSVC)
- **C++ 标准**: C++17/23
