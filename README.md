# 现代C++ Lua解释器

> **从零开始用C++17/20/23实现Lua 5.1.5解释器**

[![Tests](https://img.shields.io/badge/tests-986%2F986-brightgreen)]()
[![Coverage](https://img.shields.io/badge/coverage-100%25-brightgreen)]()
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue)]()
[![Platform](https://img.shields.io/badge/platform-Windows-blue)]()
[![Progress](https://img.shields.io/badge/progress-90%25-yellow)]()
[![Code](https://img.shields.io/badge/code-18k%20lines-blue)]()
[![Last Updated](https://img.shields.io/badge/updated-2026--04--10-blue)]()

---

## 🎯 项目概览

本项目是一个**使用现代 C++ 重新实现 Lua 5.1.5 解释器**的实验性工程，当前主要面向学习、验证和逐步补全实现。

### 项目目标

本项目旨在从零开始，用现代 C++ 重建 Lua 5.1.5 的核心执行链路，包括词法分析、语法分析、字节码生成、虚拟机执行、垃圾回收和标准库。

**核心特点**：
-  **技术路线明确**：Windows + Visual Studio 2026 + MSVC + C++17
- ✨ **实现风格现代**：使用 `std::variant`、类型别名、STL 容器等现代 C++ 手段组织 Lua 运行时
- 🎓 **偏学习型工程**：强调结构可读、模块可追踪、便于理解 Lua 语言设计

---

## 📊 当前进度

### 总体状态

- **整体完成度**：约 90%
- **代码规模**：约 85 个源文件，约 18k 行有效代码
- **核心链路**：类型系统、编译器前端、字节码执行引擎已基本成型
- **主要短板**：应用层入口能力仍需补完

### 当前判断

- ✅ **已较稳定的部分**：Value / Table / Function / Lexer / Parser / CodeGen / VM 主体
- ✅ **已补齐的初始化部分**：GlobalState 元方法、保留字、固定字符串初始化
- ✅ **GC / pcall / xpcall**：已全部通过测试，基础行为稳定
- ✅ **标准库状态**：math / os / io / string / table / coroutine / debug 库已全部完成

### 已完成模块（26个核心模块）

| 模块 | 文件 | 代码行数 | 功能描述 | 完成度 |
|------|------|----------|---------|--------|
| **基础类型系统** | `src/common/types.hpp` | 386 | 类型别名定义（Vec、HashMap、usize等） | ✅ 100% |
| **配置系统** | `src/common/config.hpp` | 378 | 编译配置和常量定义 | ✅ 100% |
| **宏定义** | `src/common/macros.hpp` | 377 | 实用宏定义 | ✅ 100% |
| **Value类** | `src/core/value.hpp/cpp` | 490 | Lua值的C++表示（std::variant） | ✅ 100% |
| **GCObject基类** | `src/core/gc_object.hpp/cpp` | 308 | GC对象基类（三色标记） | ✅ 100% |
| **GCString类** | `src/core/gc_string.hpp/cpp` | 251 | GC管理的字符串对象 | ✅ 100% |
| **StringPool类** | `src/core/string_pool.hpp/cpp` | 260 | 字符串驻留池（单例模式） | ✅ 100% |
| **Table类** | `src/core/table.hpp/cpp` | 724 | Lua表（数组+哈希混合存储） | ✅ 95% |
| **Function类** | `src/core/function.hpp/cpp` | 1,096 | 函数对象（Proto + Closure + Upvalue） | ✅ 100% |
| **Upvalue类** | `src/core/upvalue.hpp/cpp` | 419 | 闭包上值管理（Open/Closed状态） | ✅ 100% |
| **Userdata类** | `src/core/userdata.hpp/cpp` | 282 | 用户数据（C++数据包装） | ✅ 100% |
| **Metatable元方法系统** | `src/core/metatable.hpp/cpp` | 697 | 17种元方法支持（算术、比较、索引等） | ✅ 95% |
| **GarbageCollector** | `src/gc/garbage_collector.hpp/cpp` | 535 | 垃圾回收器（标记-清除算法） | ✅ 90% |
| **GlobalState类** | `src/vm/global_state.hpp/cpp` | 261 | 全局状态管理（单例模式） | ✅ 95% |
| **Stack类** | `src/vm/stack.hpp/cpp` | 394 | 值栈管理（动态扩展） | ✅ 100% |
| **CallInfo类** | `src/vm/call_info.hpp` | 197 | 调用信息（函数调用上下文） | ✅ 100% |
| **LuaState类** | `src/vm/lua_state.hpp/cpp` | 1,095 | Lua状态（线程执行环境） | ✅ 95% |
| **Lexer词法分析器** | `src/compiler/lexer.hpp/cpp` + `token.hpp` | 1,167 | 词法分析（Token流生成） | ✅ 100% |
| **Parser语法分析器** | `src/compiler/parser.hpp/cpp` + `ast.hpp/cpp` | 2,031 | 语法分析（AST生成） | ✅ 100% |
| **CodeGenerator字节码生成器** | `src/compiler/codegen.hpp/cpp` + `opcode.hpp/cpp` | 2,249 | 字节码生成（AST→Bytecode） | ✅ 95% |
| **VM字节码执行引擎** | `src/vm/vm.hpp/cpp` | 2,030 | 字节码解释执行（38条指令） | ✅ 95% |
| **I/O系统** | `src/io/*.hpp/cpp` | 670 | InputStream + DynamicBuffer | ✅ 100% |
| **基础库（Base Library）** | `src/lib/baselib.hpp/cpp` | 730 | 26/30函数（print、type、pcall、xpcall、unpack、load等） | 🔄 87% |
| **数学库（Math Library）** | `src/lib/mathlib.hpp/cpp` | 681 | 22/22函数（完整实现） | ✅ 100% |
| **I/O库（I/O Library）** | `src/lib/iolib.hpp/cpp` | 1,111 | 11/11函数 + 7/7文件方法（完整实现） | ✅ 100% |
| **协程库（Coroutine Library）** | `src/lib/coroutinelib.hpp/cpp` | ~210 | 6/6函数（create、resume、yield、status、running、wrap） | ✅ 100% |
| **Thread类** | `src/core/thread.hpp/cpp` | ~300 | 协程执行引擎，独立 LuaState + 栈转移 | ✅ 95% |
| **调试库（Debug Library）** | `src/lib/debuglib.hpp/cpp` | ~1,000 | 10/10函数（getinfo、getlocal、setlocal、getupvalue、setupvalue、traceback、sethook、gethook、getregistry、debug） | ✅ 100% |
| **库管理系统** | `src/lib/lib_manager.hpp/cpp` | 73 | 标准库注册和管理 | ✅ 100% |

### 测试统计（2026-04-10更新）✅

```
测试框架：自定义轻量级测试框架（零外部依赖）
测试套件：28个
  - Core模块：Value（16）、GCString（9）、StringPool（11）、Table（13）、Function（20）
  - VM模块：VM Core（84）、LuaState Init（20）
  - GC模块：GC系统（18）
  - 编译器：Binary/Unary Expressions（54）、Function Codegen（27）、
           Lua File Compilation（5）、Syntax Sugar（71）、
           Table Indexed Access（18）、Method Call（23）、Variable Storage（11）、
           Lexer Number（10）、Lexer Lookahead（27）、
           Parser Recursion（4）、Parser Error（8）、Parser Memory Pool（31）
  - 标准库：Base Library（136）、String Library（68）、
           Table Library（41）、OS Library（23）、Coroutine Library（62）、
           Debug Library（144）
  - 元方法：Metamethod（8）、Complete Metamethods（24）
总测试数：986个单元测试 ✅
通过率：  100% (986/986)
失败测试：0个
编译状态：Debug版本无警告，无链接冲突
平台：    Windows + MSVC (Visual Studio 2026)
```

### 接下来优先做什么

- **补充 base 库缺失函数**：`require` 等尚未实现（`load`、`unpack` 已完成）
- **完善应用入口**：继续打磨 REPL、脚本执行和调试辅助工具

### 核心实现亮点

✅ **Value类**：使用`std::variant`实现类型安全的动态类型系统
✅ **GCObject**：三色标记（White/Gray/Black）支持增量GC
✅ **Table类**：混合存储（数组部分 + 哈希部分），自动优化
✅ **Function类**：支持C函数和Lua函数两种闭包类型，集成Upvalue管理
✅ **Upvalue类**：闭包上值管理，支持Open/Closed状态转换，共享机制
✅ **Userdata类**：完整用户数据支持，8字节对齐，元表支持，GC集成
✅ **Metatable元方法系统**：完整支持17种元方法（__add、__sub、__mul、__div、__mod、__pow、__unm、__eq、__lt、__le、__index、__newindex、__call、__concat、__len、__gc、__mode），包括快速元方法缓存优化
✅ **Lexer词法分析器**：完整Lua 5.1词法规则，支持所有关键字、运算符、字面量、注释
✅ **Parser语法分析器**：递归下降解析，完整AST生成，正确的运算符优先级和结合性
✅ **CodeGenerator字节码生成器**：AST→字节码转换，寄存器分配，常量表管理，跳转回填
✅ **OpCode指令集**：完整Lua 5.1指令集（38条指令），iABC/iABx/iAsBx三种格式
✅ **VM字节码执行引擎**：完整38条指令实现，Upvalue操作，函数调用（C函数），循环指令（FORLOOP/FORPREP/TFORLOOP），闭包创建，表初始化（SETLIST）
✅ **基础库（Base Library）**：26/30函数（print、type、tostring、tonumber、error、assert、pcall、xpcall、pairs、ipairs、next、select、rawget、rawset、rawequal、loadstring、loadfile、dofile、collectgarbage、unpack、load 等），87%完成
✅ **数学库（Math Library）**：22/22函数完整实现（abs, floor, ceil, sqrt, sin, cos, tan, log, exp, random 等），包括数学常量 math.pi 和 math.huge
✅ **I/O库（I/O Library）**：11/11函数 + 7/7文件方法完整实现（io.open, io.close, io.read, io.write, file:read, file:write 等），100%完成
✅ **协程库（Coroutine Library）**：6/6函数实现（coroutine.create、resume、yield、status、running、wrap），支持独立栈协程执行
✅ **调试库（Debug Library）**：10/10函数实现（debug.getinfo、getlocal、setlocal、getupvalue、setupvalue、traceback、sethook、gethook、getregistry、debug），支持完整的运行时调试能力
✅ **库管理系统**：模块化的标准库注册机制，支持全局函数注册和表函数注册
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

#### BaseLib（基础库）

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
openBaseLib(L);  // 注册所有函数到全局环境

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
- `require` 尚未实现（`load`、`unpack` 已完成）

**测试覆盖**：105个测试用例全部通过

---

## 🏗️ 项目结构

### 目录结构

```
├── src/                           # 核心源码
│   ├── common/                    # 基础类型、配置、宏
│   ├── compiler/                  # Lexer / Parser / AST / CodeGen / Bytecode Printer
│   ├── core/                      # Value / Table / Function / String / Metatable 等核心对象
│   ├── gc/                        # 垃圾回收器
│   ├── io/                        # 输入流、动态缓冲区
│   ├── lib/                       # base / math / io / os / string / table / coroutine 等标准库
│   ├── vm/                        # GlobalState / LuaState / Stack / VM
│   ├── main.cpp                   # `lua_app` 入口
│   ├── repl.cpp/.hpp              # REPL 支持
│   └── bytecode_main.cpp          # `lua_bytecode` 入口
├── tests/
│   ├── lua/                       # Lua 脚本级测试样例
│   └── unit/                      # C++ 单元测试
│       ├── compiler/              # 编译器相关测试
│       ├── core/                  # 核心对象测试
│       ├── framework/             # 测试框架自身
│       ├── gc/                    # GC 测试
│       ├── io/                    # I/O 测试
│       ├── metamethod/            # 元方法测试
│       ├── stdlib/                # 标准库测试
│       └── vm/                    # VM / LuaState 测试
├── docs/                          # 项目文档
├── bin/                           # 编译批处理脚本
├── lua.slnx                       # Visual Studio 解决方案
├── lua.vcxproj                    # 核心静态库项目
├── lua_app.vcxproj                # REPL / 临时执行入口项目
├── lua_test.vcxproj               # 单元测试项目
├── lua_bytecode.vcxproj           # 字节码打印与对比工具项目
├── .gitignore
└── README.md
```

### 关键文件说明

| 文件 | 用途 | 重要性 |
|------|------|--------|
| `src/` | 解释器核心实现目录，日常代码修改的主战场 | ⭐⭐⭐ |
| `src/main.cpp` | `lua_app.exe` 的入口文件 | ⭐⭐⭐ |
| `src/bytecode_main.cpp` | `lua_bytecode.exe` 的入口文件 | ⭐⭐⭐ |
| `src/compiler/bytecode_printer.cpp` | 字节码打印工具的核心实现 | ⭐⭐⭐ |
| `tests/unit/` | 单元测试目录，是验证 C++ 模块行为的第一入口 | ⭐⭐⭐ |
| `tests/lua/` | Lua 脚本级样例与回归测试输入，现已按语法/功能分类整理 | ⭐⭐ |
| `docs/ARCHITECTURE.md` | 架构设计说明，适合先建立整体认识 | ⭐⭐⭐ |
| `docs/DEVELOPMENT_GUIDE.md` | 开发规范与类型系统约定 | ⭐⭐⭐ |
| `lua.slnx` | 当前 Visual Studio 解决方案入口 | ⭐⭐⭐ |

### 目录说明补充

- `src/`、`tests/`、`docs/` 是最值得长期关注的三个目录

---



## 🚀 快速开始

### 环境要求

- **操作系统**：Windows 10/11
- **编译器**：Visual Studio 2026（MSVC）
- **C++标准**：C++17

### 建议阅读顺序

1. 先看本文档开头的“必读约定”
2. 再看 `docs/ARCHITECTURE.md`
3. 最后进入 `src/` 和 `tests/unit/` 开始实际开发或排错

---

## 📚 重要文档索引

### 项目文档（lua/docs/）

| 文档 | 描述 | 用途 |
|------|------|------|
| **ARCHITECTURE.md** | 架构设计文档 | 理解系统整体架构和模块设计 |
| **DEVELOPMENT_GUIDE.md** | 开发规范 | 编码规范、类型系统使用指南、质量标准 |
| **PROJECT_OVERVIEW.md** | 项目总览 | 项目概况和技术栈 |
| **PROJECT_SUMMARY_CN.md** | 中文项目总结 | 项目进展总结（中文） |

## 💡 快速上手指南（给新AI会话）

### 2分钟快速理解项目

1. **这是什么项目？**
   - 一个用现代 C++ 重写 Lua 5.1.5 的项目

2. **已经完成了什么？**
   - 编译器前端、VM 主体、GC 框架、核心对象系统已经成型
   - 当前仍有少量失败测试和标准库补完工作
   - 重点不再是“从零搭骨架”，而是“持续补功能、修边界、稳行为”

3. **下一步做什么？**
   - 先看当前失败测试和最近在改的模块
   - 优先补标准库、错误处理和工具链边缘能力

4. **在哪里找详细信息？**
   - 架构设计：`docs/ARCHITECTURE.md`
   - 编码规范：`docs/DEVELOPMENT_GUIDE.md`
   - 项目总览：`docs/PROJECT_OVERVIEW.md`

---

## 🔧 技术细节

### 类型系统使用规范

本项目统一使用 `src/common/types.hpp` 中定义的类型别名，以保持代码风格和语义一致：

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

**重要**：新增代码优先使用这些类型别名，而不是直接散落使用标准库原始类型。详细规范见 `docs/DEVELOPMENT_GUIDE.md`。

### 核心实现约定

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

2. **GC系统**：采用三色标记（White/Gray/Black）+ 标记-清除算法
   - White：未标记（可回收）
   - Gray：已标记但未扫描子对象
   - Black：已标记且已扫描子对象

3. **Table类**：采用数组部分 + 哈希部分的混合存储
   - 数组部分：`Vec<Value>`（连续存储，快速索引）
   - 哈希部分：`std::unordered_map<Value, Value>`（键值对存储）

4. **StringPool**：使用字符串驻留（Interning）
   - 单例模式
   - 相同内容的字符串只存储一份
   - 使用指针比较代替字符串比较

---

## 🧪 测试和质量保证

### 测试框架

本项目使用**自定义轻量级测试框架**。它的目标不是功能最全，而是足够轻、足够直接，便于在解释器这种底层项目里快速定位问题。

**测试框架特性**：
- ✅ 简单的断言宏（`ASSERT_TRUE`、`ASSERT_FALSE`、`ASSERT_EQ`）
- ✅ 测试套件组织（`TestSuite`类）
- ✅ 自动测试注册（`TestRegistry`单例）
- ✅ 清晰的测试报告（通过/失败统计）
- ✅ 零外部依赖（无需Google Test等第三方库）

**测试文件结构**：
```
lua/tests/unit/
├── test_framework.hpp          # 测试框架核心
├── test_registry.hpp           # 测试注册函数声明
├── test_value.cpp              # Value类测试（16个测试）
├── test_gc_string.cpp          # GCString和StringPool测试（20个测试）
├── test_table.cpp              # Table类测试（13个测试）
├── test_vm_core.cpp            # VM核心测试（23个测试）
├── test_function.cpp           # Function和Proto测试（20个测试）
├── test_gc.cpp                 # GC系统测试（18个测试）
├── test_binary_unary_expr.cpp  # 二元/一元表达式测试（10个测试）
├── test_function_codegen.cpp   # 函数代码生成测试（16个测试）
├── test_baselib.cpp            # 基础库测试（6个测试，当前跳过）
├── test_lua_functions.cpp      # Lua文件编译测试（5个测试）
└── test_runner.cpp             # 测试运行器（main函数）
```

## 📊 技术栈和工具

### 核心技术

| 技术 | 版本 | 用途 |
|------|------|------|
| **C++** | C++17 | 主实现语言 |
| **MSVC** | Visual Studio 2026 | 编译器与 IDE 工具链 |
| **Windows** | 10/11 | 当前主要开发平台 |
| **Git** | Latest | 版本管理 |

### C++17特性使用

| 特性 | 应用场景 | 示例 |
|------|---------|------|
| `std::variant` | Value类动态类型 | `std::variant<std::monostate, bool, f64, ...>` |
| `std::string_view` | 字符串视图（类型别名StrView） | 函数参数传递 |
| 结构化绑定 | 简化代码 | `auto [key, val] : hash_` |
| `if constexpr` | 编译期条件 | 模板元编程 |
| 内联变量 | 单例模式 | `inline static StringPool instance` |

## 🔗相关链接

### 项目仓库

- **GitHub**: [https://github.com/YanqingXu/lua](https://github.com/YanqingXu/lua)
- **本地路径**: `e:\Programming2\lua_in_cpp\lua`

### 参考资源

- **Lua官方网站**: [https://www.lua.org/](https://www.lua.org/)
- **Lua 5.1参考手册**: [https://www.lua.org/manual/5.1/](https://www.lua.org/manual/5.1/)

---

## 🙏 致谢

- **Lua团队**：创造了优秀的Lua语言

---

## 📄 许可证

本项目采用 **MIT 许可证**。

---

## 🏗️ 子项目说明（Visual Studio 解决方案）

本仓库包含一个 Visual Studio 解决方案，用于组织核心库、运行入口、测试程序和字节码分析工具。

### 解决方案与构建信息

- **解决方案文件**：`lua.slnx`
- **构建工具**：MSBuild
- **MSBuild路径**：`D:\VS2026\2026\MSBuild\Current\Bin\MSBuild.exe`
- **默认构建配置**：Debug
- **默认目标平台**：x64
- **默认输出目录**：`x64/Debug/`

### 编译批处理脚本（`bin/`）

所有脚本默认执行 `Rebuild`，并固定使用 `Debug|x64`：

| 脚本 | 对应项目 | 产物 |
|------|----------|------|
| `bin/build_lua.bat` | `lua.vcxproj` | `lua.lib`（核心静态库） |
| `bin/build_app.bat` | `lua_app.vcxproj` | `lua_app.exe` |
| `bin/build_test.bat` | `lua_test.vcxproj` | `lua_test.exe` |
| `bin/build_bytecode.bat` | `lua_bytecode.vcxproj` | `lua_bytecode.exe` |

示例：

```bat
cd lua
bin\build_lua.bat
bin\build_app.bat
bin\build_test.bat
bin\build_bytecode.bat
```

### 四个子项目

| 项目文件 | 输出类型 | 说明 |
|---------|---------|------|
| `lua.vcxproj` | 静态库（`lua.lib`） | 核心库，包含 Lexer、Parser、CodeGen、VM、GC 等所有产品源码，供其他子项目链接使用 |
| `lua_app.vcxproj` | 可执行文件（`lua_app.exe`） | 临时交互式执行入口（REPL），`main()` 函数为临时实现，后续可能重构 |
| `lua_test.vcxproj` | 可执行文件（`lua_test.exe`） | 单元测试运行器，覆盖 compiler、core、gc、vm 等各模块的测试用例 |
| `lua_bytecode.vcxproj` | 可执行文件（`lua_bytecode.exe`） | 字节码分析工具，将源码编译后以人类可读格式打印字节码，用于与原生 Lua（官方实现）的字节码进行对比测试 |

### 使用建议

- 开发核心功能时，优先修改并维护 `lua.vcxproj` 对应的库源码
- 需要手动运行解释器流程时，使用 `lua_app.vcxproj`
- 需要验证回归和模块正确性时，使用 `lua_test.vcxproj`
- 需要分析编译结果或对比官方 Lua 字节码时，使用 `lua_bytecode.vcxproj`

### 一句话理解

> `lua.vcxproj` 是核心静态库；`lua_app.vcxproj` 是临时 REPL 入口；`lua_test.vcxproj` 是测试运行器；`lua_bytecode.vcxproj` 是字节码对比分析工具。
