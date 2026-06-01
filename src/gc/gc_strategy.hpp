#pragma once

#include "common/types.hpp"
#include <functional>

namespace Lua {

class GarbageCollector;
class LuaState;
class StringPool;

struct GCContext {
    GarbageCollector& collector;
    StringPool& stringPool;
    LuaState* currentState;
};

class GCStrategy {
public:
    virtual ~GCStrategy() = default;

    [[nodiscard]] virtual usize collect(GCContext& context) const = 0;
    [[nodiscard]] virtual const char* name() const noexcept = 0;
    [[nodiscard]] virtual const char* summary() const noexcept = 0;
};

class MarkSweepGC final : public GCStrategy {
public:
    [[nodiscard]] usize collect(GCContext& context) const override;
    [[nodiscard]] const char* name() const noexcept override;
    [[nodiscard]] const char* summary() const noexcept override;
};

class IncrementalGC final : public GCStrategy {
public:
    [[nodiscard]] usize collect(GCContext& context) const override;
    [[nodiscard]] const char* name() const noexcept override;
    [[nodiscard]] const char* summary() const noexcept override;
};

[[nodiscard]] const GCStrategy& markSweepGCStrategy() noexcept;
[[nodiscard]] const GCStrategy& incrementalGCStrategy() noexcept;
[[nodiscard]] Opt<std::reference_wrapper<const GCStrategy>> findGCStrategy(StrView name) noexcept;

}  // namespace Lua
