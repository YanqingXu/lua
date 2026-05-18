---
status: planned
verified_against: docs/PROJECT_STATUS.md
last_checked: 2026-05-18
applies_to: future architecture blueprint, not current implementation
---

下面直接给你一份可落盘的工程级蓝图初稿。

---

# 《C++20 重写 Lua 的编译器/虚拟机整体架构 v1（工程级蓝图）》

目标：

**在尽量保持 Lua 语言语义与使用体验的前提下，用 C++20 重构其前端、IR、字节码生成器、虚拟机、运行时与工具链，形成一个可维护、可扩展、可调试、可嵌入的现代工程化实现。**

设计原则：

1. **语义兼容优先**：优先对齐 Lua 5.1/5.4 的核心语义。
2. **分层清晰**：Parser / Semantic / IR / Lowering / VM / Runtime 完全解耦。
3. **可观测性强**：编译期和运行期都支持 dump、trace、debug hook。
4. **可扩展性强**：后续可挂 JIT、AOT、静态分析、LSP、优化器。
5. **性能与可维护并重**：不是只追求“快”，而是追求长期演进能力。
6. **C++20 风格**：RAII、variant、span、pmr、coroutine-ready、模块化设计。

---

# 一、总体分层架构

整体建议分成 8 层：

```text
Source Text
   ↓
Lexer
   ↓
Parser
   ↓
AST
   ↓
Semantic / Scope Resolve
   ↓
High IR (HIR)
   ↓
Lowering / Optimize
   ↓
Bytecode IR / VM Instruction
   ↓
VM Execute
   ↓
Runtime / GC / Table / Closure / Coroutine / StdLib
```

再加两条横切系统：

1. **Diagnostics 系统**

   * 词法错误
   * 语法错误
   * 语义错误
   * 运行时错误
   * trace / dump / profile

2. **Tooling 系统**

   * AST dump
   * IR dump
   * bytecode dump
   * VM trace
   * disassembler
   * debugger hooks

---

# 二、推荐目录结构

下面这个目录是偏“工程落地型”的，不是玩具项目结构。

```text
lua20/
├─ CMakeLists.txt
├─ cmake/
├─ docs/
│  ├─ architecture/
│  │  ├─ 01_overview.md
│  │  ├─ 02_parser.md
│  │  ├─ 03_ir.md
│  │  ├─ 04_vm.md
│  │  ├─ 05_runtime.md
│  │  └─ 06_gc.md
│  ├─ bytecode/
│  │  ├─ instruction_set.md
│  │  └─ examples.md
│  └─ roadmap.md
├─ tools/
│  ├─ luac20/             # 编译器驱动
│  ├─ luavm20/            # 虚拟机驱动
│  ├─ luadis20/           # 反汇编工具
│  └─ luadump20/          # AST/IR/Proto dump
├─ tests/
│  ├─ lexer/
│  ├─ parser/
│  ├─ semantic/
│  ├─ ir/
│  ├─ codegen/
│  ├─ vm/
│  ├─ runtime/
│  ├─ gc/
│  └─ compatibility/
├─ benchmarks/
│  ├─ micro/
│  └─ scripts/
├─ examples/
│  ├─ hello.lua
│  ├─ closure.lua
│  ├─ coroutine.lua
│  └─ metatable.lua
├─ include/
│  └─ lua20/
│     ├─ api/
│     │  ├─ state.hpp
│     │  ├─ stack.hpp
│     │  ├─ compile.hpp
│     │  └─ config.hpp
│     ├─ common/
│     │  ├─ source_location.hpp
│     │  ├─ diagnostics.hpp
│     │  ├─ result.hpp
│     │  ├─ assert.hpp
│     │  └─ noncopyable.hpp
│     ├─ frontend/
│     │  ├─ token.hpp
│     │  ├─ lexer.hpp
│     │  ├─ parser.hpp
│     │  ├─ ast.hpp
│     │  ├─ visitor.hpp
│     │  └─ semantic.hpp
│     ├─ ir/
│     │  ├─ hir.hpp
│     │  ├─ hir_builder.hpp
│     │  ├─ hir_pass.hpp
│     │  ├─ bytecode_ir.hpp
│     │  └─ constant_pool.hpp
│     ├─ codegen/
│     │  ├─ lower_to_hir.hpp
│     │  ├─ lower_to_bytecode.hpp
│     │  ├─ register_alloc.hpp
│     │  └─ proto_builder.hpp
│     ├─ vm/
│     │  ├─ opcode.hpp
│     │  ├─ instruction.hpp
│     │  ├─ dispatch.hpp
│     │  ├─ frame.hpp
│     │  ├─ vm.hpp
│     │  └─ debugger.hpp
│     ├─ runtime/
│     │  ├─ value.hpp
│     │  ├─ table.hpp
│     │  ├─ string.hpp
│     │  ├─ function.hpp
│     │  ├─ closure.hpp
│     │  ├─ upvalue.hpp
│     │  ├─ coroutine.hpp
│     │  ├─ metatable.hpp
│     │  ├─ state.hpp
│     │  └─ call.hpp
│     ├─ gc/
│     │  ├─ gc_object.hpp
│     │  ├─ tracer.hpp
│     │  ├─ heap.hpp
│     │  ├─ barrier.hpp
│     │  └─ collector.hpp
│     └─ stdlib/
│        ├─ base_lib.hpp
│        ├─ math_lib.hpp
│        ├─ string_lib.hpp
│        ├─ table_lib.hpp
│        └─ coroutine_lib.hpp
└─ src/
   ├─ api/
   ├─ common/
   ├─ frontend/
   ├─ ir/
   ├─ codegen/
   ├─ vm/
   ├─ runtime/
   ├─ gc/
   └─ stdlib/
```

---

# 三、模块职责划分

---

## 1. common

基础公共设施层。

### 职责

* SourceLocation / Span
* Diagnostics 报错系统
* Result/Expected 风格返回
* Arena / PMR 内存辅助
* 日志、断言、配置

### 关键点

* 所有 AST / IR 节点都带 source range
* 所有错误都统一走 DiagnosticEngine
* 不在 Parser/VM 里散落 printf

---

## 2. frontend

前端层：词法、语法、AST、作用域、语义约束。

### 职责

* Lexer：把源码切成 token
* Parser：构建 AST
* AST：表达语言结构
* ScopeResolver：构建作用域链
* SemanticAnalyzer：检查 break/goto/return/vararg/upvalue 等语义合法性

### 输出

* 完整 AST
* 符号解析信息
* 局部变量 / upvalue / 闭包捕获信息

---

## 3. ir

中间表示层。

建议你不要直接 AST → Bytecode，而是插一个 **HIR（High-level IR）**，这样整个系统会顺很多。

### 职责

* 把 AST 变成结构化 HIR
* 做早期规范化
* 控制流显式化
* 常量池抽象
* Pass 基础框架

### 输出

* HIR Module / HIR Function / HIR BasicBlock
* 便于优化和 Lowering

---

## 4. codegen

代码生成层。

### 职责

* HIR → Bytecode IR
* 寄存器分配
* 常量池布局
* jump patch
* Proto/Chunk 构建

### 输出

* VM 可直接执行的 Prototype / Chunk

---

## 5. vm

解释执行层。

### 职责

* 加载 Prototype
* 管理 CallFrame
* 执行字节码
* 函数调用
* 闭包和 upvalue
* Hook / Debug / Trace

### 特点

* 先做寄存器机 VM
* 后续可加 threaded dispatch / superinstruction / quickening

---

## 6. runtime

运行时对象系统。

### 职责

* Value 表示
* String / Table / Closure / Userdata
* Metatable / metamethod
* Coroutine / ThreadState
* 调用约定
* C API

### 核心要求

* Value 高效
* Table 高效
* 闭包捕获正确
* metamethod 路径清晰

---

## 7. gc

垃圾回收层。

### 职责

* GCObject 基类
* 堆对象注册
* 标记清扫或增量 GC
* 写屏障
* root tracing

### 建议

v1 先做：

* **tri-color mark-sweep**
* 可停顿版 or 简单增量版

不要一开始就做复杂 generational + incremental 混合，否则容易把系统搞炸。

---

## 8. stdlib

标准库层。

### 职责

* base / math / string / table / coroutine
* 对外注册函数
* 用统一 NativeFunction 接口挂入 VM

---

# 四、核心 class 设计

下面给你一版“够工程化”的类设计。

---

## 1. 源码与诊断

```cpp
namespace lua20 {

struct SourcePos {
    uint32_t line;
    uint32_t column;
    uint32_t offset;
};

struct SourceRange {
    SourcePos begin;
    SourcePos end;
};

class SourceFile {
public:
    std::string_view name() const noexcept;
    std::string_view text() const noexcept;
};

enum class DiagnosticLevel {
    Note,
    Warning,
    Error,
    Fatal
};

struct Diagnostic {
    DiagnosticLevel level;
    SourceRange range;
    std::string message;
};

class DiagnosticEngine {
public:
    void report(Diagnostic d);
    bool hasError() const noexcept;
    void printAll() const;
};

}
```

---

## 2. Lexer

```cpp
enum class TokenKind {
    Eof,
    Identifier,
    Number,
    String,
    KwIf, KwThen, KwElse, KwElseIf, KwEnd,
    KwWhile, KwDo, KwFor, KwIn,
    KwFunction, KwLocal, KwReturn, KwBreak,
    KwRepeat, KwUntil,
    KwNil, KwTrue, KwFalse,
    KwAnd, KwOr, KwNot,
    KwGoto, KwContinue, // 如需扩展
    Plus, Minus, Mul, Div, Mod, Pow,
    Assign,
    Eq, Ne, Lt, Le, Gt, Ge,
    LParen, RParen,
    LBrace, RBrace,
    LBracket, RBracket,
    Comma, Dot, Colon, Semi,
    Concat,
    VarArg
};

struct Token {
    TokenKind kind;
    std::string_view lexeme;
    SourceRange range;
};

class Lexer {
public:
    Lexer(const SourceFile&, DiagnosticEngine&);
    Token next();
    Token peek(size_t n = 0);
};
```

---

## 3. AST

建议 AST 使用 `std::variant + 节点 struct`，或者统一基类 + arena 分配都行。

如果你追求访问性能和简洁，**基类指针 AST** 更适合大工程。

### AST 基类

```cpp
class AstNode {
public:
    SourceRange range;
    virtual ~AstNode() = default;
};

class Expr : public AstNode {};
class Stmt : public AstNode {};
```

### 表达式节点

```cpp
class NilExpr : public Expr {};
class BoolExpr : public Expr { bool value; };
class NumberExpr : public Expr { double value; };
class StringExpr : public Expr { std::string value; };
class VarargExpr : public Expr {};

class NameExpr : public Expr {
public:
    std::string name;
};

class UnaryExpr : public Expr {
public:
    TokenKind op;
    Expr* operand;
};

class BinaryExpr : public Expr {
public:
    TokenKind op;
    Expr* lhs;
    Expr* rhs;
};

class TableExpr : public Expr {
public:
    std::vector<class TableField*> fields;
};

class FunctionExpr : public Expr {
public:
    std::vector<std::string> params;
    bool hasVararg;
    std::vector<Stmt*> body;
};

class CallExpr : public Expr {
public:
    Expr* callee;
    std::vector<Expr*> args;
};

class IndexExpr : public Expr {
public:
    Expr* object;
    Expr* index;
};

class MemberExpr : public Expr {
public:
    Expr* object;
    std::string member;
};

class MethodCallExpr : public Expr {
public:
    Expr* object;
    std::string method;
    std::vector<Expr*> args;
};
```

### 语句节点

```cpp
class AssignStmt : public Stmt {
public:
    std::vector<Expr*> lhs;
    std::vector<Expr*> rhs;
};

class LocalStmt : public Stmt {
public:
    std::vector<std::string> names;
    std::vector<Expr*> init;
};

class LocalFunctionStmt : public Stmt {
public:
    std::string name;
    FunctionExpr* func;
};

class FunctionStmt : public Stmt {
public:
    Expr* nameExpr;
    FunctionExpr* func;
};

class ReturnStmt : public Stmt {
public:
    std::vector<Expr*> values;
};

class BreakStmt : public Stmt {};
class DoStmt : public Stmt { public: std::vector<Stmt*> body; };
class WhileStmt : public Stmt { public: Expr* cond; std::vector<Stmt*> body; };
class RepeatStmt : public Stmt { public: std::vector<Stmt*> body; Expr* cond; };

class IfStmt : public Stmt {
public:
    struct Branch {
        Expr* cond; // else 分支可为 nullptr
        std::vector<Stmt*> body;
    };
    std::vector<Branch> branches;
};

class ForNumStmt : public Stmt {
public:
    std::string varName;
    Expr* init;
    Expr* limit;
    Expr* step;
    std::vector<Stmt*> body;
};

class ForInStmt : public Stmt {
public:
    std::vector<std::string> names;
    std::vector<Expr*> exprs;
    std::vector<Stmt*> body;
};

class ExprStmt : public Stmt {
public:
    Expr* expr;
};
```

---

## 4. 作用域与符号系统

```cpp
enum class SymbolKind {
    Local,
    Upvalue,
    Global,
    Parameter
};

struct Symbol {
    std::string name;
    SymbolKind kind;
    uint32_t index;
};

class Scope {
public:
    Scope* parent() const noexcept;
    Symbol* defineLocal(std::string name);
    Symbol* resolve(std::string_view name);
private:
    Scope* parent_ = nullptr;
    std::unordered_map<std::string, Symbol> symbols_;
};

class ScopeResolver {
public:
    void resolve(class AstModule&);
};
```

这里必须额外记录：

* local slot
* captured?
* captured by which child function?
* declared scope / end pc

因为后面生成 debug local info、upvalue mapping 都要用到。

---

## 5. HIR 设计

建议 HIR 是“结构化 CFG + SSA-lite”风格，先别上满血 SSA。

---

### HIR 总体结构

```cpp
class HirModule;
class HirFunction;
class HirBasicBlock;
class HirInst;
class HirValue;
```

### HIR Function

```cpp
class HirFunction {
public:
    uint32_t id;
    std::string name;
    std::vector<std::string> params;
    bool hasVararg = false;

    std::vector<std::unique_ptr<HirBasicBlock>> blocks;
    HirBasicBlock* entry = nullptr;

    uint32_t localCount = 0;
    uint32_t upvalueCount = 0;
};
```

### HIR BasicBlock

```cpp
class HirBasicBlock {
public:
    uint32_t id;
    std::vector<std::unique_ptr<HirInst>> insts;
};
```

### HIR 指令基类

```cpp
enum class HirOpcode {
    ConstNil,
    ConstBool,
    ConstNumber,
    ConstString,

    LoadLocal,
    StoreLocal,
    LoadGlobal,
    StoreGlobal,
    LoadUpvalue,
    StoreUpvalue,

    NewTable,
    GetTable,
    SetTable,

    Move,
    UnaryOp,
    BinaryOp,
    Concat,

    Call,
    TailCall,
    Return,

    Closure,
    CloseUpvalues,

    Jump,
    BranchIf,
    BranchIfNot,

    Phi,          // 可选
    NumericForPrep,
    NumericForLoop,
    GenericForPrep,
    GenericForLoop,
};
```

### HIR 指令示意

```cpp
class HirInst {
public:
    HirOpcode opcode;
    virtual ~HirInst() = default;
};

class HirConstNumberInst : public HirInst {
public:
    double value;
};

class HirBinaryOpInst : public HirInst {
public:
    enum class Op {
        Add, Sub, Mul, Div, Mod, Pow,
        Eq, Ne, Lt, Le, Gt, Ge,
        And, Or
    };
    Op op;
    uint32_t dst;
    uint32_t lhs;
    uint32_t rhs;
};

class HirCallInst : public HirInst {
public:
    uint32_t funcReg;
    std::vector<uint32_t> args;
    uint32_t resultBase;
    uint32_t resultCount; // 0 表示 multret
};

class HirBranchInst : public HirInst {
public:
    uint32_t condReg;
    HirBasicBlock* thenBlock;
    HirBasicBlock* elseBlock;
};

class HirReturnInst : public HirInst {
public:
    std::vector<uint32_t> values;
    bool multRet = false;
};
```

---

## 6. Bytecode / Proto 设计

建议编译产物不是直接一堆 instruction，而是：

```cpp
class Prototype {
public:
    std::string sourceName;
    uint32_t stackSize = 0;
    uint32_t paramCount = 0;
    bool isVararg = false;

    std::vector<Instruction> code;
    std::vector<Constant> constants;
    std::vector<std::unique_ptr<Prototype>> children;
    std::vector<UpvalueDesc> upvalues;
    std::vector<LocalVarInfo> locals;
    std::vector<LineInfo> lineInfo;
};
```

### 常量池

```cpp
using Constant = std::variant<
    std::monostate,   // nil
    bool,
    double,
    std::string
>;
```

### Upvalue 描述

```cpp
struct UpvalueDesc {
    std::string name;
    bool inStack;
    uint32_t index;
};
```

---

## 7. Value 设计

v1 建议先做 16-byte Value，别一上来极限 NaN-boxing。

```cpp
enum class ValueType : uint8_t {
    Nil,
    Boolean,
    Number,
    String,
    Table,
    Function,
    Closure,
    NativeFunction,
    Thread,
    Userdata
};

class GcObject;

class Value {
public:
    ValueType type() const noexcept;

    static Value Nil();
    static Value Boolean(bool);
    static Value Number(double);
    static Value Object(GcObject*, ValueType);

    bool asBoolean() const;
    double asNumber() const;
    template<class T> T* asObject() const;
private:
    ValueType type_;
    union {
        bool b_;
        double n_;
        GcObject* obj_;
    };
};
```

后续稳定后可切 NaN-boxing。

---

## 8. 运行时对象体系

```cpp
enum class GcKind : uint8_t {
    String,
    Table,
    Closure,
    LuaFunction,
    NativeFunction,
    Upvalue,
    Thread,
    Userdata
};

class GcObject {
public:
    GcKind kind;
    bool marked = false;
    GcObject* next = nullptr;
    virtual ~GcObject() = default;
};
```

### String

```cpp
class StringObject : public GcObject {
public:
    std::string value;
    uint64_t hash;
};
```

### Table

```cpp
class TableObject : public GcObject {
public:
    Value get(const Value& key);
    void set(const Value& key, const Value& value);

private:
    // 数组区 + 哈希区
    std::vector<Value> arrayPart_;
    std::unordered_map<Key, Value, KeyHash> hashPart_;

    TableObject* metatable_ = nullptr;
};
```

### Function / Closure

```cpp
class LuaFunctionObject : public GcObject {
public:
    Prototype* proto = nullptr;
};

class NativeFunctionObject : public GcObject {
public:
    using Fn = int(*)(class LuaState*);
    Fn fn = nullptr;
};

class UpvalueObject : public GcObject {
public:
    Value* location = nullptr;  // 打开状态时指向栈
    Value closed;               // 关闭状态时存这里
    bool isClosed = false;
};

class ClosureObject : public GcObject {
public:
    Value callable; // LuaFunctionObject or NativeFunctionObject
    std::vector<UpvalueObject*> upvalues;
};
```

---

## 9. VM 状态与调用帧

```cpp
class CallFrame {
public:
    ClosureObject* closure = nullptr;
    Prototype* proto = nullptr;
    uint32_t pc = 0;

    Value* base = nullptr;
    Value* top = nullptr;

    uint32_t resultBase = 0;
    int expectedResults = 0;
};

class LuaThread : public GcObject {
public:
    std::vector<Value> stack;
    std::vector<CallFrame> frames;

    Value* stackTop = nullptr;

    // 打开的 upvalue 链
    std::vector<UpvalueObject*> openUpvalues;
};

class LuaState {
public:
    LuaThread* mainThread();
    DiagnosticEngine& diagnostics();
    class Vm& vm();
    class Heap& heap();
};
```

---

# 五、编译流程设计

---

## 1. 编译主流程

```text
SourceFile
  → Lexer
  → Parser
  → AST
  → ScopeResolver
  → SemanticAnalyzer
  → AST Lowering
  → HIR
  → HIR Passes
  → Bytecode Lowering
  → Register Allocation
  → Jump Patch
  → Prototype
```

---

## 2. CompilerDriver

```cpp
class CompileOptions {
public:
    bool dumpTokens = false;
    bool dumpAst = false;
    bool dumpHir = false;
    bool dumpBytecode = false;
    bool enableOptimize = false;
};

class CompileResult {
public:
    std::unique_ptr<Prototype> mainProto;
};

class CompilerDriver {
public:
    CompileResult compile(const SourceFile&, const CompileOptions&);
};
```

---

# 六、IR 设计思想

你这里最关键的一点是：

**不要把 Lua AST 的复杂语义直接压到 VM 指令里。**

应先在 HIR 阶段做规范化。

---

## 1. HIR 目标

HIR 主要解决：

1. AST 结构太贴近语法，不适合优化和 codegen
2. Lua 的短路逻辑、多返回值、table constructor、闭包捕获很复杂
3. jump patch、寄存器规划、隐式值流需要先摊平

---

## 2. HIR 层应该做哪些“语义展开”

### 例1：短路逻辑

Lua：

```lua
local x = a and b
```

HIR 展开成：

```text
t0 = load a
if not t0 goto Lfalse
t1 = load b
x  = t1
goto Lend
Lfalse:
x = t0
Lend:
```

这样 VM 层不用理解“and/or 的返回值规则”这种高层语义。

---

### 例2：数值 for

Lua：

```lua
for i = a, b, c do
    body
end
```

HIR：

* prep block
* loop block
* body block
* exit block

并显式化内部控制变量：

* internal_index
* internal_limit
* internal_step

---

### 例3：泛型 for

Lua：

```lua
for k, v in pairs(t) do
    body
end
```

HIR 变成显式 iterator triplet：

* generator
* state
* control

---

### 例4：闭包捕获

Lua：

```lua
local x = 1
return function() return x end
```

HIR 不只表示 “Closure(childFunc)”；
还要明确：

* child function 捕获哪些变量
* 捕获来源是 local 还是 upvalue
* close 时机在哪里

---

# 七、VM 指令设计

建议采用 **寄存器机**，不要栈机。

原因：

1. Lua 原生就是寄存器机思路，适合表达多返回值
2. 对闭包、table、call 指令表达更清晰
3. 后续优化更方便

---

## 1. Instruction 基础格式

你可以先做固定 32-bit 指令。

### 方案 A：Lua 风格 iABC / iABx / iAsBx

```cpp
using Instruction = uint32_t;
```

编码格式：

```text
[ opcode:6 | A:8 | B:9 | C:9 ]      iABC
[ opcode:6 | A:8 | Bx:18 ]          iABx
[ opcode:6 | A:8 | sBx:18 ]         iAsBx
```

优点：

* 成熟
* 紧凑
* 方便移植 Lua 经验

缺点：

* 现代扩展性一般
* 某些复杂操作不够宽

---

### 方案 B：自定义 64-bit 指令

v1 不建议。
先 32-bit，够用了。

---

## 2. Opcode 分层

建议把字节码分成这些大类：

### 常量/移动

* LOADNIL
* LOADBOOL
* LOADK
* MOVE

### 局部/全局/upvalue

* GETGLOBAL
* SETGLOBAL
* GETUPVAL
* SETUPVAL
* GETLOCAL
* SETLOCAL（可省，寄存器本身即 local）

### Table

* NEWTABLE
* GETTABLE
* SETTABLE
* GETFIELD
* SETFIELD
* GETI
* SETI
* SELF

### 算术/比较

* ADD
* SUB
* MUL
* DIV
* MOD
* POW
* UNM
* NOT
* LEN
* CONCAT
* EQ
* LT
* LE

### 控制流

* JMP
* TEST
* TESTSET
* FORPREP
* FORLOOP
* TFORPREP
* TFORCALL
* TFORLOOP

### 函数/闭包

* CALL
* TAILCALL
* RETURN
* CLOSURE
* CLOSEUPVALS
* VARARG

### 元方法/运行时支持

* MM_BINOP
* MM_INDEX
* MM_NEWINDEX
* MM_CALL
* MM_LEN
* MM_CONCAT

v1 里是否单独拆出 MM_* 指令，看你想法：

* 如果希望 VM 更简单：遇到 ADD 时直接内部检查 metamethod
* 如果希望运行时慢路径更清晰：Lowering 时生成通用算术指令 + fallback path

我建议 v1 先走第一种：**指令语义里内建慢路径**。

---

## 3. 指令语义建议表

下面给你一版核心指令集。

---

### 常量与移动

```text
LOADNIL   A              R[A] = nil
LOADBOOL  A B            R[A] = (bool)B
LOADK     A Bx           R[A] = K[Bx]
MOVE      A B            R[A] = R[B]
```

---

### 全局 / upvalue

```text
GETGLOBAL A Bx           R[A] = _ENV[K[Bx]]
SETGLOBAL A Bx           _ENV[K[Bx]] = R[A]

GETUPVAL  A B            R[A] = Up[B]
SETUPVAL  A B            Up[B] = R[A]
```

如要兼容 5.1 `_G` 模式，也可以 main closure 默认挂 global env upvalue。

---

### Table

```text
NEWTABLE  A B C          R[A] = new table(arrayHint=B, hashHint=C)
GETTABLE  A B C          R[A] = R[B][RK(C)]
SETTABLE  A B C          R[A][RK(B)] = RK(C)

GETFIELD  A B C          R[A] = R[B][K[C]]
SETFIELD  A B C          R[A][K[B]] = RK(C)

SELF      A B C          R[A+1] = R[B]; R[A] = R[B][K[C]]
```

`SELF` 对 method call 非常重要。

---

### 算术

```text
ADD       A B C          R[A] = RK(B) + RK(C)
SUB       A B C
MUL       A B C
DIV       A B C
MOD       A B C
POW       A B C
UNM       A B
NOT       A B
LEN       A B
CONCAT    A B C          R[A] = concat(R[B]..R[C])
```

这里 RK 编码保留很有价值。

---

### 比较与测试

```text
EQ        A B C          if ((RK(B) == RK(C)) != A) pc++
LT        A B C          if ((RK(B) <  RK(C)) != A) pc++
LE        A B C          if ((RK(B) <= RK(C)) != A) pc++

TEST      A C            if (!toBoolean(R[A]) == C) pc++
TESTSET   A B C          if (toBoolean(R[B]) == C) R[A] = R[B] else pc++
```

这套对 and/or 的 lowering 很方便。

---

### 跳转

```text
JMP       sBx            pc += sBx
```

可以扩展成：

```text
JMP       A sBx          if (A != 0) close upvalues >= R[A-1]; pc += sBx
```

这很像 Lua 原版，适合 block 退出时关闭 upvalue。

---

### 函数与返回

```text
CALL      A B C
TAILCALL  A B C
RETURN    A B
VARARG    A B
CLOSURE   A Bx
```

#### CALL 约定

* `R[A]` 是函数
* 参数在 `R[A+1]...R[A+B-1]`
* 返回值写回 `R[A]...`
* `B == 0` 表示参数数量是 top - A
* `C == 0` 表示返回数量为 multret

这个约定非常重要，建议直接沿用 Lua 思想。

---

### 循环

```text
FORPREP   A sBx
FORLOOP   A sBx

TFORPREP  A sBx
TFORCALL  A C
TFORLOOP  A sBx
```

也可以更现代一点，把泛型 for 拆得更明确。

---

### 闭包 / upvalue

```text
CLOSURE      A Bx        R[A] = closure(proto[Bx], captured upvalues...)
CLOSEUPVALS  A           close all open upvalues >= R[A]
```

---

# 八、寄存器分配设计

Lua 类 VM 的关键之一就是寄存器布局。

---

## 1. 设计原则

每个函数编译时维护：

* `activeLocals`
* `tempRegs`
* `maxStackSize`

### 建议区分三类寄存器概念：

1. **固定 local slot**
2. **表达式求值临时寄存器**
3. **调用窗口寄存器**

---

## 2. RegisterAllocator

```cpp
class RegisterAllocator {
public:
    uint32_t allocTemp();
    void freeTemp(uint32_t r);

    uint32_t allocLocal(std::string_view name);
    uint32_t localReg(std::string_view name) const;

    uint32_t maxStackSize() const noexcept;
};
```

---

## 3. 编译策略建议

### 局部变量

* 一旦声明，分配固定寄存器
* 生命周期结束时释放

### 临时值

* 表达式求值期间短暂占用
* 表达式结束即可回收

### 调用窗口

调用前确保：

```text
R[A]     = callee
R[A+1...] = args
```

返回后结果也从 `R[A]` 开始覆盖。

这是最自然的调用 ABI。

---

# 九、闭包与 upvalue 设计

这部分是重写 Lua 成败关键。

---

## 1. Upvalue 两态模型

必须支持：

1. **打开状态**：指向栈上的 local
2. **关闭状态**：local 生命周期结束后，把值搬入 upvalue 自己内部

```cpp
class UpvalueObject : public GcObject {
public:
    Value* location;
    Value closed;
    bool isClosed;

    Value get() const;
    void set(const Value&);
    void close();
};
```

---

## 2. openUpvalues 管理

每个 thread 维护按栈位置排序的 openUpvalues。

```cpp
UpvalueObject* captureUpvalue(Value* slot);
void closeUpvalues(Value* from);
```

### 规则

* 捕获同一个 slot 的 closure 共享一个 UpvalueObject
* block 退出 / return / jmp 跨作用域时触发 close

---

## 3. 编译期捕获描述

子函数需要记录：

* 捕获名
* 来源是父函数 local 还是父函数 upvalue
* 对应 index

```cpp
struct UpvalueRef {
    bool inParentStack;
    uint32_t index;
};
```

---

# 十、多返回值与 vararg 设计

Lua 的难点之一。

---

## 1. 多返回值传播规则

Lua 的规则不是“任何地方都展开多返回”。

只在这些位置全展开：

1. return 末尾表达式
2. 实参列表末尾表达式
3. 赋值 RHS 末尾表达式
4. table constructor 最后一个 field（取决于语义）

因此编译器里必须区分：

* **single-result context**
* **multi-result context**

建议定义：

```cpp
enum class ValueMode {
    Single,
    Multi
};
```

表达式生成接口：

```cpp
struct ExprResult {
    uint32_t reg;
    bool isMultiRet;
};

ExprResult emitExpr(Expr* expr, ValueMode mode);
```

---

## 2. VARARG 指令

```text
VARARG A B
```

* `B > 0`：取固定数量
* `B == 0`：取全部剩余参数

---

# 十一、表构造与表访问设计

---

## 1. Table 内部结构

建议直接采用：

* **array part**
* **hash part**

### 原因

Lua 的表本质就是混合表。
如果你只用 unordered_map，数值索引性能会很差。

---

## 2. Table API

```cpp
class TableObject : public GcObject {
public:
    Value rawGet(const Value& key) const;
    void rawSet(const Value& key, const Value& value);

    Value get(const Value& key, LuaState&);
    void set(const Value& key, const Value& value, LuaState&);

    TableObject* metatable() const noexcept;
    void setMetatable(TableObject*);
};
```

### 区分

* rawGet/rawSet：不触发元方法
* get/set：走 __index / __newindex

---

# 十二、元表与元方法设计

元方法不要散落到所有地方，建议做统一 runtime helper。

```cpp
class MetaOps {
public:
    static Value getTable(LuaState&, const Value& obj, const Value& key);
    static void  setTable(LuaState&, const Value& obj, const Value& key, const Value& val);

    static Value add(LuaState&, const Value&, const Value&);
    static Value sub(LuaState&, const Value&, const Value&);
    static bool  eq (LuaState&, const Value&, const Value&);
    static bool  lt (LuaState&, const Value&, const Value&);
    static bool  le (LuaState&, const Value&, const Value&);
    static Value call(LuaState&, const Value& callable, std::span<Value> args);
};
```

这样 VM 指令只做：

```cpp
case OpCode::ADD:
    R[A] = MetaOps::add(L, RK(B), RK(C));
    break;
```

VM 会干净很多。

---

# 十三、VM 执行器设计

---

## 1. Vm 类

```cpp
class Vm {
public:
    Value execute(LuaThread& thread, ClosureObject* entry);

private:
    void runFrame(LuaThread& thread, CallFrame& frame);
    void dispatch(CallFrame& frame);

    void opLoadK(CallFrame&);
    void opCall(CallFrame&);
    void opReturn(CallFrame&);
    // ...
};
```

---

## 2. Dispatch 策略

v1 建议：

1. 先写清晰版 `switch(opcode)`
2. 跑通后再做：

   * computed goto
   * superinstructions
   * inline cache
   * quickening

不要一开始追求“最强 dispatch”，否则调试极痛苦。

---

## 3. 调用流程

### Lua function 调用

* 创建新 frame
* 设置 base/top
* 拷贝参数
* 初始化缺省参数为 nil
* pc = 0

### Native function 调用

* 传入 LuaState*
* 从线程栈/调用窗口读参数
* 返回结果数量

---

# 十四、GC 架构设计

---

## 1. v1 推荐方案

**可停顿 mark-sweep** 或 **简化增量 mark-sweep**

### 原因

你现在的主目标不是炫技，而是先把：

* 闭包
* table
* coroutine
* metamethod
* debug
* bytecode

这些系统整合成功。

---

## 2. Heap 设计

```cpp
class Heap {
public:
    template<class T, class... Args>
    T* allocate(Args&&... args);

    void collect(LuaState&);

private:
    GcObject* allObjects_ = nullptr;
    size_t bytesAllocated_ = 0;
    size_t nextGcThreshold_ = 0;
};
```

---

## 3. Root 枚举

GC root 包括：

* 全局表
* registry
* main thread
* thread stack
* call frames
* open upvalues
* active closures
* stdlib references
* compiler interned strings（若共享）

---

## 4. 写屏障

如果后面做增量 GC，必须预留 barrier 接口：

```cpp
void writeBarrier(GcObject* owner, GcObject* value);
```

v1 可以先接口留着，逻辑先简单实现。

---

# 十五、调试与可观测性设计

这是你项目非常值得做强的部分。

---

## 1. Dump 能力

建议一开始就支持：

* `--dump-token`
* `--dump-ast`
* `--dump-scope`
* `--dump-hir`
* `--dump-bc`
* `--trace-vm`

---

## 2. Disassembler

```cpp
class BytecodeDisassembler {
public:
    void dump(const Prototype&);
};
```

输出形式建议类似：

```text
[0001] LOADK      R0 K1       ; 123
[0002] GETGLOBAL  R1 K2       ; print
[0003] MOVE       R2 R0
[0004] CALL       R1 2 1
[0005] RETURN     R0 1
```

---

## 3. VM Trace Hook

```cpp
class VmDebugger {
public:
    virtual void beforeInstruction(const CallFrame&, Instruction) {}
    virtual void afterInstruction(const CallFrame&, Instruction) {}
    virtual void onCall(const CallFrame&) {}
    virtual void onReturn(const CallFrame&) {}
};
```

这对你后面“清晰输出底层源码执行路径”非常有帮助。

---

# 十六、工程阶段建议

---

## v1 最小闭环

先只做这些：

1. lexer
2. parser
3. AST
4. scope resolve
5. HIR
6. bytecode
7. VM
8. runtime value/table/string
9. closure/upvalue
10. basic stdlib
11. dump/disasm/trace

### v1 支持语法范围

* nil / bool / number / string
* local / assign
* if / while / do / repeat
* function / closure / call / return
* numeric for
* table constructor
* upvalue
* vararg
* and/or/not
* 基本 metamethod

---

## v2 再做

* generic for
* tail call 完整优化
* coroutine yield/resume
* full debug api
* better table layout
* string interning
* bytecode verify
* incremental gc

---

## v3 再做

* inline cache
* quickening
* superinstructions
* IR optimization passes
* JIT hook points
* AOT mode
* language server/tooling

---

# 十七、建议的核心类清单

为了方便你直接开工，我给你一版“第一批必须出现”的核心类。

```text
SourceFile
DiagnosticEngine

Lexer
Parser

AstNode
Expr
Stmt
AstArena

Scope
ScopeResolver
SemanticAnalyzer

HirModule
HirFunction
HirBasicBlock
HirInst
HirBuilder

AstToHirLowerer
HirOptimizer

Instruction
Opcode
ConstantPool
Prototype
BytecodeBuilder
RegisterAllocator
JumpPatcher

Value
GcObject
StringObject
TableObject
LuaFunctionObject
NativeFunctionObject
ClosureObject
UpvalueObject

LuaThread
CallFrame
LuaState
Vm
MetaOps

Heap
Collector
Tracer

BaseLib
MathLib
StringLib
TableLib
CoroutineLib

BytecodeDisassembler
AstPrinter
HirPrinter
VmDebugger
CompilerDriver
```

---

# 十八、一个推荐的数据流示例

以这段 Lua 为例：

```lua
local x = 1
local function f(a)
    return a + x
end
print(f(10))
```

---

## 1. AST

* LocalStmt(x = 1)
* LocalFunctionStmt(f = function(a) return a + x end)
* ExprStmt(print(f(10)))

---

## 2. Scope Resolve

主函数：

* local x → reg0
* local f → reg1

子函数 f：

* param a → reg0
* x → upvalue0

---

## 3. HIR

主函数：

* const 1
* store local x
* create closure f capture x
* store local f
* load global print
* load local f
* const 10
* call f(10)
* call print(...)
* return

子函数：

* load param a
* load upvalue x
* add
* return

---

## 4. Bytecode

主函数类似：

```text
LOADK      R0 K0        ; 1
CLOSURE    R1 P0        ; child proto 0
GETGLOBAL  R2 K1        ; print
MOVE       R3 R1
LOADK      R4 K2        ; 10
CALL       R3 2 2
CALL       R2 2 1
RETURN     R0 1
```

子函数：

```text
GETUPVAL   R1 0
ADD        R2 R0 R1
RETURN     R2 2
```

---

# 十九、架构上的关键决策总结

如果你问我，这个项目最关键的架构决策是什么，我会给你这 8 条：

1. **必须有 AST 和 HIR 两层，不要 AST 直接打字节码**
2. **VM 用寄存器机，不要栈机**
3. **Value v1 不要上 NaN-boxing，先做稳定版 tagged value**
4. **Table 必须 array+hash 双区**
5. **Closure/Upvalue 两态模型必须一开始就设计对**
6. **多返回值上下文必须显式建模**
7. **元方法逻辑要集中在 MetaOps，不要分散**
8. **debug / dump / disasm / trace 必须从 v1 就接入**

---

# 二十、可以直接落盘的 docs 总纲

如果你要把这套蓝图真正放进仓库，我建议先创建这些文档：

```text
docs/architecture/01_overview.md
docs/architecture/02_frontend.md
docs/architecture/03_hir.md
docs/architecture/04_bytecode.md
docs/architecture/05_vm.md
docs/architecture/06_runtime.md
docs/architecture/07_gc.md
docs/architecture/08_debugging.md
docs/architecture/09_roadmap.md
```

---

# 二十一、我给你的开工顺序

建议严格按下面顺序做：

### 第一阶段：前端闭环

1. token / lexer
2. parser
3. ast printer
4. scope resolver
5. semantic analyzer

### 第二阶段：中间层

6. ast → hir lowering
7. hir printer
8. hir basic pass

### 第三阶段：执行闭环

9. opcode / instruction
10. prototype / constant pool
11. hir → bytecode
12. disassembler
13. vm skeleton
14. callframe / stack / value

### 第四阶段：语言特性

15. table
16. closure / upvalue
17. function call / multret / vararg
18. control flow / loops

### 第五阶段：运行时能力

19. metatable / metamethod
20. stdlib
21. gc
22. debugger / hook / trace

---

如果你愿意，我下一步可以继续直接给你补一份：

**《C++20 重写 Lua：opcode.hpp + instruction.hpp + Prototype 设计稿 v1》**

我会直接按这个蓝图，把：

* `enum class OpCode`
* `Instruction 编码/解码`
* `Prototype / Constant / UpvalueDesc`
* `BytecodeBuilder`

整套头文件草案给你写出来。
