# Compare with Official Lua 5.1

## 对比总览

| 维度 | 官方 Lua 5.1 | 本项目 |
|------|-------------|--------|
| **语言** | C (ANSI C) | C++17/23 |
| **Value** | TValue (union+tag) | `std::variant` |
| **GC** | Incremental Mark-Sweep | Mark-Sweep (+ 策略接口) |
| **VM Dispatch** | computed goto (GCC) | switch-case / table |
| **错误处理** | setjmp/longjmp | C++ 异常 + `std::expected` |
| **内存管理** | 手动 realloc | RAII + smart pointers |
| **字符串** | 手动驻留 | StringPool 单例 |
| **代码风格** | C 宏 + 全局变量 | C++ 类 + 命名空间 |
| **构建** | Makefile | CMake + .vcxproj |

## 各有优势

### 官方 Lua 5.1 优势
- 久经考验 (20+ 年使用历史)
- 极致小巧 (核心 ~20k 行 C)
- 增量 GC (低延迟)
- 广泛移植

### 本项目优势
- 类型安全 (std::variant, RAII)
- 易于调试 (C++ 工具链)
- 易于扩展 (模块化设计)
- 教学友好 (清晰的代码结构)
- 策略可切换 (GC, Dispatch)
