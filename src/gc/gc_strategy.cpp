/**
 * @file gc_strategy.cpp
 * @brief 垃圾回收策略的收集入口与描述信息实现
 */

#include "common/types.hpp"
#include "gc/gc_strategy.hpp"

#include "gc/garbage_collector.hpp"

namespace Lua {

usize MarkSweepGC::collect(GCContext& context) const {
    return context.collector.collectMarkSweep(context.stringPool, context.currentState);
}

CharPtr MarkSweepGC::name() const noexcept {
    return "mark-sweep";
}

CharPtr MarkSweepGC::summary() const noexcept {
    return "stop-the-world tri-color mark and sweep";
}

usize IncrementalGC::collect(GCContext& context) const {
    return context.collector.collectIncrementalCycle(context.stringPool, context.currentState);
}

CharPtr IncrementalGC::name() const noexcept {
    return "incremental";
}

CharPtr IncrementalGC::summary() const noexcept {
    return "phased mark/atomic/sweep/finalize collection driven by GC debt and step budget";
}

const GCStrategy& markSweepGCStrategy() noexcept {
    static const MarkSweepGC strategy;
    return strategy;
}

const GCStrategy& incrementalGCStrategy() noexcept {
    static const IncrementalGC strategy;
    return strategy;
}

Opt<std::reference_wrapper<const GCStrategy>> findGCStrategy(StrView name) noexcept {
    if (name == markSweepGCStrategy().name()) {
        return std::cref(markSweepGCStrategy());
    }
    if (name == incrementalGCStrategy().name()) {
        return std::cref(incrementalGCStrategy());
    }
    return std::nullopt;
}

} // namespace Lua
