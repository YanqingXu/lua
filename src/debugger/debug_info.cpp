/**
 * @file debug_info.cpp
 * @brief Source normalization and safe Proto debug-information indexing.
 */

#include "debugger/debug_info.hpp"

#include "core/function.hpp"
#include "core/gc_string.hpp"

#include <algorithm>
#include <utility>

namespace Lua::Debugger {

namespace {

bool hasWindowsDrivePrefix(StrView path) {
    if (path.size() < 2 || path[1] != ':') {
        return false;
    }
    const char drive = path[0];
    return (drive >= 'A' && drive <= 'Z') || (drive >= 'a' && drive <= 'z');
}

Str asciiLower(Str value) {
    for (char& ch : value) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch - 'A' + 'a');
        }
    }
    return value;
}

Str normalizeFilePathText(StrView sourcePath) {
    Str path(sourcePath);
    for (char& ch : path) {
        if (ch == '\\') {
            ch = '/';
        }
    }

    Str root;
    usize cursor = 0;
    bool absolute = false;
    if (hasWindowsDrivePrefix(path)) {
        root.assign(path.data(), 2);
        cursor = 2;
        if (cursor < path.size() && path[cursor] == '/') {
            absolute = true;
            ++cursor;
        }
    } else if (path.starts_with("//")) {
        root = "//";
        cursor = 2;
        absolute = true;
    } else if (!path.empty() && path.front() == '/') {
        root = "/";
        cursor = 1;
        absolute = true;
    }

    Vec<Str> segments;
    while (cursor <= path.size()) {
        const usize separator = path.find('/', cursor);
        const usize end = separator == Str::npos ? path.size() : separator;
        Str segment = path.substr(cursor, end - cursor);
        if (segment.empty() || segment == ".") {
            // Repeated separators and current-directory segments do not
            // participate in source identity.
        } else if (segment == "..") {
            if (!segments.empty() && segments.back() != "..") {
                segments.pop_back();
            } else if (!absolute) {
                segments.push_back(std::move(segment));
            }
        } else {
            segments.push_back(std::move(segment));
        }

        if (separator == Str::npos) {
            break;
        }
        cursor = separator + 1;
    }

    Str result = root;
    if (absolute && hasWindowsDrivePrefix(result) && !result.ends_with('/')) {
        result.push_back('/');
    }
    for (const Str& segment : segments) {
        if (!result.empty() && !result.ends_with('/') && !(result.size() == 2 && result[1] == ':' && !absolute)) {
            result.push_back('/');
        }
        result += segment;
    }

    if (result.empty()) {
        return absolute ? Str("/") : Str(".");
    }
    return result;
}

bool usesWindowsIdentity(StrView originalPath, StrView normalizedPath) {
    return hasWindowsDrivePrefix(normalizedPath) || originalPath.find('\\') != StrView::npos;
}

} // namespace

NormalizedSource normalizeSourceName(StrView rawSource) {
    if (rawSource.empty()) {
        return {};
    }

    if (rawSource.front() == '=') {
        Str display(rawSource.substr(1));
        return {SourceKind::Named, display, Str("name:") + display};
    }

    if (rawSource.front() != '@') {
        Str display(rawSource);
        return {SourceKind::Memory, display, Str("memory:") + display};
    }

    const StrView originalPath = rawSource.substr(1);
    Str display = normalizeFilePathText(originalPath);
    Str identityPath = usesWindowsIdentity(originalPath, display) ? asciiLower(display) : display;
    return {SourceKind::File, std::move(display), Str("file:") + identityPath};
}

NormalizedSource normalizeFileSourcePath(StrView path) {
    if (path.empty()) {
        return {};
    }
    Str display = normalizeFilePathText(path);
    Str identityPath = usesWindowsIdentity(path, display) ? asciiLower(display) : display;
    return {SourceKind::File, std::move(display), Str("file:") + identityPath};
}

DebugInfoIndex::DebugInfoIndex(const Proto& root) {
    indexProto(root, {});
}

void DebugInfoIndex::indexProto(const Proto& proto, const NormalizedSource& inheritedSource) {
    NormalizedSource source = inheritedSource;
    if (proto.getSource() != nullptr) {
        source = normalizeSourceName(proto.getSource()->view());
    }

    const usize instructionCount = proto.getInstructionCount();
    const usize lineInfoCount = proto.getLineInfo().size();
    statuses_.push_back({&proto, source, instructionCount, lineInfoCount});

    const usize indexedCount = std::min(instructionCount, lineInfoCount);
    for (usize pc = 0; pc < indexedCount; ++pc) {
        const i32 line = proto.getLine(pc);
        if (line > 0 && source.valid()) {
            locations_.push_back({&proto, pc, line, source});
        }
    }

    for (usize index = 0; index < proto.getSubProtoCount(); ++index) {
        const Proto* child = proto.getSubProto(index);
        if (child != nullptr) {
            indexProto(*child, source);
        }
    }
}

Vec<DebugCodeLocation> DebugInfoIndex::locationsForLine(StrView source, i32 line) const {
    Vec<DebugCodeLocation> result;
    if (line <= 0) {
        return result;
    }

    const NormalizedSource requestedSource = normalizeSourceName(source);
    if (!requestedSource.valid()) {
        return result;
    }

    for (const DebugCodeLocation& location : locations_) {
        if (location.line == line && location.source.identity == requestedSource.identity) {
            result.push_back(location);
        }
    }
    return result;
}

std::optional<i32> DebugInfoIndex::resolveExecutableLine(StrView source, i32 requestedLine) const {
    if (requestedLine <= 0) {
        return std::nullopt;
    }

    const NormalizedSource requestedSource = normalizeSourceName(source);
    if (!requestedSource.valid()) {
        return std::nullopt;
    }

    std::optional<i32> resolved;
    for (const DebugCodeLocation& location : locations_) {
        if (location.source.identity != requestedSource.identity || location.line < requestedLine) {
            continue;
        }
        if (!resolved || location.line < *resolved) {
            resolved = location.line;
        }
    }
    return resolved;
}

std::optional<DebugCodeLocation> DebugInfoIndex::locationForPc(const Proto& proto, usize pc) const {
    for (const DebugCodeLocation& location : locations_) {
        if (location.proto == &proto && location.pc == pc) {
            return location;
        }
    }
    return std::nullopt;
}

} // namespace Lua::Debugger
