# Current Status — 当前状态

## 1. 这个模块解决什么问题？

记录项目当前的成熟度，明确**已验证的能力**、**未完全验证的能力**和**已知限制**。

## 2. 已验证能力

### 编译器前端
- ✅ 完整 Lua 5.1 词法分析（所有关键字、运算符、字面量、注释）
- ✅ 完整递归下降语法分析（所有语句、表达式、控制流）
- ✅ AST 生成（14 种表达式节点、13 种语句节点）
- ✅ 字节码生成（38 条指令全覆盖）

### VM 执行
- ✅ 38 条指令均有执行分支
- ✅ 基础表达式执行
- ✅ 函数调用、尾调用优化
- ✅ 闭包创建和 Upvalue 管理
- ✅ 控制流（if/while/repeat/for/break/return）
- ✅ Table 操作（数组 + 哈希）

### 运行时对象
- ✅ Value 动态类型系统
- ✅ Table 混合存储（数组 + 哈希）
- ✅ Function（Lua/C 闭包）
- ✅ Upvalue（Open/Closed 状态转换）
- ✅ Userdata（8 字节对齐）
- ✅ Metatable 元方法系统

### 标准库
- ✅ Base 库（print, type, pcall, xpcall 等）
- ✅ Math 库（28/28 函数）
- ✅ String 库（14/14 函数）
- ✅ Table 库
- ✅ OS 库
- ✅ I/O 库
- ✅ Coroutine 库（6/6 函数）
- ✅ Debug 库（14/14 函数）
- ✅ Package 库

### GC
- ✅ 标记-清除算法
- ✅ 弱表（__mode）
- ✅ Userdata finalizer（__gc）
- ✅ 分阶段 step 推进

### 测试
- ✅ 668 个注册测试，3404 个断言，0 失败
- ✅ Lua 5.1 官方测试套件 staged smoke 接入
- ✅ 多类别单元测试（compiler/core/vm/gc/stdlib/metamethod）

## 3. 未完全验证能力

- 🟡 官方 `testC` helper (ltests.c) 未接入
- 🟡 官方 Lua 5.1 binary chunk 格式不兼容
- 🟡 部分标准库函数精细边界仍为简化实现
- 🟡 IncrementalGC 策略为教学占位
- 🟡 跨平台兼容性（主要 Windows）

## 4. 当前阶段判断

**项目已进入真实 Lua 代码执行阶段。**

- 核心链路完整：Lexer → Parser → CodeGen → VM → Runtime
- 测试覆盖充分：668 测试全绿
- 官方兼容良好：官方测试套件主路径通过
- 主要短板：官方 binary chunk、testC helper、精细语义边界

## 5. 统计数据（2026-06）

| 指标 | 数值 |
|------|------|
| **整体完成度** | 约 95% |
| **代码行数** | 约 19k |
| **源文件数** | 约 89 |
| **注册测试** | 668 |
| **断言结果** | 3404 |
| **通过率** | 100% |
| **编译状态** | Release\|x64 /W4 无警告 |
| **C++ 标准** | C++17/23 |
| **平台** | Windows + MSVC |
