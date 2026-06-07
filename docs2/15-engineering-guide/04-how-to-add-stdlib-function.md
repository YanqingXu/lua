# How to Add a StdLib Function — 如何新增标准库函数

## 步骤

### 1. 实现 C++ 函数
```cpp
// src/lib/mylib.cpp

static int my_function(lua_State* L) {
    // 获取参数
    i32 n = lua_gettop(L);
    f64 x = lua_tonumber(L, 1);
    f64 y = lua_tonumber(L, 2);
    
    // 计算结果
    f64 result = x + y;
    
    // 推入返回值
    lua_pushnumber(L, result);
    
    return 1;  // 1 个返回值
}
```

### 2. 注册函数
```cpp
void openMyLib(LuaState* L) {
    // 注册到全局表
    lua_register(L, "my_function", my_function);
    
    // 或注册为模块表
    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, my_function);
    lua_setfield(L, -2, "my_function");
    lua_setglobal(L, "mylib");
}
```

### 3. 添加到库目录 (可选)
```cpp
// src/lib/lib_catalog.cpp
void LibCatalog::registerAll(LuaState* L) {
    openBaseLib(L);
    openMathLib(L);
    // ... existing ...
    openMyLib(L);  // 新增
}
```

### 4. 写测试
```lua
-- tests/lua/stdlib/test_mylib.lua
local result = my_function(1, 2)
assert(result == 3)
```

### 5. C++ 单元测试
```cpp
// tests/unit/stdlib/test_mylib.cpp
TEST_CASE(MyLibSuite, TestMyFunction) {
    LuaState* L = LuaState::create();
    openMyLib(L);
    // ... test ...
}
```

## Checklist

- [ ] 参数验证 (类型、数量)
- [ ] 返回值数量正确
- [ ] 错误处理 (参数错误时 throw)
- [ ] 与 Lua 5.1 官方行为一致
- [ ] 测试覆盖基本场景
- [ ] 测试覆盖边界条件
- [ ] 测试覆盖错误路径
