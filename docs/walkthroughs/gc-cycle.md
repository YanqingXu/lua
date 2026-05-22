---
status: current
verified_against: bin/lua_app.exe; bin/lua_bytecode.exe; src/lib/baselib.cpp; src/lib/iolib.cpp; src/gc/garbage_collector.cpp; src/gc/gc_mark.cpp; src/gc/gc_weak.cpp; src/gc/gc_finalize.cpp; src/gc/gc_sweep.cpp; src/core/table.cpp; src/core/userdata.cpp; src/vm/state/global_state.cpp; tests/unit/gc/test_gc.cpp
last_checked: 2026-05-23
applies_to: full mark-sweep collection, weak tables, userdata finalizers, and collectgarbage("collect")
---

# GC Cycle End-to-End

这篇 walkthrough 追踪一个同时触发 weak table 和 userdata `__gc` 的最小脚本：

```lua
local weak = setmetatable({}, { __mode = "v" })
local f = io.tmpfile()
weak.handle = f
print("before", weak.handle ~= nil)
f = nil
collectgarbage("collect")
print("after first", weak.handle ~= nil)
collectgarbage("collect")
print("after second", weak.handle == nil)
```

它的关键现象是：第一次 `collectgarbage("collect")` 会发现 `io.tmpfile()` 返回的 file userdata 已经不可达，但因为它有 `__gc`，GC 会先把它复活一轮并运行 finalizer；弱表里的值因此还没有被清掉。第二次完整 GC 时，这个 userdata 已经带着 `FINALIZED` 标记，不会再排队终结，于是 weak value 被移除，sweep 真正释放对象。

## 可复现命令

```powershell
$tmp = Join-Path $env:TEMP 'gc_cycle_walkthrough.lua'
@'
local weak = setmetatable({}, { __mode = "v" })
local f = io.tmpfile()
weak.handle = f
print("before", weak.handle ~= nil)
f = nil
collectgarbage("collect")
print("after first", weak.handle ~= nil)
collectgarbage("collect")
print("after second", weak.handle == nil)
'@ | Set-Content -LiteralPath $tmp -NoNewline -Encoding UTF8
.\bin\lua_bytecode.exe $tmp full
.\bin\lua_app.exe $tmp
```

`lua_app` 输出：

```text
before	true
after first	true
after second	true
```

最后一行的 `true` 表示 `weak.handle == nil` 成立。

`lua_bytecode full` 输出 57 条顶层指令。这里摘出与 GC 行为直接相关的部分：

```text
0000 | line 1 | GETGLOBAL | A=0 Bx=0 ; K[0] = string "setmetatable"
0002 | line 1 | NEWTABLE | A=2 B=0 C=0
0003 | line 1 | NEWTABLE | A=3 B=0 C=1
0004 | line 1 | SETTABLE | A=3 B=257 C=258 ; B=K[1] = string "__mode"; C=K[2] = string "v"
0005 | line 1 | CALL | A=1 B=3 C=2
0006 | line 1 | MOVE | A=0 B=1 C=0
0007 | line 2 | GETGLOBAL | A=1 Bx=3 ; K[3] = string "io"
0008 | line 2 | GETTABLE | A=2 B=1 C=260 ; C=K[4] = string "tmpfile"
0010 | line 2 | CALL | A=3 B=1 C=2
0011 | line 2 | MOVE | A=1 B=3 C=0
0012 | line 3 | SETTABLE | A=0 B=261 C=1 ; B=K[5] = string "handle"
0024 | line 5 | LOADNIL | A=1 B=1 C=0
0025 | line 6 | GETGLOBAL | A=2 Bx=8 ; K[8] = string "collectgarbage"
0028 | line 6 | CALL | A=3 B=2 C=1
0040 | line 8 | GETGLOBAL | A=2 Bx=8 ; K[8] = string "collectgarbage"
0043 | line 8 | CALL | A=3 B=2 C=1
```

读这段输出时先抓住四件事：

1. `setmetatable({}, { __mode = "v" })` 构造了一个弱值表。
2. `io.tmpfile()` 创建 file userdata，它的元表里有 `__gc`。
3. `weak.handle = f` 只把 userdata 放进弱值槽。
4. `f = nil` 让局部变量不再强引用 userdata；后面的两个 `collectgarbage("collect")` 观察两轮完整回收。

这个示例故意不把 `f` 传给额外函数做类型打印。当前 `markState()` 会扫描活动调用帧的寄存器范围；把 userdata 临时传给函数可能让旧临时寄存器延长它的可达性，从而遮住 weak table / finalizer 本身的行为。

## 1. 标准库入口：`collectgarbage("collect")`

`collectgarbage` 在基础库中注册，执行入口是 `luaB_collectgarbage()`，见 `src/lib/baselib.cpp:1158`。当参数是 `"collect"` 时，它调用：

```text
gc.collect(L)
```

这里的 `L` 很重要。带 `LuaState*` 的入口会把当前线程栈、主线程栈、registry、基础类型元表、元方法名称和 running thread 都纳入根集；这条路径由 `GarbageCollector::collect(LuaState*)` 转到 `collect(StringPool&, LuaState*)`，见 `src/gc/garbage_collector.cpp:181` 和 `src/gc/garbage_collector.cpp:185`。

当前完整 GC 的顺序是：

```text
mark(currentState)
prepareFinalizers()
propagateMarks()
clearWeakTableEntries()
sweep(stringPool)
runFinalizers(currentState)
```

这个顺序在 `src/gc/garbage_collector.cpp:185` 的实现里直接展开。

## 2. 例子里的对象图

执行到 `f = nil` 后，局部寄存器的关键状态可以理解成：

```text
R0 = weak table
R1 = nil

weak table
  metatable.__mode = "v"
  ["handle"] = file userdata    -- weak value

file userdata
  metatable.__gc = io_gc
```

`io.tmpfile()` 返回的 file userdata 来自 `createFileHandle()`，见 `src/lib/iolib.cpp:269`。文件元表的 `__gc` 在 `IOLibModule::registerFunctions()` 中注册，见 `src/lib/iolib.cpp:969` 和 `src/lib/iolib.cpp:973`；实际 finalizer 是 `io_gc()`，见 `src/lib/iolib.cpp:894`，它会关闭还没关闭的文件句柄。

所以这不是普通 table 或 string，而是“可终结的 userdata”。这就是为什么第一次完整 GC 不能直接 sweep 掉它。

## 3. Mark：根集和弱表

`GarbageCollector::mark(LuaState*)` 从 `src/gc/gc_mark.cpp:41` 开始。它先把所有对象重新设为白色，清空灰色列表和本轮 weak table 列表，然后标记根：

| 根来源 | 相关实现 |
|---|---|
| 显式 `roots_` | `src/gc/gc_mark.cpp:63` |
| registry、memerrmsg、元方法名称、基础类型元表 | `src/vm/state/global_state.cpp:95` |
| 当前 LuaState 栈、CallInfo 范围、open upvalue、debug hook | `src/gc/gc_mark.cpp:127` |
| main thread / running thread | `src/vm/state/global_state.cpp:108` |

当标记传播遇到 `weak` 表时，会调用 `Table::mark()`，见 `src/core/table.cpp:209`，再进入 `GarbageCollector::markTable()`，见 `src/gc/gc_weak.cpp:14`。`markTable()` 读取元表中的 `__mode`：

```text
__mode contains "v"
  -> weakValues = true
  -> table marked with WEAKVALUE
  -> table recorded in weakTables_
```

接着 `Table::markContents()` 会跳过弱值，见 `src/core/table.cpp:212`。这意味着 `weak.handle` 指向的 file userdata 不会因为弱表而变黑。到第一次 mark 结束时，它仍然是白色。

## 4. Prepare Finalizers：第一次回收为什么还没清 weak value

普通白色对象会在 sweep 阶段被释放，但带 `__gc` 的 userdata 先走 `prepareFinalizers()`，见 `src/gc/gc_finalize.cpp:28`。

这个函数扫描 `allObjects_`，寻找满足这些条件的对象：

```text
type == Userdata
color == White
not FIXED
not FINALIZED
has __gc finalizer
```

我们的 file userdata 正好满足条件。于是 GC 做三件事：

```text
set FINALIZED bit
push userdata into pendingFinalizers_
markObject(userdata)
```

随后 `collect()` 再调用一次 `propagateMarks()`，见 `src/gc/garbage_collector.cpp:191`。这一步把刚刚复活的 userdata 和它的元表引用图标成存活。结果是：第一次 `clearWeakTableEntries()` 执行时，`weak.handle` 的值已经不是“dead value”了，所以弱表条目仍然保留。

这解释了脚本里的第二行输出：

```text
after first	true
```

它不是说文件句柄仍然可正常使用；它只说明 weak table 里那个 userdata 对象还活到本轮 finalizer 执行结束。

## 5. Weak Cleanup：第二次才清掉条目

弱表清理由 `clearWeakTableEntries()` 完成，见 `src/gc/gc_weak.cpp:66`。它遍历本轮 `markTable()` 收集到的 `weakTables_`，再调用 `Table::removeWeakEntries()`，见 `src/core/table.cpp:234`。

第一次完整 GC 中，file userdata 被 `prepareFinalizers()` 复活，所以不会被移除。第一次 `runFinalizers()` 执行后，它带着 `FINALIZED` 标记留在对象链表里。

第二次完整 GC 中，file userdata 仍然不可达，但这次 `prepareFinalizers()` 不会再次排队它，因为条件里排除了 `FINALIZED`。因此它保持白色。随后 `clearWeakTableEntries()` 会看到弱值已经死亡：

```text
weakValues == true
value is white and not FIXED
  -> set array slot to nil or erase hash entry
```

这个脚本使用的是 hash entry，所以 `weak["handle"]` 被删除。于是最后一行输出：

```text
after second	true
```

也就是 `weak.handle == nil`。

## 6. Sweep：真正释放白色对象

清扫阶段在 `GarbageCollector::sweep(StringPool&)`，见 `src/gc/gc_sweep.cpp:12`。它遍历 `allObjects_` 的侵入式链表：

```text
if object is White and not FIXED:
  unlink from allObjects_
  decrement objectCount_
  remove GCString from StringPool when needed
  delete object
else:
  reset color to White for next cycle
```

第一次完整 GC 中，file userdata 已经被复活，所以 sweep 不释放它。第二次完整 GC 中，它不再被复活，weak table entry 也已经先被清掉，于是 sweep 会释放这个 userdata。

`StringPool&` 参数的存在也在这里体现：如果被删除的是 `GCString`，sweep 会同步调用 `stringPool.remove(...)`，避免驻留表留下悬空指针。

## 7. Run Finalizers：为什么放在 sweep 后

终结器执行在 `runFinalizers()`，见 `src/gc/gc_finalize.cpp:47`。它把 `pendingFinalizers_` 交换到局部列表，然后对每个 userdata：

```text
save stack / call info
push finalizer
push userdata
VM::call(state, 1, 0)
restore stack / call info
```

当前实现会吞掉单个 finalizer 抛出的异常，避免一个失败的 `__gc` 打断整轮收集，见 `src/gc/gc_finalize.cpp:74`。

把 finalizer 放在 sweep 之后有两个好处：

1. 本轮其它白色垃圾已经释放，finalizer 不会阻塞无关对象回收。
2. 被复活的 userdata 已经带有 `FINALIZED`，即使 finalizer 或外部路径再次接触它，也不会第二次进入 `__gc` 队列。

这条规则由 `GC::collectgarbage Runs Userdata Finalizer` 测试锁住，见 `tests/unit/gc/test_gc.cpp:358`。纯 Lua 侧目前没有 `newproxy`，所以自定义 userdata finalizer 的最小观察点在 C++ 单测里；Lua 侧可观察的带 `__gc` userdata 是 I/O 文件句柄。

## 8. 对照测试

这篇 walkthrough 对应三组 GC 单测：

| 行为 | 测试 | 说明 |
|---|---|---|
| weak value 不强保活对象 | `tests/unit/gc/test_gc.cpp:297` | `__mode = "v"` 时 value 可以被收走 |
| weak key 不强保活对象 | `tests/unit/gc/test_gc.cpp:328` | `__mode = "k"` 时 key 可以被移除 |
| userdata `__gc` 只运行一次 | `tests/unit/gc/test_gc.cpp:358` | 不可达 userdata 先复活、运行 finalizer，再在后续周期释放 |

如果你想先读测试，再回到源码，这三组测试会比从 `GarbageCollector::collect()` 入口直接读更容易抓住行为边界。

## 读完后的检查点

1. 能解释为什么 weak table 的弱值不会在 mark 阶段保活对象。
2. 能解释为什么带 `__gc` 的 userdata 在第一次完整 GC 后仍然留在 weak table 里。
3. 能解释为什么第二次完整 GC 才把 `weak.handle` 清成 `nil`。
4. 能解释 `prepareFinalizers()`、`clearWeakTableEntries()`、`sweep()`、`runFinalizers()` 的顺序为什么不能随意交换。
