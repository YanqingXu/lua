---
status: current
verified_against: docs/architecture/overview.md; docs/vm/instruction-set.md; docs/architecture/runtime-services.md; docs/architecture/gc.md; docs2/02-source-code-map/00-directory-map.md; README.md
last_checked: 2026-06-13
applies_to: Chinese high-level architecture overview
---

# 整体架构

## 1. 这个模块解决什么问题？

回答：**这个解释器整体上是怎么组成的？** 建立对项目的全局认知。

## 2. 执行链路全景

```
Lua Source (.lua 文件或字符串)
   ↓
Lexer (词法分析)       → Token 流
   ↓
Parser (语法分析)      → AST (抽象语法树)
   ↓
CodeGen (字节码生成)   → Proto (函数原型 + 字节码指令)
   ↓
VM (虚拟机执行)        → 执行字节码，产生运行时行为
   ↓
Runtime Objects        → Value / Table / Closure / Userdata
   ↓
StdLib / Host Binding  → 标准库函数、C API
```

## 3. 核心模块

| 模块 | 职责 | 关键文件 | 关键类/函数 |
|------|------|---------|------------|
| **Lexer** | 源码转 Token | `src/compiler/lexer/lexer.cpp` | `Lexer::nextToken()` |
| **Parser** | Token 转 AST | `src/compiler/parser/parser.cpp` | `Parser::parse()` |
| **CodeGen** | AST 转 Proto/字节码 | `src/compiler/codegen/codegen.cpp` | `CodeGenerator::generate()` |
| **OpCode** | 38条指令定义 | `src/compiler/opcode.hpp` | `OpCode`, `Instruction` |
| **VM** | 执行字节码 | `src/vm/vm.cpp` | `VM::executeProto()` |
| **Value** | 运行时值表示 | `src/core/value.hpp` | `Value` (std::variant) |
| **Table** | 表/Hash | `src/core/table.cpp` | `Table` |
| **Function** | 函数/闭包/Proto | `src/core/function.cpp` | `Proto`, `Closure`, `Function` |
| **Upvalue** | 闭包上值 | `src/core/upvalue.cpp` | `Upval` (Open/Closed) |
| **GC** | 垃圾回收 | `src/gc/garbage_collector.cpp` | `GarbageCollector` |
| **LuaState** | 执行状态 | `src/vm/state/lua_state.cpp` | `LuaState` |
| **Stack** | 值栈 | `src/vm/state/stack.cpp` | `Stack` |
| **StdLib** | 标准库 | `src/lib/baselib.cpp` 等 | `openBaseLib()` 等 |

## 4. 核心数据结构

### Value — 动态类型

```cpp
using ValueVariant = std::variant<
    std::monostate,     // Nil
    bool,               // Boolean
    void*,              // LightUserdata
    LuaNumber,          // Number (double)
    GCString*,          // String
    Table*,             // Table
    Function*,          // Function
    Userdata*,          // Userdata
    Thread*             // Thread
>;
```

### Instruction — 32位指令

```
iABC:  [OP:6][A:8][C:9][B:9]
iABx:  [OP:6][A:8][Bx:18]
iAsBx: [OP:6][A:8][sBx:18] (有符号)
```

### Proto — 函数原型

包含：字节码指令列表、常量表、局部变量信息、子 Proto 列表、Upvalue 描述、调试信息。

## 5. 核心函数调用链

```text
main()
  ↓
Parser::parse()           — 源码 → AST
  ↓
CodeGenerator::generate() — AST → Proto
  ↓
LuaState::setupMainCallInfo() — 准备调用帧
  ↓
VM::executeProto()        — Proto → 执行
  ↓  (主循环)
  switch(opcode) {
    case MOVE: ...
    case ADD: ...
    case CALL: ...
    case RETURN: ...
  }
```

## 6. 项目定位

- **兼容目标**：Lua 5.1.5 语法和核心语义
- **技术路线**：现代 C++ (C++17/23)，MSVC + CMake
- **设计理念**：可读性 > 极限性能

构建、测试与兼容性边界等会变化的项目事实统一由根目录 `README.md` 维护。

## 7. 三层架构

```
┌─────────────────────────────────┐
│  StdLib / C API / REPL          │  ← 应用层
├─────────────────────────────────┤
│  Compiler (Lexer/Parser/CodeGen)│  ← 编译层
├─────────────────────────────────┤
│  VM (执行引擎)                   │  ← 执行层
├─────────────────────────────────┤
│  Runtime (Value/Table/Function)  │  ← 运行时层
├─────────────────────────────────┤
│  GC / Memory                    │  ← 内存管理层
└─────────────────────────────────┘
```

## 8. 关键设计决策

1. **Value 用 `std::variant`** 而非 C union — 类型安全，易调试
2. **VM 用 switch-case dispatch** + 可选的 table dispatch — 清晰、可追踪
3. **GC 用三色标记-清除** — 经典算法，教学友好
4. **Parser 用递归下降** — 每个语法规则一个函数，结构清晰
5. **Proto/Closure 分离** — Proto 是编译产物（不可变），Closure 是运行时对象（可变 upvalue）
6. **StringPool 驻留** — 相同内容字符串只存一份，指针比较代替内容比较

## 9. 常见 Bug 定位

| 现象 | 优先检查 |
|------|---------|
| 语法报错但 Lua 官方能运行 | Lexer token 是否正确、Parser 是否支持该语法 |
| 执行结果和官方不同 | Bytecode 是否正确、VM 指令实现是否正确 |
| 函数返回值不对 | OP_CALL、OP_RETURN、多返回值规则 |
| 闭包变量丢失 | Upvalue 捕获、close 时机 |

## 10. 当前限制

- 官方 `testC` helper (ltests.c) 尚未接入
- 官方 Lua 5.1 binary chunk 格式不兼容（使用项目本地格式）
- 部分标准库函数仍为简化实现
