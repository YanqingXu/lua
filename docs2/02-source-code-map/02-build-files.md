# Build Files — 构建文件说明

## 1. 这个模块解决什么问题？

说明项目的构建体系和各个构建文件的作用。

## 2. 构建入口

| 文件 | 类型 | 用途 |
|------|------|------|
| `CMakeLists.txt` | CMake | 跨平台构建配置 |
| `lua.slnx` | VS Solution | Visual Studio 解决方案 |
| `lua.vcxproj` | VS Project | 核心静态库 |
| `lua_app.vcxproj` | VS Project | 解释器/REPL |
| `lua_test.vcxproj` | VS Project | 测试运行器 |
| `lua_bytecode.vcxproj` | VS Project | 字节码工具 |

## 3. 编译批处理

| 脚本 | 命令 | 产物 |
|------|------|------|
| `bin/build_lua.bat` | MSBuild lua.vcxproj | `lua.lib` |
| `bin/build_app.bat` | MSBuild lua_app.vcxproj | `lua_app.exe` |
| `bin/build_test.bat` | MSBuild lua_test.vcxproj | `lua_test.exe` |
| `bin/build_bytecode.bat` | MSBuild lua_bytecode.vcxproj | `lua_bytecode.exe` |

## 4. CMake 构建

```bash
# 配置
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# 构建核心库
cmake --build build --target lua_core

# 构建并运行测试
cmake --build build --target lua_test
ctest --test-dir build
```

## 5. CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `LUA_CPP_BUILD_TOOLS` | ON | 构建 lua_app 和 lua_bytecode |
| `LUA_CPP_BUILD_TESTS` | ON | 构建 lua_test |

## 6. 编译器设置

```cmake
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC 警告设置
target_compile_options(... PRIVATE /W4 /permissive- /utf-8 /FS)

# GCC/Clang 警告设置
target_compile_options(... PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
```

## 7. 运行时栈大小

```cmake
# Debug 构建需要更大的栈
target_link_options(... PRIVATE "/STACK:16777216")  # 16 MB
```

## 8. 依赖关系

```
lua_core (静态库)
  ↑
  ├── lua_app (链接 lua_core)
  ├── lua_bytecode (链接 lua_core)
  └── lua_test (链接 lua_core)
```
