#pragma once

/**
 * @file source_registry.hpp
 * @brief Stable session-local IDs for normalized Lua sources.
 */

#include "debugger/debug_info.hpp"
#include "debugger/debug_types.hpp"

namespace Lua::Debugger {

struct RegisteredSource {
    SourceId id;
    NormalizedSource source;
};

class SourceRegistry {
public:
    [[nodiscard]] SourceId registerSource(StrView rawSource);
    [[nodiscard]] SourceId registerFilePath(StrView path);
    [[nodiscard]] SourceId registerNormalizedSource(const NormalizedSource& source);
    [[nodiscard]] Opt<SourceId> find(StrView rawSource) const;
    [[nodiscard]] Opt<SourceId> findFilePath(StrView path) const;
    [[nodiscard]] const RegisteredSource* lookup(SourceId id) const noexcept;

    [[nodiscard]] usize size() const noexcept {
        return sources_.size();
    }

private:
    [[nodiscard]] SourceId registerNormalized(NormalizedSource normalized);
    [[nodiscard]] Opt<SourceId> findNormalized(const NormalizedSource& normalized) const;

    HashMap<Str, SourceId> identityToId_;
    Vec<RegisteredSource> sources_;
};

} // namespace Lua::Debugger
