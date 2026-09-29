---
status: current
verified_against: docs/compiler/codegen-responsibility-map.md; docs/compatibility/lua51/overview.md; docs/compatibility/lua-c-api-coverage.md; docs/runtime/execution-policy.md; docs/runtime/memory-contract.md; docs/gc/implementation.md; docs/testing/testing-strategy.md; tools/add_source.ps1; CMakeLists.txt; lua.vcxproj; lua_test.vcxproj
last_checked: 2026-09-29
applies_to: behavior-preserving changes and complexity review across repository-owned implementations, tests, examples, and tools
---

# 重构规则

范围、增量采用和等级见 [规则入口](README.md)。重构改变内部结构；修复行为、修改接口或改变资源策略应单独说明，不能统称为“清理”。复杂度信号触发调查，不自动授权整模块重写。

### R-REFACTOR-01 明确需要保持的可观察行为

- **等级：MUST。**
- **范围：** 类型迁移、重命名、提取、移动、封装及算法结构调整。
- **要求：** 修改前确认受影响合同与可观察行为，包括返回/错误对象、Lua 栈效果、C API/ABI、元方法调用顺序、数值与字符串语义、字节码、source/line、资源释放及失败后状态。重入、yield/resume、预算与 GC 可达性属于行为，不是内部细节。
- **例外：** 任务明确要求行为变化时同步合同和回归证据；纯布局只做对应差异检查。
- **验证：** 对照 [兼容边界](../compatibility/lua51/overview.md)、[C API 合同](../compatibility/lua-c-api-coverage.md) 和受影响主题，建立风险相符的基线；编译通过不是等价证明。

### R-REFACTOR-02 重命名覆盖真实引用

- **等级：MUST。**
- **范围：** 文件、类型、函数、字段、脚本导出或注册项改名。
- **要求：** 先澄清含义，再同步源码、字符串注册、构建清单、测试、工具参数和文档引用；保留既有明确术语，不按缩写词表全仓替换。
- **例外：** 对外名称、C ABI 与来源固定接口不能孤立改名；内部名称仅文字相似不表示属于同一概念。
- **验证：** 搜索旧名和映射，检查实际调用路径；新增 C++ 源文件沿用 [add_source.ps1](../../tools/add_source.ps1) 维护工程清单，移动或删除时核对 CMake 与 Visual Studio 两侧。

### R-REFACTOR-03 按职责拆分函数与参数

- **等级：REVIEW。**
- **范围：** 长函数、长参数列表、深嵌套和反复传递的参数组。
- **要求：** 根据独立阶段、抽象层和稳定领域概念判断是否提取。参数对象需表达真实不变量；函数分拆应形成可命名职责。
- **例外：** 表驱动、固定顺序流程和天然层级遍历可保持集中；不为降低计数包装一层调用、传整个 context 或暴露内部状态。
- **验证：** 人工核对数据依赖、参数方向、默认值、借用、清理与失败位置；拆分后调用方应更容易理解完整流程。

### R-REFACTOR-04 抽象与移动回应当前需要

- **等级：REVIEW。**
- **范围：** 新接口、工厂、策略、强类型、模块移动与薄封装删除。
- **要求：** 说明当前变化轴、误用风险、复用或测试边界。按 [代码生成职责](../compiler/codegen-responsibility-map.md) 等已有边界放置行为，避免调用方反复穿透他模块内部结构。
- **例外：** ABI、protected 调用、allocator、平台和兼容封装有独立价值；不能因其薄就删除，也不为假设的第二实现创建框架。
- **验证：** 人工核对真实使用方、依赖方向、类型/ABI 等价和运行成本；通用 `using` 别名不代替强类型验证。

### R-REFACTOR-05 去重保持权威来源与独立证据

- **等级：SHOULD。**
- **范围：** 重复知识、生成产物、共享 helper 和测试 oracle。
- **要求：** 确认是同一变化原因再合并；把派生内容交给生成或一致性检查。自有生成代码修改生成源并核对产物，不手工修补输出。
- **例外：** 独立 oracle、来源固定的第三方材料和仅语法相似的代码可保留；不得让测试直接复用被测算法以消除重复。
- **验证：** 跟踪一处规则变更的全部使用方，人工确认未形成第二份权威状态、遗漏生成物或削弱测试区分能力。

### R-REFACTOR-06 控制流变换保留求值与收尾

- **等级：MUST。**
- **范围：** 提前返回、合并条件、拆分循环、谓词提取和分派重组。
- **要求：** 保留短路、求值次数与顺序、临时对象有效期、回调/元方法次数、错误位置、栈恢复和资源释放。清楚的复合条件可以保留；提取须表达独立含义。
- **例外：** 为修复已确认行为问题而改变上述顺序时，明确行为变化及依据；单纯折行无需生成完整决策矩阵。
- **验证：** 对实际复杂变换列出简要分支对照，覆盖被短路跳过的路径、边界值与失败出口；热点循环额外检查实际成本，不能仅比较最终返回值。

### R-REFACTOR-07 所有权与执行合同不得顺带改写

- **等级：MUST。**
- **范围：** GC、容器、智能指针、allocator、VM 与 C callback 相关重构。
- **要求：** 保持 GC root/edge/barrier 和非拥有裸指针、自定义 allocator/deleter、失败回滚及 protected 错误边界。Lua→C→Lua 重入与 yield/resume 继续共享所属执行窗口，保留独立的 per-drain finalizer 规则。
- **例外：** 明确批准的所有权或执行策略变更需作为行为变更单独实施；默认 `Vec/UPtr` 不能替代 `LuaVector/LuaOwnedPtr`，短名统一不是例外依据。
- **验证：** 对照 [GC 实现](../gc/implementation.md)、[内存合同](../runtime/memory-contract.md)、[执行治理](../runtime/execution-policy.md)，按影响验证 root、释放、OOM、重试、预算、重入与 yield。

### R-REFACTOR-08 小步修改并取得对应证据

- **等级：MUST。**
- **范围：** 人工和工具辅助重构。
- **要求：** 明确范围与基线，按可解释的小步变换修改，检查 diff 与受影响引用，在有意义的检查点执行对应验证。保留无关已有工作；格式整理、行为变化与结构调整保持可区分。验证失败先定位，不在未解释的失败上继续堆叠修改。
- **例外：** 机械且同质的迁移可按责任区成批执行；无运行条件时如实列出未验证内容，不假称通过或等价。
- **验证：** 依据下表和 [测试策略](../testing/testing-strategy.md) 检查实际结果；不通过扩大基线、削弱断言、修改上游 fixture 或放宽门禁掩盖回归。

| 影响范围 | 应选择的证据 |
|---|---|
| 文档、纯排版 | 相对链接、事实头、示例、格式与 diff；不新增行为测试 |
| 局部表达、别名、helper | 受影响目标编译、必要的类型等价断言、现有定向测试及边界审查 |
| Parser、codegen、VM 语义 | 对应单元、Lua 行为/回归与必要的官方差分；检查错误、字节码或 trace 的实际合同 |
| 公开 C API、ABI | C 编译、C++ 签名、静态/共享链接与导出、所影响消费者和兼容合同 |
| GC、allocator、借用 | 可达性、释放、OOM/fail-on-N、失败后栈与容器状态、解除限制后重试 |
| 重入、coroutine、执行治理 | Lua→C→Lua、yield/resume、共享预算、取消与失败终态 |
| 热点和平台边界 | 受影响规模的基准或目标平台验证；不以本机单一构建代替其他平台证据 |

选择与改动风险相符的现有检查；缺少能区分回归的证据时补最小测试。检查通过后，除非有新改动、失败或未决风险，不重复扩大验证范围。
