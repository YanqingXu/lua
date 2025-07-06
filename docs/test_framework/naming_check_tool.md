# 测试文件命名规范检查工具

这个工具集用于检查 Lua 项目中测试文件的命名是否符合规范。

## 📁 文件说明

- **`check_naming.py`** - 完整功能版本，包含详细报告和统计信息
- **`check_naming_simple.py`** - 简化版本，基于 README.md 中的原始代码
- **`check_naming.bat`** - Windows 批处理文件，双击即可运行
- **`NAMING_CHECK_README.md`** - 本说明文件

## 🎯 命名规范

### 主测试文件
- **格式**: `test_{模块名}_{子模块名}.hpp`
- **示例**: `test_parser_expr.hpp`, `test_vm_state.hpp`

### 子模块测试文件
- **格式**: `{模块名}_{功能名}_test.hpp` 和 `{模块名}_{功能名}_test.cpp`
- **示例**: `parser_binary_expr_test.hpp`, `vm_stack_test.cpp`

### 通用规则
- 只使用小写字母、数字和下划线
- 不使用大写字母或特殊字符
- 头文件使用 `.hpp` 扩展名
- 实现文件使用 `.cpp` 扩展名

## 🚀 使用方法

### 方法一：Windows 批处理文件（推荐）

1. 双击 `check_naming.bat` 文件
2. 脚本会自动检查 `src/tests` 目录
3. 查看检查结果

### 方法二：Python 命令行

#### 完整版本
```bash
# 检查默认目录 src/tests
python check_naming.py

# 检查指定目录
python check_naming.py src/tests

# 显示详细输出
python check_naming.py --verbose src/tests

# 只显示统计信息
python check_naming.py --stats-only src/tests

# 显示修复建议
python check_naming.py --suggest-fixes src/tests
```

#### 简化版本
```bash
# 检查默认目录
python check_naming_simple.py

# 检查指定目录
python check_naming_simple.py src/tests
```

## 📊 输出示例

### 成功情况
```
🔍 正在检查目录: src/tests
--------------------------------------------------
✅ 主测试文件: parser/test_parser_expr.hpp
✅ 子模块测试文件: parser/parser_binary_expr_test.hpp
✅ 子模块测试文件: parser/parser_binary_expr_test.cpp

🎉 恭喜! 所有文件都符合命名规范

📊 统计信息:
------------------------------
总文件数: 15
主测试文件: 5
子模块测试文件: 8
其他文件: 2
问题文件: 0
规范符合率: 100.0%
```

### 发现问题
```
❌ 发现 3 个命名规范问题:
============================================================

📁 主测试文件 (1 个问题):
  ❌ parser/TestParser.hpp
     问题: 命名格式不正确
     期望: test_{模块名}_{子模块名}.hpp
     实际: TestParser.hpp

📁 子模块测试文件 (2 个问题):
  ❌ vm/StackTest.cpp
     问题: 命名格式不正确
     期望: {模块名}_{功能名}_test.hpp/cpp
     实际: StackTest.cpp

💡 修复建议:
------------------------------
• TestParser.hpp → test_parser.hpp
• StackTest.cpp → vm_stack_test.cpp
```

## ⚙️ 高级功能（完整版本）

### 命令行参数
- `-v, --verbose` - 显示详细输出
- `--stats-only` - 只显示统计信息
- `--suggest-fixes` - 显示修复建议

### 自动跳过目录
脚本会自动跳过以下目录：
- 隐藏目录（以 `.` 开头）
- 构建目录（`build`, `Debug`, `Release`）

### 检查范围
- 主测试文件命名格式
- 子模块测试文件命名格式
- 文件扩展名正确性
- 大写字母和特殊字符检查

## 🔧 集成到 CI/CD

### GitHub Actions
```yaml
- name: Check naming convention
  run: |
    python check_naming.py src/tests
    if [ $? -ne 0 ]; then
      echo "命名规范检查失败"
      exit 1
    fi
```

### 本地 Git Hook
```bash
#!/bin/sh
# .git/hooks/pre-commit
python check_naming.py src/tests
if [ $? -ne 0 ]; then
    echo "提交被拒绝：测试文件命名不符合规范"
    exit 1
fi
```

## 🐛 故障排除

### 常见问题

**Q: 提示 "未找到 Python"**
A: 请安装 Python 3.6+ 并确保添加到 PATH 环境变量

**Q: 提示 "未找到检查脚本文件"**
A: 确保脚本文件在当前目录，或使用完整路径

**Q: 提示 "未找到 src/tests 目录"**
A: 确保在项目根目录运行脚本，或指定正确的测试目录路径

**Q: 脚本运行但没有输出**
A: 检查目录中是否有 .hpp 或 .cpp 文件

### 调试模式
```bash
# 使用详细模式查看更多信息
python check_naming.py --verbose src/tests
```

## 📝 自定义规则

如需修改命名规则，请编辑 `check_naming.py` 中的正则表达式：

```python
# 主测试文件命名规范
self.main_test_pattern = re.compile(r'^test_[a-z]+(_[a-z]+)*\.hpp$')

# 子模块测试文件命名规范
self.sub_test_hpp_pattern = re.compile(r'^[a-z]+(_[a-z]+)*_test\.hpp$')
self.sub_test_cpp_pattern = re.compile(r'^[a-z]+(_[a-z]+)*_test\.cpp$')
```

## 📄 许可证

本工具遵循项目的许可证条款。