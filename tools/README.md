# Lua 解释器项目开发工具

本目录包含了用于确保代码质量和规范符合性的各种工具和配置文件。

## 📁 文件说明

### 代码规范检查工具
- **`check_standards.sh`** - Linux/macOS 代码规范检查脚本
- **`check_standards_ascii.bat`** - Windows ASCII版本脚本（推荐，英文输出）
- **`check_standards_en.bat`** - Windows 英文版本脚本
- **`check_standards.bat`** - Windows 中文版本脚本

### 构建和质量控制
- **`cmake_standards.cmake`** - CMake 代码质量检查配置
- **`../.clang-format`** - 代码格式化配置文件
- **`../.clang-tidy`** - 静态代码分析配置文件

---

## 🚀 快速开始

### 1. 检查单个文件是否符合规范

#### Linux/macOS:
```bash
# 给脚本执行权限
chmod +x tools/check_standards.sh

# 检查文件
./tools/check_standards.sh src/lib/base_lib_new.cpp
```

#### Windows:
```cmd
# 推荐使用ASCII版本（英文输出，最佳兼容性）
tools\check_standards_ascii.bat src\lib\base_lib_new.cpp

# 或使用英文版本
tools\check_standards_en.bat src\lib\base_lib_new.cpp

# 或使用中文版本（可能在某些环境下有乱码）
tools\check_standards.bat src\lib\base_lib_new.cpp
```

### 2. 使用 CMake 构建系统集成质量检查

```cmake
# 在主 CMakeLists.txt 中包含标准配置
include(tools/cmake_standards.cmake)

# 为库目标启用严格编译检查
add_library(my_lib src/my_lib.cpp)
enable_strict_compilation(my_lib)

# 为测试启用严格检查
add_strict_test(my_test src/tests/my_test.cpp)
```

### 3. 代码格式化

```bash
# 安装 clang-format (如果尚未安装)
# Ubuntu/Debian: sudo apt install clang-format
# macOS: brew install clang-format
# Windows: 通过 LLVM 官网下载

# 格式化所有代码
make format

# 检查格式是否正确 (不修改文件)
make format-check
```

### 4. 静态代码分析

```bash
# 运行 clang-tidy 分析
make cppcheck

# 运行所有质量检查
make check-all
```

---

## 🔧 工具详细说明

### 代码规范检查脚本功能

脚本会检查以下项目：

#### ✅ 类型系统检查
- 确保使用 `types.hpp` 中定义的类型
- 检测禁用的原生类型使用 (`int`, `std::string`, `double` 等)
- 验证是否正确包含 `types.hpp`

#### ✅ 注释语言检查
- 检测中文字符的使用
- 确保所有注释使用英文

#### ✅ 现代C++特性检查
- 检测裸指针内存管理 (`new`/`delete`)
- 验证智能指针的使用
- 检查 `auto` 类型推导的使用

#### ✅ 文档注释检查
- 验证公共类和函数是否有文档注释
- 检查注释的完整性

#### ✅ 线程安全检查
- 检测静态变量但缺少互斥保护的情况
- 验证线程安全设计模式的使用

### 示例输出

```
🔍 Lua 解释器项目代码规范检查
========================================
检查文件: src/lib/base_lib_new.cpp
----------------------------------------

ℹ️  检查类型系统使用规范...
✅ 类型系统使用检查

ℹ️  检查注释语言规范...
✅ 注释语言检查

ℹ️  检查现代C++特性使用...
✅ 发现使用智能指针
✅ 现代C++特性检查

========================================
📊 检查结果汇总
========================================
通过项目: 8
失败项目: 0
🎉 所有检查通过！代码符合规范要求。
```

---

## ⚙️ 配置文件说明

### .clang-format
基于 Google Style Guide 的代码格式化配置，特点：
- 缩进：4个空格
- 行长度：100字符
- 指针对齐：左对齐 (`int* ptr`)
- 包含文件排序：项目头文件 → 第三方库 → 标准库

### .clang-tidy
全面的静态代码分析配置，包括：
- **bugprone-\*** - 检测潜在bug
- **performance-\*** - 性能优化建议
- **modernize-\*** - 现代C++特性使用
- **readability-\*** - 代码可读性
- **cppcoreguidelines-\*** - C++ Core Guidelines 符合性

### cmake_standards.cmake
CMake 配置提供：
- 编译器警告设置 (`-Wall -Wextra -Werror`)
- Debug 模式下的 AddressSanitizer
- Release 模式下的链接时优化 (LTO)
- 自动化的代码质量检查目标

---

## 🎯 集成到开发流程

### 1. 预提交检查
在 git 提交前运行规范检查：

```bash
# 添加到 .git/hooks/pre-commit
#!/bin/bash
echo "运行代码规范检查..."
for file in $(git diff --cached --name-only --diff-filter=ACM | grep '\.\(cpp\|hpp\)$'); do
    ./tools/check_standards.sh "$file" || exit 1
done
echo "代码规范检查通过"
```

### 2. 持续集成 (CI)
在 CI 流水线中集成检查：

```yaml
# GitHub Actions 示例
- name: Code Quality Check
  run: |
    # 安装工具
    sudo apt-get install clang-format clang-tidy cppcheck
    
    # 构建并运行检查
    mkdir build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Debug
    make check-all
```

### 3. IDE 集成
- **VS Code**: 安装 C/C++ 扩展，自动使用项目配置
- **CLion**: 自动识别 `.clang-format` 和 `.clang-tidy`
- **Visual Studio**: 通过 LLVM 扩展使用 clang-format

---

## 🛠️ 故障排除

### 常见问题

#### 1. 脚本权限问题 (Linux/macOS)
```bash
chmod +x tools/check_standards.sh
```

#### 2. 工具未找到
确保安装了必要的工具：
```bash
# Ubuntu/Debian
sudo apt install clang-format clang-tidy cppcheck

# macOS
brew install clang-format llvm cppcheck

# Windows
# 从 LLVM 官网下载安装包
```

#### 3. Windows控制台乱码问题
如果遇到中文显示乱码：

**方案1：使用ASCII版本（推荐）**
```cmd
tools\check_standards_ascii.bat src\lib\base_lib_new.cpp
```

**方案2：使用英文版本**
```cmd
tools\check_standards_en.bat src\lib\base_lib_new.cpp
```

**方案3：设置UTF-8编码后使用中文版本**
```cmd
chcp 65001  # 切换到 UTF-8 编码
tools\check_standards.bat src\lib\base_lib_new.cpp
```

#### 4. PowerShell 替代方案
如果批处理脚本有问题，可以使用PowerShell：
```powershell
# 设置UTF-8编码
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
# 运行检查
.\tools\check_standards_ascii.bat src\lib\base_lib_new.cpp
```

### 获取帮助

如果遇到问题或需要调整配置：
1. 查看 [DEVELOPMENT_STANDARDS.md](../DEVELOPMENT_STANDARDS.md) 完整规范
2. 参考 [项目开发计划](../current_develop_plan.md)
3. 检查工具的官方文档：
   - [clang-format 文档](https://clang.llvm.org/docs/ClangFormat.html)
   - [clang-tidy 文档](https://clang.llvm.org/extra/clang-tidy/)
   - [cppcheck 手册](https://cppcheck.sourceforge.io/manual.pdf)

---

**记住**：代码质量工具是为了帮助我们写出更好的代码，而不是阻碍开发。如果某个规则在特定情况下不适用，可以通过注释临时禁用，但需要有合理的理由。
