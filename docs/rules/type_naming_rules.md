---
status: current
verified_against: src/common/types.hpp; src/runtime/lua_allocator.hpp; src/lua.h; src/lauxlib.h; src/lualib.h; src/lua_runtime.h; src/lua_cpp_version.h; tests/compatibility/public_api_c_compile.c; tests/compatibility/public_api_cpp_consumer.cpp; examples/embedding.cpp; examples/production_worker.cpp; lua_test/include/test_framework/test_framework.hpp; CMakeLists.txt
last_checked: 2026-09-29
applies_to: unified aliases in repository-owned C++, preserving C headers, SDK/ABI boundaries, GC semantics, and custom allocator/deleter types
---

# 类型命名规则

通用短类型的唯一声明来源是 [src/common/types.hpp](../../src/common/types.hpp)，位于 `Lua` 命名空间。规范适用于全部自有 C++，新增和实际修改代码先执行，历史迁移按 [采用状态](adoption-status.md) 分批推进。别名只改变拼写，**不提供强类型隔离**。

### R-TYPES-01 已有等价别名统一使用

- **等级：MUST。**
- **范围：** 自有 C++ 中显式书写的成员、局部变量、参数、返回值、模板实参、转换和领域别名。
- **要求：** 实际类型与现有别名完全相同时使用对应短名；在 `Lua` 外使用 `Lua::` 限定或明确的局部 using 声明。映射以下表为索引，实际定义以 `types.hpp` 为准。
- **例外：** C/SDK/ABI 与独立消费契约适用 R-TYPES-04；别名定义本身、类型等价测试和无法由现有模板表达的自定义参数类型保留准确拼写。`auto`/`decltype` 可在推导清楚时使用，不用它们掩盖宽度和所有权。
- **验证：** 搜索显式类型位置后逐处语义审查，排除注释、字符串及上述例外；当前没有完整自动短类型检查器。

| 底层类型 | `Lua` 中的统一名称 |
|---|---|
| `int8_t/int16_t/int32_t/int64_t` 及对应 `std::` 类型 | `i8/i16/i32/i64` |
| `uint8_t/uint16_t/uint32_t/uint64_t` 及对应 `std::` 类型 | `u8/u16/u32/u64` |
| `size_t`、`ptrdiff_t` 及对应 `std::` 类型 | `usize`、`isize` |
| `float`、`double` | `f32`、`f64` |
| `const char*`、`std::string`、`std::string_view` | `CharPtr`、`Str`、`StrView` |
| `std::vector<T>`、`std::array<T, N>` | `Vec<T>`、`Arr<T, N>` |
| `std::span<T, N>` | `Span<T, N>`；省略 `N` 时为 `std::dynamic_extent` |
| 默认参数的 `std::unordered_map<K, V>`、`std::unordered_set<T>` | `HashMap<K, V>`、`HashSet<T>` |
| `std::variant<T...>`、`std::optional<T>` | `Var<T...>`、`Opt<T>` |
| `std::expected<T, E>`、`std::unexpected<E>` | `Expect<T, E>`、`Unexpect<E>` |
| `std::function<Signature>` | `Func<Signature>` |
| `std::shared_ptr<T>`、`std::weak_ptr<T>`、默认 deleter 的 `std::unique_ptr<T>` | `Ptr<T>`、`WPtr<T>`、`UPtr<T>` |
| `std::atomic<T>`、`std::mutex`、`std::scoped_lock<Mtx>` | `Atom<T>`、`Mtx`、`ScopedLock` |

现有 Lua 领域名 `LuaInteger`、`LuaNumber`、`LuaBoolean`、枚举与前向声明继续保留；适合表达 Lua 语义的地方优先使用这些领域名。`bool`、`char`、`void` 没有新增通用短名；普通控制标志可保留 `bool`，不因存在 `LuaBoolean` 就机械转换。当前不增加 `Map`、`Set`，不能把有序容器换成哈希容器以套用现有别名。

### R-TYPES-02 定义集中，依赖直接

- **等级：MUST。**
- **范围：** 使用或新增通用别名的 C++ 文件。
- **要求：** 使用通用别名时直接包含 `common/types.hpp`，不依赖偶然的间接 include；新增通用短名只在该头文件定义。保留已有别名和 `makePtr`、`makeUnique` 工厂，不另建平行别名层。项目构建使用 C++23；新增标准库设施仍需目标工具链验证。
- **例外：** 稳定领域类型、allocator/deleter 专用类型在其责任模块定义；这些定义可以使用标准库模板所需的精确参数。C 头文件不得包含 C++ 别名头；R-TYPES-04 的独立消费边界不为别名引入内部依赖。
- **验证：** 人工检查直接 include、别名唯一来源及实际目标编译；通用别名变更验证完整模板参数和默认参数。

### R-TYPES-03 拼写迁移不改变类型语义

- **等级：MUST。**
- **范围：** 所有别名替换及由此产生的接口调整。
- **要求：** 保持宽度、符号、`const`、引用、指针层级、模板参数、重载、布局与 ABI。`usize` 表达大小和索引，不默认作为持久化或字节码宽度；`Span`/`StrView` 仍是借用。`Expect` 不要求把现有错误协议统一改成 expected。
- **例外：** GC 非拥有裸指针保持其 root/edge/observer 语义；`LuaVector`、`LuaOwnedVector`、`LuaOwnedPtr` 及其他自定义 allocator、hash、比较器或 deleter 类型不得降为默认别名。标准函数如 `std::move`、`std::make_unique` 不是类型，不因本规则改名。
- **验证：** 适当使用类型等价编译断言并人工检查实际模板参数，迁移遵守 [重构规则](refactoring_rules.md)；不能因平台上暂时同宽就把 `int`、SDK 句柄或字符机械替换为 `i32/u8`。

### R-TYPES-04 C 与外部接口保留准确契约

- **等级：MUST。**
- **范围：** 公开 C 头文件、SDK 类型、平台入口、回调、导出、独立消费者及第三方生成接口。
- **要求：** `lua.h`、`lauxlib.h`、`lualib.h`、`lua_runtime.h`、`lua_cpp_version.h` 保持 C 可编译性及既有原型、类型名、宏与 ABI。外部专用类型和调用约定按其合同保留；进入内部后的转换需表达范围与失败行为。只消费安装 SDK 的示例、宿主和 package consumer 保持公开依赖；不为使用 `Lua::` 别名引入 `common/types.hpp`。
- **例外：** C++ 内部适配层中与项目别名确实相同且不损失契约信息的类型仍遵守 R-TYPES-01；不能把整个适配目录都当作例外。已有明确项目无关合同的基础测试框架可保留标准类型，其 Lua 适配层仍按依赖边界核对。第三方控制的生成产物按上游合同保留。
- **验证：** 对照 [C API 合同](../compatibility/lua-c-api-coverage.md)，按影响运行 C 编译、C++ 精确签名、链接/导出、安装消费者或独立框架测试；检查没有新引入内部头依赖，外观相似不能替代类型和 ABI 证明。

### R-TYPES-05 历史迁移按语义分批

- **等级：MUST。**
- **范围：** 短类型专项整改、搜索结果和生成代码。
- **要求：** 按责任区记录已迁移、保留例外和待迁移部分，覆盖自有测试、工具与示例中的 C++；自有生成代码修改模板或生成源。每批检查真实类型和行为，不做全仓盲替换。
- **例外：** 未触及的历史代码留待后续批次，来源固定的上游材料排除；纯格式变更不触发整文件类型迁移。
- **验证：** 人工核对 diff、调用方和 [采用状态](adoption-status.md)；搜索结果归零、格式或 tidy 通过均不能单独宣告全仓类型合规。
