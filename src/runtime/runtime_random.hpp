#pragma once

/**
 * @file runtime_random.hpp
 * @brief Lua 数学库使用的确定性上下文级随机流
 */

#include "common/types.hpp"

#include <limits>

namespace Lua {

/** @brief 隔离在运行时上下文内的伪随机数生成器。 */
class RuntimeRandom {
public:
    using State = u64;

    static constexpr State DefaultSeed = 0;

    explicit RuntimeRandom(State seedValue = DefaultSeed) noexcept : state_(seedValue) {}

    void seed(State seedValue) noexcept {
        state_ = seedValue;
    }

    [[nodiscard]] State state() const noexcept {
        return state_;
    }

    void restore(State savedState) noexcept {
        state_ = savedState;
    }

    [[nodiscard]] u64 nextU64() noexcept {
        /**
         * @brief SplitMix64 体积小、在各平台上均具有确定性，且不共享进程状态。
         *
         * 其状态也可直接制作快照。
         */
        u64 value = (state_ += 0x9e3779b97f4a7c15ULL);
        value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
        value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
        return value ^ (value >> 31U);
    }

    [[nodiscard]] u64 bounded(u64 bound) noexcept {
        if (bound == 0) {
            return 0;
        }

        /** @brief 拒绝会使取模缩减产生偏差的短前缀区间。 */
        const u64 threshold = (0ULL - bound) % bound;
        for (;;) {
            const u64 value = nextU64();
            if (value >= threshold) {
                return value % bound;
            }
        }
    }

    [[nodiscard]] LuaNumber unitInterval() noexcept {
        constexpr LuaNumber scale = 1.0 / 9007199254740992.0; // 2^53
        return static_cast<LuaNumber>(nextU64() >> 11U) * scale;
    }

private:
    State state_;
};

} // namespace Lua
