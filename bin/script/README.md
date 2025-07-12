# Lua 解释器测试脚本目录

## 📁 目录结构

### 🔧 `metamethods/` - 元方法和元表测试
包含所有与元方法（__index, __newindex, __call等）和元表相关的测试脚本。

**核心测试脚本**：
- `basic_metamethod_test.lua` - 基础元方法功能测试
- `comprehensive_metamethod_test.lua` - 综合元方法测试
- `minimal_metamethod_test.lua` - 最小元方法测试
- `metatable_basic_test.lua` - 基础元表操作测试
- `validate_metamethods.lua` - 元方法验证测试

**高级测试脚本**：
- `advanced_metamethod_test.lua` - 高级元方法功能测试
- `complete_metamethod_test.lua` - 完整元方法测试（包含函数形式）
- `metamethod_step_by_step_test.lua` - 分步元方法测试

**验证和报告**：
- `final_metamethod_validation.lua` - 最终元方法验证
- `metamethod_test_report.lua` - 元方法测试报告
- `validate_metamethods_simple.lua` - 简化验证测试

### 📊 `table/` - 表操作和字面量测试
包含表创建、字段访问、表字面量等相关的测试脚本。

**基础表测试**：
- `basic_table_test.lua` - 基础表操作测试
- `table_literal_test.lua` - 表字面量语法测试
- `field_access_test.lua` - 表字段访问测试

**调试脚本**：
- `debug_table_*.lua` - 各种表操作调试脚本
- `step_by_step_table_test.lua` - 分步表操作测试

### 🔤 `string/` - 字符串操作测试
包含字符串连接、字符串键等相关的测试脚本。

**字符串测试**：
- `string_concat_test.lua` - 字符串连接测试
- `string_key_test.lua` - 字符串键测试

### 🏗️ `basic/` - 基础语言功能测试
包含变量赋值、基本语法等最基础的语言功能测试。

**基础测试**：
- `simple_test.lua` - 简单基础测试
- `basic_assignment_test.lua` - 基础赋值测试
- `minimal_test.lua` - 最小功能测试
- `ultra_basic_test.lua` - 超基础测试

### 🔍 `debug/` - 调试和诊断测试
包含用于调试编译器、VM等组件的诊断脚本。

**调试脚本**：
- `debug_*.lua` - 各种调试脚本
- `diagnostic_test.lua` - 诊断测试

### 🧪 `test/` - 综合和专项测试
包含综合功能测试、性能测试等其他测试脚本。

**综合测试**：
- `core_functionality_test.lua` - 核心功能测试
- `fixed_comprehensive_test.lua` - 修复后的综合测试
- `final_validation.lua` - 最终验证测试

**专项测试**：
- `function_test.lua` - 函数功能测试
- `game_config_test.lua` - 游戏配置测试
- `test_constant_limits.lua` - 常量限制测试

## 🚀 使用方法

### 运行单个测试
```bash
# 基础功能测试
bin\lua.exe bin\script\basic\simple_test.lua

# 元方法测试
bin\lua.exe bin\script\metamethods\basic_metamethod_test.lua

# 表操作测试
bin\lua.exe bin\script\table\table_literal_test.lua
```

### 批量运行测试
```bash
# 运行所有元方法测试
Get-ChildItem bin\script\metamethods\*.lua | ForEach-Object { bin\lua.exe $_.FullName }

# 运行所有基础测试
Get-ChildItem bin\script\basic\*.lua | ForEach-Object { bin\lua.exe $_.FullName }
```

## 📊 测试状态

### ✅ 正常工作的测试类别
- **基础功能** (basic/) - 100% 通过
- **元方法核心功能** (metamethods/) - 核心功能100%通过
- **表操作** (table/) - 动态操作100%通过
- **字符串操作** (string/) - 基础功能正常

### ⚠️ 部分功能需要改进
- **高级元方法** - 函数形式元方法需要完善
- **表字面量** - 键值对语法需要修复
- **调试工具** - 调试输出需要优化

## 🔄 最后更新
- **日期**: 2025年7月12日
- **版本**: 元方法核心功能验证完成版本
- **状态**: 核心功能测试通过，高级功能开发中
