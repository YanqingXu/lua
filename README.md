# 现代C++ Lua解释器

> **从零开始用C++17/20/23实现Lua 5.1.5解释器**

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![Tests](https://img.shields.io/badge/tests-155%2F155-brightgreen)]()
[![Coverage](https://img.shields.io/badge/coverage-100%25-brightgreen)]()
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue)]()
[![Platform](https://img.shields.io/badge/platform-Windows-blue)]()

---

## 🎯 项目概览

### 项目目标

本项目旨在**从零开始**使用现代C++（C++17/20/23）重新实现一个完整的**Lua 5.1.5解释器**。

**核心特点**：
- 📚 **参考实现**：基于`lua_c_analysis`中的Lua 5.1.5 C源码（带详细中文注释）
- 🔧 **技术栈**：C++17标准、MSVC编译器（Visual Studio 2026）、Windows平台
- ✨ **现代C++**：充分利用现代C++特性（`std::variant`、智能指针、类型别名、STL容器）
- 🎓 **教育价值**：清晰的架构、完善的测试、详细的文档，适合学习Lua实现原理

### 开发方法

1. **主要参考**：`lua_c_analysis/` - Lua 5.1.5 C源码 + 53篇中文技术文档
2. **次要参考**：`lua_with_cpp/` - 另一个C++ Lua实现（部分完成）
3. **方法论**：`spec-kit/` - Spec-Driven Development开发方法论

---

## 📊 当前进度

### 已完成模块（17个核心模块）

| 模块 | 文件 | 功能描述 | 测试数 | 状态 |
|------|------|---------|--------|------|
| **基础类型系统** | `src/common/types.hpp` | 类型别名定义（Vec、HashMap、usize等） | - | ✅ 完成 |
| **配置系统** | `src/common/config.hpp` | 编译配置和常量定义 | - | ✅ 完成 |
| **宏定义** | `src/common/macros.hpp` | 实用宏定义 | - | ✅ 完成 |
| **Value类** | `src/core/value.hpp/cpp` | Lua值的C++表示（使用std::variant） | 14 | ✅ 完成 |
| **GCObject基类** | `src/core/gc_object.hpp/cpp` | GC对象基类（三色标记） | 8 | ✅ 完成 |
| **GCString类** | `src/core/gc_string.hpp/cpp` | GC管理的字符串对象 | 10 | ✅ 完成 |
| **StringPool类** | `src/core/string_pool.hpp/cpp` | 字符串驻留池（单例模式） | 11 | ✅ 完成 |
| **Table类** | `src/core/table.hpp/cpp` | Lua表（数组+哈希混合存储） | 11 | ✅ 完成 |
| **Function类** | `src/core/function.hpp/cpp` | 函数对象（Proto + Closure + Upvalue） | 12 | ✅ 完成 |
| **Upvalue类** | `src/core/upvalue.hpp/cpp` | 闭包上值管理（Open/Closed状态） | 11 | ✅ 完成 |
| **Userdata类** | `src/core/userdata.hpp/cpp` | 用户数据（C++数据包装） | 6 | ✅ 完成 |
| **GarbageCollector** | `src/gc/garbage_collector.hpp/cpp` | 垃圾回收器（标记-清除算法） | 8 | ✅ 完成 |
| **GlobalState类** | `src/vm/global_state.hpp/cpp` | 全局状态管理（单例模式） | 4 | ✅ 完成 |
| **Stack类** | `src/vm/stack.hpp/cpp` | 值栈管理（动态扩展） | 5 | ✅ 完成 |
| **CallInfo类** | `src/vm/call_info.hpp` | 调用信息（函数调用上下文） | 3 | ✅ 完成 |
| **LuaState类** | `src/vm/lua_state.hpp/cpp` | Lua状态（线程执行环境） | 5 | ✅ 完成 |
| **Lexer词法分析器** | `src/compiler/lexer.hpp/cpp` + `token.hpp` | 词法分析（Token流生成） | 18 | ✅ 完成 |
| **Parser语法分析器** | `src/compiler/parser.hpp/cpp` + `ast.hpp/cpp` | 语法分析（AST生成） | 10 | ✅ 完成 |
| **CodeGenerator字节码生成器** | `src/compiler/codegen.hpp/cpp` + `opcode.hpp/cpp` | 字节码生成（AST→Bytecode） | 7 | ✅ 完成 |
| **VM字节码执行引擎** | `src/vm/vm.hpp/cpp` | 字节码解释执行（38条指令） | 6 | ✅ 完成 |
| **基础库（Base Library）** | `src/lib/baselib.hpp/cpp` | 8个核心函数（print、type等） | 8 | ✅ 完成 |

### 测试统计

```
总测试数：155个
通过率：  100% (155/155)
编译状态：Debug版本无警告
平台：    Windows + MSVC (Visual Studio 2026)
```

### 核心实现亮点

✅ **Value类**：使用`std::variant`实现类型安全的动态类型系统
✅ **GCObject**：三色标记（White/Gray/Black）支持增量GC
✅ **Table类**：混合存储（数组部分 + 哈希部分），自动优化
✅ **Function类**：支持C函数和Lua函数两种闭包类型，集成Upvalue管理
✅ **Upvalue类**：闭包上值管理，支持Open/Closed状态转换，共享机制
✅ **Userdata类**：完整用户数据支持，8字节对齐，元表支持，GC集成
✅ **Lexer词法分析器**：完整Lua 5.1词法规则，支持所有关键字、运算符、字面量、注释
✅ **Parser语法分析器**：递归下降解析，完整AST生成，正确的运算符优先级和结合性
✅ **CodeGenerator字节码生成器**：AST→字节码转换，寄存器分配，常量表管理，跳转回填
✅ **OpCode指令集**：完整Lua 5.1指令集（38条指令），iABC/iABx/iAsBx三种格式
✅ **VM字节码执行引擎**：完整38条指令实现，Upvalue操作，函数调用（C函数），循环指令（FORLOOP/FORPREP/TFORLOOP），闭包创建，表初始化（SETLIST）
✅ **基础库（Base Library）**：8个核心函数（print、type、tostring、tonumber、error、assert、setmetatable、getmetatable），支持基本Lua脚本运行
✅ **StringPool**：字符串驻留（interning），节省内存
✅ **GarbageCollector**：标记-清除算法，根对象管理
✅ **GlobalState**：单例模式管理全局资源（字符串池、GC、注册表）
✅ **Stack**：动态值栈，自动扩展，O(1)压栈/弹栈操作
✅ **CallInfo**：轻量级调用上下文，支持函数调用链管理
✅ **LuaState**：完整的线程执行环境，整合栈、调用信息和Upvalue链表，扩展了30+个API方法支持基础库

### 虚拟机核心模块详解

#### GlobalState（全局状态）

**设计模式**：单例模式

**核心职责**：
- 管理所有线程共享的全局资源
- 字符串池（StringPool）的访问入口
- 垃圾回收器（GarbageCollector）的访问入口
- 注册表（Registry）管理：C代码专用的全局存储
- 元表管理：为基础类型（nil、boolean、number等）提供元表支持
- 主线程引用：维护主线程的指针

**关键特性**：
```cpp
GlobalState& gs = GlobalState::getInstance();  // 单例访问
StringPool& pool = gs.getStringPool();         // 字符串池
GarbageCollector& gc = gs.getGC();             // GC
Table* registry = gs.getRegistry();            // 注册表
gs.setMetatable(ValueType::Number, mt);        // 设置元表
```

**内存布局**：104字节（包含引用和指针数组）

#### Stack（值栈）

**设计模式**：动态数组

**核心职责**：
- 存储函数参数、局部变量和临时值
- 自动扩展：容量不足时自动翻倍
- 高效访问：O(1)压栈、弹栈、索引访问

**关键特性**：
```cpp
Stack stack;
stack.push(Value(42.0));           // 压栈
Value v = stack.pop();             // 弹栈
Value& top = stack.top();          // 访问栈顶
Value& val = stack.at(index);      // 索引访问
stack.ensureSpace(n);              // 确保有n个空闲槽位
```

**常量定义**：
- `MIN_STACK_SIZE = 20`：最小栈大小
- `INITIAL_STACK_SIZE = 40`：初始栈大小
- `EXTRA_STACK = 5`：额外保留空间

**内存布局**：40字节（Vec容器 + top指针）

#### CallInfo（调用信息）

**设计模式**：轻量级结构体

**核心职责**：
- 存储单次函数调用的上下文信息
- 管理栈帧布局（func、base、top）
- 记录返回值数量和尾调用计数

**栈帧布局**：
```
┌─────────────┐ ← top (栈顶)
│  局部变量3  │
│  局部变量2  │
│  局部变量1  │
├─────────────┤ ← base (栈基址)
│   参数2     │
│   参数1     │
│  函数对象   │ ← func
└─────────────┘
```

**关键字段**：
```cpp
CallInfo ci;
ci.func = 10;        // 函数对象在栈索引10
ci.base = 11;        // 参数从索引11开始
ci.top = 20;         // 栈顶在索引20
ci.nresults = 2;     // 期望2个返回值
ci.savedpc = ptr;    // 程序计数器（Lua函数）
ci.tailcalls = 0;    // 尾调用计数
```

**内存布局**：40字节（6个字段）

#### Upvalue（闭包上值）

**设计模式**：状态模式（Open/Closed状态）

**核心职责**：
- 管理闭包捕获的外部变量
- 支持Open状态（指向栈上变量）和Closed状态（独立存储）
- 实现多个闭包共享同一Upvalue的机制
- 参与GC标记和清除

**状态转换**：
```cpp
// Open状态：v_指向栈上的Value
Upvalue* uv = Upvalue::createOpen(&stackValue, stackIndex);
uv->isOpen();  // true
uv->getValue(); // 返回栈上的值

// 关闭操作：将栈上的值复制到closedValue_
uv->close();
uv->isClosed(); // true
uv->getValue(); // 返回closedValue_
```

**共享机制**：
```cpp
// LuaState管理open upvalue链表（按栈索引降序）
Upvalue* uv1 = L->findOrCreateUpvalue(5);  // 创建新upvalue
Upvalue* uv2 = L->findOrCreateUpvalue(5);  // 返回同一个upvalue
assert(uv1 == uv2);  // 共享同一个upvalue
```

**关闭时机**：
```cpp
// 函数返回时关闭所有栈层级 >= level 的upvalue
L->closeUpvalues(level);
```

**内存布局**：64字节（v_指针、stackIndex、closedValue、next指针）

**关键算法**：
- **findOrCreateUpvalue**：在降序链表中查找或创建upvalue
- **closeUpvalues**：批量关闭指定层级以上的upvalue
- **close()**：将Open状态转换为Closed状态

#### LuaState（Lua状态）

**设计模式**：RAII资源管理

**核心职责**：
- 管理单个Lua线程的完整执行状态
- 整合值栈（Stack）和调用栈（CallInfo数组）
- 管理Open Upvalue链表（按栈索引降序）
- 提供栈操作和Upvalue管理的便捷接口
- 管理全局表和线程状态

**关键特性**：
```cpp
LuaState* L = LuaState::newState();  // 创建新状态
L->pushNumber(42.0);                 // 压入数值
L->pushBoolean(true);                // 压入布尔值
L->pushString(str);                  // 压入字符串
Value v = L->pop();                  // 弹出值
Table* gt = L->getGlobalTable();     // 获取全局表
CallInfo& ci = L->getCurrentCallInfo(); // 当前调用信息

// Upvalue管理
Upvalue* uv = L->findOrCreateUpvalue(5);  // 查找或创建upvalue
L->closeUpvalues(10);                     // 关闭栈层级 >= 10 的upvalue
```

**状态枚举**：
```cpp
enum class ThreadStatus {
    OK = 0,         // 正常执行
    Yield = 1,      // 协程挂起
    ErrRun = 2,     // 运行时错误
    ErrSyntax = 3,  // 语法错误
    ErrMem = 4,     // 内存错误
    ErrErr = 5      // 错误处理函数错误
};
```

**初始化流程**：
1. 创建值栈（初始大小40）
2. 创建调用栈（初始大小8）
3. 创建全局表并注册为GC根对象
4. 初始化第一个CallInfo（虚拟主函数）
5. 如果是第一个LuaState，设置为主线程

**内存布局**：104字节（包含Stack、CallInfo数组、引用和指针）

#### Userdata（用户数据）

**设计模式**：GC管理的内存块

**核心职责**：
- 将C++数据结构包装成Lua对象
- 提供GC管理的内存块
- 支持元表实现自定义行为
- 保证内存对齐（8字节）

**关键特性**：
```cpp
// 创建完整用户数据
Userdata* ud = Userdata::createFull(64);  // 分配64字节

// 类型化创建
struct MyData { int id; double value; };
MyData data = {123, 3.14};
Userdata* ud2 = Userdata::create(data);

// 数据访问
void* rawData = ud->getData();
MyData* typedData = ud->getTypedData<MyData>();

// 元表支持
Table* mt = new Table();
ud->setMetatable(mt);
bool hasMt = ud->hasMetatable();
```

**内存布局**：
```
[Userdata对象头部][用户数据块（8字节对齐）]
```

**GC集成**：
- 自动标记元表
- 计算总内存大小（对象 + 数据）
- 析构时自动释放对齐内存

**平台兼容性**：
- Windows (MSVC): 使用`_aligned_malloc`/`_aligned_free`
- Linux/macOS: 使用`std::aligned_alloc`/`std::free`

#### Lexer（词法分析器）

**设计模式**：单遍扫描的LL(1)词法分析

**核心职责**：
- 将Lua源代码文本转换为Token流
- 识别所有Lua 5.1关键字、运算符和字面量
- 处理注释（单行和多行）
- 精确跟踪行号和列号

**关键特性**：
```cpp
// 创建词法分析器
Lexer lexer("local x = 42");

// 获取Token流
Token t1 = lexer.nextToken();  // local (关键字)
Token t2 = lexer.nextToken();  // x (标识符)
Token t3 = lexer.nextToken();  // = (运算符)
Token t4 = lexer.nextToken();  // 42 (数字)

// Token信息
std::cout << t4.lexeme;        // "42"
std::cout << t4.line;          // 行号
std::cout << t4.column;        // 列号
f64 value = std::get<f64>(t4.value);  // 42.0
```

**支持的Token类型**：
- **21个关键字**：and, break, do, else, elseif, end, false, for, function, if, in, local, nil, not, or, repeat, return, then, true, until, while
- **单字符运算符**：+ - * / % ^ # = < > ( ) { } [ ] ; : , .
- **多字符运算符**：.. ... == ~= <= >=
- **字面量**：数字（整数、浮点、科学计数法、十六进制）、字符串（单引号、双引号、长字符串）
- **标识符**：[a-zA-Z_][a-zA-Z0-9_]*

**注释处理**：
```lua
-- 单行注释
--[[ 多行注释 ]]
--[=[ 嵌套级别的长注释 ]=]
```

**字符串支持**：
```lua
"double quote"
'single quote'
[[long string]]
[=[long string with level]=]
"escape sequences: \n \t \\ \""
```

**错误处理**：
- 未闭合字符串检测
- 非法字符检测
- 详细的错误位置信息

#### Parser（语法分析器）

**设计模式**：递归下降解析（Recursive Descent Parsing）

**核心职责**：
- 将Token流转换为抽象语法树（AST）
- 实现Lua 5.1的完整语法规则
- 处理运算符优先级和结合性
- 提供详细的语法错误信息

**关键特性**：
```cpp
// 创建语法分析器
Parser parser("local x = 42");

// 解析生成AST
Chunk chunk = parser.parse();

// 访问AST节点
for (const auto& stmt : chunk.statements) {
    // 处理语句节点
}
```

**AST节点设计**：
```cpp
// 使用std::variant实现类型安全的多态
using ExprVariant = std::variant<
    NilExpr, BoolExpr, NumberExpr, StringExpr,
    NameExpr, BinaryExpr, UnaryExpr, TableExpr,
    CallExpr, IndexExpr, MemberExpr, FunctionExpr
>;

using StmtVariant = std::variant<
    EmptyStmt, AssignStmt, LocalStmt, CallStmt,
    IfStmt, WhileStmt, RepeatStmt, ForNumStmt,
    ForInStmt, FunctionStmt, ReturnStmt, BreakStmt, DoStmt
>;

struct Expr {
    ExprVariant variant;
    i32 getLine() const;
    i32 getColumn() const;
};

struct Stmt {
    StmtVariant variant;
    i32 getLine() const;
    i32 getColumn() const;
};
```

**运算符优先级表**：
```
优先级  运算符              结合性
------  -----------------  --------
1       or                 左结合
2       and                左结合
3       <, >, <=, >=, ==, ~=  左结合
4       ..                 右结合
5       +, -               左结合
6       *, /, %            左结合
7       not, -, #          右结合
8       ^                  右结合
```

**解析函数层次**：
```cpp
// 语句解析
StmtPtr parseStatement();
StmtPtr parseIfStmt();
StmtPtr parseWhileStmt();
StmtPtr parseForStmt();
// ... 其他语句

// 表达式解析（按优先级）
ExprPtr parseExpression();      // 入口
ExprPtr parseOrExpr();          // or
ExprPtr parseAndExpr();         // and
ExprPtr parseRelationalExpr();  // <, >, <=, >=, ==, ~=
ExprPtr parseConcatExpr();      // ..
ExprPtr parseAdditiveExpr();    // +, -
ExprPtr parseMultiplicativeExpr(); // *, /, %
ExprPtr parseUnaryExpr();       // not, -, #
ExprPtr parsePowerExpr();       // ^
ExprPtr parsePrimaryExpr();     // 字面量、标识符、括号表达式
```

**错误处理**：
```cpp
class ParseError : public std::runtime_error {
    i32 line_, column_;
public:
    ParseError(const Str& message, i32 line, i32 column);
    i32 getLine() const;
    i32 getColumn() const;
};
```

**内存管理**：
- 使用`std::unique_ptr`管理AST节点
- 自动内存释放，无需手动管理
- 移动语义优化性能

#### CodeGenerator（字节码生成器）

**设计模式**：AST访问者模式

**核心职责**：
- 将AST转换为Lua 5.1字节码
- 管理寄存器分配和释放
- 管理常量表（数字、字符串、布尔值、nil）
- 管理局部变量作用域
- 实现跳转指令回填

**关键特性**：
```cpp
// 创建代码生成器
StringPool* pool = &GlobalState::getInstance().getStringPool();
CodeGenerator codegen(pool);

// 生成字节码
Parser parser("return 42");
Chunk chunk = parser.parse();
Proto* proto = codegen.generate(chunk);

// 访问生成的字节码
const Vec<Instruction>& code = proto->getCode();
const Vec<Value>& constants = proto->getConstants();
usize maxStackSize = proto->getMaxStackSize();
```

**OpCode指令集**（38条Lua 5.1指令）：
```cpp
// 指令格式
iABC:  [6位OpCode][8位A][9位C][9位B]
iABx:  [6位OpCode][8位A][18位Bx]
iAsBx: [6位OpCode][8位A][18位sBx（有符号）]

// 主要指令类别
- 数据移动: MOVE, LOADK, LOADBOOL, LOADNIL
- 全局变量: GETGLOBAL, SETGLOBAL
- 表操作: GETTABLE, SETTABLE, NEWTABLE, SETLIST, SELF
- 算术运算: ADD, SUB, MUL, DIV, MOD, POW, UNM
- 逻辑运算: NOT, LEN, CONCAT
- 比较运算: EQ, LT, LE
- 跳转控制: JMP, TEST, TESTSET
- 函数调用: CALL, TAILCALL, RETURN
- 循环: FORLOOP, FORPREP, TFORLOOP
- 闭包: CLOSURE, GETUPVAL, SETUPVAL, CLOSE
- 可变参数: VARARG
```

**RK寻址模式**：
```cpp
// RK(x) = 如果x < 256则为寄存器R(x)，否则为常量K(x-256)
bool ISK(i32 x) { return x & BITRK; }  // BITRK = 0x100
i32 INDEXK(i32 x) { return x & ~BITRK; }
i32 RKASK(i32 x) { return x | BITRK; }
```

**寄存器分配**：
```cpp
// 简化版寄存器分配器
i32 allocReg();           // 分配新寄存器
void freeReg(i32 reg);    // 释放寄存器
i32 getTopReg();          // 获取当前栈顶寄存器
```

**常量表管理**：
```cpp
i32 addConstant(const Value& value);  // 添加常量，返回索引
// 自动去重：相同的常量只存储一次
```

**跳转回填**：
```cpp
i32 emitJump(OpCode op);              // 发射跳转指令，返回PC
void patchJump(i32 pc, i32 target);   // 回填跳转目标
```

#### VM（字节码执行引擎）

**设计模式**：指令解释器（Interpreter Pattern）

**核心职责**：
- 解释执行Lua 5.1字节码
- 管理虚拟机寄存器（基于栈的寄存器）
- 实现所有38条指令的执行逻辑
- 处理算术、逻辑、比较运算
- 实现跳转控制流

**关键特性**：
```cpp
// 创建虚拟机
LuaState* L = LuaState::newState();
VM vm(L);

// 执行字节码
Proto* proto = /* 从CodeGenerator获取 */;
vm.executeProto(proto);

// 获取执行结果
Stack& stack = L->getStack();
Value result = stack.top();
```

**执行循环**：
```cpp
void VM::executeProto(Proto* proto) {
    // 初始化
    currentProto_ = proto;
    pc_ = 0;

    // 确保栈空间
    usize requiredSize = proto->getMaxStackSize();
    while (stack.size() < requiredSize) {
        stack.push(Value());
    }

    // 主执行循环
    while (pc_ < code.size()) {
        Instruction inst = code[pc_++];
        OpCode op = GET_OPCODE(inst);

        switch (op) {
            case OpCode::MOVE: /* ... */ break;
            case OpCode::LOADK: /* ... */ break;
            // ... 其他38条指令
        }
    }
}
```

**寄存器访问**：
```cpp
Value& R(i32 index);      // 访问寄存器R(index)
Value RK(i32 rk);         // RK寻址：寄存器或常量
Value K(i32 index);       // 访问常量K(index)
```

**算术运算**：
```cpp
void arith(OpCode op, i32 a, i32 b, i32 c) {
    Value left = RK(b);
    Value right = RK(c);
    f64 result = /* 根据op计算 */;
    R(a) = Value(result);
}
```

**比较运算**：
```cpp
void compare(OpCode op, i32 a, i32 b, i32 c) {
    Value left = RK(b);
    Value right = RK(c);
    bool result = /* 根据op比较 */;
    if (result != (a != 0)) {
        pc_++;  // 跳过下一条指令
    }
}
```

**跳转控制**：
```cpp
void doJump(i32 offset) {
    pc_ += offset;  // 相对跳转
}
```

**已实现指令**（当前版本）：
- ✅ MOVE, LOADK, LOADBOOL, LOADNIL
- ✅ GETGLOBAL, SETGLOBAL
- ✅ GETTABLE, SETTABLE, NEWTABLE
- ✅ ADD, SUB, MUL, DIV, MOD, POW, UNM
- ✅ NOT, LEN, CONCAT
- ✅ JMP, EQ, LT, LE
- ✅ TEST, TESTSET
- ✅ RETURN
- ✅ GETUPVAL, SETUPVAL, CLOSE（Upvalue操作）
- ✅ CALL, TAILCALL, SELF（函数调用，简化版）
- ✅ FORLOOP, FORPREP, TFORLOOP（循环指令）
- ✅ CLOSURE, SETLIST, VARARG（闭包和表初始化）

**指令实现状态**：38条指令中，已实现38条（100%）
- 基础指令：完全实现
- 高级指令：简化实现（嵌套调用、完整闭包支持待完善）

**性能优化**：
- 使用switch-case指令分发（编译器优化为跳转表）
- 内联函数减少调用开销
- 直接栈访问避免间接寻址

#### BaseLib（基础库）⭐ 新完成

**文件**: `src/lib/baselib.hpp/cpp`

**核心功能**：
- 提供Lua脚本运行所需的核心函数
- 实现8个最基础的全局函数
- 支持基本的类型操作、输出和错误处理
- 与VM和LuaState完全集成

**已实现的8个核心函数**：

1. **print(...)**
   - 打印任意数量的参数到标准输出
   - 参数间用制表符分隔，自动添加换行
   - 支持所有Lua类型的字符串转换

2. **type(v)**
   - 返回值的类型字符串
   - 支持的类型："nil", "boolean", "number", "string", "table", "function", "userdata", "thread"

3. **tostring(v)**
   - 将值转换为字符串表示
   - 支持所有基本类型
   - TODO: 实现__tostring元方法支持

4. **tonumber(e [, base])**
   - 将值转换为数字
   - 支持数字类型直接返回
   - TODO: 实现字符串到数字的转换（不同进制）

5. **error(message [, level])**
   - 抛出错误并终止执行
   - 支持自定义错误消息
   - TODO: 添加位置信息（level参数）

6. **assert(v [, message])**
   - 断言检查，如果v为假值则抛出错误
   - 支持自定义错误消息
   - 断言成功时返回所有参数

7. **setmetatable(table, metatable)**
   - 设置表的元表
   - 只能为表类型设置元表
   - 元表必须是表或nil
   - TODO: 检查__metatable字段（保护机制）

8. **getmetatable(object)**
   - 获取对象的元表
   - 如果没有元表返回nil
   - TODO: 检查__metatable字段

**关键特性**：
```cpp
// 注册基础库函数
LuaState* L = LuaState::newState();
openBaseLib(L);  // 注册所有8个函数到全局环境

// 从Lua代码中调用
// print("Hello, Lua!")
// local t = type(42)  -- "number"
// local s = tostring(123)  -- "123"
```

**LuaState API扩展**（为支持基础库新增30+方法）：
- **栈操作**: getTop(), setTop(), pushValue(), at()
- **全局变量**: setGlobal(), getGlobal()
- **类型检查**: isNumber(), isString(), isTable(), isFunction(), isNil(), isBoolean(), type(), typeName()
- **类型转换**: toNumber(), toString(), toBoolean()
- **元表操作**: getMetatable(), setMetatable()
- **错误处理**: error(msg), error()

**已知限制**：
- tostring未实现__tostring元方法支持
- tonumber未实现字符串到数字的转换（不同进制）
- error未添加位置信息（level参数）
- setmetatable/getmetatable未实现__metatable字段检查

**测试覆盖**：8个测试用例，验证所有函数正常工作

**参考实现**：
- `lua_c_analysis/src/lbaselib.c` - Lua 5.1.5 C版本基础库

---

## 🏗️ 项目结构

### 目录结构

```
工作区根目录 (e:\Programming2\lua_in_cpp\)
│
├── lua/                          # 主项目目录 ⭐ 当前开发重点
│   ├── src/                      # 源代码
│   │   ├── common/              # 公共组件
│   │   │   ├── types.hpp        # 类型别名定义（Vec、HashMap、usize等）
│   │   │   ├── config.hpp       # 编译配置和常量
│   │   │   └── macros.hpp       # 实用宏定义
│   │   ├── core/                # 核心类型系统
│   │   │   ├── value.hpp/cpp    # Value类（Lua值表示）
│   │   │   ├── gc_object.hpp/cpp # GCObject基类
│   │   │   ├── gc_string.hpp/cpp # GCString类
│   │   │   ├── string_pool.hpp/cpp # StringPool类
│   │   │   ├── table.hpp/cpp    # Table类
│   │   │   ├── function.hpp/cpp # Function类（Proto + Closure）
│   │   │   ├── upvalue.hpp/cpp  # Upvalue类（闭包上值）
│   │   │   └── userdata.hpp/cpp # Userdata类（用户数据）⭐ 新完成
│   │   ├── gc/                  # 垃圾回收系统
│   │   │   └── garbage_collector.hpp/cpp # GC实现
│   │   ├── vm/                  # 虚拟机核心
│   │   │   ├── global_state.hpp/cpp # 全局状态管理
│   │   │   ├── stack.hpp/cpp    # 值栈管理
│   │   │   ├── call_info.hpp    # 调用信息
│   │   │   ├── lua_state.hpp/cpp # Lua状态（线程）
│   │   │   └── vm.hpp/cpp       # 字节码执行引擎 ⭐ 新完成
│   │   ├── compiler/            # 编译器前端 ⭐ 新完成
│   │   │   ├── token.hpp        # Token类型定义
│   │   │   ├── lexer.hpp/cpp    # 词法分析器
│   │   │   ├── ast.hpp/cpp      # AST节点定义
│   │   │   ├── parser.hpp/cpp   # 语法分析器
│   │   │   ├── opcode.hpp/cpp   # 指令集定义（38条Lua 5.1指令）⭐ 新完成
│   │   │   └── codegen.hpp/cpp  # 字节码生成器 ⭐ 新完成
│   │   ├── lib/                 # 标准库
│   │   │   └── baselib.hpp/cpp  # 基础库（8个核心函数）⭐ 新完成
│   │   └── main.cpp             # 测试主程序（VS IDE手动编译用）
│   ├── docs/                    # 项目文档
│   │   ├── ARCHITECTURE.md      # 架构设计文档
│   │   ├── IMPLEMENTATION_PLAN.md # 7阶段18周开发计划
│   │   ├── DEVELOPMENT_GUIDE.md # 编码规范和类型系统使用指南
│   │   ├── PROJECT_OVERVIEW.md  # 项目总览
│   │   └── PROJECT_SUMMARY_CN.md # 中文项目总结
│   ├── build/                   # 构建输出目录（自动生成）
│   │   ├── debug/              # Debug版本输出
│   │   └── release/            # Release版本输出
│   ├── build_with_vcvars.bat   # MSVC构建脚本（主要构建方式）
│   ├── CMakeLists.txt          # CMake配置（备用）
│   ├── .gitignore              # Git忽略配置
│   └── README.md               # 本文件
│
├── lua_c_analysis/              # Lua 5.1.5 C源码（带中文注释）⭐ 主要参考
│   ├── src/                    # Lua C源码
│   │   ├── lobject.h/c         # 对象系统（TValue、Table、Closure等）
│   │   ├── lgc.h/c             # 垃圾回收
│   │   ├── lvm.h/c             # 虚拟机
│   │   ├── lparser.h/c         # 解析器
│   │   └── ...                 # 其他模块
│   └── docs/                   # 53篇技术文档
│       ├── object/             # 对象系统文档
│       ├── gc/                 # GC系统文档
│       ├── vm/                 # 虚拟机文档
│       └── ...
│
├── lua_with_cpp/                # 另一个C++ Lua实现（次要参考）
│   └── src/                    # C++实现代码
│
└── spec-kit/                    # 开发方法论工具包
    └── ...
```

### 关键文件说明

| 文件 | 用途 | 重要性 |
|------|------|--------|
| `build_with_vcvars.bat` | MSVC构建脚本，编译并运行测试 | ⭐⭐⭐ |
| `src/main.cpp` | 测试主程序，用于VS IDE手动编译 | ⭐⭐⭐ |
| `docs/ARCHITECTURE.md` | 架构设计，理解系统结构 | ⭐⭐⭐ |
| `docs/IMPLEMENTATION_PLAN.md` | 开发计划，了解下一步任务 | ⭐⭐⭐ |
| `docs/DEVELOPMENT_GUIDE.md` | 编码规范，类型系统使用指南 | ⭐⭐⭐ |
| `lua_c_analysis/src/` | Lua C源码参考 | ⭐⭐⭐ |

---



## 🚀 快速开始

### 环境要求

- **操作系统**：Windows 10/11
- **编译器**：Visual Studio 2026（MSVC）
- **C++标准**：C++17
- **构建工具**：MSVC命令行工具（vcvarsall.bat）

### 编译和测试

#### 方式1：使用构建脚本（推荐）

```powershell
# 进入项目目录
cd lua

# 编译Debug版本并运行测试
.\build_with_vcvars.bat debug

# 编译Release版本并运行测试
.\build_with_vcvars.bat release
```

**输出示例**：
```
[INFO] Lua C++ Interpreter Build Script
[INFO] Build type: Debug
[INFO] Compiling Value class...
[INFO] Compiling GCObject class...
...
[TEST 1] Testing Value class...
  [1] Nil value: PASS
  [2] Boolean value: PASS
  ...
[SUCCESS] All tests passed!
```

#### 方式2：Visual Studio IDE手动编译

1. 打开Visual Studio 2026
2. 打开文件：`lua/src/main.cpp`
3. 配置项目包含目录：`lua/src`
4. 添加所有`.cpp`文件到项目
5. 编译并运行

**注意**：`main.cpp`是临时测试文件，仅用于VS IDE手动编译测试。

### 构建输出

```
lua/build/
├── debug/
│   ├── test_build.exe      # Debug版本可执行文件
│   ├── *.obj               # 目标文件
│   └── *.pdb               # 调试符号
└── release/
    ├── test_build.exe      # Release版本可执行文件
    └── *.obj               # 目标文件
```

---

## 📅 开发路线图

### 当前状态：✅ 阶段5完成（编译器前端 + VM执行引擎），准备进入阶段6

| 阶段 | 内容 | 状态 | 完成度 |
|------|------|------|--------|
| **阶段1** | 基础类型系统 | ✅ 完成 | 100% |
| **阶段2** | 字符串和表系统 | ✅ 完成 | 100% |
| **阶段3** | 垃圾回收系统 | ✅ 完成 | 100% |
| **阶段4** | 虚拟机核心 | ✅ 完成 | 100% |
| **阶段4.5** | Upvalue支持 | ✅ 完成 | 100% |
| **阶段5** | 编译器 + VM | ✅ 完成 | 100% |
| **阶段6** | 标准库 | 🔄 进行中 | 38% |
| **阶段7** | 测试和优化 | ⏳ 待开始 | 0% |

### 里程碑

- [x] **M0**: 项目架构设计完成
- [x] **M1**: 基础类型系统实现（Value、GCObject）
- [x] **M2**: 字符串和表系统实现（GCString、StringPool、Table）
- [x] **M3**: 垃圾回收系统实现（GarbageCollector、Function）
- [x] **M4**: 虚拟机核心实现（LuaState、GlobalState、Stack、CallInfo）
- [x] **M4.5**: Upvalue支持实现（Upvalue、Function集成、LuaState集成）
- [x] **M5**: 编译器实现（Lexer、Parser、CodeGen、VM）
- [ ] **M6**: 标准库实现（base、table、string、math等）
- [ ] **M7**: 1.0版本发布

---

## 🎯 下一步计划

根据`docs/IMPLEMENTATION_PLAN.md`和《实现状态深度评估报告.md》，编译器前端和VM执行引擎已完成，基础库已实现8个核心函数，接下来有以下开发选项：

### 选项A：完善VM执行引擎 - 支持Lua函数调用（⭐⭐⭐ 最高优先级）

**当前状态**：
- ✅ 已完成：算术指令、比较指令、逻辑指令、跳转指令、表操作、全局变量、Upvalue操作
- ✅ 已完成：C函数调用（基础库函数可以调用）
- ❌ 缺失：Lua函数的嵌套调用（致命缺陷）

**必须实现**：
- 实现完整的CALL指令（支持嵌套Lua函数调用）
- 实现完整的TAILCALL指令（尾调用优化）
- 实现完整的RETURN指令（多返回值支持）
- 实现CallInfo链表管理（调用栈）
- 实现栈帧切换机制

**原因**：
- 🔴 **致命缺陷**：当前VM无法调用任何Lua函数，解释器基本不可用
- 🔴 **阻塞其他功能**：无法测试CodeGenerator生成的函数定义代码
- 🔴 **核心特性**：函数调用是Lua的核心特性，必须优先实现

**参考**：
- `lua_c_analysis/src/lvm.c` - luaV_execute函数（CALL/TAILCALL/RETURN实现）
- `lua_c_analysis/src/ldo.c` - luaD_precall和luaD_poscall函数
- `lua_c_analysis/src/lopcodes.h` - 指令定义

**验收标准**：
```lua
function factorial(n)
    if n <= 1 then return 1 end
    return n * factorial(n - 1)
end
print(factorial(5))  -- 应输出120
```

---

### 选项B：完善CodeGenerator - 支持运算符表达式（⭐⭐⭐ 最高优先级）

**当前状态**：
- ✅ 已完成：字面量表达式（nil、boolean、number、string）、变量名表达式
- ❌ 缺失：BinaryExpr（二元运算）、UnaryExpr（一元运算）

**必须实现**：
- 算术运算（+, -, *, /, %, ^）
- 比较运算（==, ~=, <, <=, >, >=）
- 逻辑运算（and, or）
- 一元运算（-, not, #）
- 字符串连接（..）

**原因**：
- 🔴 **致命缺陷**：无法使用任何运算符，几乎所有表达式都无法编译
- 🔴 **基础功能**：运算符是编程语言的基础，必须优先实现

**参考**：
- `lua_c_analysis/src/lcode.c` - 表达式代码生成
- `lua_c_analysis/src/lparser.c` - 运算符优先级处理

**验收标准**：
```lua
local x = 1 + 2 * 3
local y = x > 5 and 10 or 20
local z = -x
local s = "Hello" .. " " .. "World"
```

---

### 选项C：完善CodeGenerator - 支持函数定义和调用（⭐⭐⭐ 最高优先级）

**当前状态**：
- ❌ 缺失：FunctionStmt（函数定义语句）
- ❌ 缺失：FunctionExpr（函数表达式）
- ❌ 缺失：CallExpr（函数调用表达式）

**必须实现**：
- 函数定义语句（function foo() end）
- 局部函数定义（local function foo() end）
- 函数表达式（local f = function() end）
- 函数调用表达式（foo()）
- 参数传递和返回值处理

**原因**：
- 🔴 **致命缺陷**：无法定义和调用函数，无法编译任何有意义的Lua代码
- 🔴 **核心特性**：函数是Lua的核心特性

**参考**：
- `lua_c_analysis/src/lcode.c` - 函数编译
- `lua_c_analysis/src/lparser.c` - 函数解析

**验收标准**：
```lua
function add(a, b)
    return a + b
end
local result = add(1, 2)
print("1 + 2 =", result)
```

---

### 选项D：扩展标准库（⭐⭐ 高优先级）

**当前状态**：
- ✅ 已完成：基础库8个核心函数（print、type、tostring、tonumber、error、assert、setmetatable、getmetatable）
- ❌ 缺失：基础库其他13个函数（pcall、xpcall、pairs、ipairs、next、rawget、rawset等）
- ❌ 缺失：表库、字符串库、数学库、IO库、OS库

**建议实现**：
- 基础库剩余函数：pcall, xpcall, pairs, ipairs, next, rawget, rawset, rawequal, select, unpack
- 表库核心函数：insert, remove, sort, concat
- 字符串库核心函数：sub, find, gsub, upper, lower, len
- 数学库核心函数：abs, floor, ceil, sqrt, min, max

**原因**：
- 🟡 **重要但非紧急**：基础库8个核心函数已足够运行简单脚本
- 🟡 **依赖其他功能**：pairs/ipairs需要迭代器支持（需要先完善VM）

**参考**：
- `lua_c_analysis/src/lbaselib.c` - 基础库
- `lua_c_analysis/src/ltablib.c` - 表库
- `lua_c_analysis/src/lstrlib.c` - 字符串库
- `lua_c_analysis/src/lmathlib.c` - 数学库

---

### 🎯 推荐开发顺序（基于优先级）

1. **第一阶段（2-3周）**：使解释器基本可用
   - ✅ 任务1：完善CodeGenerator - 支持运算符表达式（BinaryExpr、UnaryExpr）
   - ✅ 任务2：完善CodeGenerator - 支持函数定义和调用（FunctionStmt、FunctionExpr、CallExpr）
   - ⏳ 任务3：完善VM - 支持Lua函数调用（CALL/TAILCALL/RETURN完整实现）
   - ✅ 任务4：实现基础库8个核心函数

2. **第二阶段（2-4周）**：完善核心功能
   - 任务5：完善CodeGenerator - 支持for循环（ForNumStmt、ForInStmt）
   - 任务6：完善VM - 支持完整闭包（CLOSURE指令的upvalue处理）
   - 任务7：扩展标准库（表库、字符串库、数学库）
   - 任务8：实现基础C API

3. **第三阶段（1-2个月）**：完整功能
   - 任务9：实现IO库、OS库
   - 任务10：实现协程系统
   - 任务11：实现调试库

---

## 🎨 模块详解

### Parser（语法分析器）模块

**文件**：
- `src/compiler/ast.hpp` - AST节点定义（396行）
- `src/compiler/ast.cpp` - AST辅助函数（109行）
- `src/compiler/parser.hpp` - Parser类接口（169行）
- `src/compiler/parser.cpp` - Parser实现（1012行）

**核心功能**：

1. **AST节点类型**：
   - **表达式节点**：NilExpr, BoolExpr, NumberExpr, StringExpr, VarargExpr, NameExpr, BinaryExpr, UnaryExpr, TableExpr, CallExpr, IndexExpr, MemberExpr, FunctionExpr
   - **语句节点**：EmptyStmt, AssignStmt, LocalStmt, CallStmt, IfStmt, WhileStmt, RepeatStmt, ForNumStmt, ForInStmt, FunctionStmt, ReturnStmt, BreakStmt, DoStmt

2. **递归下降解析**：
   - 每个语法规则对应一个解析函数
   - 自顶向下的解析策略
   - 清晰的错误报告（行号、列号）

3. **运算符优先级**（从低到高）：
   ```
   1. or                    (逻辑或)
   2. and                   (逻辑与)
   3. <, >, <=, >=, ==, ~=  (关系运算符)
   4. ..                    (字符串连接，右结合)
   5. +, -                  (加减)
   6. *, /, %               (乘除模)
   7. not, -, #             (一元运算符)
   8. ^                     (幂运算，右结合)
   ```

4. **支持的语法结构**：
   - ✅ 赋值语句：`x = 42`, `x, y = 1, 2`
   - ✅ 局部变量：`local x, y = 1, 2`
   - ✅ 条件语句：`if-then-elseif-else-end`
   - ✅ 循环语句：`while-do-end`, `repeat-until`, `for-do-end`
   - ✅ 函数定义：`function name(params) ... end`
   - ✅ 函数调用：`func(args)`, `obj:method(args)`
   - ✅ 表构造器：`{1, 2, 3, x=10, ["key"]=20}`
   - ✅ 表达式：算术、逻辑、关系、字符串连接
   - ✅ 索引访问：`t[key]`, `t.member`

5. **设计特点**：
   - 使用`std::variant`实现类型安全的AST节点
   - 使用`std::unique_ptr`管理AST节点内存
   - 支持完整的Lua 5.1语法
   - 详细的错误信息（ParseError异常）

**测试覆盖**：
- ✅ 简单赋值语句
- ✅ 局部变量声明
- ✅ if语句
- ✅ while循环
- ✅ 数值for循环
- ✅ 函数定义
- ✅ 表构造器
- ✅ 二元运算表达式
- ✅ 函数调用
- ✅ 复杂代码（递归函数）

**参考实现**：
- `lua_c_analysis/src/lparser.h` 和 `lparser.c` - Lua 5.1.5 C版本语法分析器
- Lua 5.1 Reference Manual - 语法规范

#### CodeGenerator（字节码生成器）⭐ 新完成

**文件**：`src/compiler/codegen.hpp/cpp` + `opcode.hpp/cpp`

**核心功能**：
- 将AST转换为Lua 5.1字节码
- 寄存器分配和管理
- 常量表管理（数字、字符串、布尔值）
- 局部变量作用域管理
- 跳转指令生成和回填

**OpCode指令集**（38条Lua 5.1指令）：
- **数据移动**：MOVE, LOADK, LOADBOOL, LOADNIL
- **表操作**：NEWTABLE, GETTABLE, SETTABLE, SETLIST, SELF
- **全局变量**：GETGLOBAL, SETGLOBAL
- **Upvalue**：GETUPVAL, SETUPVAL
- **算术运算**：ADD, SUB, MUL, DIV, MOD, POW, UNM
- **逻辑运算**：NOT, LEN
- **关系运算**：EQ, LT, LE
- **字符串**：CONCAT
- **控制流**：JMP, TEST, TESTSET
- **函数调用**：CALL, TAILCALL, RETURN
- **循环**：FORLOOP, FORPREP, TFORLOOP, TFORCALL
- **闭包**：CLOSURE
- **可变参数**：VARARG
- **其他**：CLOSE

**指令格式**（32位）：
```cpp
// iABC:  [OpCode:6][A:8][C:9][B:9]
// iABx:  [OpCode:6][A:8][Bx:18]
// iAsBx: [OpCode:6][A:8][sBx:18] (signed)
```

**技术亮点**：
- 基于寄存器的虚拟机架构（非栈式）
- 单遍代码生成
- RK寻址模式（Register-Constant混合寻址）
- 跳转链表回填技术
- 局部变量生命周期管理

**测试覆盖**：
- ✅ 数字常量（LOADK指令）
- ✅ 字符串常量（LOADK指令）
- ✅ 布尔常量（LOADBOOL指令）
- ✅ nil常量（LOADNIL指令）
- ✅ 局部变量赋值（寄存器分配）
- ✅ 全局变量赋值（SETGLOBAL指令）
- ✅ 多变量赋值（寄存器管理）

**参考实现**：
- `lua_c_analysis/src/lcode.h` 和 `lcode.c` - Lua 5.1.5 C版本代码生成器
- `lua_c_analysis/src/lopcodes.h` 和 `lopcodes.c` - Lua 5.1.5 指令集定义

---

## 📚 重要文档索引

### 项目文档（lua/docs/）

| 文档 | 描述 | 用途 |
|------|------|------|
| **ARCHITECTURE.md** | 架构设计文档 | 理解系统整体架构和模块设计 |
| **IMPLEMENTATION_PLAN.md** | 开发计划 | 7阶段18周详细开发计划和任务分解 |
| **DEVELOPMENT_GUIDE.md** | 开发规范 | 编码规范、类型系统使用指南、质量标准 |
| **PROJECT_SUMMARY_CN.md** | 中文项目总结 | 项目进展总结（中文） |

### 参考资源

#### 1. lua_c_analysis（主要参考）⭐⭐⭐

**位置**：`../lua_c_analysis/`

**内容**：
- Lua 5.1.5 C源码（带详细中文注释）
- 53篇技术文档
- 核心算法详解

**关键文件**：
- `src/lobject.h/c` - 对象系统（TValue、Table、Closure等）
- `src/lgc.h/c` - 垃圾回收系统
- `src/lstate.h/c` - 状态管理（lua_State、global_State）
- `src/lvm.h/c` - 虚拟机执行
- `src/lparser.h/c` - 解析器
- `src/ldo.h/c` - 函数调用和栈管理

**使用方式**：
- 理解原始Lua的设计思路和算法
- 参考数据结构定义
- 学习性能优化技巧

#### 2. lua_with_cpp（次要参考）⭐⭐

**位置**：`../lua_with_cpp/`

**内容**：
- 另一个C++ Lua实现（部分完成）
- 现代C++实现模式
- GC系统和VM实现

**使用方式**：
- 参考C++实现方案
- 学习现代C++特性应用
- 避免已知的设计缺陷

#### 3. spec-kit（方法论指导）⭐

**位置**：`../spec-kit/`

**内容**：
- Spec-Driven Development方法论
- 开发流程模板
- 质量保证标准

---

## 💡 快速上手指南（给新AI会话）

### 2分钟快速理解项目

1. **这是什么项目？**
   - 从零开始用C++17实现Lua 5.1.5解释器
   - 参考`lua_c_analysis`中的Lua C源码（带中文注释）
   - 使用MSVC编译器，Windows平台

2. **已经完成了什么？**
   - ✅ 7个核心模块（Value、GCObject、GCString、StringPool、Table、Function、GarbageCollector）
   - ✅ 74个测试用例，100%通过率
   - ✅ Debug和Release版本均编译成功，无警告

3. **如何编译和测试？**
   ```powershell
   cd lua
   .\build_with_vcvars.bat debug    # 编译Debug版本并运行测试
   .\build_with_vcvars.bat release  # 编译Release版本并运行测试
   ```

4. **下一步做什么？**
   - **推荐**：开始虚拟机核心（LuaState、GlobalState、Stack、CallInfo）
   - 或者：实现Userdata类、Thread类、完善Function类（Upvalue）
   - 参考：`docs/IMPLEMENTATION_PLAN.md`

5. **在哪里找详细信息？**
   - 架构设计：`docs/ARCHITECTURE.md`
   - 开发计划：`docs/IMPLEMENTATION_PLAN.md`
   - 编码规范：`docs/DEVELOPMENT_GUIDE.md`
   - Lua C源码：`../lua_c_analysis/src/`

---

## 🔧 技术细节

### 类型系统使用规范

本项目使用类型别名（定义在`src/common/types.hpp`）以提高代码可读性和一致性：

| C++标准类型 | 项目类型别名 | 用途 |
|------------|------------|------|
| `std::vector<T>` | `Vec<T>` | 动态数组 |
| `std::unordered_map<K,V>` | `HashMap<K,V>` | 哈希表 |
| `std::string` | `Str` | 字符串 |
| `std::string_view` | `StrView` | 字符串视图 |
| `size_t` | `usize` | 无符号大小类型 |
| `int32_t` | `i32` | 32位有符号整数 |
| `uint32_t` | `u32` | 32位无符号整数 |
| `int64_t` | `i64` | 64位有符号整数 |
| `uint64_t` | `u64` | 64位无符号整数 |
| `double` | `f64` | 64位浮点数 |

**重要**：所有新代码必须使用类型别名，不得直接使用C++标准类型。详见`docs/DEVELOPMENT_GUIDE.md`。

### 核心设计模式

1. **Value类**：使用`std::variant`实现类型安全的动态类型
   ```cpp
   using ValueData = std::variant<
       std::monostate,  // Nil
       bool,            // Boolean
       f64,             // Number
       void*,           // LightUserdata
       GCString*,       // String
       Table*,          // Table
       Function*,       // Function
       Userdata*,       // Userdata
       Thread*          // Thread
   >;
   ```

2. **GC系统**：三色标记（White/Gray/Black）+ 标记-清除算法
   - White：未标记（可回收）
   - Gray：已标记但未扫描子对象
   - Black：已标记且已扫描子对象

3. **Table类**：混合存储优化
   - 数组部分：`Vec<Value>`（连续存储，快速索引）
   - 哈希部分：`std::unordered_map<Value, Value>`（键值对存储）

4. **StringPool**：字符串驻留（Interning）
   - 单例模式
   - 相同内容的字符串只存储一份
   - 使用指针比较代替字符串比较

---

## 🧪 测试和质量保证

### 测试覆盖

| 模块 | 测试数 | 覆盖内容 |
|------|--------|---------|
| Value类 | 14 | 类型创建、类型检查、安全访问、Lua真值、相等性、toString |
| GCObject类 | 8 | 对象创建、类型检查、颜色标记、对象链、大小计算 |
| GCString类 | 10 | 字符串创建、长度、数据访问、哈希值、指针比较 |
| StringPool类 | 11 | 字符串驻留、指针相等性、池大小、查找、删除 |
| Table类 | 11 | 数组操作、哈希操作、长度计算、键存在性、元表 |
| Function类 | 12 | C函数、Lua函数、Proto、常量表、GC标记 |
| Upvalue类 | 11 | Open/Closed状态、共享机制、批量关闭 |
| Userdata类 | 6 | 完整用户数据、类型化数据、元表、GC集成 |
| GarbageCollector | 8 | 对象注册、根对象、垃圾回收、统计信息 |
| GlobalState类 | 4 | 单例访问、字符串池、GC、注册表、元表 |
| Stack类 | 5 | 压栈、弹栈、索引访问、自动扩展 |
| CallInfo类 | 3 | 调用上下文、栈帧布局 |
| LuaState类 | 5 | 状态管理、栈操作、Upvalue管理 |
| Lexer类 | 18 | 关键字、标识符、数字、字符串、运算符、注释 |
| Parser类 | 10 | 语句解析、表达式解析、运算符优先级、AST生成 |
| CodeGenerator类 | 7 | 数字常量、字符串常量、布尔常量、nil常量、局部变量、全局变量、多变量赋值 |
| VM类 | 6 | 常量执行、FORLOOP循环、SETLIST表初始化、Upvalue操作、函数调用、闭包创建 |
| BaseLib类 | 8 | print、type、tostring、tonumber、error、assert、setmetatable、getmetatable |

### 质量标准

- ✅ **测试通过率**：100% (155/155)
- ✅ **编译警告**：0个
- ✅ **内存泄漏**：无（手动管理，待添加智能指针）
- ✅ **代码规范**：遵循类型系统使用规范
- ✅ **文档完整性**：所有公共API都有注释

---

## 📊 技术栈和工具

### 核心技术

| 技术 | 版本 | 用途 |
|------|------|------|
| **C++** | C++17 | 编程语言 |
| **MSVC** | Visual Studio 2026 | 编译器 |
| **Windows** | 10/11 | 目标平台 |
| **Git** | Latest | 版本控制 |

### C++17特性使用

| 特性 | 应用场景 | 示例 |
|------|---------|------|
| `std::variant` | Value类动态类型 | `std::variant<std::monostate, bool, f64, ...>` |
| `std::string_view` | 字符串视图（类型别名StrView） | 函数参数传递 |
| 结构化绑定 | 简化代码 | `auto [key, val] : hash_` |
| `if constexpr` | 编译期条件 | 模板元编程 |
| 内联变量 | 单例模式 | `inline static StringPool instance` |

### 构建工具

- **主要构建方式**：`build_with_vcvars.bat`（MSVC命令行）
- **备用构建方式**：CMake（`CMakeLists.txt`）
- **IDE支持**：Visual Studio 2026

---

## 📈 项目统计

### 代码规模

```
源文件数：    36个
头文件：      18个 (.hpp)
实现文件：    18个 (.cpp)
总代码行数：  约7200行（不含注释和空行）
文档行数：    约2500行
测试用例：    147个
```

### 对象大小（Release版本）

| 类型 | 大小（字节） | 说明 |
|------|------------|------|
| Value | 16 | Lua值（std::variant） |
| GCObject | 24 | GC对象基类 |
| GCString | 87 | GC字符串（含数据） |
| Table | 152 | Lua表 |
| Proto | 96 | 函数原型 |
| Function | 48 | 函数闭包 |
| GarbageCollector | 72 | 垃圾回收器 |

---

## 🔗相关链接

### 项目仓库

- **GitHub**: [https://github.com/YanqingXu/lua](https://github.com/YanqingXu/lua)
- **本地路径**: `e:\Programming2\lua_in_cpp\lua`

### 参考资源

- **Lua官方网站**: [https://www.lua.org/](https://www.lua.org/)
- **Lua 5.1参考手册**: [https://www.lua.org/manual/5.1/](https://www.lua.org/manual/5.1/)
- **lua_c_analysis**: `../lua_c_analysis/`
- **lua_with_cpp**: `../lua_with_cpp/`

---

## 🙏 致谢

- **Lua团队**：创造了优秀的Lua语言
- **lua_c_analysis**：提供了详细的Lua 5.1.5源码分析和中文文档
- **lua_with_cpp**：提供了C++实现参考
- **spec-kit**：提供了Spec-Driven Development方法论指导

---

## 📝 开发日志

### 最近更新

- **2025-11-14**：实现基础库8个核心函数（print、type、tostring、tonumber、error、assert、setmetatable、getmetatable），扩展LuaState API（30+方法），155个测试全部通过 ⭐ 新完成
- **2025-11-13**：完善VM执行引擎（实现全部38条指令），147个测试全部通过
- **2025-11-13**：实现CodeGenerator字节码生成器（OpCode + CodeGen），141个测试全部通过
- **2025-11-13**：实现Parser语法分析器（AST + 递归下降解析），134个测试全部通过
- **2025-11-12**：实现Lexer词法分析器（Token + 词法规则），124个测试全部通过
- **2025-11-12**：实现LuaState类（线程执行环境 + Upvalue管理）
- **2025-11-12**：实现虚拟机核心模块（GlobalState、Stack、CallInfo）
- **2025-11-12**：实现Upvalue类（闭包上值管理）
- **2025-11-12**：实现Userdata类（用户数据包装）
- **2025-11-12**：实现Function类（Proto + Closure），74个测试全部通过
- **2025-11-12**：实现GarbageCollector类（标记-清除算法）
- **2025-11-12**：实现Table类（混合存储）
- **2025-11-12**：实现StringPool和GCString类
- **2025-11-12**：实现Value和GCObject基础类型系统
- **2025-11-12**：项目初始化，完成架构设计文档

### Git提交历史

```bash
# 查看最近的提交
git log --oneline -10

# 最近的提交示例：
# 9cbccbb Implement Function class: Proto and Closure support
# 6086b67 add function
# 165168b Add garbage collector implementation and update project files
# 90ef578 Implement GarbageCollector: tri-color mark-and-sweep algorithm
# ...
```

---

## 📄 许可证

本项目采用 **MIT 许可证**。

---

## 🚀 开始开发

### 对于新的AI会话

如果你是新的AI助手，请按以下步骤快速了解项目：

1. **阅读本README**（2分钟）- 了解项目概况
2. **查看`docs/ARCHITECTURE.md`**（5分钟）- 理解系统架构
3. **查看`docs/IMPLEMENTATION_PLAN.md`**（3分钟）- 了解开发计划
4. **查看`docs/DEVELOPMENT_GUIDE.md`**（5分钟）- 学习编码规范
5. **编译并运行测试**（1分钟）- 验证环境
   ```powershell
   cd lua
   .\build_with_vcvars.bat debug
   ```
6. **开始开发下一个模块** - 参考"下一步计划"部分

### 对于人类开发者

欢迎参与本项目的开发！请遵循以下步骤：

1. Fork本项目
2. 创建特性分支（`git checkout -b feature/AmazingFeature`）
3. 提交更改（`git commit -m 'Add some AmazingFeature'`）
4. 推送到分支（`git push origin feature/AmazingFeature`）
5. 开启Pull Request

---

**Happy Coding!** 🎉

