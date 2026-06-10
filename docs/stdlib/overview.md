---
status: current
verified_against: src/lib/lib_catalog.cpp; src/lib/lib_manager.cpp; src/lib/lib_registry.hpp; src/lib/baselib.cpp; src/gc/gc_strategy.hpp; src/lib/mathlib.cpp; src/lib/iolib.cpp; src/lib/stringlib.cpp; src/lib/tablelib.cpp; src/lib/oslib.cpp; src/lib/coroutinelib.cpp; src/lib/debuglib.cpp; src/lib/packagelib.cpp; tests/unit/stdlib/test_lib_catalog.cpp; tests/unit/stdlib/test_baselib.cpp; tests/unit/stdlib/; tests/unit/gc/test_gc.cpp
last_checked: 2026-05-31
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

PR-73 评估了但有意拒绝了 `LibRegistrar` 自注册层。当前显式的 `constexpr` 目录是标准库装配的唯一权威来源，因为它使加载顺序可见、保持测试直接，并避免了静态初始化 / MSVC 链接器保活的意外。

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

## 已知兼容性缺口

当前项目测试全绿，但这并不意味着完全与官方 Lua 5.1.5 兼容。已知的高价值缺口包括：

- 官方 `testC` / `ltests.c` 辅助库对 `api.lua` 和 `code.lua` 的覆盖
- 官方 Lua 5.1 二进制 chunk 兼容性；当前 dump/load 为项目本地格式
- 非主流路径中逐字节级别的错误/traceback 文本兼容性
- debug 库中极端栈层级和 traceback 格式化细节
- 精确的 Lua 5.1 GC 工作量核算和 `IncrementalGC` 策略语义；`collectgarbage("step")` 已有分阶段推进，但 `collectgarbage("strategy", "incremental")` 仍选择等价的教学占位实现来替代完整的 `collect()`
- 更多入口点迁移到 owning `EngineContext`；owning context 已存在，但单例兼容入口点仍保留

## 验证

```powershell
bin\lua_test.exe --filter "Standard Library Catalog"
bin\lua_test.exe --filter "Base Library"
bin\lua_test.exe --filter "Package Library"
```
