# Lua Interpreter Project Development Tools

This directory contains various tools and configuration files to ensure code quality and compliance with development standards.

## 📁 File Overview

### Code Standards Checking Tools
- **`check_standards.sh`** - Linux/macOS code standards checking script
- **`check_standards_ascii.bat`** - Windows ASCII version script (recommended, English output)
- **`check_standards_en.bat`** - Windows English version script
- **`check_standards.bat`** - Windows Chinese version script

### Build and Quality Control
- **`cmake_standards.cmake`** - CMake code quality checking configuration
- **`../.clang-format`** - Code formatting configuration file
- **`../.clang-tidy`** - Static code analysis configuration file

---

## 🚀 Quick Start

### 1. Check if a single file complies with standards

#### Linux/macOS:
```bash
# Give the script execution permissions
chmod +x tools/check_standards.sh

# Check file
./tools/check_standards.sh src/lib/base/base_lib.cpp
```

#### Windows:
```cmd
# Recommended to use ASCII version (English output, best compatibility)
tools\check_standards_ascii.bat src\lib\base_lib.cpp

# Or use English version
tools\check_standards_en.bat src\lib\base_lib.cpp

# Or use Chinese version (may have encoding issues in some environments)
tools\check_standards.bat src\lib\base_lib.cpp
```

### 2. Integrate quality checks with CMake build system

```cmake
# Include standard configuration in main CMakeLists.txt
include(tools/cmake_standards.cmake)

# Enable strict compilation checks for library targets
add_library(my_lib src/my_lib.cpp)
enable_strict_compilation(my_lib)

# Enable strict checks for tests
add_strict_test(my_test src/tests/my_test.cpp)
```

### 3. Code Formatting

```bash
# Install clang-format (if not already installed)
# Ubuntu/Debian: sudo apt install clang-format
# macOS: brew install clang-format
# Windows: Download from LLVM official website

# Format all code
make format

# Check if format is correct (does not modify files)
make format-check
```

### 4. Static Code Analysis

```bash
# Run clang-tidy analysis
make cppcheck

# Run all quality checks
make check-all
```

---

## 🔧 Detailed Tool Description

### Code Standards Checking Script Features

The script checks the following items:

#### ✅ Type System Check
- Ensure use of types defined in `types.hpp`
- Detect prohibited native type usage (`int`, `std::string`, `double`, etc.)
- Verify correct inclusion of `types.hpp`

#### ✅ Comment Language Check
- Detect use of Chinese characters
- Ensure all comments use English

#### ✅ Modern C++ Features Check
- Detect raw pointer memory management (`new`/`delete`)
- Verify use of smart pointers
- Check use of `auto` type deduction

#### ✅ Documentation Comment Check
- Verify that public classes and functions have documentation comments
- Check completeness of comments

#### ✅ Thread Safety Check
- Detect static variables without mutex protection
- Verify use of thread-safe design patterns

### Example Output

```
🔍 Lua Interpreter Project Code Standards Check
========================================
Checking file: src/lib/base_lib.cpp
----------------------------------------

ℹ️  Checking type system usage standards...
✅ Type system usage check

ℹ️  Checking comment language standards...
✅ Comment language check

ℹ️  Checking modern C++ features usage...
✅ Found smart pointer usage
✅ Modern C++ features check

========================================
📊 Check Results Summary
========================================
Passed items: 8
Failed items: 0
🎉 All checks passed! Code meets standard requirements.
```

---

## ⚙️ Configuration File Description

### .clang-format
Code formatting configuration based on Google Style Guide, featuring:
- Indentation: 4 spaces
- Line length: 100 characters
- Pointer alignment: left-aligned (`int* ptr`)
- Include file sorting: project headers → third-party libraries → standard library

### .clang-tidy
Comprehensive static code analysis configuration including:
- **bugprone-\*** - Detect potential bugs
- **performance-\*** - Performance optimization suggestions
- **modernize-\*** - Modern C++ feature usage
- **readability-\*** - Code readability
- **cppcoreguidelines-\*** - C++ Core Guidelines compliance

### cmake_standards.cmake
CMake configuration providing:
- Compiler warning settings (`-Wall -Wextra -Werror`)
- AddressSanitizer in Debug mode
- Link Time Optimization (LTO) in Release mode
- Automated code quality check targets

---

## 🎯 Integration into Development Workflow

### 1. Pre-commit Check
Run standards check before git commit:

```bash
# Add to .git/hooks/pre-commit
#!/bin/bash
echo "Running code standards check..."
for file in $(git diff --cached --name-only --diff-filter=ACM | grep '\.\(cpp\|hpp\)$'); do
    ./tools/check_standards.sh "$file" || exit 1
done
echo "Code standards check passed"
```

### 2. Continuous Integration (CI)
Integrate checks in CI pipeline:

```yaml
# GitHub Actions example
- name: Code Quality Check
  run: |
    # Install tools
    sudo apt-get install clang-format clang-tidy cppcheck
    
    # Build and run checks
    mkdir build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Debug
    make check-all
```

### 3. IDE Integration
- **VS Code**: Install C/C++ extension, automatically uses project configuration
- **CLion**: Automatically recognizes `.clang-format` and `.clang-tidy`
- **Visual Studio**: Use clang-format through LLVM extension

---

## 🛠️ Troubleshooting

### Common Issues

#### 1. Script Permission Issues (Linux/macOS)
```bash
chmod +x tools/check_standards.sh
```

#### 2. Tools Not Found
Ensure necessary tools are installed:
```bash
# Ubuntu/Debian
sudo apt install clang-format clang-tidy cppcheck

# macOS
brew install clang-format llvm cppcheck

# Windows
# Download installer from LLVM official website
```

#### 3. Windows Console Encoding Issues
If encountering Chinese display garbled text:

**Solution 1: Use ASCII version (recommended)**
```cmd
tools\check_standards_ascii.bat src\lib\base_lib.cpp
```

**Solution 2: Use English version**
```cmd
tools\check_standards_en.bat src\lib\base_lib.cpp
```

**Solution 3: Set UTF-8 encoding then use Chinese version**
```cmd
chcp 65001  # Switch to UTF-8 encoding
tools\check_standards.bat src\lib\base_lib.cpp
```

#### 4. PowerShell Alternative
If batch scripts have issues, use PowerShell:
```powershell
# Set UTF-8 encoding
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
# Run check
.\tools\check_standards_ascii.bat src\lib\base_lib.cpp
```

### Getting Help

If encountering problems or need to adjust configuration:
1. Check [DEVELOPMENT_STANDARDS.md](../DEVELOPMENT_STANDARDS.md) for complete standards
2. Refer to [Project Development Plan](../current_develop_plan.md)
3. Check official documentation for tools:
   - [clang-format Documentation](https://clang.llvm.org/docs/ClangFormat.html)
   - [clang-tidy Documentation](https://clang.llvm.org/extra/clang-tidy/)
   - [cppcheck Manual](https://cppcheck.sourceforge.io/manual.pdf)

---

**Remember**: Code quality tools are meant to help us write better code, not hinder development. If a certain rule doesn't apply in a specific situation, it can be temporarily disabled through comments, but there needs to be a reasonable justification.
