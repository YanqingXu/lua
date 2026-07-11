---
status: current
verified_against: docs/stdlib/overview.md; src/lib/; tests/unit/stdlib/
last_checked: 2026-06-13
applies_to: Chinese standard-library overview
---

# Standard Library Overview — 标准库概览

## 1. 标准库清单

| 库 | 文件 | 主要入口 |
|----|------|----------|
| **Base** | `baselib.cpp` | 全局基础函数 |
| **Math** | `mathlib.cpp` | `math` 表 |
| **String** | `stringlib.cpp` | `string` 表与字符串元方法 |
| **Table** | `tablelib.cpp` | `table` 表 |
| **I/O** | `iolib.cpp` | `io` 表与文件方法 |
| **OS** | `oslib.cpp` | `os` 表 |
| **Coroutine** | `coroutinelib.cpp` | `coroutine` 表 |
| **Debug** | `debuglib.cpp` | `debug` 表 |
| **Package** | `packagelib.cpp` | `require`、`module` 与 loader |

## 2. C 函数注册模式

```cpp
// 标准库函数都是 C 函数
int print(lua_State* L) {
    i32 n = lua_gettop(L);
    for (i32 i = 1; i <= n; i++) {
        if (i > 1) printf("\t");
        printf("%s", lua_tostring(L, i));
    }
    printf("\n");
    return 0;  // 0 个返回值
}

// 注册
lua_register(L, "print", print);
```

## 3. 库管理系统

```cpp
// lib_catalog.cpp - 库目录
// lib_manager.cpp - 库管理 (catalog 驱动的注册)
// lib_registry.cpp - 库注册

void openBaseLib(LuaState* L) {
    // 注册所有 base 函数到全局表
    lua_register(L, "print", base_print);
    lua_register(L, "type", base_type);
    // ...
}
```
