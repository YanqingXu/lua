---
status: current
verified_against: docs/roadmap/c-style-refactoring-roadmap.md; src/compiler/ast_visitor.hpp; src/compiler/codegen/codegen_types.hpp; src/compiler/codegen/gc_allocation_guard.hpp; src/compiler/lexer/lexer.cpp; src/compiler/opcode.hpp; src/vm/vm.cpp; src/vm/vm_handlers.cpp; src/vm/vm_switch_dispatch.hpp; src/runtime/runtime_services.hpp; src/gc/garbage_collector.hpp; src/gc/garbage_collector.cpp; src/core/value.hpp; src/core/metatable.cpp; src/core/userdata.cpp; src/lib/lib_catalog.cpp; src/lib/lib_registry.cpp; src/lib/iolib.cpp; src/lib/packagelib.cpp; src/lib/stringlib.cpp; tools/run_quality_gate.ps1; tools/check_doc_drift.ps1; tools/check_c_style_patterns.ps1; tools/check_opcode_coverage_matrix.ps1
last_checked: 2026-07-11
applies_to: modern C++ teaching audit for src/ compiler, VM, runtime, and standard library modules
---

# 现代 C++ 教学示范项目审计报告

## 审计结论

本项目已经不再是“C++ 包了一层 C 风格 Lua”的状态。核心路径中已经能看到清晰的现代 C++ 主线：`Value` / `ValueResult` 使用 `std::variant`，Parser / Codegen / VM 的关键 public 边界开始使用 `std::expected`，VM 字节码读取边界已经收窄到 `std::span<const Instruction>`，Codegen 引入了 `GCAllocationGuard`，标准库注册表用 `std::span` 暴露只读 catalog，质量门也已经覆盖 C-style 模式、opcode 矩阵、文档漂移和 ValueResult variant-only 边界。

距离“C++20/23 工业级教学标杆”还差的不是单点语法升级，而是三类一致性：

- **所有权一致性**：GC 托管对象可以继续以裸指针作为 non-owning observer，但创建和销毁语义应集中到 GC factory / RAII guard；本轮已把产品代码裸 `new` 清零，唯一裸 `delete` 和唯一 `std::free` 均由位置 allowlist 标记为所有权边界。
- **错误通道一致性**：`std::expected` 已在 Parser / Codegen / VM 入口落地，本轮追加到 `FunctionRegistrar` 和 I/O open adapter；catalog lookup 已改成 optional reference。pattern matching 等热路径内部仍保留 `nullptr` / `-1` 哨兵，适合后续用外层 adapter 包住。
- **教学自解释一致性**：CRTP Visitor、`Value`、元方法表、opcode metadata、switch handler table 和 opcode matrix 已形成 compile-time / script contract；后续重点是把更多 sentinel 组合迁入 sum type 或显式 invariant helper。

## 证据摘要

- `tools/check_c_style_patterns.ps1` 当前通过，登记值为：`bare new=0`、`bare delete=1`、`std::free/free=1`、`simple #define=51`、`return nullptr=60`；裸 `delete` / `std::free` 使用位置 allowlist，raw array / `char* end` / 测试手动所有权为 warning 规则。
- `std::expected` 当前覆盖 `Parser::parse()`、`CodeGenerator::tryGenerate()`、`VM::tryExecuteProto()`、`LuaState::tryPCall()`、bytecode CLI option parsing、`FunctionRegistrar::tryCreate*()` 和 I/O open adapter。
- `std::format` 已广泛进入 REPL、debug、bytecode printer 和标准库错误文本。
- `std::ranges::find_if` 已落地到标准库 catalog lookup；`std::views` 仍建议留给 coverage/report/trace 这类非热路径。
- C-style guard 已从纯数量限制升级为数量 + 位置 allowlist + advisory warning，避免“删一处旧命中又新增一处新命中”静默通过。

## Compiler

### 发现

| 等级 | 位置 | 发现 | 建议 |
|---|---|---|---|
| P1 | `src/compiler/lexer/lexer.cpp` | 十进制/十六进制数字仍使用 `char* end` + `std::strtod` / `std::strtoll`，这是典型 C 解析边界。 | 增加 `parseLuaNumber(StrView) -> std::expected<LuaNumber, LexError>`，把 locale fallback 和 malformed number 诊断集中起来。 |
| P1 | `src/compiler/opcode.hpp` | `OpcodeMetadata::name` 已改为 `StrView`，并增加 metadata 顺序 / group 完整性 `static_assert`。 | 继续保持 NUL 终止只出现在 `getOpName()` 等兼容输出边界。 |
| P2 | `src/compiler/ast_visitor.hpp` | `VisitsNode` / `VisitsExprNodes` 已能检查 visitor 覆盖所有 variant alternative；本轮补充 `kExprNodeCount` / `kStmtNodeCount` 和 const / void visitor 编译期测试。 | 新增 AST 节点时必须同步 variant、visitor、测试、文档和 codegen。 |
| P2 | `src/compiler/codegen/codegen_types.hpp` | `ValueResult` 已 variant-only，但 `LValueRef` / `CallResultInfo` 仍是 enum + sentinel 字段组合。 | 如果教学目标是展示 sum type，把 LValue / CallResult 也逐步迁为小型 `std::variant`，或至少为 sentinel 组合加 invariant helper 和测试。 |
| P2 | `src/compiler/codegen/codegen.cpp` | `generate()` 仍返回 `Proto*` 并抛异常，`tryGenerate()` 才是现代 expected 入口。 | 文档和示例应统一推荐 `tryGenerate()`；旧 `generate()` 标注 compatibility facade。 |

### 当前/落地前代码 vs 优化后代码

当前 Lexer 数字解析：

```cpp
char* end = nullptr;
const char* cstr = lexemeBuffer_.c_str();
f64 value = std::strtod(cstr, &end);

bool ok = end != nullptr && static_cast<usize>(end - cstr) == lexemeBuffer_.size();
if (!ok) {
    return errorToken("Malformed number");
}
```

建议抽成结构化解析边界：

```cpp
struct NumberParseError {
    Str message;
};

[[nodiscard]] std::expected<LuaNumber, NumberParseError>
parseLuaNumber(StrView text, const std::locale& locale);

Token Lexer::decimalNumber() {
    consumeDecimalNumberText();
    Token token = makeToken(TokenType::Number);

    auto parsed = parseLuaNumber(lexemeBuffer_, std::locale());
    if (!parsed) {
        return errorToken(parsed.error().message);
    }

    token.value = *parsed;
    return token;
}
```

当前 Visitor concept 已经不错：

```cpp
template <typename Visitor, typename R = void>
concept VisitsExprNodes = detail::visitsVariantNodes<Visitor, ExprVariant, R>(
    std::make_index_sequence<std::variant_size_v<ExprVariant>>{});
```

建议补教学锚点：

```cpp
inline constexpr usize kExprNodeCount = 14;
inline constexpr usize kStmtNodeCount = 13;

static_assert(std::variant_size_v<ExprVariant> == kExprNodeCount,
              "Add the new Expr node to visitors, tests, AST docs, and codegen.");
static_assert(std::variant_size_v<StmtVariant> == kStmtNodeCount,
              "Add the new Stmt node to visitors, tests, AST docs, and codegen.");
```

## VM

### 发现

| 等级 | 位置 | 发现 | 建议 |
|---|---|---|---|
| P1 | `src/vm/vm_switch_dispatch.hpp` + `src/vm/vm.cpp` | switch dispatch 已改为 `kSwitchHandlers` constexpr table，并通过 `static_assert` 验证 size/order/non-null。 | 后续可进一步让 table dispatch 与 switch dispatch 共享生成输入，当前至少消除了手写 switch 漂移。 |
| P1 | `tests/unit/vm/opcode_coverage_matrix.md` | opcode 矩阵仍是可读 checklist；脚本本轮已校验 opcode 行、顺序、group 和 `mayInvokeMetamethod`。 | 下一步可解析测试注册名，证明 Positive / Boundary / Metamethod path 指向真实测试。 |
| P2 | `src/vm/vm_handlers.cpp` | `handlerFor()` 返回 `Opt<OpHandler>` 已比 `nullptr` 好，但 `HandlerEntry::handler` 内部仍以 `nullptr` 表示未注册。 | 可引入 `RegisteredHandler` builder，构表结束后验证每个 opcode 已注册；release 路径保持零开销。 |
| P2 | `src/vm/vm.cpp` | `VMContext` 和 `OpExecutionContext` 仍携带多个裸指针和 `Value*& base` / `usize& pc` 引用。它们多数是 non-owning 热路径句柄，可以保留。 | 用命名类型表达 observer：`NotNull<LuaState*>`、`RegisterWindow`、`ProgramCounter`，把教学语义从“裸指针”提升为“非拥有执行视图”。 |
| P3 | VM 指令调度 | 未使用 `std::views`；当前循环是命令式 while/switch。 | 不建议在热路径强行 ranges 化；适合在 opcode coverage、trace diff、bytecode CFG 这类非热路径用 `views::filter/transform` 做教学展示。 |

### 当前/落地前代码 vs 优化后代码

落地前 switch backend 需要重复列出 opcode：

```cpp
switch (op) {
    case OpCode::MOVE:
        status = VM::detail::execOpMove(opContext, inst);
        break;
    case OpCode::LOADK:
        status = VM::detail::execOpLoadK(opContext, inst);
        break;
    // ...
}
```

建议把 opcode 到 handler 的关系做成单一 constexpr 数据源：

```cpp
using SwitchEntry = std::pair<OpCode, VM::detail::SwitchOpHandler>;

inline constexpr std::array<SwitchEntry, static_cast<usize>(NUM_OPCODES)> kSwitchHandlers{{
    {OpCode::MOVE, VM::detail::execOpMove},
    {OpCode::LOADK, VM::detail::execOpLoadK},
    // ...
}};

static_assert(kSwitchHandlers.size() == static_cast<usize>(NUM_OPCODES));

[[nodiscard]] inline Opt<VM::detail::SwitchOpHandler> switchHandlerFor(OpCode op) noexcept {
    const usize index = static_cast<usize>(op);
    if (index >= kSwitchHandlers.size() || kSwitchHandlers[index].first != op) {
        return std::nullopt;
    }
    return kSwitchHandlers[index].second;
}
```

## Runtime

### 发现

| 等级 | 位置 | 发现 | 建议 |
|---|---|---|---|
| P1 | `src/vm/state/lua_state.cpp` | `LuaState::create(...) -> UPtr<LuaState>` 已落地，`newState()` 作为 compatibility facade 调用 `release()`；产品裸 `new` 清零。 | 后续可逐步把测试 helper 迁到 owning factory，减少 `delete L` 教学噪音。 |
| P1 | `src/gc/garbage_collector.cpp` | `destroyObject()` 集中了唯一裸 `delete obj`，这是合理的 owner 边界，但质量门无法区分“唯一合法销毁点”和普通裸 delete。 | 为 GC 内部销毁点增加专门注释和脚本位置 allowlist；中长期可探索 collector 持有 `UPtr<GCObject>` 链表节点，或保持侵入式链表但把删除函数做成唯一 friend 边界。 |
| P1 | `src/core/userdata.cpp` | `UserdataBufferDeleter` 中仍有唯一 `std::free`；这是 POSIX aligned allocation 的合理 C allocator 边界。 | 保留在 deleter 内，但质量门应按位置登记，而不是仅按数量登记。 |
| P2 | `src/core/userdata.cpp` | `Userdata::createFullOwned() -> UPtr<Userdata>` 已落地，`createFull()` 保留为 legacy facade。 | 标准产品路径继续优先走 `GarbageCollector::create<Userdata>()`。 |
| P2 | `src/core/value.hpp` | `ValueType` 与 `ValueVariant` alternative 顺序已由 `static_assert` 逐项绑定。 | 新增 Lua value 类型时必须同步 enum、variant 和访问器。 |
| P2 | `src/core/metatable.cpp` | `kMetamethodNames` 已改为 `inline constexpr std::array<StrView, ...>`，并与 `TMS` 通过 `static_assert` 绑定。 | 新增/调整元方法时必须同步数组和相关测试。 |

### 当前/落地前代码 vs 优化后代码

落地前 Userdata 暴露裸所有权工厂：

```cpp
Userdata* Userdata::createFull(usize size) {
    if (size == 0) {
        throw std::invalid_argument("Userdata size cannot be zero");
    }

    return std::make_unique<Userdata>(size).release();
}
```

建议让所有权在类型上可见：

```cpp
[[nodiscard]] UPtr<Userdata> Userdata::createFullOwned(usize size) {
    if (size == 0) {
        throw std::invalid_argument("Userdata size cannot be zero");
    }
    return makeUnique<Userdata>(size);
}

// Runtime product path:
Userdata* ud = L->getGlobalState().getGC().create<Userdata>(sizeof(FileHandleData));
```

落地前 `ValueType` 与 `ValueVariant` 的关系隐含在顺序中：

```cpp
using ValueVariant = std::variant<std::monostate, bool, void*, LuaNumber,
                                  GCString*, Table*, Function*, Userdata*, Thread*>;
```

建议加 compile-time contract：

```cpp
static_assert(std::variant_size_v<Value::ValueVariant> == 9);
static_assert(static_cast<usize>(ValueType::Number) == 3);
static_assert(std::is_same_v<
    std::variant_alternative_t<static_cast<usize>(ValueType::Number), Value::ValueVariant>,
    LuaNumber>);
```

## Stdlib

### 发现

| 等级 | 位置 | 发现 | 建议 |
|---|---|---|---|
| P1 | `src/lib/stringlib.cpp` | pattern engine 内部仍大量使用 `const char*` 游标、原生数组 `MatchCapture capture[LUA_MAXCAPTURES]`、`nullptr` 表示匹配失败。外层已有 `PatternCursor` / `MatchResult`，但内部还是 C 风格核心。 | 分两层教学化：内部保持指针热路径，外层用 `PatternSlice` / `std::span<MatchCapture>` / `MatchResult` 包住；把 `-1` / `nullptr` 失败路径集中到 adapter。 |
| P1 | `src/lib/lib_registry.cpp` | `tryCreateClosure()` / `tryCreateLibTable()` 已用 `std::expected<..., LibRegistrationError>` 表达参数错误，旧 API 保持薄包装。 | 后续可让注册调用链消费 error，而不是静默 return。 |
| P2 | `src/lib/lib_catalog.hpp` | `LibCatalogEntry` 的 `id/name` 已改为 `StrView`，`findStandardLibrary()` 返回 optional reference。 | catalog 是非热路径，可继续作为 C++23 ranges 教学样例。 |
| P2 | `src/lib/iolib.cpp` | I/O open path 已增加 `tryFopen() -> std::expected<FILE*, FileOpenError>` adapter；底层仍用 `FILE*` 和 `unique_ptr<FILE, FileCloser>` 管理 C API 边界。 | 后续可把路径参数进一步统一为 `StrView` / `std::filesystem::path`。 |
| P3 | stdlib catalog / registration | `std::ranges::find_if` 已落地在 catalog lookup。 | `std::views::filter/transform` 更适合继续放到报告生成、trace filtering 等非热路径。 |

### 当前/落地前代码 vs 优化后代码

落地前 catalog lookup：

```cpp
const LibCatalogEntry* findStandardLibrary(StrView id) {
    const auto catalog = getStandardLibraryCatalog();
    const auto iter = std::find_if(catalog.begin(), catalog.end(), [id](const LibCatalogEntry& entry) {
        return StrView(entry.id) == id;
    });

    return iter == catalog.end() ? nullptr : &(*iter);
}
```

建议改为 optional reference，并顺手展示 ranges：

```cpp
struct LibCatalogEntry {
    StrView id;
    StrView name;
    LibOpenFunction open;
};

[[nodiscard]] Opt<std::reference_wrapper<const LibCatalogEntry>>
findStandardLibrary(StrView id) {
    auto catalog = getStandardLibraryCatalog();
    auto iter = std::ranges::find_if(catalog, [id](const LibCatalogEntry& entry) {
        return entry.id == id;
    });

    if (iter == catalog.end()) {
        return std::nullopt;
    }
    return std::cref(*iter);
}
```

当前 pattern capture 是原生数组：

```cpp
struct MatchState {
    const char* src_init;
    const char* src_end;
    const char* p_end;
    LuaState* L;
    i32 level;
    MatchCapture capture[LUA_MAXCAPTURES];
};
```

建议先替换固定容量表达：

```cpp
struct MatchState {
    StrView source;
    StrView pattern;
    LuaState* L = nullptr;
    i32 level = 0;
    std::array<MatchCapture, LUA_MAXCAPTURES> captures{};

    [[nodiscard]] std::span<MatchCapture> activeCaptures() noexcept {
        return std::span(captures).first(static_cast<usize>(level));
    }
};
```

## 工程化与质量门禁

### 已做得好的部分

- `tools/run_quality_gate.ps1` 顺序合理：format/tidy smoke、ValueResult variant-only、C-style guard、
  MSBuild、opcode executable contract、doc drift、unit tests。
- `check_doc_drift.ps1` 动态运行 `bin/lua_test.exe` 解析测试数量，避免 README/status 文档硬编码测试数漂移。
- `check_opcode_coverage_matrix.ps1` 能捕获 opcode 增删、重排、matrix 行缺失和测试注册名漂移。
- `check_value_result_variant_only.ps1` 能防止旧 `ValueResult` mirror 字段回流。
- `.github/workflows/ci.yml` 在 PR/push 上运行 MSBuild、doc drift、quality gate smoke 和单元测试。

### 漂移风险

| 等级 | 风险 | 说明 | 建议 |
|---|---|---|---|
| 已收口 | C-style 位置漂移 | `tools/c_style_allowlist.json` 记录 `rule/path/line/textHash/rationale`，产品严格规则不再使用数量门；同数量换位置由配置烟测锁住。 | tests/advisory 已建立显式存量；后续按目录递减 495 个手工所有权位置。 |
| 已收口 | opcode 测试引用漂移 | `opcode_coverage_contract.json` 为 38 条 opcode 记录 Positive / Boundary / Metamethod 测试 key，脚本通过 `lua_test.exe --list` 验证精确 ID 和源码路径。 | 若需要证明测试执行时真实命中对应 opcode，可再增加 handler hit bitmap；当前已证明测试注册和 metadata 契约。 |
| P2 | clang-tidy smoke 覆盖面窄 | 当前只跑少量 preferred files，且 CI 中 `run_quality_gate.ps1 -SkipClangTidy`。 | 增加 nightly/all scope；PR 中至少对 changed source 执行 clang-tidy，MSVC 环境不具备时明确降级。 |
| 已收口 | current 文档发现与活动事实 | doc drift 会从 front matter 自动发现 `status: current` 文档，检查全部本地 `verified_against` 路径；`live-facts` 区块校验动态测试摘要、已删除兼容面和依赖提交日期。 | 历史完成记录保持 `status: historical`，避免历史数字被误当活动事实。 |
| P3 | 质量门与覆盖率缺少量化覆盖 | README 有 coverage badge，但当前门禁没有 gcov/llvm-cov/OpenCppCoverage 产物。 | Windows 可接 OpenCppCoverage 或 VS coverage；先对 compiler/vm/lib 分模块输出函数/行覆盖趋势。 |

## 优先级路线图

1. **P1-A：位置化 C-style baseline**
   - 把 `tools/check_c_style_patterns.ps1` 从 count gate 升级为 location-aware baseline。
   - 增加 raw array、`char* end`、`const char* const[]`、测试目录裸 `new/delete` 的 warning 规则。
   - **完成记录：** 已迁移到 JSON 位置基线和显式更新命令；产品裸 `new` 清零，GC delete / userdata free 的 owner 边界有专门 rationale，tests/advisory 可递减。

2. **P1-B：VM opcode 单一事实源**
   - 让 opcode enum、metadata、handler table、switch dispatch、coverage matrix 共享同一 machine-readable contract。
   - 至少增加 `static_assert` 和脚本校验：metadata name/order/group/metamethod 与 matrix 一致。
   - **完成记录：** `kOpcodeMetadata`、`kSwitchHandlers`、handler table 和 matrix 已受 compile-time/test/script 契约约束；机器可读 sidecar 进一步锁住精确测试注册 ID、源码路径和 metamethod 适用性。

3. **P1-C：Runtime 所有权收口**
   - 将 `LuaState::newState()` 的裸 `new` 包装到 `create()` / `UPtr` 入口。
   - 将 `Userdata::createFull()` 改为 owned factory 或标为 legacy test helper。
   - 保留 GC `destroyObject()` 唯一裸 delete，但用位置 allowlist 明确这是 owner 边界。
   - **完成记录：** `LuaState::create()`、`Userdata::createFullOwned()` 已落地，GC `delete obj;` 是唯一 allowlisted destroy boundary。

4. **P2-A：Stdlib 错误通道现代化**
   - `LibRegistry`、`findStandardLibrary`、package dynamic lookup、I/O open path 分批用 `expected` / `optional reference`。
   - 不强迫 Lua C API 外观改变；让旧 API 做薄包装。
   - **完成记录：** `FunctionRegistrar::tryCreate*()`、`findStandardLibrary()` optional reference、`tryFopen()` expected adapter 和 package dynamic `tryLoadDynamic*()` lookup 已落地。

5. **P2-B：教学型 static_assert 契约**
   - `ValueType <-> ValueVariant`
   - `TMS <-> kMetamethodNames`
   - `OpCode <-> kOpcodeMetadata <-> HandlerTable`
   - `ExprVariant/StmtVariant <-> Visitor tests/docs`
   - **完成记录：** 上述四组已分别由 `static_assert`、单测或脚本锁定。

6. **P3：C++23 ranges 的克制引入**
   - 用在 catalog、coverage matrix generation、bytecode CFG、trace filtering。
   - 不在 VM 指令热循环和 string pattern inner loop 强行使用 ranges。
   - **完成记录：** catalog lookup 已使用 `std::ranges::find_if`；其余场景保留为后续非热路径扩展。

## 总体评价

如果目标是“现代 C++ 教学示范”，本项目最有价值的教学主线已经出现：`std::variant` 表达动态类型和编译结果，Concepts 守住 visitor 覆盖面，`std::expected` 让 public 边界可组合，RAII guard 为 GC 托管对象提供异常安全过渡，PowerShell 质量门把工程事实自动化。

下一阶段应避免只做语法糖式现代化。真正能把项目提升到标杆级的是：让每一个隐含约束都变成可编译、可测试、可脚本验证的 contract。也就是让学生不只看到“这里用了 C++23”，而是看到“为什么这里该用 C++23，以及它如何阻止真实工程漂移”。
