# Dependency Map — 依赖关系图

## 1. 这个模块解决什么问题？

说明各模块之间的依赖关系，避免循环依赖。

## 2. 模块依赖图

```
                    ┌─────────────┐
                    │ common/      │  (基础类型、配置、宏)
                    └──────┬──────┘
           ┌───────────────┼───────────────┐
           ▼               ▼               ▼
    ┌──────────┐    ┌──────────┐    ┌──────────┐
    │ core/     │    │ io/       │    │ compiler/ │
    │ Value,    │    │ InputStream│   │ Lexer,     │
    │ Table,    │    │ FileLoader │   │ Parser,    │
    │ Function  │    │            │   │ CodeGen    │
    └─────┬─────┘    └─────┬─────┘    └─────┬─────┘
          │               │               │
          └───────────────┼───────────────┘
                          ▼
                   ┌──────────┐
                   │ vm/       │  (VM, LuaState, Stack, GlobalState)
                   └─────┬────┘
                         │
              ┌──────────┼──────────┐
              ▼          ▼          ▼
       ┌──────────┐ ┌──────┐ ┌──────────┐
       │ gc/       │ │ lib/  │ │ api/      │
       │ GC, Mark, │ │ StdLib│ │ lapi      │
       │ Sweep     │ │       │ │           │
       └──────────┘ └──────┘ └──────────┘
```

## 3. 层级说明

| 层级 | 模块 | 依赖 |
|------|------|------|
| **L0 基础层** | `common/` | 无（只依赖 STL） |
| **L1 核心层** | `core/`, `io/`, `compiler/` | `common/` |
| **L2 执行层** | `vm/` | `common/`, `core/`, `compiler/`, `io/` |
| **L3 服务层** | `gc/`, `lib/`, `api/` | `vm/`, `core/` |
| **L4 应用层** | `app/`, `bytecode/`, `repl/`, `main.cpp` | 所有下层 |

## 4. 文件间依赖（关键路径）

### 编译路径
```
io/input_stream.hpp
  ← compiler/lexer/lexer.hpp
    ← compiler/parser/parser.hpp
      ← compiler/codegen/codegen.cpp
        ← vm/vm.cpp
```

### 执行路径
```
core/value.hpp
  ← core/table.hpp
    ← core/function.hpp
      ← core/upvalue.hpp
        ← vm/state/lua_state.hpp
          ← vm/vm.cpp
```

### GC 路径
```
core/gc_object.hpp
  ← core/gc_string.hpp
  ← core/table.hpp
  ← core/function.hpp
  ← gc/garbage_collector.hpp
    ← gc/gc_mark.cpp, gc_sweep.cpp, gc_finalize.cpp, gc_weak.cpp
```

## 5. 注意避免的循环依赖

- `core/` ↔ `vm/`: core 定义类型，vm 使用类型。不要反向依赖。
- `compiler/` ↔ `vm/`: compiler 产生 Proto，vm 消费 Proto。不要反向。
- `gc/` → `core/`: gc 访问 core 对象进行标记。core 对象不应主动调用 gc。
