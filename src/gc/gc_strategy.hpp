#pragma once

/**
 * @file gc_strategy.hpp
 * @brief 垃圾回收策略接口及标记清除策略定义
 */

#include "common/types.hpp"
#include <functional>

namespace Lua {

class GarbageCollector;
class LuaState;
class StringPool;

/** @brief 垃圾回收策略执行所需的收集器、字符串驻留池与当前状态。 */
struct GCContext {
    GarbageCollector& collector;
    StringPool& stringPool;
    LuaState* currentState;
};

/** @brief 可替换的垃圾回收策略接口。 */
class GCStrategy {
public:
    virtual ~GCStrategy() = default;

    /** @brief 使用指定上下文执行一次完整收集。 */
    [[nodiscard]] virtual usize collect(GCContext& context) const = 0;
    /** @brief 获取稳定的策略名称。 */
    [[nodiscard]] virtual const char* name() const noexcept = 0;
    /** @brief 获取策略的简短说明。 */
    [[nodiscard]] virtual const char* summary() const noexcept = 0;
};

/** @brief 停顿式三色标记清除策略。 */
class MarkSweepGC final : public GCStrategy {
public:
    [[nodiscard]] usize collect(GCContext& context) const override;
    [[nodiscard]] const char* name() const noexcept override;
    [[nodiscard]] const char* summary() const noexcept override;
};

/** @brief 保持等价行为的教学用增量策略占位实现。 */
class IncrementalGC final : public GCStrategy {
public:
    [[nodiscard]] usize collect(GCContext& context) const override;
    [[nodiscard]] const char* name() const noexcept override;
    [[nodiscard]] const char* summary() const noexcept override;
};

/** @brief 获取共享的标记清除策略实例。 */
[[nodiscard]] const GCStrategy& markSweepGCStrategy() noexcept;
/** @brief 获取共享的增量策略实例。 */
[[nodiscard]] const GCStrategy& incrementalGCStrategy() noexcept;
/** @brief 按名称查找垃圾回收策略。 */
[[nodiscard]] Opt<std::reference_wrapper<const GCStrategy>> findGCStrategy(StrView name) noexcept;

} // namespace Lua
