#pragma once

/**
 * @file debug_info.hpp
 * @brief Editor-independent source and bytecode debug-information index.
 *
 * The debugger must tolerate chunks with partial or stripped debug tables.  This
 * layer therefore never assumes that Proto::code and Proto::lineInfo have the
 * same size, and it never exposes Proto pointers as protocol identifiers.
 */

#include "common/types.hpp"

#include <optional>

namespace Lua {

class Proto;

namespace Debugger {

enum class SourceKind : u8 {
    Unknown,
    File,
    Named,
    Memory,
};

/** A display form and a comparison key derived from a Lua chunk source name. */
struct NormalizedSource {
    SourceKind kind = SourceKind::Unknown;
    Str displayName;
    Str identity;

    [[nodiscard]] bool valid() const noexcept {
        return kind != SourceKind::Unknown && !identity.empty();
    }

    [[nodiscard]] bool bindableFile() const noexcept {
        return kind == SourceKind::File;
    }
};

/**
 * Normalize Lua source names without touching the filesystem.
 *
 * `@path` denotes a file, `=name` a named virtual chunk, and every other
 * non-empty value a memory/string chunk.  Windows-looking file names use a
 * case-insensitive identity even when tests run on another host platform.
 */
[[nodiscard]] NormalizedSource normalizeSourceName(StrView rawSource);

/** Normalize an editor/host disk path that does not carry Lua's `@` marker. */
[[nodiscard]] NormalizedSource normalizeFileSourcePath(StrView path);

struct DebugCodeLocation {
    /** Internal observer only.  Protocol layers must allocate an integer ID. */
    const Proto* proto = nullptr;
    usize pc = 0;
    i32 line = 0;
    NormalizedSource source;
};

struct ProtoDebugInfoStatus {
    const Proto* proto = nullptr;
    NormalizedSource source;
    usize instructionCount = 0;
    usize lineInfoCount = 0;

    [[nodiscard]] bool hasSource() const noexcept {
        return source.valid();
    }

    [[nodiscard]] bool hasCompleteLineInfo() const noexcept {
        return instructionCount == lineInfoCount;
    }
};

/**
 * Immutable index over one Proto tree.
 *
 * Child protos with a null source inherit their parent's source for debugger
 * lookup, matching Lua binary-chunk semantics.  Only positive line entries
 * whose PC is inside the instruction array become executable locations.
 */
class DebugInfoIndex {
public:
    explicit DebugInfoIndex(const Proto& root);

    [[nodiscard]] const Vec<ProtoDebugInfoStatus>& protoStatuses() const noexcept {
        return statuses_;
    }

    [[nodiscard]] const Vec<DebugCodeLocation>& allLocations() const noexcept {
        return locations_;
    }

    [[nodiscard]] Vec<DebugCodeLocation> locationsForLine(StrView source, i32 line) const;
    [[nodiscard]] std::optional<i32> resolveExecutableLine(StrView source, i32 requestedLine) const;
    [[nodiscard]] std::optional<DebugCodeLocation> locationForPc(const Proto& proto, usize pc) const;

private:
    void indexProto(const Proto& proto, const NormalizedSource& inheritedSource);

    Vec<ProtoDebugInfoStatus> statuses_;
    Vec<DebugCodeLocation> locations_;
};

} // namespace Debugger
} // namespace Lua
