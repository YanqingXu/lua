# Enhanced Error Reporting Test Guide

## 📋 概述

本指南说明如何运行和验证增强的错误报告系统功能，确保其与Lua 5.1官方错误格式完全兼容。

## 🗂️ 测试文件结构

```
tests/
├── lua_samples/                          # Lua测试文件
│   ├── error_test_unexpected_symbol.lua  # 意外符号测试
│   ├── error_test_missing_end.lua        # 缺少end测试
│   ├── error_test_unfinished_string.lua  # 未完成字符串测试
│   ├── error_test_malformed_number.lua   # 格式错误数字测试
│   ├── error_test_unexpected_eof.lua     # 意外EOF测试
│   ├── error_test_missing_parenthesis.lua # 缺少括号测试
│   ├── error_test_invalid_escape.lua     # 无效转义测试
│   ├── error_test_multiple_errors.lua    # 多重错误测试
│   ├── error_test_nested_structures.lua  # 嵌套结构测试
│   └── error_test_table_syntax.lua       # 表语法测试
├── enhanced_error_reporting_validation.cpp # 完整验证程序
├── simple_error_format_test.cpp          # 简化测试程序
├── error_output_analyzer.cpp             # 输出分析工具
└── lua51_reference_outputs.txt           # Lua 5.1参考输出
```

## 🚀 运行测试

### 方法1: 简化测试（推荐开始）

```bash
# 运行简化的错误格式测试
run_simple_error_test.bat
```

这个测试不依赖完整的Parser实现，专注于验证错误格式化逻辑。

### 方法2: 完整集成测试

```bash
# 运行完整的错误报告验证
run_error_reporting_tests.bat
```

这个测试需要完整的Parser和相关组件编译成功。

## 📊 测试内容

### 1. 错误格式验证

验证以下错误类型的格式是否符合Lua 5.1标准：

#### ✅ 意外符号错误
```lua
-- 输入: local x = 1 @
-- 期望: stdin:1: unexpected symbol near '@'
```

#### ✅ 缺少结束符错误
```lua
-- 输入: if true then
--       print("hello")
-- 期望: stdin:2: 'end' expected
```

#### ✅ 未完成字符串错误
```lua
-- 输入: local s = "hello world
-- 期望: stdin:1: unfinished string near '"hello world'
```

#### ✅ 格式错误数字
```lua
-- 输入: local n = 123.45.67
-- 期望: stdin:1: malformed number near '123.45.67'
```

#### ✅ 意外文件结束
```lua
-- 输入: function test()
--       print("hello")
-- 期望: stdin:2: 'end' expected
```

### 2. 本地化测试

验证中英文错误信息：

```cpp
// 英文
LocalizationManager::setLanguage(Language::English);
// 输出: "stdin:1: unexpected symbol near '@'"

// 中文  
LocalizationManager::setLanguage(Language::Chinese);
// 输出: "stdin:1: 在 '@' 附近出现意外符号"
```

### 3. 格式兼容性验证

检查错误输出是否符合Lua 5.1标准格式：
- 位置格式: `filename:line:`
- 错误描述: 使用标准Lua 5.1术语
- Token引用: 使用单引号包围
- 简洁性: 避免冗余信息

## 📈 预期结果

### 成功输出示例

```
🚀 Enhanced Error Reporting Validation Suite
=============================================
Testing 10 error cases...

============================================================
Testing: Unexpected symbol '@'
File: error_test_unexpected_symbol.lua
------------------------------------------------------------
Source code:
local x = 1 @

--- English Error Testing ---
Expected: stdin:1: unexpected symbol near '@'
Actual  : stdin:1: unexpected symbol near '@'
✅ English format matches Lua 5.1 standard

--- Chinese Error Testing ---
Expected: stdin:1: 在 '@' 附近出现意外符号
Actual  : stdin:1: 在 '@' 附近出现意外符号
✅ Chinese format matches expected output

✅ TEST PASSED

[... 其他测试用例 ...]

============================================================
📊 TEST SUMMARY
============================================================
Total Tests: 10
✅ Passed: 10
❌ Failed: 0
📈 Success Rate: 100.0%

🎉 ALL TESTS PASSED! Enhanced error reporting is working correctly.
============================================================
```

### 失败输出示例

```
❌ TEST FAILED

--- Difference Analysis ---
❌ Missing 'stdin:' location prefix
❌ Missing 'unexpected symbol near' message format
❌ Missing quoted token in error message
--- End Analysis ---
```

## 🔧 故障排除

### 编译错误

1. **MSVC环境未找到**
   ```
   Error: Could not find MSVC environment
   ```
   - 确保安装了Visual Studio 2019或2022
   - 检查安装路径是否正确

2. **缺少头文件**
   ```
   fatal error C1083: Cannot open include file
   ```
   - 检查项目结构是否完整
   - 确保所有必要的源文件都存在

### 运行时错误

1. **文件未找到**
   ```
   Cannot open file: tests/lua_samples/xxx.lua
   ```
   - 确保所有测试文件都已创建
   - 检查文件路径是否正确

2. **本地化失败**
   ```
   Localization error
   ```
   - 检查LocalizationManager是否正确初始化
   - 确保消息模板已正确加载

## 📋 验证清单

运行测试后，验证以下项目：

- [ ] 所有错误消息都以 `filename:line:` 格式开始
- [ ] 错误描述使用标准Lua 5.1术语
- [ ] Token使用单引号包围
- [ ] 行号准确定位错误位置
- [ ] 中英文本地化正常工作
- [ ] 错误信息简洁明了
- [ ] 与Lua 5.1参考输出高度一致（相似度≥80%）

## 🎯 性能基准

预期性能指标：
- 错误格式化时间: <1ms per error
- 内存开销: <10KB additional memory
- 兼容性: 100%向后兼容
- 相似度: ≥90% with Lua 5.1 reference

## 📚 参考资料

- `tests/lua51_reference_outputs.txt` - Lua 5.1官方错误输出参考
- `docs/parser_error_reporting_guide.md` - 详细实现指南
- [Lua 5.1 Manual](https://www.lua.org/manual/5.1/) - 官方文档

## 🔄 持续验证

建议定期运行这些测试以确保：
1. 新功能不会破坏错误报告格式
2. 错误信息保持与Lua 5.1兼容
3. 本地化功能正常工作
4. 性能保持在可接受范围内
