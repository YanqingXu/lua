---
status: historical
verified_against: docs/archive/refactors/refactor-expdesc-pr-checklist.md; src/compiler/codegen_types.hpp; src/compiler/codegen.hpp
last_checked: 2026-05-18
applies_to: completed legacy expression pipeline refactor planning
---

下面直接给你一份工程级版本。

执行清单见：[refactor-expdesc-pr-checklist.md](./refactor-expdesc-pr-checklist.md)

---

# 《移除 expdesc 后，C++20 Lua 编译器会新增哪些核心后端结构》

先说结论：

**移除 `expdesc` 以后，你不是“少了一个结构体”，而是要把它原来混在一起承担的职责，拆成一组更清晰的后端结构。**

Lua 5.1 里 `expdesc` 一口气承担了这些职责：

* 表达式语义类别
* 左值 / 右值区分
* 当前值的落点信息
* 布尔控制流的真假跳转链
* 部分多返回值传播语义
* 部分 parser → codegen 的胶水状态

而在 **C++20 + AST** 架构下，这些应该拆成独立模块。

我先给你总图，再逐个展开。

---

# 一、总览：新增的核心后端结构

移除 `expdesc` 之后，建议至少新增下面这些核心结构：

### 1. 表达式结果结构

* `ValueResult`

### 2. 条件控制流结果结构

* `CondResult`

### 3. 可赋值位置结构

* `LValueRef`

### 4. 跳转回填结构

* `PatchList`

### 5. 作用域 / 符号解析结构

* `SymbolRef`
* `ScopeFrame`
* `ResolvedExprInfo` 或符号绑定表

### 6. 代码生成上下文

* `CodeGenContext`

### 7. 寄存器管理器

* `RegisterAllocator`

### 8. 标签 / 基本块 / 跳转辅助

* `Label`
* `JumpRef`

### 9. 多返回值传播描述

* `CallResultInfo` 或 `MultiRetPolicy`

### 10. 语句级控制流上下文

* `LoopContext`
* `FunctionContext`

### 11. 常量池与闭包元数据管理

* `ConstantPoolBuilder`
* `UpvalueLayout`
* `ProtoBuilder`

---

# 二、推荐的整体分层图

你可以把后端设计成这样：

```cpp
Parser
  -> AST
  -> Resolver / Binder
  -> CodeGenerator
       |- CodeGenContext
       |- RegisterAllocator
       |- ConstantPoolBuilder
       |- ProtoBuilder
       |- emitExpr()   -> ValueResult
       |- emitCond()   -> CondResult
       |- emitLValue() -> LValueRef
       |- emitStmt()
```

也就是：

* AST 负责“表达式是什么”
* Resolver 负责“名字绑定到谁”
* CodeGen 负责“怎么变成 bytecode / IR”

---

# 三、核心结构 1：ValueResult

这是替代 `expdesc` 里“普通右值表达式状态”的第一核心结构。

## 作用

表示：

> “一个表达式作为右值被求值后，它现在以什么形式可被后续指令消费？”

## 推荐定义

```cpp
struct ValueResult {
    enum class Kind {
        Register,   // 值已经在寄存器里
        Constant,   // 值是常量池项
        Immediate,  // 可直接编码的小立即数/布尔/nil（可选）
        MultiRet    // 来自函数调用/vararg 的多返回值
    } kind = Kind::Register;

    int reg = -1;          // kind == Register 时有效
    int constIndex = -1;   // kind == Constant 时有效

    bool isTemp = false;   // 这个寄存器是不是临时寄存器
    bool multiRet = false; // 是否允许延续多返回语义
};
```

## 它解决什么问题

原来 `expdesc` 里这些状态：

* `VK`
* `VKNUM`
* `VNONRELOC`
* `VRELOCABLE`
* `VCALL`
* `VVARARG`

在 AST 后端里，不应该再用一个 enum 全包住，而应该转成更明确的结果类型。

## 为什么必须有它

因为 `emitExpr()` 不能只返回 `int reg`。
否则你会丢失：

* 这个值是不是常量
* 是不是多返回值
* 是不是还没落寄存器
* 后续能不能直接作为 RK 操作数

---

# 四、核心结构 2：CondResult

这是替代 `expdesc.t / expdesc.f` 的核心结构。

## 作用

表示：

> “一个表达式在条件语境下，会产生哪些待回填的真分支和假分支跳转？”

## 推荐定义

```cpp
struct PatchList {
    std::vector<int> jumps;
};

struct CondResult {
    PatchList trueList;
    PatchList falseList;
};
```

也可以稍微增强：

```cpp
struct CondResult {
    PatchList trueList;
    PatchList falseList;
    bool knownConstant = false;
    bool constantValue = false;
};
```

## 为什么它非常关键

Lua 里的布尔表达式不是简单“算一个 bool 值”。

例如：

```lua
if a and b then ... end
if x or y then ... end
if not p then ... end
```

这些表达式更高效的生成方式不是：

1. 先算出 true/false 到寄存器
2. 再根据寄存器跳转

而是直接生成控制流。

所以 `emitCond()` 必须返回 `CondResult`。

## 它替代了 expdesc 的哪部分

* `t`
* `f`
* `VJMP`
* 布尔表达式短路回填机制

---

# 五、核心结构 3：LValueRef

这是替代 `VLOCAL / VUPVAL / VGLOBAL / VINDEXED` 最关键的结构。

## 作用

表示：

> “一个表达式如果被放在赋值号左边，它引用的是哪一种可写位置？”

## 推荐定义

```cpp
struct LValueRef {
    enum class Kind {
        Local,
        Upvalue,
        Global,
        Indexed,
        Field
    } kind;

    int localSlot = -1;      // Local
    int upvalueIndex = -1;   // Upvalue

    int tableReg = -1;       // Indexed / Field
    int keyReg = -1;         // Indexed
    int valueBaseReg = -1;   // 可选：table base

    std::string globalName;  // Global
    std::string fieldName;   // Field
};
```

## 为什么要单独拆出来

因为“表达式求值”和“表达式作为赋值目标”是两套完全不同的后端逻辑。

例如：

```lua
a = 1
t[k] = v
obj.x = y
```

左边不是普通表达式值，而是一个“写入目标”。

如果不引入 `LValueRef`，你最后还是会退回到 `expdesc` 那种“大一统状态机”。

## 推荐接口

```cpp
ValueResult emitExpr(const ast::Expr& expr);
LValueRef emitLValue(const ast::Expr& expr);
void emitStore(const LValueRef& lhs, const ValueResult& rhs);
```

---

# 六、核心结构 4：PatchList

虽然上面在 `CondResult` 里已经用了，但它值得独立讲。

## 作用

表示：

> “这些跳转指令的目标地址还没定，稍后统一回填。”

## 推荐定义

```cpp
struct PatchList {
    std::vector<int> pcs;

    void add(int pc) {
        pcs.push_back(pc);
    }

    bool empty() const {
        return pcs.empty();
    }
};
```

## 配套 API

```cpp
void patchToHere(PatchList& list);
void patchToTarget(PatchList& list, int targetPc);
PatchList mergePatchList(PatchList a, PatchList b);
```

## 它的重要性

移除 `expdesc` 后，**PatchList 反而要更显式**。

因为原来 `t/f` 隐藏在 `expdesc` 里；现在你要明确承认：

* 条件表达式
* break
* goto（如果你以后支持）
* 尾部跳转
* repeat/until

这些全都依赖回填链。

---

# 七、核心结构 5：SymbolRef

这是“作用域解析结果”的基础结构。

## 作用

表示：

> “一个名字解析后，绑定到哪个实体？”

## 推荐定义

```cpp
struct SymbolRef {
    enum class Kind {
        Local,
        Upvalue,
        Global
    } kind;

    int index = -1;              // local slot 或 upvalue index
    std::string name;
};
```

## 为什么需要它

AST 节点 `NameExpr("a")` 只知道名字叫 `a`。
但 codegen 需要知道：

* 它是不是当前函数局部变量
* 是不是上值
* 还是全局环境访问

所以必须有一个 resolver/binder 阶段，把 AST 上的名字解析成 `SymbolRef`。

## 推荐做法

不要在 `NameExpr` 里直接塞 codegen 字段。
而是建立绑定表：

```cpp
std::unordered_map<const ast::Expr*, SymbolRef> resolvedSymbols;
```

或给 AST 节点挂一层语义 info。

---

# 八、核心结构 6：ScopeFrame

这是 resolver 阶段和 function codegen 阶段都需要的结构。

## 作用

表示：

> “当前作用域里有哪些局部变量、它们的生命周期和槽位是什么？”

## 推荐定义

```cpp
struct LocalInfo {
    std::string name;
    int slot = -1;
    int beginPc = -1;
    int endPc = -1;
    bool captured = false;
};

struct ScopeFrame {
    std::vector<LocalInfo> locals;
    int baseRegister = 0;
    bool breakable = false;
};
```

## 为什么需要它

Lua 有很强的块级作用域和上值捕获语义：

```lua
do
    local x = 1
end
```

以及：

```lua
local x = 1
return function() return x end
```

这些都必须靠 `ScopeFrame` 或类似结构维护。

---

# 九、核心结构 7：CodeGenContext

这是整个后端的“总状态对象”。

## 作用

集中保存：

* 当前函数的 bytecode
* 当前寄存器状态
* 常量池
* 局部变量布局
* break/loop 上下文
* 当前 patching 状态
* 调试信息

## 推荐定义

```cpp
struct CodeGenContext {
    std::vector<Instruction> code;
    ConstantPoolBuilder* constants = nullptr;
    RegisterAllocator* regs = nullptr;

    std::vector<ScopeFrame> scopes;
    std::vector<LoopContext> loops;

    std::unordered_map<const ast::Expr*, SymbolRef> resolvedSymbols;

    int pc() const { return static_cast<int>(code.size()); }
};
```

## 为什么它重要

Lua 5.1 里很多状态散在：

* `FuncState`
* `LexState`
* `BlockCnt`
* `expdesc`

你现在重写时，应该把“后端生成态”集中在 `CodeGenContext`，而不是分散到 AST 节点里。

---

# 十、核心结构 8：RegisterAllocator

移除 `expdesc` 后，寄存器管理不能再隐式附着在表达式状态里，必须独立出来。

## 作用

负责：

* 分配临时寄存器
* 回收临时寄存器
* 管理当前函数最大寄存器数
* 配合局部变量固定槽位

## 推荐定义

```cpp
class RegisterAllocator {
public:
    int alloc();
    void free(int reg);

    int mark();
    void restore(int mark);

    int maxUsed() const;

private:
    int nextReg_ = 0;
    int maxReg_ = 0;
};
```

## 为什么重要

原来 `expdesc` 会隐式参与：

* 值是否可重定位
* 值是否已经进入寄存器
* 是否要搬运

现在这些都应该由 `RegisterAllocator + emitExpr` 配合完成。

---

# 十一、核心结构 9：Label / JumpRef

如果你想把后端写得更现代，而不想全程直接操作 `pc int`，可以加这两个辅助结构。

## 推荐定义

```cpp
struct Label {
    int pc = -1;
};

struct JumpRef {
    int fromPc = -1;
};
```

## 用处

这样你在写 `if / while / repeat / break` 时，逻辑会清晰很多：

```cpp
Label loopBegin = newLabelHere();
JumpRef exitJump = emitJumpPlaceholder();
bindLabel(loopEnd);
patchJump(exitJump, loopEnd);
```

如果你不想抽象太多，也可以直接用 `int pc`。
但在 C++20 项目里，这种薄封装通常是值得的。

---

# 十二、核心结构 10：CallResultInfo / MultiRetPolicy

Lua 的函数调用和 vararg 很特殊，最后一个表达式可能传播多返回值。

例如：

```lua
return f()
local a, b = f()
g(f())
local x = (f())
```

这些语义不能只靠一个 `reg` 表示。

## 推荐定义

```cpp
struct CallResultInfo {
    int baseReg = -1;
    bool openMultiRet = false; // 是否作为开放返回值传播
};
```

或者更明确：

```cpp
enum class MultiRetPolicy {
    Single,
    Open
};
```

## 为什么必须有它

这是 Lua codegen 最容易错的地方之一。
原来 `VCALL / VVARARG` 隐式帮助处理了这部分。

你去掉 `expdesc` 后，必须显式表示：

* 当前调用是只取一个值
* 还是开放多返回
* 还是在 return 语境下全量返回

---

# 十三、核心结构 11：LoopContext

这个结构是处理 `break`、循环跳转和局部变量作用域恢复的关键。

## 推荐定义

```cpp
struct LoopContext {
    int loopStartPc = -1;
    PatchList breakList;
    int scopeDepth = 0;
};
```

## 为什么需要它

例如：

```lua
while cond do
    if x then break end
end
```

`break` 的目标在看到循环结束前还不知道，所以必须挂到 `LoopContext.breakList`。

同时，break 还往往伴随：

* 离开哪些局部变量作用域
* 是否需要关闭 upvalue
* 恢复哪些寄存器

所以循环上下文必须独立保存。

---

# 十四、核心结构 12：FunctionContext

每个函数体都应该有自己的独立上下文，而不是所有信息塞在一个全局 codegen 里。

## 推荐定义

```cpp
struct FunctionContext {
    CodeGenContext gen;
    std::vector<UpvalueDesc> upvalues;
    int numParams = 0;
    bool isVararg = false;
};
```

## 作用

处理：

* 当前函数自己的常量池 / 指令流
* 参数槽位
* 闭包捕获
* 子函数嵌套

Lua 的 `Proto` 非常函数化，所以这个结构很自然。

---

# 十五、核心结构 13：ConstantPoolBuilder

原来 `expdesc` 里有常量类别状态，现在建议统一交给常量池管理器。

## 推荐定义

```cpp
class ConstantPoolBuilder {
public:
    int internNil();
    int internBoolean(bool v);
    int internNumber(double v);
    int internString(const std::string& s);

private:
    std::vector<Constant> constants_;
    std::unordered_map<std::string, int> stringMap_;
};
```

## 好处

* 常量去重
* 统一索引分配
* 与 RK 编码配合
* 与常量折叠配合

---

# 十六、核心结构 14：UpvalueLayout

这是你以后做闭包时一定需要的结构。

## 推荐定义

```cpp
struct UpvalueDesc {
    std::string name;
    bool inStack = false;  // 来自父函数栈槽还是父函数upvalue
    int index = -1;
};

struct UpvalueLayout {
    std::vector<UpvalueDesc> items;
};
```

## 为什么必须有

Lua 的闭包不是简单捕获值，而是捕获变量位置语义。
如果没有独立 `UpvalueLayout`，闭包生成很快就会乱。

---

# 十七、核心结构 15：ProtoBuilder

如果你的目标仍然是 Lua 风格 VM，那么最终必须把当前函数编成 `Proto`。

## 推荐定义

```cpp
class ProtoBuilder {
public:
    void emit(Instruction ins);
    int addConstant(Constant c);
    void addLocal(LocalInfo localInfo);
    void addUpvalue(UpvalueDesc uv);

    Proto finish();
};
```

## 作用

它相当于把：

* 指令流
* 常量池
* 局部变量表
* upvalue 表
* 子 Proto
* 调试信息

统一收口。

---

# 十八、这些结构之间怎么协作

下面给你一条完整链路。

---

## 场景 1：普通表达式

```lua
local c = a + b
```

### 流程

1. AST：

   * `LocalDeclStmt`
   * `BinaryExpr(Add, NameExpr("a"), NameExpr("b"))`

2. resolver：

   * `a -> SymbolRef(Local, slotA)`
   * `b -> SymbolRef(Local, slotB)`

3. codegen：

   * `emitExpr(NameExpr("a")) -> ValueResult{Register, reg=slotA}`
   * `emitExpr(NameExpr("b")) -> ValueResult{Register, reg=slotB}`
   * 申请 `dst`
   * 发 `ADD dst, slotA, slotB`
   * 返回 `ValueResult{Register, reg=dst}`

---

## 场景 2：赋值目标

```lua
t[k] = x
```

### 流程

1. `emitLValue(IndexExpr(t, k)) -> LValueRef{Indexed, tableReg=?, keyReg=?}`
2. `emitExpr(NameExpr("x")) -> ValueResult`
3. `emitStore(lhs, rhs)`

这里左边和右边完全分流，这就是去掉 `expdesc` 后最大的清晰化。

---

## 场景 3：条件短路

```lua
if a and b then
end
```

### 流程

1. `emitCond(a and b)`
2. 左边 `a` 生成条件跳转
3. `trueList` patch 到右边 `b`
4. 右边 `b` 再产生真假链
5. then 块开始后 patch `trueList`
6. if 末尾 patch `falseList`

这部分就是 `CondResult + PatchList` 的核心价值。

---

## 场景 4：值语境中的布尔表达式

```lua
local x = a and b
```

### 流程

1. `emitCond(a and b)` 先得到真假链
2. 分配 `dst`
3. false 路径给 `dst = false`
4. true 路径给 `dst = true`
5. 收口

这相当于把条件结果物化为普通值。

---

# 十九、最终建议的最小可落地结构集

如果你现在要马上开工，不想一次设计太多，我建议你先实现这 8 个，作为第一批最小闭环：

### 第一批必须有

* `ValueResult`
* `CondResult`
* `PatchList`
* `LValueRef`
* `SymbolRef`
* `CodeGenContext`
* `RegisterAllocator`
* `LoopContext`

### 第二批很快会补上

* `FunctionContext`
* `ConstantPoolBuilder`
* `UpvalueLayout`
* `ProtoBuilder`
* `CallResultInfo`

---

# 二十、给你一份推荐代码骨架

下面这套命名，你几乎可以直接拿去建目录。

```cpp
// backend/value_result.h
struct ValueResult;

// backend/cond_result.h
struct PatchList;
struct CondResult;

// backend/lvalue_ref.h
struct LValueRef;

// sema/symbol_ref.h
struct SymbolRef;

// sema/scope_frame.h
struct ScopeFrame;

// backend/register_allocator.h
class RegisterAllocator;

// backend/constant_pool_builder.h
class ConstantPoolBuilder;

// backend/loop_context.h
struct LoopContext;

// backend/function_context.h
struct FunctionContext;

// backend/codegen_context.h
struct CodeGenContext;

// backend/proto_builder.h
class ProtoBuilder;

// backend/code_generator.h
class CodeGenerator {
public:
    ValueResult emitExpr(const ast::Expr&);
    CondResult emitCond(const ast::Expr&);
    LValueRef emitLValue(const ast::Expr&);
    void emitStmt(const ast::Stmt&);
};
```

---

# 二十一、你可以把 expdesc 的职责映射成这张替换表

| Lua 5.1 expdesc 职责 | C++20 AST 后端替代结构                     |
| ------------------ | ------------------------------------ |
| 表达式种类              | AST 节点本身                             |
| 变量/索引/可赋值位置        | `LValueRef`                          |
| 普通右值结果             | `ValueResult`                        |
| 真/假跳转链             | `CondResult` + `PatchList`           |
| VCALL / VVARARG    | `CallResultInfo` / `MultiRetPolicy`  |
| 名字解析               | `SymbolRef` + Resolver               |
| 寄存器落点              | `RegisterAllocator` + `ValueResult`  |
| 函数生成态              | `FunctionContext` / `CodeGenContext` |

---

# 二十二、最后给你一个工程级判断标准

如果你把 `expdesc` 删掉以后，后端还能自然回答这几个问题，说明结构设计是对的：

### 1

这个表达式作为右值时，结果在哪里？
看 `ValueResult`

### 2

这个表达式作为条件时，真假分支怎么跳？
看 `CondResult`

### 3

这个表达式作为赋值左边时，怎么写回？
看 `LValueRef`

### 4

这个名字到底是 local / upvalue / global？
看 `SymbolRef`

### 5

这个函数调用是一值还是多返回传播？
看 `CallResultInfo`

### 6

当前函数的寄存器、常量池、循环上下文在哪？
看 `CodeGenContext`

只要这六个问题都能由独立结构回答，你就真正完成了从 `expdesc` 思维到 AST 后端思维的迁移。

---

如果你愿意，我下一步可以继续直接给你补一份：

**《C++20 Lua 编译器后端头文件设计草案（ValueResult / CondResult / LValueRef / CodeGenContext）》**

我可以直接按接近可编译的 C++20 头文件形式写出来。
