---
status: current
verified_against: src/lib/lib_catalog.cpp; src/lib/lib_manager.cpp; src/lib/lib_registry.hpp; src/lib/baselib.cpp; src/gc/gc_strategy.hpp; src/lib/mathlib.cpp; src/lib/iolib.cpp; src/lib/stringlib.cpp; src/lib/tablelib.cpp; src/lib/oslib.cpp; src/lib/coroutinelib.cpp; src/lib/debuglib.cpp; src/lib/packagelib.cpp; tests/unit/stdlib/test_lib_catalog.cpp; tests/unit/stdlib/test_baselib.cpp; tests/unit/stdlib/; tests/unit/gc/test_gc.cpp; src/lib/; src/api/; tests/lua/stdlib/
last_checked: 2026-07-11
applies_to: current standard library implementation overview
---

# 标准库总览

标准库通过 `StandardLibrary` 和 `src/lib/lib_catalog.cpp` 中的目录进行注册。

当前目录加载顺序：

1. `base`
2. `math`
3. `io`
4. `string`
5. `table`
6. `os`
7. `coroutine`
8. `debug`
9. `package`

`StandardLibrary::openAll()` 遍历此目录。单库加载应使用 `StandardLibrary::openCatalogLibrary(L, "<id>")`；旧有的 `openMath()` / `openPackage()` 等便捷包装函数是同一路径上已弃用的兼容垫片。

没有采用 `LibRegistrar` 自注册层。显式的 `constexpr` 目录是标准库装配的权威来源，因为它使加载顺序可见、保持测试直接，并避免静态初始化和 MSVC 链接器保活带来的隐式行为。

## 注册模型

每个库通常遵循以下形态：

```cpp
class XxxLibModule : public LibModule {
public:
    void registerFunctions(LuaState* L) override;
    void initialize(LuaState* L) override;
};
```

`FunctionRegistrar` 为全局函数和表函数提供了流畅的辅助方法。

## 库文件

| 库 | 文件 | 备注 |
|---|---|---|
| base | `baselib.hpp/.cpp` | 全局函数，`_G`，`_VERSION`，`pcall`，`xpcall`，加载辅助函数（包括 stdin `loadfile/dofile`），环境辅助函数，GC 门面（包括 `collectgarbage("strategy")` 和有状态的 `setpause` / `setstepmul` 控制） |
| math | `mathlib.hpp/.cpp` | 数学函数和常量 |
| io | `iolib.hpp/.cpp` | 文件 userdata，`io` 表，文件方法，`io.lines/file:lines` 读取格式 |
| string | `stringlib.hpp/.cpp` | 字符串操作，模式匹配函数，`string.dump` |
| table | `tablelib.hpp/.cpp` | insert/remove/sort/concat 及 5.2 风格便捷辅助函数 |
| os | `oslib.hpp/.cpp` | 日期/时间、环境变量、命令执行、remove/rename/tmpname |
| coroutine | `coroutinelib.hpp/.cpp` | `create`、`resume`、`yield`、`status`、`running`、`wrap` |
| debug | `debuglib.hpp/.cpp` | 栈/上值/调试 hook/traceback 功能面 |
| package | `packagelib.hpp/.cpp` | `require`、`module`、`package.*`、Lua 和 C 加载器路径 |

## 兼容与验证边界

库注册、栈 delta 和参数错误由 `tests/unit/stdlib/` 锁定，语言组合行为由 `tests/lua/stdlib/` 与 official scripts 验证。兼容性差异只在 [Lua 5.1 兼容边界](../compatibility/lua51/overview.md) 维护，本页不复制项目状态或缺口清单。
