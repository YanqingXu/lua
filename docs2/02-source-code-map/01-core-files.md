# Core Files — 核心文件说明

## 1. 这个模块解决什么问题？

深入说明每个核心文件的职责和关键内容。

## 2. 编译器前端文件

### `src/compiler/lexer/lexer.hpp/cpp`
- **职责**：词法分析，源码 → Token 流
- **核心方法**：`nextToken()`, `peekToken()`, `scanToken()`
- **关键点**：LL(1) 前瞻，关键字哈希表，字符串转义

### `src/compiler/parser/parser.hpp/cpp`
- **职责**：语法分析，Token 流 → AST
- **核心方法**：`parse()` → `Chunk`
- **关键点**：递归下降，Pratt parser 表达式解析

### `src/compiler/ast.hpp`
- **职责**：AST 节点定义
- **关键点**：14 种 Expr 节点 + 13 种 Stmt 节点，全部用 variant

### `src/compiler/opcode.hpp`
- **职责**：38 条 Lua 5.1 指令定义
- **关键点**：指令格式（iABC/iABx/iAsBx），RK 寻址

### `src/compiler/codegen/codegen.cpp`
- **职责**：字节码生成入口
- **核心方法**：`generate(Chunk)` → `Proto`
- **关键点**：常量池管理，寄存器分配，跳转回填

## 3. 运行时核心文件

### `src/core/value.hpp/cpp`
- **职责**：运行时值表示
- **关键点**：`std::variant` 表示 9 种 Lua 类型
- **大小**：~16 字节（64位系统）

### `src/core/table.hpp/cpp`
- **职责**：Lua 表实现
- **关键点**：数组部分 + 哈希部分混合存储
- **代码量**：~683 行

### `src/core/function.hpp/cpp`
- **职责**：Proto、Closure、Function 实现
- **关键点**：Lua Closure vs C Closure，Upvalue 绑定
- **代码量**：~1096 行

### `src/core/upvalue.hpp/cpp`
- **职责**：闭包上值管理
- **关键点**：Open/Closed 状态转换，多个闭包共享
- **代码量**：~419 行

### `src/core/metatable.hpp/cpp`
- **职责**：元方法系统
- **关键点**：17 种元方法名称，table/userdata/基础类型元表查找
- **代码量**：~697 行

## 4. VM 核心文件

### `src/vm/vm.cpp`
- **职责**：VM 执行引擎入口 + 主循环
- **核心方法**：`executeProto()`, `call()`, `tryExecuteProto()`
- **关键点**：switch-case dispatch，goto reentry

### `src/vm/vm_handlers/`（9 个文件）
- **职责**：38 条指令的具体实现
- **分类**：arith, branch, call, closure, data, global_upvalue, loop, table, unary

### `src/vm/state/lua_state.cpp`
- **职责**：LuaState 管理（栈、调用帧、Upvalue 链表）
- **代码量**：~1095 行

### `src/vm/state/global_state.cpp`
- **职责**：GlobalState 单例，全局资源管理
- **关键点**：字符串池、GC、注册表、元表

## 5. GC 文件

### `src/gc/garbage_collector.cpp`
- **职责**：GC 主控制器
- **关键点**：标记-清除流程，根集扫描
- **代码量**：~808+ 行

### `src/gc/gc_mark.cpp` / `gc_sweep.cpp` / `gc_finalize.cpp` / `gc_weak.cpp`
- **职责**：标记/清除/终结/弱表各阶段实现
