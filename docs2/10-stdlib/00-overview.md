# Standard Library Overview — 标准库概览

## 1. 标准库清单

| 库 | 文件 | 函数数 | 完成度 |
|----|------|--------|--------|
| **Base** | `baselib.cpp` | ~30 | ✅ 97% |
| **Math** | `mathlib.cpp` | 28/28 | ✅ 100% |
| **String** | `stringlib.cpp` | 14/14 | ✅ 95% |
| **Table** | `tablelib.cpp` | ~10 | ✅ 97% |
| **I/O** | `iolib.cpp` | 11+7方法 | ✅ 97% |
| **OS** | `oslib.cpp` | 11/11 | ✅ 97% |
| **Coroutine** | `coroutinelib.cpp` | 6/6 | ✅ 100% |
| **Debug** | `debuglib.cpp` | 14/14 | 🔄 94% |
| **Package** | `packagelib.cpp` | require/module | ✅ 98% |

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
