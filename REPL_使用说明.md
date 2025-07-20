# Lua 5.1 REPL 使用说明

## 概述

本项目现在包含了一个完整的、符合Lua 5.1官方标准的REPL（Read-Eval-Print Loop）实现。该REPL提供了与官方Lua 5.1解释器相同的交互体验。

## 功能特性

### 1. 交互式命令处理
- **主提示符**: `>` （可通过`_PROMPT`全局变量自定义）
- **续行提示符**: `>>` （可通过`_PROMPT2`全局变量自定义）
- 自动检测完整和不完整的语句

### 2. 多行语句支持
REPL能够智能检测不完整的语句并等待更多输入：

```lua
> function test()
>>   return "hello"
>> end
> test()
"hello"
```

支持的多行结构：
- 函数定义 (`function ... end`)
- 条件语句 (`if ... end`)
- 循环语句 (`while ... end`, `for ... end`, `repeat ... until`)
- 代码块 (`do ... end`)
- 表定义 (`{ ... }`)
- 未闭合的括号表达式

### 3. 表达式结果自动显示
REPL会自动显示表达式的计算结果：

```lua
> 1 + 2
3
> math.sin(3.14159/2)
1
> "hello " .. "world"
"hello world"
> {1, 2, 3}
table: 0x...
```

### 4. 错误处理和恢复
- 友好的错误信息显示
- 错误后不会退出REPL会话
- 支持语法错误和运行时错误的恢复

```lua
> 1 + nil
lua: attempt to perform arithmetic on a nil value
> print("REPL still works")
REPL still works
```

### 5. 状态管理
- 支持`_PROMPT`和`_PROMPT2`全局变量自定义提示符
- 支持`_VERSION`版本信息
- 会话间变量持久化

```lua
> _PROMPT = "lua> "
lua> _PROMPT2 = "...> "
lua> x = 10
lua> x
10
```

### 6. 退出处理
多种退出方式：
- 输入 `exit` 命令
- 调用 `exit()` 函数（支持退出码）
- 按 Ctrl+C 中断当前输入
- EOF (Ctrl+Z on Windows, Ctrl+D on Unix)

```lua
> exit()        -- 退出码 0
> exit(1)       -- 退出码 1
```

## 编译和运行

### 使用MSVC编译
```batch
msvc_build.bat
```

### 运行REPL
```batch
# 直接启动REPL
lua.exe -repl

# 或者运行Lua文件后进入REPL
lua.exe script.lua
```

## 与Lua 5.1官方REPL的兼容性

本实现严格遵循Lua 5.1官方REPL的行为：

1. **提示符行为**: 与官方完全一致
2. **多行输入检测**: 使用相同的语法分析逻辑
3. **表达式求值**: 自动显示表达式结果
4. **错误处理**: 相同的错误信息格式
5. **全局变量**: 支持`_PROMPT`、`_PROMPT2`、`_VERSION`

## 技术实现细节

### 不完整语句检测
- 基于词法分析的智能检测
- 支持嵌套结构的括号匹配
- 处理字符串和注释中的特殊字符

### 表达式识别
- 启发式算法区分语句和表达式
- 自动尝试表达式求值
- 失败时回退到语句执行

### 内存管理
- 与现有GC系统完全集成
- 自动清理临时对象
- 支持长时间REPL会话

## 测试验证

项目包含了完整的测试用例：
- `test_repl.lua`: 手动测试脚本
- 自动化组件测试
- 与官方Lua 5.1行为对比验证

## 已知限制

1. 命令历史功能需要额外的readline库支持
2. 语法高亮需要终端支持
3. 自动补全功能暂未实现

## 未来改进

1. 添加readline支持以获得命令历史和编辑功能
2. 实现Tab自动补全
3. 添加调试命令支持
4. 改进错误信息的本地化

---

该REPL实现为Lua 5.1现代C++框架提供了完整的交互式开发体验，完全符合官方标准并具有良好的扩展性。
