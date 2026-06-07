# Design Goals — 设计目标

## 1. 这个模块解决什么问题？

明确项目定位，让所有人（包括未来的自己）知道：**这个项目要做什么，不做什么。**

## 2. 核心目标

### 目标 1：兼容 Lua 5.1 语法和核心语义

- 支持 Lua 5.1 完整的语法规范（语句、表达式、控制流）
- 支持 Lua 5.1 核心语义（作用域、闭包、元表、协程）
- 标准库最大化兼容 Lua 5.1 行为
- 官方测试套件全绿执行

### 目标 2：使用现代 C++ 实现更易读、更易调试的解释器

- `std::variant` 替代 C union — 类型安全的 Value 表示
- `std::unique_ptr` / `std::shared_ptr` — 自动内存管理
- 类型别名 (`i32`, `u32`, `f64`, `Vec<T>`, `Str`) — 统一的代码风格
- RAII — 资源自动释放
- C++17/20/23 特性 — `if constexpr`, `std::string_view`, `std::optional`

### 目标 3：服务于游戏服务器脚本系统

- 支持热更新（Hot Reload）架构预留
- 沙箱隔离能力预留
- C++ 绑定接口设计
- 可嵌入的运行时设计

### 目标 4：支持未来扩展

- **调试器** — VM trace 系统已预留，支持指令级追踪
- **热更** — Proto 和 Closure 分离，支持字节码级别的替换
- **沙箱** — 独立的 LuaState / EngineContext 隔离
- **C++ 绑定** — Userdata 和 LightUserdata 支持

### 目标 5：优先追求可理解、可维护、可扩展

- **不是**优先追求极限性能
- 代码结构清晰，每个模块职责单一
- 充分的文档和注释
- 完整的测试覆盖
- 教学化的实现（例如 GCStrategy 策略边界）

## 3. 非目标（明确不做的事情）

- ❌ 追求极限执行性能（JIT编译、寄存器分配优化）
- ❌ 100% 复刻 Lua 5.1 C 源码的实现细节
- ❌ 支持 Lua 5.2/5.3/5.4 的新增语法
- ❌ 兼容 LuaJIT 扩展
- ❌ 跨所有平台的兼容性（当前主要 Windows + MSVC）

## 4. 设计原则

1. **可读性优先** — 代码应自解释，命名清晰
2. **模块边界清晰** — 每个文件职责单一，依赖关系明确
3. **错误要早暴露** — 使用 `[[nodiscard]]`、`std::expected`、static_assert
4. **测试驱动兼容** — 每个语言特性有对应测试
5. **渐进增强** — 先做对，再做好，再做快

## 5. 技术选型

| 技术 | 选择 | 理由 |
|------|------|------|
| **语言** | C++17/23 | 现代特性丰富，工业标准 |
| **编译器** | MSVC 2026 | Windows 主力平台 |
| **构建系统** | CMake + .vcxproj | 双重路线灵活 |
| **Value 表示** | `std::variant` | 类型安全，调试友好 |
| **VM dispatch** | switch-case / table | 可切换，教学友好 |
| **GC 算法** | 标记-清除 | 经典算法，可扩展为增量式 |
| **测试框架** | 自研轻量框架 | 零外部依赖，足够直接 |

## 6. 与官方 Lua 的关键差异

| 方面 | 官方 Lua 5.1 | 本项目 |
|------|-------------|--------|
| Value 表示 | C union + 手动 tag | `std::variant` 自动 tag |
| 内存管理 | 手动 malloc/free + GC | RAII + shared_ptr/unique_ptr + GC |
| VM dispatch | computed goto (GCC扩展) | switch-case / table dispatch |
| 错误处理 | setjmp/longjmp | C++ 异常 + `std::expected` |
| 字符串 | 手动 interning | StringPool 单例 |
| 闭包 | C 结构体 + 手动管理 | C++ 类 + RAII |
