/**
 * @file source_registry.cpp
 * @brief Stable session-local source registry implementation.
 */

#include "debugger/source_registry.hpp"

namespace Lua::Debugger {

SourceId SourceRegistry::registerSource(StrView rawSource) {
    return registerNormalized(normalizeSourceName(rawSource));
}

SourceId SourceRegistry::registerFilePath(StrView path) {
    return registerNormalized(normalizeFileSourcePath(path));
}

SourceId SourceRegistry::registerNormalizedSource(const NormalizedSource& source) {
    return registerNormalized(source);
}

SourceId SourceRegistry::registerNormalized(NormalizedSource normalized) {
    if (!normalized.valid()) {
        return {};
    }

    if (const auto existing = identityToId_.find(normalized.identity); existing != identityToId_.end()) {
        return existing->second;
    }

    const SourceId id{static_cast<u64>(sources_.size()) + 1};
    identityToId_.emplace(normalized.identity, id);
    sources_.push_back(RegisteredSource{id, std::move(normalized)});
    return id;
}

Opt<SourceId> SourceRegistry::find(StrView rawSource) const {
    return findNormalized(normalizeSourceName(rawSource));
}

Opt<SourceId> SourceRegistry::findFilePath(StrView path) const {
    return findNormalized(normalizeFileSourcePath(path));
}

Opt<SourceId> SourceRegistry::findNormalized(const NormalizedSource& normalized) const {
    if (!normalized.valid()) {
        return std::nullopt;
    }

    const auto found = identityToId_.find(normalized.identity);
    if (found == identityToId_.end()) {
        return std::nullopt;
    }
    return found->second;
}

const RegisteredSource* SourceRegistry::lookup(SourceId id) const noexcept {
    if (!id.valid() || id.value() > sources_.size()) {
        return nullptr;
    }
    return &sources_[static_cast<usize>(id.value() - 1)];
}

} // namespace Lua::Debugger
