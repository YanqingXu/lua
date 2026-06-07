# Load Source — 源码加载

## 1. 这个模块解决什么问题？

将 `.lua` 文件或字符串读入内存，准备进行词法分析。

## 2. 在整体执行链路中的位置

```
Load Source → Tokenize → Parse → Compile → VM Execute
    ↑
   (第一阶段)
```

## 3. 核心文件

| 文件 | 作用 |
|------|------|
| `src/io/file_loader.hpp/cpp` | 文件加载器 |
| `src/io/input_stream.hpp/cpp` | 输入流抽象 |
| `src/main.cpp` | lua_app 入口 |

## 4. 加载路径

### 从文件加载
```cpp
// src/io/file_loader.cpp
Str source = FileLoader::load("hello.lua");
Parser parser(source);
auto result = parser.parse();
```

### 从字符串加载（REPL / loadstring）
```cpp
Str source = "print('hello')";
Parser parser(source);
auto result = parser.parse();
```

### 从 stdin 加载
```cpp
std::string source;
std::string line;
while (std::getline(std::cin, line)) {
    source += line + "\n";
}
```

## 5. InputStream 设计

```
InputStream (抽象接口)
  ├── StringInputStream: 从字符串读取
  ├── FileInputStream: 从文件读取
  └── 提供逐字符访问接口

核心方法：
  - peek(): 读取当前字符（不前进）
  - advance(): 读取并前进一个字符
  - isAtEnd(): 是否到达末尾
```

## 6. 加载方式对比

| 方式 | 入口 | 用途 |
|------|------|------|
| `lua_app hello.lua` | 命令行参数 | 执行脚本文件 |
| `lua_app` (无参数) | stdin / REPL | 交互式执行 |
| `loadfile("hello.lua")` | Lua 标准库 | 运行时加载 |
| `loadstring("code")` | Lua 标准库 | 编译字符串 |
| `dofile("hello.lua")` | Lua 标准库 | 加载并执行 |
