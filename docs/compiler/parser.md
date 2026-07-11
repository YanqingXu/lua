---
status: current
verified_against: src/compiler/parser/parser.hpp; src/compiler/parser/parser_impl.hpp; src/compiler/parser/parser.cpp; src/compiler/parser/parser_stmt.cpp; src/compiler/parser/parser_expr.cpp; src/compiler/parser/parser_primary.cpp; src/compiler/parser/parser_func.cpp; src/compiler/parser/parser_table.cpp; src/compiler/parser/parser_utils.hpp; src/compiler/parser/token.hpp; src/compiler/ast.hpp; tests/unit/compiler/test_parser_boundaries.cpp; tests/unit/compiler/test_parser_error_recovery.cpp
last_checked: 2026-05-24
applies_to: current Parser implementation under src/compiler/parser
---

# Lua Parser 实现说明

本文档是 `src/compiler/parser/` 模块当前实现的权威事实源。旧的表达式专项文档已经并入本文；后续如果 Parser 的语法能力、错误恢复、物理分片或 public API 发生变化，应更新本文，而不是再创建平行的 Parser 文档。

## 1. 职责边界

Parser 位于编译器前端的中间层：上游是 `Lexer`，下游是 `CodeGenerator`。它不直接处理字符输入，也不生成字节码；它只消费 token 流并产出 AST。

```text
source text
  -> Lexer
  -> Token stream
  -> Parser
  -> Chunk / Expr / Stmt AST
  -> CodeGenerator
  -> Proto / bytecode
```

当前 Parser 的核心任务有三类。

| 任务 | 当前实现 |
|---|---|
| Token 到 AST | `Parser::Impl` 通过递归下降解析 token，生成 `Chunk`、`Stmt` 和 `Expr` 节点。AST 类型定义在 `src/compiler/ast.hpp`。 |
| 语法错误报告 | `error()` / `errorAt()` 构造 `ParseError`，保留行号、列号和带 `near '<token>'` 的诊断文本。词法错误 token 也会通过相同路径进入 ParseError。 |
| 错误恢复与诊断收集 | `diagnostics()` 暴露本轮解析收集到的 `Vec<ParseError>`；`ParserOptions::recoveryMode` 控制是首错返回还是按语句边界继续收集。 |

### 1.1 Public API 与 Pimpl 边界

`parser.hpp` 只暴露稳定的调用边界：

- `Parser(const Str& source)`：默认构造，使用 `FailFast`。
- `Parser(const Str& source, ParserOptions options)`：显式指定恢复策略。
- `Parser(const Str& source, RuntimeServices& services)` 及其 options 重载：为后续服务注入保留入口；当前 `services_` 仅保存指针，不参与语法决策。
- `std::expected<Chunk, ParseError> parse()`：解析入口。
- `const Vec<ParseError>& diagnostics() const noexcept`：读取诊断集合。

实现细节全部收在 `Parser::Impl` 中，`Parser` 只持有 `UPtr<Impl> impl_`。这个 Pimpl 边界的作用是：

- 隐藏 `Lexer`、`TokenStream`、`ParseState`、AST 工厂、恢复策略等内部类型。
- 让 `parser.hpp` 不随实现分片频繁膨胀，减少包含它的调用方重编译压力。
- 保持 Parser 对外是可移动、不可复制的资源对象；内部 token 流和诊断状态不会被意外复制。
- 允许 `parser_*.cpp` 共享同一个 `Impl` 私有接口，而不把这些 helper 暴露成 public API。

### 1.2 与 Lexer 的边界

Parser 不扫描字符。`Parser::Impl::TokenStream` 内部持有 `Lexer`：

- 构造时先调用 `lexer_.nextToken()` 填充 `current_`。
- `current()` 返回当前 token。
- `advance()` 用 `nextToken()` 替换当前 token。
- `peek()` 调用 `lexer_.peekToken()` 做一 token 前瞻。
- `check(type)` 只比较当前 token 类型。

因此 Parser 的输入模型是 LL(1) 风格的 token 游标。局部歧义通过 `peek()` 解决，目前最典型的是表构造器里的 `name = value` 与普通数组字段 `name`。

### 1.3 与 CodeGenerator 的边界

Parser 生成的是语法树，不做字节码 lowering，也不做寄存器分配。它会尽量保留后续阶段需要的语义信息，例如：

- `ParenExpr` 保留括号，供多返回值收敛语义使用。
- `CallExpr::isMethodCall` 记录冒号调用。
- `FunctionStmt::tablePath` / `isMethod` / `isLocal` 保留函数定义形式。
- `TableField::key == nullptr` 表示数组部分字段；非空 key 表示显式键值字段。

名称绑定、local/upvalue/global 分类、赋值目标合法性、方法调用里的隐式 `self` 具体 lowering，都属于后续编译阶段。

## 2. 物理架构与分片设计

当前 Parser 已从单个大实现文件拆成一组按语法主题组织的分片。所有分片都包含 `parser_impl.hpp`，在同一个 `Parser::Impl` 私有类上实现成员函数。

| 文件 | 职责 |
|---|---|
| `parser.hpp` | Public API、`ParseRecoveryMode`、`ParserOptions`、Pimpl 声明。 |
| `parser_impl.hpp` | `Parser::Impl` 私有状态、内部 helper、恢复策略、递归保护、所有分片函数声明。 |
| `parser.cpp` | Parser 构造 / 移动 / 析构，`parse()` 外壳，Token 管理，错误发布，恢复策略创建和同步。 |
| `parser_stmt.cpp` | `parseBlock()`、语句分派、控制流、循环、局部声明、return/break、赋值语句和调用语句。 |
| `parser_expr.cpp` | 表达式优先级链、二元/一元运算、`parseExprList()`。 |
| `parser_primary.cpp` | 字面量、名字、括号、函数/表表达式入口，以及调用、索引、成员、方法调用等后缀链。 |
| `parser_func.cpp` | 函数声明、匿名函数表达式、参数列表和 vararg 整理。 |
| `parser_table.cpp` | 表构造器字段、键值项、数组项、分隔符和尾随分隔符。 |
| `parser_utils.hpp` | 不依赖 Parser 状态的跨分片工具，目前提供 `ParserUtils::tokenString()`。 |
| `token.hpp` | Parser 消费的 token 类型、语义值、词素和位置信息。 |

这个拆分的维护原则是：`parser.cpp` 保持为基础设施层，具体语法产生式留在主题分片中。新增语法时优先放进对应分片；只有跨分片共享、且不需要 Parser 状态的逻辑才进入 `parser_utils.hpp`。

### 2.1 `parser_impl.hpp` 的内部结构

`Parser::Impl` 内部有几组关键私有类型。

| 内部类型 | 作用 |
|---|---|
| `AstFactory` | 统一封装 `makeExpr<T>()` / `makeStmt<T>()` 和二元、一元表达式节点构造。 |
| `TokenStream` | 把 `Lexer` 包装成 Parser 需要的 `current` / `advance` / `peek` / `check` token 游标。 |
| `ParseState` | 保存递归深度计数。 |
| `RecursionGuard` | RAII 方式限制语法递归深度，超过上限时报 `chunk has too many syntax levels`。 |
| `ParseDiagnosticCollector` | 收集本轮解析中发布的 `ParseError`。 |
| `ErrorRecoveryStrategy` | 恢复策略接口；当前有 FailFast 和 StatementBoundary 两个实现。 |

`RecursionGuard` 不是每个小函数都进入，而是在可能引入深层嵌套的入口使用：`parseExpression()`、`parseBlock()`、`parseTableConstructor()`、`parseFunctionExpr()`。默认表达式/复合结构上限是 `MAX_RECURSION_DEPTH = 92`，块嵌套使用 `MAX_BLOCK_RECURSION_DEPTH = 91`。

### 2.2 `parser_utils.hpp`

`ParserUtils::tokenString(const Token&) -> StrView` 是跨分片共享的 token 文本 helper：

- 如果 token 的语义值里有 `Str`，返回解码后的字符串值，例如字符串字面量。
- 否则返回 `token.lexeme`，例如名字 token。

它返回借用型 `StrView`。所有需要存入 AST 的地方都会显式构造 `Str`，例如名字、函数参数、成员名和 `name = value` 表字段 key。这样能清楚地区分“当前 token 临时借用”和“AST 拥有数据”。

## 3. 解析流程与算法

### 3.1 顶层流程

`Parser::parse()` 委托给 `Parser::Impl::parse()`。内部流程如下：

```text
diagnosticCollector_.clear()
try
  Chunk chunk
  chunk.statements = parseBlock()
  expect Eos
  if diagnostics not empty
    return unexpected(first diagnostic)
  return chunk
catch ParseError
  return unexpected(error)
```

注意：内部解析函数仍使用异常作为局部控制流；public 边界把异常转换成 `std::expected<Chunk, ParseError>`。这让调用方不需要捕获异常即可处理语法失败。

`parseBlock()` 是 chunk 和嵌套块共用的入口。它持续解析语句，直到遇到块结束 token：

- `Eos`
- `end`
- `else`
- `elseif`
- `until`

如果遇到 `return`，会解析 return 语句并停止当前 block。这个行为对齐 Lua：`return` 只能作为 block 的最后一条语句出现。

### 3.2 递归下降结构

当前 Parser 是手写递归下降解析器，每个重要语法规则对应一个函数。语句层先按当前 token 分派：

```text
parseStatement
  If       -> parseIfStmt
  While    -> parseWhileStmt
  Do       -> parseDoStmt
  For      -> parseForStmt
  Repeat   -> parseRepeatStmt
  Function -> parseFunctionStmt
  Local    -> parseLocalStmt
  Break    -> parseBreakStmt
  default  -> parseExprStmt
```

表达式层按优先级拆成显式调用链：

```text
parseExpression
  -> parseOrExpr
    -> parseAndExpr
      -> parseRelationalExpr
        -> parseConcatExpr
          -> parseAdditiveExpr
            -> parseMultiplicativeExpr
              -> parseUnaryExpr
                -> parsePowerExpr
                  -> parsePrimaryExpr
                    -> parsePostfixExpr
```

这种实现没有使用 Pratt parser 或统一的 precedence climbing。它的优点是优先级和函数层次一一对应，读代码时能直接看到 Lua 表达式规则。

### 3.3 表达式优先级与结合性

Lua 5.1 的表达式优先级从低到高，在当前实现中映射如下：

| Lua 运算符 | Parser 函数 | AST op | 结合性实现 |
|---|---|---|---|
| `or` | `parseOrExpr()` | `BinaryExpr::Op::Or` | 左结合，`while` 循环。 |
| `and` | `parseAndExpr()` | `BinaryExpr::Op::And` | 左结合，`while` 循环。 |
| `< > <= >= == ~=` | `parseRelationalExpr()` | `Lt/Gt/Le/Ge/Eq/Ne` | 当前只消费一个关系运算符。 |
| `..` | `parseConcatExpr()` | `Concat` | 右结合，递归调用当前层解析右侧。 |
| `+ -` | `parseAdditiveExpr()` | `Add/Sub` | 左结合，`while` 循环。 |
| `* / %` | `parseMultiplicativeExpr()` | `Mul/Div/Mod` | 左结合，`while` 循环。 |
| `not - #` | `parseUnaryExpr()` | `Not/Neg/Len` | 前缀一元，递归解析操作数。 |
| `^` | `parsePowerExpr()` | `Pow` | 右结合，递归调用当前层解析右侧。 |
| literals/name/function/table/paren/postfix | `parsePrimaryExpr()` / `parsePostfixExpr()` | 对应 AST 节点 | 后缀链用循环。 |

左结合运算符使用循环构造左折叠 AST。例如：

```text
a - b - c
=> (a - b) - c
```

右结合运算符使用递归解析右侧。例如：

```text
a .. b .. c
=> a .. (b .. c)

a ^ b ^ c
=> a ^ (b ^ c)
```

一元运算符的操作数继续调用 `parseUnaryExpr()`，没有一元运算符时才进入 `parsePowerExpr()`。结合 `parsePowerExpr()` 的右递归，这会让幂运算比一元负号绑定更紧，符合 Lua 5.1 对 `-2^2` 的规则：它解析为 `-(2^2)`。

当前实现有两个需要明确的边界：

- `parseRelationalExpr()` 使用单次 `if`，不是 `while`，因此不会把 `a < b < c` 折叠成关系链。
- `parsePrimaryExpr()` 会把包括字面量在内的基础表达式都送入 `parsePostfixExpr()`。这让后缀处理统一，但 parser 阶段没有严格限制“只有 Lua prefixexp 才能接后缀”；后续阶段仍可根据 AST 形状做更细的合法性约束。

### 3.4 基础 token 工具调用规范

`current()`、`advance()`、`check()`、`match()`、`expect()` 是所有分片共享的基础动作。

| 工具 | 语义 | 使用规范 |
|---|---|---|
| `current()` | 读取当前 token，不消费。 | 用于检查 token 类型、读取位置、读取语义值。不要长期保存 `StrView` 指向当前 token 后再 `advance()`。 |
| `advance()` | 消费当前 token，读取下一个 token。 | 只在已经确定当前 token 应被吃掉时调用。若需要位置信息，先保存 token 或行列号。 |
| `peek()` | 查看下一个 token，不消费当前 token。 | 只用于局部歧义判断，例如表字段 `name = value`。不要用它实现大范围回溯。 |
| `check(type)` | 判断当前 token 是否为指定类型，不消费。 | 用于可选分支或循环条件。 |
| `match(type)` | 若当前 token 匹配则消费并返回 true。 | 用于可选 token、分隔符和后缀入口。 |
| `expect(type, message)` | 要求当前 token 匹配；不匹配则报错并抛出 `ParseError`。 | 用于必须出现的闭合符、关键字和结构边界。 |

调用上有一个简单原则：可选结构用 `match()`，硬性语法边界用 `expect()`，只看不吃用 `check()` 或 `peek()`。

## 4. Lua 5.1 语义实现映射

当前 Parser 的语法目标是 Lua 5.1.5 风格的 chunk/block/stat/expr 结构。它不是官方 `lparser.c` 的逐行移植，而是用 C++ AST 和递归下降函数表达同一组核心语法。

### 4.1 语句与控制流

| Lua 语法 | 当前 AST | 实现位置 |
|---|---|---|
| `if exp then block {elseif exp then block} [else block] end` | `IfStmt`，`branches` 保存 if/elseif，`elseBranch` 保存 else。 | `parseIfStmt()` |
| `while exp do block end` | `WhileStmt` | `parseWhileStmt()` |
| `do block end` | `DoStmt` | `parseDoStmt()` |
| `repeat block until exp` | `RepeatStmt` | `parseRepeatStmt()` |
| `for Name = exp, exp [, exp] do block end` | `ForNumStmt`，未写 step 时合成数字 `1.0`。 | `parseForStmt()` |
| `for namelist in explist do block end` | `ForInStmt` | `parseForStmt()` |
| `break` | `BreakStmt` | `parseBreakStmt()` |
| `return [explist]` | `ReturnStmt` | `parseReturnStmt()` |

`parseBlock()` 在遇到 `return` 后结束当前块。这与 Lua 的 block 规则一致：`return` 后不能再继续解析同一块中的普通语句。

### 4.2 局部变量与函数定义

局部声明有两条路径：

- `local namelist [= explist]` 生成 `LocalStmt`。
- `local function Name funcbody` 生成 `FunctionStmt`，并设置 `isLocal = true`。

函数定义语句支持：

- `function foo(...) ... end`
- `function t.a.b.foo(...) ... end`
- `function t:method(...) ... end`

`parseFunctionStmt()` 会把点路径保存在 `FunctionStmt::tablePath`，最终函数名保存在 `name`。遇到冒号形式时设置 `isMethod = true`，并在参数列表前插入 `"self"`。`parseLocalStmt()` 的 `local function` 分支只接受简单函数名，不解析 `t.a` 或 `t:method` 路径。

匿名函数表达式由 `parseFunctionExpr()` 处理，形如：

```lua
function (params) block end
```

参数列表由 `parseParamList()` 共享解析，支持空参数、普通名字列表和 `...`。调用者检查最后一个参数是否为 `"..."`，若是则设置 `isVararg = true` 并从固定参数列表中移除这个哨兵。

### 4.3 赋值与调用语句

默认语句分支走 `parseExprStmt()`。它先解析一个表达式，然后根据后续 token 决定语句类型：

- 后面是 `,` 或 `=`：生成 `AssignStmt`，左侧 target 列表和右侧 values 列表都使用表达式列表。
- 没有赋值符：只有当表达式 AST 是 `CallExpr` 时才生成 `CallStmt`。
- 其他表达式单独成语句会报 `unexpected symbol`。

当前 Parser 在语法层允许任意表达式进入赋值 target 列表；具体是否为合法 lvalue 由后续阶段负责判断。

### 4.4 表构造器

`parseTableConstructor()` 支持 Lua 5.1 的三种字段形态：

| 源码形态 | AST 表示 |
|---|---|
| `[exp] = exp` | `TableField::key` 为解析出的 key 表达式，`value` 为右侧表达式。 |
| `name = exp` | `name` 被降解成 `StringExpr` key，等价于 `["name"] = exp`。 |
| `exp` | `key == nullptr`，表示数组部分字段。 |

字段分隔符支持 `,` 和 `;`，并允许尾随分隔符。字段按源码顺序保存到 `TableExpr::fields`，因为表构造器字段求值顺序对 Lua 语义有意义。

### 4.5 Primary 与后缀表达式

`parsePrimaryExpr()` 支持这些基础表达式：

- `nil`
- `true` / `false`
- number
- string
- `...`
- `{...}`
- `function(...) ... end`
- `(exp)`
- name

每个基础表达式随后进入 `parsePostfixExpr(base)`，后者循环吸收后缀：

| 后缀 | AST |
|---|---|
| `(args)` | `CallExpr` |
| `[exp]` | `IndexExpr` |
| `.Name` | `MemberExpr` |
| `:Name(args)` | `MemberExpr` 包成 `CallExpr`，并设置 `isMethodCall = true` |
| `"literal"` | 单字符串参数调用糖，生成 `CallExpr` |
| `{...}` | 单表参数调用糖，生成 `CallExpr` |

括号表达式不会被丢弃，而是生成 `ParenExpr`。这是必要语义信息：Lua 中 `(f())` 会在多返回值语境中强制收敛为单值，后续阶段需要知道源码里是否写过括号。

### 4.6 运算符映射

当前运算符到 AST 的映射如下：

| Lua token | AST |
|---|---|
| `+` | `BinaryExpr::Op::Add` |
| `-` 二元 | `BinaryExpr::Op::Sub` |
| `*` | `BinaryExpr::Op::Mul` |
| `/` | `BinaryExpr::Op::Div` |
| `%` | `BinaryExpr::Op::Mod` |
| `^` | `BinaryExpr::Op::Pow` |
| `..` | `BinaryExpr::Op::Concat` |
| `< <= > >= == ~=` | `Lt/Le/Gt/Ge/Eq/Ne` |
| `and` / `or` | `BinaryExpr::Op::And` / `Or` |
| `not` | `UnaryExpr::Op::Not` |
| `-` 一元 | `UnaryExpr::Op::Neg` |
| `#` | `UnaryExpr::Op::Len` |

### 4.7 当前 Lua 5.1 边界

本文描述的是当前仓库实现，而不是一份抽象的 Lua 5.1 语法承诺。维护时需要特别注意这些边界：

- `EmptyStmt` AST 类型存在，但 `parser_stmt.cpp` 目前没有普通解析 `;` 空语句；`;` 主要用于错误恢复同步，表构造器字段分隔符也支持 `;`。
- 普通调用支持 Lua 的单参数糖：`f"str"` 和 `f{...}`。冒号方法调用当前要求 `obj:method(args)`，还没有接受 `obj:method"str"` 或 `obj:method{...}` 这两种 `args` 形式。
- 关系表达式当前只消费一个关系运算符，不解析连续比较链。
- `break` 是否位于循环内、`...` 是否位于 vararg 函数内、赋值左侧是否为合法 lvalue，当前主要留给后续语义 / 代码生成阶段处理。
- `local function` 分支只接受简单名字，这与 Lua 5.1 的 `local function Name funcbody` 规则一致；全局/表成员函数路径由普通 `function` 语句处理。

## 5. 错误处理与恢复策略

### 5.1 `std::expected<Chunk, ParseError>` 返回机制

Parser 的 public 入口不把 `ParseError` 抛给调用者，而是返回：

- 成功：`std::expected<Chunk, ParseError>` 持有 `Chunk`。
- 失败：返回 `std::unexpected(ParseError)`。

内部 helper 仍然通过 `throw ParseError` 快速中断当前产生式。`Parser::Impl::parse()` 捕获后转换为 `unexpected`。这是一种边界清晰的折中：内部递归下降保持简单，外部 API 保持显式错误返回。

诊断信息通过 `publishDiagnostic()` 发布给 `diagnosticObservers_`。当前默认观察者是 `ParseDiagnosticCollector`，因此所有 `errorAt()` 和 `reportErrorAt()` 都会进入 `diagnostics()`。

`ParseError` 的消息由 `errorWithNear()` 生成：

- 普通 token：`<message> near '<lexeme or token string>'`
- EOF：使用 `<eof>`
- `TokenType::Error` 且带 `errorMessage`：优先使用 lexer 提供的错误文本，再追加 near 信息

### 5.2 FailFast 模式

`ParseRecoveryMode::FailFast` 是默认模式。对应策略 `FailFastRecoveryStrategy` 的 `canRecover()` 返回 `false`。

行为是：

1. 解析函数发现错误，发布诊断并抛出 `ParseError`。
2. `parseBlock()` 捕获后询问策略。
3. 策略拒绝恢复，错误继续向外抛出。
4. 顶层 `parse()` 返回 `std::unexpected(error)`。

这个模式通常只收集第一个错误，适合普通执行、REPL 快速判错和测试中的精确错误断言。

### 5.3 StatementBoundary 模式

`ParseRecoveryMode::StatementBoundary` 使用 `StatementBoundaryRecoveryStrategy`。它允许 `parseBlock()` 在某条语句失败后同步到下一个语句边界，然后继续解析。

同步逻辑在 `synchronize()` 中：

- 如果遇到 `;`，消费后停止。
- 如果当前 token 是块边界，停止：`end`、`else`、`elseif`、`until`。
- 如果当前 token 看起来是新语句开头，停止：`local`、`function`、`if`、`while`、`for`、`repeat`、`return`、`break`。
- 否则持续 `advance()`，直到 `Eos`。

最终结果仍然是失败：如果诊断集合非空，顶层 `parse()` 返回第一条诊断作为 `unexpected`。但调用方可以通过 `parser.diagnostics()` 取得本轮收集到的多条错误。这条路径由 `Parser Error Reporting` 测试中的 statement recovery 用例覆盖。

### 5.4 恢复边界的维护约束

新增语句关键字时，需要同步检查两处：

- `parseStatement()` 的分派表。
- `synchronize()` 的“新语句开头”集合。

否则 StatementBoundary 模式可能无法在错误后跳到正确边界，导致级联诊断质量下降。

## 6. 维护要点

- 新增表达式优先级时，应优先新增一层 `parseXxxExpr()`，并把它插入现有调用链；左结合使用循环，右结合使用递归。
- 新增后缀表达式时，优先扩展 `parsePostfixExpr()` 的循环。
- 新增语句时，优先扩展 `parser_stmt.cpp`，并同步错误恢复的 statement-start 集合。
- 新增 token 字符串读取逻辑时，优先考虑是否能保持在 `ParserUtils` 这种无状态 helper 内。
- 解析阶段应保留会影响后续 Lua 语义的信息，例如括号、冒号调用、函数路径和表字段顺序；不要为了 AST 简化提前丢掉这些信号。
- 当前边界测试集中在 `Parser Boundary Sentinels` 和 `Parser Error Reporting`。修改分片职责、优先级、后缀链、表构造器或恢复策略时，应至少运行这些测试。
