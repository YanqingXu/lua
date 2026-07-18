# 审计结论

本次审计固定在 **`main@9e94e36879fd4d0f4329d83849bcd085fc0c6c43`**，不把未合入分支的代码算入结论。

这个仓库已经不是练习性质的 Lua 解释器，而是一个正在形成明确运行时合同的现代 C++23 Lua 5.1 实现：

* 编译器、寄存器 VM、GC、C API、标准库、协程和嵌入接口已经形成较完整闭环。
* Lua 5.1 的 38 个 opcode 已有执行路径，官方测试和差分测试覆盖也相当扎实。
* 当前只保留一个明确的 TestC 字节码差异 XFAIL，位于 `repeat-until` 条件生成。
* `EngineContext`、owner-thread、执行预算、取消、沙箱、allocator 失败原子性等设计，明显已经面向实际嵌入式运行时，而不是仅追求“能跑 Lua”。

我的总体判断是：

> **当前状态适合定位为高质量 Runtime Preview / Pre-production Embeddable Runtime，但还不应直接用于运行敌对或多租户游戏脚本。**

主线的最大风险已经不再是普通 Lua 语义差异，而是以下几类**不可信输入边界**：

1. 未验证二进制 chunk 直接进入 VM；
2. 脚本可控比较器直接交给 `std::sort`；
3. game-server profile 仍暴露运行期编译和共享 GC 控制；
4. 标准库长循环绕过 opcode 预算；
5. RNG、trace 等状态仍存在进程级共享；
6. Lua 数值到 C++ 整数的转换和资源上限没有统一治理；
7. allocator hard limit 尚未覆盖完整的 game-server 可达路径。

主观评分如下：

| 维度             |             评价 |
| -------------- | -------------: |
| Lua 5.1 语义与编译器 |       8.5 / 10 |
| 公共 C API 与 ABI |       8.5 / 10 |
| 测试与 CI 设计      |       8.5 / 10 |
| Context/生命周期架构 |       7.5 / 10 |
| 内存确定性          |       6.0 / 10 |
| 敌对脚本沙箱就绪度      |       5.0 / 10 |
| 综合             | **约 7.2 / 10** |

---

# 做得好的部分

## 1. Context 所有权模型已经建立

`EngineContext` 显式拥有 allocator、StringPool 和 GlobalState；服务访问会检查 owner thread，错误线程析构 Context 会直接终止，而不是悄悄产生竞态。

这是一条正确的路线。对于游戏服务器，推荐的并发模型通常不是“多个线程同时操作一个 Lua State”，而是：

* 一个 Context 固定绑定一个逻辑线程；
* 多个 Context 在不同线程并行；
* 跨线程只传消息或请求取消；
* 不共享 GC、Lua 栈和表对象。

当前实现基本沿着这个方向走。

## 2. 执行治理合同清晰

`ExecutionPolicy` 已经覆盖：

* VM instruction budget；
* monotonic deadline；
* external cancellation；
* per-drain finalizer budget。

每条 VM 指令前会依次检查取消、deadline 和指令预算。

而且文档没有过度承诺：明确指出 C/C++ callback 不能被 VM 抢占，必须自行调用协作式检查。

这比很多只提供 debug hook instruction count 的 Lua 沙箱更可靠。

## 3. 沙箱不只控制“库是否发布”，还控制操作时能力

`gameServer()` 默认只启用 Base、Math、String、Table、Coroutine 和 Package，并关闭文件系统、进程与动态原生模块。

能力检查不仅发生在打开标准库时，一些特权函数即使提前被脚本保存，策略收紧后调用仍会被拒绝。

这是正确的 capability-based sandbox 思路。

## 4. allocator 失败原子性投入很大

项目已经对 String、Table、Proto、栈、CallInfo、concat、sort、parser/AST、GC 工作队列等许多路径实施了 allocator 路由和 fail-on-N 测试。

特别值得肯定的是，文档明确承认：

> 当前 allocator-backed hard limit 仍然是 unsupported。

而不是把 GC managed-size、测试预算或部分 allocator ledger 误称为进程硬上限。

## 5. 增量 GC 的关键写屏障没有发现明显缺口

本次专门检查了运行时可变对象图的关键 setter：

* Function 的 upvalue 和 environment 修改会调用 `writeBarrier`；
* Userdata 的 metatable 和 environment 修改也会调用 `writeBarrier`。

因此当前不需要把“闭包环境/上值写屏障缺失”列为优先风险。

---

# P0：上线敌对脚本前必须解决

## P0-1：未验证二进制 chunk 可直接进入 VM，存在越界读写风险

这是当前最严重的问题。

`gameServer()` 同时开放 Base 和 String。

Base 库无条件发布：

* `loadstring`
* `load`
* `collectgarbage`

只有 `loadfile` 和 `dofile` 受 Filesystem capability 控制。

String 库又发布了二进制安全的：

* `string.char`
* `string.rep`
* `string.dump`

脚本可以自行构造包含任意字节和 NUL 的字符串。

二进制 loader 会读取 Proto 的：

* `maxStackSize`
* opcode 数组
* 常量数组
* 子 Proto
* debug metadata

并递归建立对象。

反序列化完成后直接创建可执行 Function，没有看到统一的 Proto/bytecode verifier。

问题在于 VM 栈帧只按照 `proto->getMaxStackSize()` 扩展：

而 opcode handler 直接信任 A/B/C 操作数，例如：

```cpp
context.base[a] = context.base[b];
context.base[a] = context.proto->getConstant(bx);
```

没有在 handler 入口检查寄存器是否小于 `maxStackSize`。

因此可以构造类似这样的不一致 Proto：

* `maxStackSize = 1`
* 某个 `MOVE` 的 `A = 200`
* 或 `LOADNIL A=0 B=255`
* 或 `SELF A=255`
* 或非法 jump/skip 目标

从静态控制流看，这形成了**可达的越界读写风险**。这不等同于我已经动态复现出任意代码执行，但已经足以列为 release blocker。

反序列化本身还没有统一的：

* 最大输入字节数；
* 最大 Proto 深度；
* 最大 Proto 总数；
* 最大 instruction/constant/debug entry 数；
* 最大字符串总字节数。

所以在抵达 VM 之前还存在栈溢出、CPU 和内存 DoS 风险。

### 修复顺序

第一层应立即止血：

```text
GameServerProfile:
  RuntimeCompilation = false
  BinaryChunks       = false
  GCControl          = false
```

game-server 模式默认：

* 不发布 `load`、`loadstring`；
* 或只允许 text-only；
* 拒绝首字节为 `0x1b` 的输入；
* 即使函数在策略收紧前被捕获，调用时仍重新检查 capability。

第二层实现有界 `ChunkReaderLimits`：

```cpp
struct ChunkReaderLimits {
    size_t maxInputBytes;
    size_t maxProtoDepth;
    size_t maxProtoCount;
    size_t maxInstructionCount;
    size_t maxConstantCount;
    size_t maxStringBytes;
    size_t maxDebugEntries;
};
```

第三层在创建 Function 前执行统一 `BytecodeVerifier`，至少验证：

* opcode 范围；
* A/B/C/RK 寄存器与常量索引；
* 所有寄存器访问均被 `maxStackSize` 覆盖；
* `LOADNIL`、`CALL`、`RETURN`、`VARARG` 的寄存器区间；
* jump/skip 目标在 code 范围内；
* `SETLIST C==0` 后续扩展字存在；
* `CLOSURE` 子 Proto 索引以及后续 upvalue 伪指令；
* upvalue index；
* arithmetic overflow；
* `numParams <= maxStackSize`；
* local variable 和 line metadata 范围。

验证器不能分散到各 opcode handler 中。handler 的职责应是执行**已经验证过的 Proto**，而不是一边执行一边尝试补洞。

---

## P0-2：脚本比较器直接传给 `std::sort`，违反 C++ 算法前置条件

`table.sort` 将数组复制进 `LuaVector` 后，直接调用：

```cpp
std::sort(arr.begin(), arr.end(), [&](const Value& left, const Value& right) {
    return comparator != nullptr
        ? callSortComparator(...)
        : defaultSortLess(...);
});
```

但是 Lua 脚本可以提供任意比较器：

```lua
table.sort(t, function(a, b)
    return true
end)
```

或者：

```lua
local flip = false
table.sort(t, function(a, b)
    flip = not flip
    return flip
end)
```

`std::sort` 要求 comparator 满足 strict weak ordering。脚本比较器不满足该前置条件时，C++ 标准不再保证算法行为；某些标准库实现中的 unguarded partition 可能越过边界，而不只是“排序结果错误”。

这条路径在 game-server profile 中默认开放，因此应与二进制 verifier 一起作为 P0 处理。

### 建议

不要把不可信 Lua comparator 直接传给 `std::sort`。

可选方案：

1. 实现 Lua 5.1 风格的有边界排序算法；
2. 每次 partition 明确检查索引；
3. 检测明显矛盾：

   * `comp(a, a)` 必须为 false；
   * `comp(a, b)` 和 `comp(b, a)` 不可同时为 true；
4. 达到异常状态时返回稳定的：

   * `invalid order function for sorting`；
5. 每固定数量比较调用一次 execution poll；
6. 设置最大排序元素数和最大 comparator 调用次数。

应补充以下 sanitizer 回归：

```lua
table.sort(t, function() return true end)
table.sort(t, function() return math.random() > 0.5 end)
table.sort(t, function(a, b) return a == b end)
```

---

## P0-3：game-server profile 仍允许脚本运行期编译

当前 `gameServer()` 对 Base 整体只做库级 bit 控制，没有更细的 RuntimeCompilation capability。

Base 又无条件发布 `load` 和 `loadstring`。

这意味着不可信脚本可以在运行期反复：

* 构造大源码；
* 触发 lexer/parser/AST/codegen；
* 创建大量 Proto；
* 通过 reader 回调分片聚合源码；
* 制造深层嵌套和大量局部符号。

Parser 已经有部分语法深度、locals/upvalues 边界，这是积极因素；但没有统一的编译配额对象。当前 `ParserOptions` 主要还是解析行为选项，并不是资源预算。

内存合同也明确指出，AST/codegen 和标准库部分临时 `Str`/`Vec` 仍可能绕过 `lua_Alloc`。

### 建议

生产游戏服务器默认采用：

```text
部署阶段：源码 -> 独立编译器进程 -> 已验证 artifact
运行阶段：只装载已验证 artifact，不在逻辑线程编译源码
```

增加 `CompilationPolicy`：

```cpp
struct CompilationPolicy {
    size_t maxSourceBytes;
    size_t maxReaderPieces;
    size_t maxTokens;
    size_t maxAstNodes;
    size_t maxFunctions;
    size_t maxConstants;
    size_t maxInstructions;
    size_t maxStringBytes;
    size_t maxNesting;
    Clock::time_point deadline;
};
```

每个增长点必须在 append/allocate **之前**检查，而不是在完整编译后统计。

---

## P0-4：脚本可以停止共享 GC，且标准库长任务绕过 opcode 预算

Base 库无条件发布 `collectgarbage`。

它支持：

* `collect`
* `stop`
* `restart`
* `step`
* `strategy`
* `setpause`
* `setstepmul`

而 `stop` 会直接停止整个 Context 的自动 GC，`strategy` 还允许脚本切换项目级 GC 策略。

在一个 Context 对应一个房间、地图或逻辑域时，这仍是共享运行时控制权，不应默认交给普通脚本。尤其当前 hard memory limit 尚未闭环，脚本停止 GC 后可以持续扩大 process RSS。

另一个问题是 instruction budget 只在 VM opcode dispatch 时扣费。项目文档明确承认，进入一次 C/C++ 函数后，如果函数不主动 poll，VM 无法抢占。

因此下面这些操作可能只消耗一条 Lua `CALL`，然后在 C++ 中运行很久：

* `collectgarbage("collect")`
* `table.sort`
* `table.concat`
* `string.rep`
* `string.find/match/gsub`
* `load/loadstring`

例如 `table.concat` 会在 C++ 循环里扫描整个范围并持续追加结果。

### 建议

新增细粒度 capability：

```cpp
enum class SandboxCapability {
    Filesystem,
    Process,
    NativeModules,
    RuntimeCompilation,
    BinaryChunks,
    GCControl,
};
```

game-server 默认：

* 隐藏 `collectgarbage`，或只提供只读 `count`；
* 禁止 stop/restart/strategy/setpause/setstepmul；
* full collect 只能由宿主管理。

另外新增标准库 work budget。不要试图把所有 C++ 工作强行换算成 VM opcode，可以独立定义：

```cpp
executionPolicy.consumeNativeWork(units);
executionPolicy.pollStop();
```

在有界切片之间调用，例如：

* 每 256 个字符串字符；
* 每 128 次 pattern step；
* 每 64 次 sort comparison；
* 每 256 个表元素；
* 每个 parser token 或固定字节数。

---

## P0-5：`math.random/randomseed` 是进程级状态，不是 Context 级状态

当前实现使用：

* function-static `seeded`；
* `std::srand(std::time(nullptr))`；
* `std::rand()`；
* `std::srand(seed)`。

这会产生四个问题：

1. 一个 Context 调用 `math.randomseed` 会影响所有 Context；
2. 多个 Context 在不同线程执行时，静态播种状态和 C RNG 形成数据竞争或实现相关同步；
3. 无法保证战斗回放、锁步模拟和故障重演的确定性；
4. `% range` 存在 modulo bias。

`gameServer()` 默认启用 Math，所以该问题直接可达。

### 建议

把 RNG 放入 `EngineContext`：

```cpp
class RuntimeRandom {
public:
    void seed(uint64_t seed);
    uint64_t nextU64();
    uint64_t bounded(uint64_t bound); // rejection sampling
};
```

服务器模式：

* 不使用墙钟自动播种；
* seed 由宿主明确注入；
* RNG 状态可序列化进快照；
* 每个房间/实体域可选择独立 stream；
* `randomseed` 只影响当前 Context；
* 使用 rejection sampling 消除范围偏差。

补充：

* 两个 Context 相同 seed 输出相同；
* 不同 seed 相互独立；
* 一个 Context 调用 randomseed 不影响另一个；
* TSan 下并行运行不同 Context；
* snapshot/restore 后序列一致。

---

# P1：紧随 P0 的架构与正确性工作

## 1. 统一 LuaNumber → 整数转换

代码中多处直接：

```cpp
static_cast<i32>(luaNumber)
```

例如 String 库的 `sub`、`rep`、`byte`、`char`。

Table 库也在参数处理中直接转换。

Base 的 `select`、`ipairs` 等路径同样如此。

对 NaN、±Inf 或超出目标整数范围的浮点值执行浮点到整数转换，会落入未定义或实现相关行为，不应让脚本输入直接抵达这里。

项目在 `collectgarbage` 参数上已经实现了正确范式：

* 检查 finite；
* trunc；
* 检查目标整数范围；
* 最后再转换。

应把它抽成全局工具：

```cpp
enum class IntegerConversion {
    Truncate,
    Exact,
};

Expected<i32, LuaArgumentError>
checkedLuaInteger(LuaNumber value, IntegerConversion mode);
```

测试矩阵至少包含：

* NaN；
* ±Inf；
* ±0；
* INT32_MIN/MAX；
* 边界前后的 `nextafter`；
* 极大指数；
* 数字字符串的相同输入。

---

## 2. 建立统一 ResourcePolicy，而不是零散长度检查

目前 `LUA_MAX_STRING_LENGTH` 只在 VM concat 路径看到明确检查。

但 `GCString` 构造函数只检查 `usize::max()`，没有统一的项目字符串上限。

StringPool 的 `intern` 也没有统一长度门禁。

`string.rep` 则使用默认 `Str`：

```cpp
result.reserve(len * n);
```

这里同时存在：

* 乘法溢出；
* 超大分配；
* 绕过 `lua_Alloc`；
* 单次 C++ 长循环。

建议把资源上限变成 Context 配置，而不是散落宏：

```cpp
struct ResourcePolicy {
    size_t maxStringBytes;
    size_t maxTableArraySlots;
    size_t maxTableHashEntries;
    size_t maxStackSlots;
    size_t maxReturnValues;
    size_t maxSortElements;
    size_t maxPatternSteps;
    size_t maxSourceBytes;
    size_t maxProtoBytes;
};
```

`StringPool::intern` 应作为最后一道全局不变量门禁；各 builder 再做提前失败，避免先构造巨型临时缓冲。

---

## 3. Table 的数组/哈希策略需要重做

当前正整数键在 `1..1,000,000` 之间都会被判定为 array index。

写入 array index 时，vector 会直接 resize 到目标位置。

因此：

```lua
local t = {}
t[1000000] = true
```

可能为一个元素创建百万槽位的稠密数组。脚本创建很多此类表时会形成明显的内存放大。

另一个语义问题是 `next`：

* 如果当前 hash key 已被删除；
* 实现会从 `hash_.begin()` 重新开始；
* 已经访问过的条目可能再次返回。

这与 Lua 的 dead-key 设计并不等价，并可能导致删除当前键时重复遍历。头文件又说“遍历过程中修改表行为未定义”，与实现注释声称支持删除当前键存在契约冲突。

此外，`getSize()` 对 `unordered_map` 内存只是估算，没有计入真实 bucket/node 开销。

### 推荐路线

短期：

* 基于密度决定是否扩 array；
* 对 array growth 做 ResourcePolicy 检查；
* 稀疏整数键进入 hash；
* 增加“删除当前 hash key 后继续 next”的差分测试。

中期：

* 替换 `unordered_map`；
* 实现 Lua 风格 node array/open addressing；
* 引入 dead-key tombstone；
* 统一 rehash；
* 让表内存能精确 allocator 计账；
* 改善 cache locality 和迭代行为。

这会是 P0 安全边界完成后，收益最高的数据结构重构。

---

## 4. Trace 状态仍是进程级全局变量

当前 trace sink、sequence 和调试 flag 是 namespace global：

```cpp
ITraceSink* g_traceSink;
u64 g_traceSeq;
bool g_dumpBytecode;
bool g_traceDiffEnabled;
```

setter/getter 也没有同步。

因为不同 `EngineContext` 可以分别绑定不同线程，这会造成：

* 跨 Context trace 污染；
* sequence 混用；
* 数据竞争；
* sink 生命周期竞态；
* 调试时意外访问另一个 Context 的寄存器。

应把它变成：

```cpp
EngineContext
  └── TraceRuntime
        ├── sink
        ├── sequence
        ├── dumpBytecode
        └── diffEnabled
```

并提供 allocator-aware、有容量上限的 ring buffer sink。

---

## 5. CancellationHandle 有生命周期 UAF 合同风险

`ExecutionCancellationHandle` 内部只是一个非 owning 的：

```cpp
std::atomic<bool>* requested_;
```

文档要求 handle 不得超过 Context 生命周期。

这在实际服务器中很难始终可靠：

* 定时器；
* 网络断线回调；
* actor mailbox；
* job cancellation；
* Context 已销毁但异步事件稍后到达。

更安全的设计是让 handle 持有一个独立的共享取消状态，或 weak/generation token：

```cpp
struct CancellationState {
    std::atomic<bool> requested;
    std::atomic<bool> alive;
};
```

Context 销毁后，迟到的 cancel 变成 no-op，而不是访问已释放的原子变量。

---

## 6. 错误线程关闭的可观测性不足

当前合同是：

* foreign-thread `lua_close` 不执行销毁；
* owner thread 必须重试；
* Context 在错误线程析构会 `std::terminate`。

严格 owner-thread 是正确选择，但公开的 `lua_close` 返回 `void`，宿主无法直接知道关闭失败。

建议保留兼容 `lua_close`，同时增加项目扩展：

```c
int lua_tryclose(lua_State* L);
// LUA_OK
// LUA_ERRTHREAD
// LUA_ERRBUSY
```

并提供 owner-thread teardown queue，避免服务关闭阶段发生静默泄漏或 terminate。

---

## 7. allocator hard limit 应按 game-server 可达路径收口

当前文档明确列出仍可能绕过 callback 的部分：

* AST/codegen 临时对象；
* 其余标准库；
* debug/trace；
* I/O/package；
* NativeModuleRegistry/OS loader。

下一步不需要一口气宣称“限制所有进程内存”，应先定义一个更可验证的目标：

> `GameServerProfile allocator hard limit = supported`

优先补齐从该 profile 可达的：

1. load/loadstring/compiler；
2. String/Table/Math/Coroutine/Package 临时对象；
3. trace 和错误诊断；
4. finalizer 与 userdata；
5. table hash 和稀疏数组增长。

Native loader 和 OS loader 可以继续明确归类为宿主/进程资源。

当前完成标准要求 Windows/Linux Debug/Release、ASan、UBSan 通过同一矩阵。

但现有 CI 在 Windows 和 sanitizer 作业中都设置了：

```yaml
LUA_TEST_DISABLE_MEMORY_LIMIT: 1
```

这正好避开了 allocator/失败原子性最敏感的组合，应增加独立的 allocator-failure CI lane，而不是在主要平台长期关闭。

---

## 8. 保留的 singleton 入口应逐步淘汰

`RuntimeServices::fromSingletons()` 仍然存在。

同时 `VM::call(LuaState*, ...)` 和 `VM::execute(LuaState*, ...)` 的无 services 重载会获取 singleton services。

对于 isolated Context，这类接口容易把：

* LuaState 属于 Context A；
* RuntimeServices 却来自进程 singleton；

混在同一调用中。

建议：

* production build 将 singleton 入口标记 deprecated；
* 内部代码全部显式传 `RuntimeServices&`；
* singleton 只留给 CLI 或兼容测试；
* Debug 构建断言 `&L->getGlobalState() == &services.globalState`。

---

## 9. 原生模块 loader 需要更严格的宿主策略

game-server profile 已经关闭 NativeModules，这是正确的。但 unrestricted 模式下，动态库路径、ABI、签名和缓存仍应加强。

推荐：

* canonical absolute path；
* 默认拒绝相对路径；
* Windows 使用安全的 `LoadLibraryExW` flags；
* module allowlist；
* ABI version handshake；
* 可选文件摘要或签名；
* 将 loader 视为宿主可信能力，而不是普通脚本库功能。

---

# 测试与工程流程建议

当前 CI 的广度很好：

* Windows Debug/Release；
* GCC/Clang Debug/Release；
* Lua 5.1 差分；
* official strict；
* ASan/UBSan；
* benchmark regression；
* clang-format/clang-tidy。

但还缺少与当前风险模型直接对应的门禁：

## 应新增

### 1. libFuzzer targets

至少四个：

```text
fuzz_undump
fuzz_bytecode_verifier
fuzz_parser
fuzz_stdlib_numeric_arguments
```

`fuzz_undump` 的语料应包括：

* 合法 `string.dump` 输出；
* 手工构造重复常量 chunk；
* 官方 Lua 5.1 chunk；
* 截断 header；
* 随机 opcode；
* maxStack 与寄存器不一致；
* 超深子 Proto；
* 非法 closure upvalue 指令。

现有测试已经具备手工构造 binary chunk 的辅助代码，可直接作为初始 corpus。

### 2. TSan

重点检查：

* 多 Context 并行 RNG；
* global trace；
* cancellation teardown；
* allocator 全局 bookkeeping；
* dynamic module cache。

### 3. UBSan 参数矩阵

对所有标准库整数参数自动喂入：

```text
NaN
+Inf
-Inf
DBL_MAX
-DBL_MAX
INT32_MAX ± epsilon
INT32_MIN ± epsilon
-0
```

### 4. ARM64 和 macOS

二进制 chunk 当前包含：

* endian；
* `sizeof(i32)`；
* `sizeof(usize)`；
* `sizeof(Instruction)`；
* `sizeof(LuaNumber)`。

跨平台 artifact 行为应有 CI 证据，而不是只依赖 x64 Windows/Linux。

### 5. Coverage

使用 llvm-cov，至少分别统计：

* parser/codegen；
* opcode handlers；
* GC phase transitions；
* C API；
* binary verifier；
* sandbox denied paths。

### 6. pin GitHub Actions

当前 workflow 使用 `actions/checkout@v4`、`upload-artifact@v4` 等可变 major tag。

对发布型 runtime，建议固定到 commit SHA，并由 Dependabot/Renovate 提交更新。

---

# PR 和评审流程

最近的主线大 PR 包含 41 个 commits、83 个文件和超过一万行新增，并且没有请求 reviewer。

这种 PR 对 VM/GC/allocator 的 invariant review 太大。未来应按“一条运行时不变量一个 PR”拆分，例如：

* verifier 只验证，不改 handler；
* handler bounds assertion 单独 PR；
* RNG Context 化单独 PR；
* checked integer conversion 单独 PR；
* table layout 单独设计文档和 PR；
* allocator 路由按标准库模块拆分。

仓库已有 Issue 记录 branch protection 因 private plan 限制而无法配置。

在平台规则可用前，建议至少采用：

* CODEOWNERS；
* PR 模板中的 invariant checklist；
* 禁止直接 push main 的团队约定；
* 每个 PR 附同 SHA 的 CI evidence；
* release tag 只从全绿 SHA 创建；
* 安全边界改动至少一名独立 reviewer。

README 记录的本地测试数据很强，但当前主线 SHA 的在线完整矩阵当时仍待验证。
因此发布判断应依赖**同一 commit SHA** 的测试、差分、benchmark、sanitizer 和 artifact，而不是上一基线的绿灯。

---

# 推荐开发路线图

## Milestone 1：Security Boundary

目标：让 `gameServer()` 可以真实代表“不可信游戏逻辑边界”。

交付项：

1. 新增 `RuntimeCompilation`、`BinaryChunks`、`GCControl` capability；
2. game-server 默认 text-disabled 或 text-only；
3. 禁止脚本控制共享 GC；
4. 替换 `std::sort` 脚本 comparator 路径；
5. RNG Context 化；
6. checked LuaNumber → integer；
7. 全局 string/output/source 上限；
8. cancellation handle 生命周期安全。

验收条件：

* game-server 脚本无法通过 `string.char` 构造 chunk 并执行；
* 捕获旧的 `loadstring` 后收紧策略仍然拒绝；
* 恶意 sort comparator 不触发 sanitizer；
* 一个 Context 的 randomseed 不影响另一个；
* NaN/Inf/越界参数不触发 UBSan；
* 脚本不能 stop/reconfigure Context GC；
* 当前 SHA 的完整 CI 全绿。

## Milestone 2：Verified Runtime

目标：支持受控的二进制 artifact 和有界运行期编译。

交付项：

1. `ChunkReaderLimits`；
2. 统一 `BytecodeVerifier`；
3. `CompilationPolicy`；
4. 标准库 native-work budget；
5. game-server 可达 allocator 路径闭环；
6. trace Context 化；
7. undump/parser/stdlib fuzz；
8. TSan。

验收条件：

* 任何 malformed Proto 都不能进入 VM；
* verifier 对所有 opcode 和伪指令有覆盖；
* fuzz 持续运行无 crash、OOB、leak、hang；
* built-in 标准库的最大取消延迟有明确上限；
* 明确定义的 GameServerProfile 可以支持 allocator hard limit；
* 仍不把 OS loader 和整个进程 RSS 误称为 Lua hard limit。

## Milestone 3：Compatibility & Embedding 1.0

目标：完成表结构、最后兼容差异和发布工程。

交付项：

1. Lua 风格 table node/dead-key/rehash；
2. 关闭 `repeat-until` TestC XFAIL；
3. 原生模块安全策略；
4. ARM64/macOS；
5. coverage 与长时间 soak；
6. release artifacts、ABI/version policy；
7. 明确的 owner-thread close API。

当前唯一已登记的兼容 XFAIL 就是 `repeat-until` 条件字节码差异，应在 1.0 前消除。

---

# 建议立即创建的 Issue 队列

按顺序：

1. **`[security] Make GameServerProfile text-disabled/text-only`**
2. **`[security] Add bounded binary ChunkReader and Proto verifier`**
3. **`[security] Replace std::sort for script-controlled comparators`**
4. **`[sandbox] Add RuntimeCompilation, BinaryChunks and GCControl capabilities`**
5. **`[runtime] Move math RNG into EngineContext`**
6. **`[runtime] Add checked LuaNumber-to-integer conversion`**
7. **`[runtime] Enforce canonical source/string/output/work limits`**
8. **`[runtime] Make cancellation handle teardown-safe`**
9. **`[runtime] Move trace state into EngineContext`**
10. **`[table] Remove fixed sparse-array cutoff and implement dead-key traversal`**
11. **`[memory] Close allocator gaps reachable from GameServerProfile`**
12. **`[compat] Close repeat-until TestC bytecode XFAIL`**
13. **`[ci] Add fuzzing, TSan, ARM64, macOS and coverage`**
14. **`[loader] Harden native module path and ABI policy`**
15. **`[api] Add explicit owner-thread close status`**

---

# 第一批 PR 的最佳拆法

不要第一步就重构整个 VM。第一个 PR 应尽可能小，只建立安全边界：

### PR 1：game-server capability hardening

修改：

```cpp
enum class SandboxCapability {
    Filesystem,
    Process,
    NativeModules,
    RuntimeCompilation,
    BinaryChunks,
    GCControl,
};
```

行为：

* `gameServer()` 三项新增 capability 全部关闭；
* Base 注册时不发布 `load`、`loadstring`、`collectgarbage`；
* 已捕获函数在调用时重新检查 capability；
* unrestricted profile 保持 Lua 5.1 兼容；
* 暂不实现 verifier，只先阻断不可信入口。

测试：

* game-server 中全局函数不存在；
* 策略收紧前捕获的函数调用仍被拒绝；
* `string.char` 拼接的二进制 chunk 被拒绝；
* unrestricted 中 `string.dump/loadstring` round-trip 继续通过；
* official strict 不回退。

### PR 2：per-Context RNG

独立完成，不与 verifier 或 table 重构混在一起。

### PR 3：bounded reader + verifier

先引入 verifier 和测试，但不修改 opcode handler 的正常热路径；Debug 模式可以额外保留断言。

### PR 4：safe table.sort

替换 `std::sort`，加入恶意 comparator sanitizer corpus 和 native-work polling。

---

# 暂时不建议投入的方向

在上述边界完成前，不建议优先做：

* JIT；
* NaN boxing；
* Lua 5.2/5.4 新语法；
* 多线程共享同一个 LuaState；
* 更多标准库；
* 大规模“现代 C++ 化”重构；
* REPL 界面增强；
* opcode 微优化。

目前最有价值的不是把单线程 benchmark 再提高几个百分点，而是把以下链路闭合：

```text
不可信输入
  → 有界解析
  → 验证后的 Proto
  → Context 隔离
  → 有界标准库工作
  → 可取消执行
  → 可证明的内存上限
  → 同 SHA 质量证据
```

一旦这条链路完成，这个项目就有条件从“功能完整的 Lua 5.1 C++ Runtime”升级为真正适合游戏服务器的**可治理脚本执行引擎**。

本次属于源码、历史、Issue、PR、测试配置与 CI 合同的静态深度审计；当前执行环境没有可用的本地 `gh` 客户端，因此没有动态构造恶意 chunk、运行 sanitizer 或 fuzz 来验证具体崩溃形态。二进制 chunk 和恶意 sort comparator 的结论基于可达代码路径与 C++ 前置条件，建议将动态 PoC 纳入对应 P0 Issue 的第一项验收工作。
