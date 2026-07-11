---
status: current
verified_against: src/lib/baselib.cpp; src/lib/stringlib.cpp; src/lib/tablelib.cpp; src/lib/mathlib.cpp; src/lib/coroutinelib.cpp; src/lib/packagelib.cpp; src/lib/iolib.cpp; src/lib/oslib.cpp; src/lib/debuglib.cpp; tests/unit/stdlib/; tests/lua/stdlib/; tests/lua/official/; src/lib/; src/api/
last_checked: 2026-07-11
applies_to: Lua 5.1 标准库实现边界
---

# Lua 5.1 标准库实现边界

标准库是 Lua 语义到 C++ runtime service 的适配层。各函数共享 LuaState 栈协议、参数检查、错误对象、字符串驻留和 GC 安全规则，因此集中维护比每个库一个短页更不易漂移。

## 公共调用约定

native library function 接收 `LuaState*`，从 1 起的 API 索引读取参数，把结果压栈，并返回结果数量。实现必须：

- 在可能 push/分配后重新获取可能失效的栈引用；
- 使用统一 check/optional/default 参数语义；
- 通过 Lua error 边界报告类型和范围错误；
- 对返回的 GC 对象确保已进入栈或其他 root；
- 不泄漏宿主异常、locale、路径格式或容器顺序。

库通过注册表把名称映射到 native Function。`RuntimeServices` 提供 StringPool、GC、GlobalState 和可选 VM 策略，避免库函数依赖隐藏单例。

## 模块责任矩阵

| 模块 | 关键语义 | 实现 |
|---|---|---|
| base | load/call、pcall/xpcall、type/tostring、raw 操作、环境、GC 控制 | `src/lib/baselib.cpp` |
| string | 字节串索引、pattern、format、转换 | `src/lib/stringlib.cpp` |
| table | insert/remove/sort/concat/maxn | `src/lib/tablelib.cpp` |
| math | 浮点数学、随机状态、min/max、角度转换 | `src/lib/mathlib.cpp` |
| coroutine | create/resume/yield/status/wrap | `src/lib/coroutinelib.cpp` |
| package | loaders、search path、loaded/preload、require 缓存 | `src/lib/packagelib.cpp` |
| io | file userdata、默认流、读写格式和 close | `src/lib/iolib.cpp` |
| os | 时间、日期、环境与受宿主约束的系统操作 | `src/lib/oslib.cpp` |
| debug | stack/frame/local/upvalue、hook、traceback | `src/lib/debuglib.cpp` |

## Base library

base library 最靠近核心运行时。`pcall/xpcall` 必须在展开 frame 前保留 Lua 错误对象，并关闭 open upvalue；`rawget/rawset/rawequal` 绕过 metamethod；`getfenv/setfenv` 遵守 Lua 5.1 function/thread 环境模型。`collectgarbage` 只通过 GC 策略 API 操作，不直接遍历对象容器。

## String 与 pattern

Lua string 是带长度的字节序列，不是以 NUL 结尾的文本语义。substring 索引使用 Lua 的 1-based/negative 规则。format、number conversion 和 pattern engine 必须避免宿主 locale 造成差异；pattern 不是 `std::regex` 的同义替换。

## Table library

table library 操作语言层序列概念，不暴露 array/hash 内部分界。sort comparator 可以执行 Lua 代码、抛错并触发 GC，因此排序实现不能跨 comparator 调用持有不稳定的栈引用，也必须检测不一致比较器造成的边界问题。

## Math library

math 函数围绕项目 number 表示工作。NaN、无穷、舍入、mod/fmod 和随机状态需要兼容测试。随机生成器状态属于明确的运行时/库状态，不能隐式依赖全局 C RNG 导致测试互相污染。

## Coroutine library

coroutine 把 LuaState/thread 状态机暴露为库 API。resume/yield 必须维护 frame、stack top、错误对象和状态转换；wrap 只改变错误传播形式，不复制 coroutine 实现。

## Package 与 require

`require(name)` 先查 loaded，再按 loaders/searchers 尝试加载，并在成功后缓存结果。循环加载、loader 错误聚合、路径模板替换和返回 nil 时的缓存值是高风险兼容点。文件访问通过 IO 层，路径展示不成为跨平台 golden。

## IO 与 OS

file 是带 metatable/finalizer 的 userdata，C++ RAII 管理宿主 handle，Lua GC 管理 userdata 可达性；两层生命周期必须幂等协作。OS API 明确宿主依赖，对时间 zone、locale、权限和不可用操作采用测试可识别的边界。

## Debug library

debug API 借用 LuaState 的 frame 和 Proto 信息，不拥有它们。stack level 需要处理 tailcall，`savedpc` 需要转换为当前指令，hook 回调必须防止状态损坏。`debug.traceback` 的稳定语义是帧与位置，不是推断函数名的每个字符。

## 验证原则

- 每个函数至少有成功、类型错误、边界值和 GC/异常安全证据。
- 组合行为用 Lua scripts 验证，如 xpcall + traceback、sort comparator + error、require cycle。
- official suite 决定兼容语义；C++ 单测锁定 stack delta、返回数量和对象生命周期。
- 文档记录机制与边界，不复制完整 API 清单；函数名与参数以源码注册表和测试为准。

注册架构见 [标准库总览](../overview.md)，兼容性判断见 [Lua 5.1 兼容边界](../../compatibility/lua51/overview.md)。
