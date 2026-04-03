# VM Trace 调试系统设计与实施文档

> **目标**：为 C++20 Lua 解释器实现一套 JSON + 可视化的 VM 执行追踪系统，  
> 能够记录每条字节码指令的执行状态（PC、操作码、寄存器、局部变量、调用栈），  
> 输出为 JSONL 文件，并通过独立 HTML 页面可视化回放。

---

## 一、系统架构总览

```
┌──────────────────────────────────────────────────────────────┐
│                         VM 执行引擎                          │
│  vm.cpp :: executeProto()                                    │
│                                                              │
│  ┌────────────────────────────────────┐                      │
│  │  取指 → 解码 → [TraceSink Hook] → switch 执行            │
│  └────────────────────────────────────┘                      │
│                   │                                          │
│                   ▼                                          │
│          ITraceSink* (抽象接口)                              │
└──────────┬───────────────────────────────────────────────────┘
           │
    ┌──────┴──────┐
    ▼             ▼
┌────────┐  ┌──────────────┐
│ Null   │  │ JsonTrace    │
│ Sink   │  │ Sink         │
│(默认)  │  │(.jsonl 文件) │
└────────┘  └──────┬───────┘
                   │
                   ▼ 离线读取
            ┌──────────────┐
            │ HTML Viewer  │
            │ (静态页面)    │
            └──────────────┘
```

### 核心原则

1. **零侵入**：TraceSink 关闭时 VM 零开销（NullSink 内联空实现）
2. **解耦**：VM 只依赖 `ITraceSink` 抽象接口，不依赖 JSON 库或文件 I/O
3. **离线优先**：先做 JSONL 文件 + 静态 HTML Viewer；实时调试作为未来扩展
4. **浅序列化**：Table/Function/Userdata 只输出类型 + 指针 ID，不递归展开

---

## 二、文件结构规划

```
lua/src/debug/
├── trace_types.hpp          # 事件类型枚举、TraceEvent 结构体
├── trace_sink.hpp           # ITraceSink 抽象接口 + NullTraceSink
├── json_trace_sink.hpp      # JsonTraceSink 声明
├── json_trace_sink.cpp      # JsonTraceSink 实现（JSONL 输出）
├── value_serializer.hpp     # Value → JSON 字符串的序列化工具
└── value_serializer.cpp     # Value 序列化实现

lua/src/vm/vm.cpp            # [修改] 在主循环中插入 trace hook
lua/src/main.cpp             # [修改] 添加 --trace 命令行参数

lua/tools/trace_viewer.html  # 独立 HTML 可视化页面
```

---

## 三、分步实施计划

### Phase 1：Trace 基础设施（4 个文件）

#### 步骤 1.1：创建 `trace_types.hpp`

定义事件类型枚举和通用事件结构体。

```cpp
// 事件类型
enum class TraceEventKind : u8 {
    Instruction,    // 指令执行
    Call,           // 函数调用
    Return,         // 函数返回
    Error           // 运行时错误
};
```

不使用继承体系，只用一个扁平 struct `TraceEvent` 加 `kind` 标签。  
这样序列化时不需要 dynamic_cast，也方便后续扩展。

#### 步骤 1.2：创建 `trace_sink.hpp`

定义纯虚接口 `ITraceSink`：

```cpp
class ITraceSink {
public:
    virtual ~ITraceSink() = default;
    virtual void onInstruction(const TraceEvent& evt) = 0;
    virtual void onCall(const TraceEvent& evt) = 0;
    virtual void onReturn(const TraceEvent& evt) = 0;
    virtual void onError(const TraceEvent& evt) = 0;
    virtual void flush() = 0;
};
```

同文件提供 `NullTraceSink`（全部空实现），作为默认 sink。

#### 步骤 1.3：创建 `value_serializer.hpp / .cpp`

提供：
- `serializeValue(const Value& v)` → `Str`  
  返回 JSON 片段：`"nil"`, `true`, `3.14`, `"\"hello\""`, `"table:0x7f..."` 等
- `serializeRegisters(Value* base, i32 maxStack, Proto* proto, i32 pc)` → `Str`  
  返回 JSON 数组字符串 `[{"slot":0,"name":"x","value":42}, ...]`

序列化规则：
| Value 类型     | JSON 输出                      |
|----------------|-------------------------------|
| Nil            | `null`                        |
| Boolean        | `true` / `false`              |
| Number         | 数字字面量                     |
| String         | JSON 转义字符串               |
| Table          | `"table:0xABCD"`              |
| Function       | `"function:0xABCD"`           |
| Userdata       | `"userdata:0xABCD"`           |
| Thread         | `"thread:0xABCD"`             |
| LightUserdata  | `"lightuserdata:0xABCD"`      |

#### 步骤 1.4：创建 `json_trace_sink.hpp / .cpp`

`JsonTraceSink` 构造时接收输出文件路径，打开 `std::ofstream`。  
每次 `onInstruction / onCall / onReturn / onError` 时，序列化为一行 JSON 并写入文件。  
`flush()` 刷新文件缓冲区。

**JSONL 格式示例**（每行一个 JSON 对象）：

```jsonl
{"seq":0,"kind":"instruction","pc":0,"op":"LOADK","a":0,"bx":0,"line":1,"source":"test.lua","callDepth":1,"registers":[{"slot":0,"name":"x","value":null}]}
{"seq":1,"kind":"instruction","pc":1,"op":"LOADK","a":1,"bx":1,"line":2,"source":"test.lua","callDepth":1,"registers":[{"slot":0,"name":"x","value":42},{"slot":1,"name":"y","value":null}]}
{"seq":2,"kind":"call","funcName":"print","source":"test.lua","line":3,"callDepth":2,"args":[42]}
{"seq":3,"kind":"return","callDepth":2,"results":[]}
```

---

### Phase 2：VM 插桩

#### 步骤 2.1：修改 `vm.cpp` — 添加 trace hook

**插入位置**：`executeProto()` 主循环中，解码指令之后、`switch(op)` 之前。

```cpp
// 在 while (pc < code.size()) { 的开头，解码完 op/a/b/c/bx/sbx 后：
if (traceSink) {
    TraceEvent evt;
    evt.seq       = traceSeq++;
    evt.kind      = TraceEventKind::Instruction;
    evt.pc        = static_cast<i32>(pc - 1); // pc 已经 ++ 了
    evt.op        = op;
    evt.a = a; evt.b = b; evt.c = c;
    evt.bx = bx; evt.sbx = sbx;
    evt.line      = proto->getLine(pc - 1);
    evt.source    = proto->getSource() ? proto->getSource()->c_str() : "?";
    evt.callDepth = nexeccalls;
    evt.base      = base;
    evt.maxStack  = proto->getMaxStackSize();
    evt.proto     = proto;
    traceSink->onInstruction(evt);
}
```

**CALL/RETURN hook**：在现有的 `case OpCode::CALL` 和 `case OpCode::RETURN` 分支中，  
分别调用 `traceSink->onCall(evt)` 和 `traceSink->onReturn(evt)`。

#### 步骤 2.2：trace sink 传入方式

在 `VM` 命名空间中添加全局 trace sink 指针：

```cpp
// vm.cpp 顶部
static ITraceSink* g_traceSink = nullptr;

// 公开 API（vm.hpp）
void setTraceSink(ITraceSink* sink);
ITraceSink* getTraceSink();
```

这样 main.cpp 创建 `JsonTraceSink` 后通过 `VM::setTraceSink()` 注入即可。

---

### Phase 3：CLI 集成

#### 步骤 3.1：修改 `main.cpp` — 添加 `--trace` 参数

```
lua.exe --trace trace.jsonl script.lua
```

解析到 `--trace` 时：
1. 创建 `JsonTraceSink` 实例，打开输出文件
2. 调用 `VM::setTraceSink(&sink)`
3. 执行脚本
4. 执行完后 `sink.flush()`，自动析构关闭文件

同时在 `-h` 帮助中添加说明。

---

### Phase 4：HTML 可视化 Viewer

#### 步骤 4.1：创建 `trace_viewer.html`

单文件静态页面，不依赖任何外部库。包含 4 个面板：

```
┌─────────────────────────────────────────────────────┐
│ [打开 JSONL 文件]  [搜索] [过滤]                     │
├────────────────────┬────────────────────────────────┤
│                    │                                │
│  指令时间线         │  源码/字节码面板                │
│  (左侧列表)        │  (高亮当前行)                   │
│                    │                                │
│  seq | pc | op     │  1: local x = 42      ← 高亮  │
│  0   | 0  | LOADK  │  2: local y = 10              │
│  1   | 1  | LOADK  │  3: print(x + y)             │
│  2   | 2  | ADD    │                                │
│  ...               │                                │
├────────────────────┼────────────────────────────────┤
│                    │                                │
│  调用栈面板         │  寄存器/局部变量面板            │
│                    │                                │
│  [1] main chunk    │  R[0] x = 42                  │
│  [2] print         │  R[1] y = 10                  │
│                    │  R[2]   = 52                  │
│                    │                                │
└────────────────────┴────────────────────────────────┘
```

功能：
- 通过 `<input type="file">` 加载 .jsonl 文件
- 解析后构建事件数组
- 点击左侧指令 → 右侧联动显示对应源码行、寄存器状态、调用栈
- 支持键盘上下箭头单步浏览
- 支持按 opcode 名称过滤

---

## 四、JSON 事件字段完整定义

### 4.1 instruction 事件

| 字段       | 类型     | 说明                       |
|-----------|---------|---------------------------|
| seq       | number  | 全局递增序号                |
| kind      | string  | `"instruction"`            |
| pc        | number  | 程序计数器（0-based）       |
| op        | string  | 操作码名称（如 `"LOADK"`） |
| a         | number  | A 参数                     |
| b         | number  | B 参数                     |
| c         | number  | C 参数                     |
| bx        | number  | Bx 参数                    |
| sbx       | number  | sBx 参数                   |
| line      | number  | 对应源码行号               |
| source    | string  | 源文件名                   |
| callDepth | number  | 当前调用深度               |
| registers | array   | 寄存器快照（见 4.4）       |

### 4.2 call 事件

| 字段       | 类型     | 说明                       |
|-----------|---------|---------------------------|
| seq       | number  | 全局递增序号                |
| kind      | string  | `"call"`                   |
| funcName  | string  | 函数名（可能为 `"?"`）     |
| source    | string  | 源文件名                   |
| line      | number  | 调用所在行号               |
| callDepth | number  | 调用后的深度               |

### 4.3 return 事件

| 字段       | 类型     | 说明                       |
|-----------|---------|---------------------------|
| seq       | number  | 全局递增序号                |
| kind      | string  | `"return"`                 |
| callDepth | number  | 返回前的深度               |

### 4.4 registers 数组元素

| 字段   | 类型           | 说明                           |
|--------|---------------|-------------------------------|
| slot   | number        | 寄存器索引（0-based）          |
| name   | string\|null  | 局部变量名（如果有调试信息）    |
| value  | any           | 序列化后的值（见序列化规则）    |
| type   | string        | 值类型名（`"nil"`, `"number"` 等）|

---

## 五、性能与安全考量

1. **默认关闭**：`g_traceSink` 默认为 `nullptr`，判断开销仅为一次指针比较
2. **文件 I/O 缓冲**：`JsonTraceSink` 使用 `std::ofstream` 默认缓冲，避免每条指令刷盘
3. **内存安全**：序列化只读取 Value，不修改 VM 状态
4. **字符串转义**：JSON 字符串输出时转义 `"`, `\`, 控制字符，防止注入
5. **最大事件数**：可配置上限（默认 1000000），防止无限循环产生巨大文件

---

## 六、未来扩展方向

| 扩展              | 说明                                          |
|-------------------|----------------------------------------------|
| WebSocket 实时流   | 替换 `JsonTraceSink` 为 `WsTraceSink`        |
| 断点 + 单步        | 在 traceSink 中加入阻塞逻辑，配合 REPL 命令   |
| Time Travel Debug | 增加 `SnapshotSink`，记录完整 VM 状态快照     |
| VS Code 插件      | 实现 DAP 协议适配器，复用 trace 事件          |
| Trace 级别控制     | `--trace-level opcode|registers|full`        |
