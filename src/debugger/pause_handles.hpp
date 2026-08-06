#pragma once

/**
 * @file pause_handles.hpp
 * @brief Pause-generation-aware handles for frames and expandable values.
 */

#include "debugger/debug_types.hpp"

#include <functional>
#include <limits>

namespace Lua::Debugger {

/**
 * Assigns monotonically increasing session-local IDs to pause-owned values.
 *
 * IDs are never reused. Ending a pause erases all values, so a request using an
 * ID issued by an earlier pause deterministically receives StaleReference and
 * can never alias an object from a later pause.
 */
template <typename Id, typename T> class PauseHandleTable {
public:
    [[nodiscard]] PauseGeneration beginPause() {
        active_.clear();
        paused_ = true;
        generation_ = PauseGeneration{generation_.value() + 1};
        return generation_;
    }

    void beginPause(PauseGeneration generation) {
        active_.clear();
        paused_ = generation.valid();
        generation_ = generation;
    }

    void endPause() noexcept {
        active_.clear();
        paused_ = false;
    }

    [[nodiscard]] PauseGeneration generation() const noexcept {
        return generation_;
    }

    [[nodiscard]] bool paused() const noexcept {
        return paused_;
    }

    [[nodiscard]] DebugResult<Id> add(T value) {
        if (!paused_) {
            return std::unexpected(DebugError{DebugErrorCode::InvalidState,
                                              "debug handles can only be created while execution is paused"});
        }
        if (nextId_ == std::numeric_limits<u64>::max()) {
            return std::unexpected(DebugError{DebugErrorCode::ResourceLimit, "debug handle ID space is exhausted"});
        }

        const Id id{nextId_++};
        active_.emplace(id.value(), Entry{generation_, std::move(value)});
        return id;
    }

    [[nodiscard]] DebugResult<std::reference_wrapper<const T>> lookup(Id id) const {
        if (!id.valid() || id.value() >= nextId_) {
            return std::unexpected(DebugError{DebugErrorCode::InvalidReference, "unknown debug handle"});
        }

        const auto found = active_.find(id.value());
        if (!paused_ || found == active_.end() || found->second.generation != generation_) {
            return std::unexpected(
                DebugError{DebugErrorCode::StaleReference, "debug handle belongs to an expired pause"});
        }
        return std::cref(found->second.value);
    }

    [[nodiscard]] usize size() const noexcept {
        return active_.size();
    }

private:
    struct Entry {
        PauseGeneration generation;
        T value;
    };

    HashMap<u64, Entry> active_;
    PauseGeneration generation_;
    u64 nextId_ = 1;
    bool paused_ = false;
};

} // namespace Lua::Debugger
