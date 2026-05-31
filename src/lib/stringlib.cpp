/**
 * @file stringlib.cpp
 * @brief Lua String Library Implementation
 * 
 * Implements Lua 5.1 string library functions using modern C++.
 * Follows the established pattern from mathlib.cpp and baselib.cpp.
 * 
 * @author Lua C++ Project
 * @date 2026-01-23
 */

#include "lib/stringlib.hpp"
#include "common/number_conversion.hpp"
#include "lib/lib_registry.hpp"
#include "lib/lib_manager.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/upvalue.hpp"
#include "core/function.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/state/global_state.hpp"
#include "vm/vm.hpp"
#include "vm/vm_internal.hpp"
#include "gc/garbage_collector.hpp"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <limits>

namespace Lua {

// =====================================================================
// Helper Functions
// =====================================================================

/**
 * @brief Get string argument from stack
 * @param L Lua state pointer
 * @param idx Argument index (1-based)
 * @param funcName Function name (for error messages)
 * @return String pointer and length
 */
static inline const char* getStringArg(LuaState* L, i32 idx, const char* funcName, usize* len = nullptr) {
    const char* str = L->toString(idx);
    if (str == nullptr) {
        char buffer[128];
        std::snprintf(buffer, sizeof(buffer), "bad argument #%d to 'string.%s' (string expected)", idx, funcName);
        L->error(buffer);
    }
    if (len) {
        const Value& v = L->at(idx);
        *len = v.isString() ? v.asString()->getLength() : std::strlen(str);
    }
    return str;
}

/**
 * @brief Get number argument from stack
 * @param L Lua state pointer
 * @param idx Argument index (1-based)
 * @param funcName Function name (for error messages)
 * @return Number value
 */
static inline f64 getNumberArg(LuaState* L, i32 idx, const char* funcName) {
    const Value& value = L->at(idx);
    if (value.isNumber()) {
        return value.asNumber();
    }

    if (value.isString()) {
        GCString* str = value.asString();
        LuaNumber number = 0.0;
        if (luaStringToNumber(str->view(), number)) {
            return number;
        }
    }

    {
        char buffer[128];
        std::snprintf(buffer, sizeof(buffer), "bad argument #%d to 'string.%s' (number expected)", idx, funcName);
        L->error(buffer);
    }
}

static inline const char* getStringLikeArg(LuaState* L, i32 idx, const char* funcName, usize* len = nullptr) {
    const char* str = L->toString(idx);
    if (str == nullptr) {
        char buffer[128];
        std::snprintf(buffer, sizeof(buffer), "bad argument #%d to 'string.%s' (string expected)", idx, funcName);
        L->error(buffer);
    }
    if (len) {
        const Value& v = L->at(idx);
        *len = v.isString() ? v.asString()->getLength() : std::strlen(str);
    }
    return str;
}

static inline bool isFormatFlag(char ch) {
    return ch == '-' || ch == '+' || ch == ' ' || ch == '#' || ch == '0';
}

static inline bool isSupportedFormatSpecifier(char ch) {
    switch (ch) {
        case 'c':
        case 'd':
        case 'i':
        case 'e':
        case 'E':
        case 'f':
        case 'g':
        case 'G':
        case 'o':
        case 'q':
        case 's':
        case 'u':
        case 'x':
        case 'X':
            return true;
        default:
            return false;
    }
}

[[noreturn]] static void formatError(LuaState* L, const char* message) {
    char buffer[160];
    std::snprintf(buffer, sizeof(buffer), "invalid option '%%%s' to 'format'", message);
    L->error(buffer);
}

[[noreturn]] static void formatError(LuaState* L, char specifier) {
    char buffer[8];
    if (specifier == '\0') {
        std::snprintf(buffer, sizeof(buffer), "%%");
    } else {
        std::snprintf(buffer, sizeof(buffer), "%c", specifier);
    }
    formatError(L, buffer);
}

template <typename T>
static void appendPrintfFormatted(Str& out, const Str& format, T value) {
    i32 required = std::snprintf(nullptr, 0, format.c_str(), value);
    if (required < 0) {
        throw std::runtime_error("string.format: snprintf failed");
    }

    Str buffer(static_cast<usize>(required) + 1, '\0');
    std::snprintf(buffer.data(), buffer.size(), format.c_str(), value);
    out.append(buffer.data(), static_cast<usize>(required));
}

static Str quoteLuaString(const char* str, usize len) {
    Str quoted;
    quoted.reserve(len + 2);
    quoted.push_back('"');

    for (usize i = 0; i < len; ++i) {
        unsigned char ch = static_cast<unsigned char>(str[i]);
        switch (ch) {
            case '"':
            case '\\':
            case '\n':
                quoted.push_back('\\');
                quoted.push_back(static_cast<char>(ch));
                break;
            case '\0':
                quoted.append("\\000");
                break;
            default:
                quoted.push_back(static_cast<char>(ch));
                break;
        }
    }

    quoted.push_back('"');
    return quoted;
}

/**
 * @brief Adjust string position (Lua uses 1-based indexing, supports negative indices)
 * @param pos Position from Lua (1-based, negative from end)
 * @param len String length
 * @return Adjusted 0-based position
 */
static inline usize adjustPosition(i32 pos, usize len) {
    if (pos > 0) {
        return static_cast<usize>(pos - 1);  // Convert 1-based to 0-based
    } else if (pos < 0) {
        // Negative index: count from end
        i32 adjusted = static_cast<i32>(len) + pos + 1;
        return adjusted > 0 ? static_cast<usize>(adjusted - 1) : 0;
    } else {
        // pos == 0 is treated as 1 in Lua
        return 0;
    }
}

static inline i32 luaStringPosition(i32 pos, usize len) {
    if (pos >= 0) {
        return pos;
    }

    const i32 signedLen = static_cast<i32>(len);
    if (pos < -signedLen) {
        return 0;
    }

    return signedLen + pos + 1;
}

// =====================================================================
// Basic String Functions
// =====================================================================

i32 str_len(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("string.len: missing argument");
    }
    
    usize len;
    getStringArg(L, 1, "len", &len);
    
    L->pushNumber(static_cast<f64>(len));
    return 1;
}

i32 str_sub(LuaState* L) {
    if (L->getTop() < 2) {
        L->error("string.sub: missing arguments");
    }
    
    usize len;
    const char* s = getStringArg(L, 1, "sub", &len);
    i32 start = static_cast<i32>(getNumberArg(L, 2, "sub"));
    i32 end = (L->getTop() >= 3) ? static_cast<i32>(getNumberArg(L, 3, "sub")) : static_cast<i32>(len);
    
    i32 startPos = luaStringPosition(start, len);
    i32 endPos = luaStringPosition(end, len);

    if (startPos < 1) startPos = 1;
    if (endPos > static_cast<i32>(len)) endPos = static_cast<i32>(len);

    if (startPos > endPos) {
        L->pushString(L->getGlobalState().getStringPool().intern(""));
        return 1;
    }

    // Extract substring
    usize startIndex = static_cast<usize>(startPos - 1);
    usize subLen = static_cast<usize>(endPos - startPos + 1);
    Str result(s + startIndex, subLen);
    
    GCString* str = L->getGlobalState().getStringPool().intern(result);
    L->pushString(str);
    return 1;
}

i32 str_upper(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("string.upper: missing argument");
    }

    usize len;
    const char* s = getStringArg(L, 1, "upper", &len);

    Str result(s, len);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

    GCString* str = L->getGlobalState().getStringPool().intern(result);
    L->pushString(str);
    return 1;
}

i32 str_lower(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("string.lower: missing argument");
    }

    usize len;
    const char* s = getStringArg(L, 1, "lower", &len);

    Str result(s, len);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    GCString* str = L->getGlobalState().getStringPool().intern(result);
    L->pushString(str);
    return 1;
}

i32 str_reverse(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("string.reverse: missing argument");
    }

    usize len;
    const char* s = getStringArg(L, 1, "reverse", &len);

    Str result(s, len);
    std::reverse(result.begin(), result.end());

    GCString* str = L->getGlobalState().getStringPool().intern(result);
    L->pushString(str);
    return 1;
}

i32 str_rep(LuaState* L) {
    if (L->getTop() < 2) {
        L->error("string.rep: missing arguments");
    }

    usize len;
    const char* s = getStringArg(L, 1, "rep", &len);
    i32 n = static_cast<i32>(getNumberArg(L, 2, "rep"));

    if (n <= 0) {
        L->pushString(L->getGlobalState().getStringPool().intern(""));
        return 1;
    }

    // Build repeated string
    Str result;
    result.reserve(len * n);
    for (i32 i = 0; i < n; i++) {
        result.append(s, len);
    }

    GCString* str = L->getGlobalState().getStringPool().intern(result);
    L->pushString(str);
    return 1;
}

i32 str_byte(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("string.byte: missing argument");
    }

    usize len;
    const char* s = getStringArg(L, 1, "byte", &len);

    i32 start = (L->getTop() >= 2) ? static_cast<i32>(getNumberArg(L, 2, "byte")) : 1;
    i32 startPos = luaStringPosition(start, len);
    i32 endPos = (L->getTop() >= 3)
        ? luaStringPosition(static_cast<i32>(getNumberArg(L, 3, "byte")), len)
        : startPos;

    if (startPos <= 0) startPos = 1;
    if (endPos > static_cast<i32>(len)) endPos = static_cast<i32>(len);

    if (startPos > endPos) {
        return 0;  // Return no values
    }

    // Push byte values
    i32 count = 0;
    for (i32 i = startPos; i <= endPos; i++) {
        L->pushNumber(static_cast<f64>(static_cast<unsigned char>(s[i - 1])));
        count++;
    }

    return count;
}

i32 str_char(LuaState* L) {
    i32 n = L->getTop();

    Str result;
    result.reserve(n);

    for (i32 i = 1; i <= n; i++) {
        f64 val = getNumberArg(L, i, "char");
        i32 c = static_cast<i32>(val);

        if (c < 0 || c > 255) {
            char buffer[128];
            std::snprintf(buffer, sizeof(buffer), "bad argument #%d to 'string.char' (value out of range)", i);
            L->error(buffer);
        }

        result.push_back(static_cast<char>(c));
    }

    GCString* str = L->getGlobalState().getStringPool().intern(result);
    L->pushString(str);
    return 1;
}

// =====================================================================
// Lua 5.1 Pattern Matching Engine
// =====================================================================

/// Plain text search (used by string.find with plain=true)
static i32 plainFind(const char* s, usize slen, const char* pattern, usize plen, usize init) {
    if (plen == 0) return static_cast<i32>(init);
    if (init + plen > slen) return -1;
    for (usize i = init; i <= slen - plen; i++) {
        if (std::memcmp(s + i, pattern, plen) == 0)
            return static_cast<i32>(i);
    }
    return -1;
}

static constexpr i32 LUA_MAXCAPTURES = 32;
static constexpr char L_ESC = '%';
static constexpr ptrdiff_t CAP_UNFINISHED = -1;
static constexpr ptrdiff_t CAP_POSITION = -2;

struct MatchCapture {
    const char* init;
    ptrdiff_t len;
};

struct MatchState {
    const char* src_init;
    const char* src_end;
    const char* p_end;
    LuaState* L;
    i32 level;
    MatchCapture capture[LUA_MAXCAPTURES];
};

// Forward declarations
static const char* lmatch(MatchState* ms, const char* s, const char* p);

static i32 matchclass(i32 c, i32 cl) {
    i32 res;
    i32 lcl = std::tolower(cl);
    switch (lcl) {
        case 'a': res = std::isalpha(c); break;
        case 'c': res = std::iscntrl(c); break;
        case 'd': res = std::isdigit(c); break;
        case 'l': res = std::islower(c); break;
        case 'p': res = std::ispunct(c); break;
        case 's': res = std::isspace(c); break;
        case 'u': res = std::isupper(c); break;
        case 'w': res = std::isalnum(c); break;
        case 'x': res = std::isxdigit(c); break;
        case 'z': res = (c == '\0'); break;
        default: return (cl == c) ? 1 : 0;
    }
    if (std::isupper(cl)) res = !res;
    return res ? 1 : 0;
}

static const char* classend(const char* p, const char* p_end) {
    if (p >= p_end) {
        return p;
    }

    switch (*p++) {
        case L_ESC:
            if (p >= p_end)
                return p;
            return p + 1;
        case '[':
            if (p < p_end && *p == '^') p++;
            do {
                if (p >= p_end) return p;
                if (*p == L_ESC && p + 1 < p_end) p++;
                p++;
            } while (p < p_end && *p != ']');
            return (p < p_end) ? p + 1 : p;
        default:
            return p;
    }
}

static i32 singlematch(i32 c, const char* p, const char* ep) {
    switch (*p) {
        case '.': return 1;
        case L_ESC:
            return matchclass(c, static_cast<unsigned char>(*(p + 1)));
        case '[': {
            const char* endclass = ep - 1;
            i32 sig = 1;
            if (p[1] == '^') {
                sig = 0;
                p++;
            }
            while (++p < endclass) {
                if (*p == L_ESC) {
                    p++;
                    if (matchclass(c, static_cast<unsigned char>(*p)))
                        return sig;
                } else if ((p + 2 < endclass) && p[1] == '-') {
                    p += 2;
                    if (static_cast<unsigned char>(*(p - 2)) <= c &&
                        c <= static_cast<unsigned char>(*p))
                        return sig;
                } else if (static_cast<unsigned char>(*p) == c) {
                    return sig;
                }
            }
            return !sig;
        }
        default:
            return (static_cast<unsigned char>(*p) == c) ? 1 : 0;
    }
}

static const char* matchbalance(MatchState* ms, const char* s, const char* p) {
    if (p >= ms->p_end - 1) return nullptr;
    if (*s != *p) return nullptr;
    i32 b = *p;
    i32 e = *(p + 1);
    i32 cont = 1;
    while (++s < ms->src_end) {
        if (*s == e) {
            if (--cont == 0) return s + 1;
        } else if (*s == b) {
            cont++;
        }
    }
    return nullptr;
}

static i32 check_capture(MatchState* ms, i32 l) {
    l -= '1';
    if (l < 0 || l >= ms->level || ms->capture[l].len == CAP_UNFINISHED) {
        ms->L->error("invalid capture index");
    }
    return l;
}

static const char* match_capture(MatchState* ms, const char* s, i32 l) {
    l = check_capture(ms, l);
    if (l < 0) return nullptr;
    usize len = static_cast<usize>(ms->capture[l].len);
    if (static_cast<usize>(ms->src_end - s) >= len &&
        std::memcmp(ms->capture[l].init, s, len) == 0)
        return s + len;
    return nullptr;
}

static const char* max_expand(MatchState* ms, const char* s, const char* p, const char* ep) {
    i32 i = 0;
    while (s + i < ms->src_end &&
           singlematch(static_cast<unsigned char>(*(s + i)), p, ep))
        i++;
    while (i >= 0) {
        const char* res = lmatch(ms, s + i, ep + 1);
        if (res) return res;
        i--;
    }
    return nullptr;
}

static const char* min_expand(MatchState* ms, const char* s, const char* p, const char* ep) {
    for (;;) {
        const char* res = lmatch(ms, s, ep + 1);
        if (res) return res;
        if (s < ms->src_end && singlematch(static_cast<unsigned char>(*s), p, ep))
            s++;
        else
            return nullptr;
    }
}

static const char* start_capture(MatchState* ms, const char* s, const char* p, ptrdiff_t what) {
    i32 level = ms->level;
    if (level >= LUA_MAXCAPTURES) return nullptr;
    ms->capture[level].init = s;
    ms->capture[level].len = what;
    ms->level = level + 1;
    const char* res = lmatch(ms, s, p);
    if (!res)
        ms->level--;
    return res;
}

static const char* end_capture(MatchState* ms, const char* s, const char* p) {
    for (i32 l = ms->level - 1; l >= 0; l--) {
        if (ms->capture[l].len == CAP_UNFINISHED) {
            ms->capture[l].len = s - ms->capture[l].init;
            const char* res = lmatch(ms, s, p);
            if (!res)
                ms->capture[l].len = CAP_UNFINISHED;
            return res;
        }
    }
    ms->L->error("invalid pattern capture");
}

static const char* lmatch(MatchState* ms, const char* s, const char* p) {
init:
    if (p >= ms->p_end) return s;
    switch (*p) {
        case '(':
            if (*(p + 1) == ')')
                return start_capture(ms, s, p + 2, CAP_POSITION);
            else
                return start_capture(ms, s, p + 1, CAP_UNFINISHED);
        case ')':
            return end_capture(ms, s, p + 1);
        case '$':
            if (p + 1 >= ms->p_end)
                return (s == ms->src_end) ? s : nullptr;
            goto dflt;
        case L_ESC: {
            switch (*(p + 1)) {
                case 'b': {
                    s = matchbalance(ms, s, p + 2);
                    if (!s) return nullptr;
                    p += 4;
                    goto init;
                }
                case 'f': {
                    const char* ep2;
                    i32 previous;
                    i32 current;
                    p += 2;
                    if (*p != '[') return nullptr;
                    ep2 = classend(p, ms->p_end);
                    previous = (s == ms->src_init) ? '\0' : static_cast<unsigned char>(*(s - 1));
                    current = (s < ms->src_end) ? static_cast<unsigned char>(*s) : '\0';
                    if (singlematch(previous, p, ep2) || !singlematch(current, p, ep2))
                        return nullptr;
                    p = ep2;
                    goto init;
                }
                default:
                    if (std::isdigit(static_cast<unsigned char>(*(p + 1)))) {
                        s = match_capture(ms, s, *(p + 1));
                        if (!s) return nullptr;
                        p += 2;
                        goto init;
                    }
                    goto dflt;
            }
        }
        default: dflt: {
            const char* ep = classend(p, ms->p_end);
            i32 m = (s < ms->src_end) &&
                    singlematch(static_cast<unsigned char>(*s), p, ep);
            if (ep < ms->p_end) {
                switch (*ep) {
                    case '?': {
                        if (m) {
                            const char* res = lmatch(ms, s + 1, ep + 1);
                            if (res) return res;
                        }
                        p = ep + 1;
                        goto init;
                    }
                    case '*':
                        return max_expand(ms, s, p, ep);
                    case '+':
                        return m ? max_expand(ms, s + 1, p, ep) : nullptr;
                    case '-':
                        return min_expand(ms, s, p, ep);
                }
            }
            if (!m) return nullptr;
            s++;
            p = ep;
            goto init;
        }
    }
}

/// Push one capture result (or the whole match if no captures)
static void push_onecapture(MatchState* ms, i32 i,
                             const char* s, const char* e) {
    LuaState* L = ms->L;
    if (i >= ms->level) {
        if (i == 0) {
            Str match(s, static_cast<usize>(e - s));
            L->pushString(L->getGlobalState().getStringPool().intern(match));
        } else {
            L->error("invalid capture index");
        }
    } else {
        ptrdiff_t cl = ms->capture[i].len;
        if (cl == CAP_UNFINISHED) {
            L->error("unfinished capture");
        } else if (cl == CAP_POSITION) {
            L->pushNumber(static_cast<f64>(ms->capture[i].init - ms->src_init + 1));
        } else {
            Str cap(ms->capture[i].init, static_cast<usize>(cl));
            L->pushString(L->getGlobalState().getStringPool().intern(cap));
        }
    }
}

/// Return how many captures to push (at least 1)
static i32 push_captures(MatchState* ms, const char* s, const char* e) {
    i32 nlevels = (ms->level == 0) ? 1 : ms->level;
    for (i32 i = 0; i < nlevels; i++)
        push_onecapture(ms, i, s, e);
    return nlevels;
}

/// Prepare MatchState for pattern p on string s
static void prepareMatchState(MatchState* ms, LuaState* L,
                               const char* s, usize slen,
                               const char* p, usize plen) {
    ms->L = L;
    ms->src_init = s;
    ms->src_end = s + slen;
    ms->p_end = p + plen;
    ms->level = 0;
}

// =====================================================================
// string.find(s, pattern [, init [, plain]])
// =====================================================================

i32 str_find(LuaState* L) {
    if (L->getTop() < 2) {
        L->error("string.find: missing arguments");
    }

    usize slen, plen;
    const char* s = getStringArg(L, 1, "find", &slen);
    const char* pattern = getStringArg(L, 2, "find", &plen);

    i32 init = (L->getTop() >= 3) ? static_cast<i32>(getNumberArg(L, 3, "find")) : 1;
    bool plain = (L->getTop() >= 4) ? L->toBoolean(4) : false;

    usize initPos = adjustPosition(init, slen);
    if (initPos > slen) initPos = slen;

    if (plain) {
        i32 pos = plainFind(s, slen, pattern, plen, initPos);
        if (pos >= 0) {
            L->pushNumber(static_cast<f64>(pos + 1));
            L->pushNumber(static_cast<f64>(pos + static_cast<i32>(plen)));
            return 2;
        }
        L->pushNil();
        return 1;
    }

    const char* p = pattern;
    bool anchor = false;
    if (*p == '^') {
        anchor = true;
        p++;
        plen--;
    }

    MatchState ms;
    prepareMatchState(&ms, L, s, slen, p, plen);

    for (usize i = initPos; i <= slen; i++) {
        ms.level = 0;
        const char* res = lmatch(&ms, s + i, p);
        if (res) {
            L->pushNumber(static_cast<f64>(i + 1));
            L->pushNumber(static_cast<f64>(res - s));
            // Push captures after start/end
            for (i32 c = 0; c < ms.level; c++)
                push_onecapture(&ms, c, s + i, res);
            return 2 + ms.level;
        }
        if (anchor) break;
    }

    L->pushNil();
    return 1;
}

// =====================================================================
// string.match(s, pattern [, init])
// =====================================================================

i32 str_match(LuaState* L) {
    if (L->getTop() < 2) {
        L->error("string.match: missing arguments");
    }

    usize slen, plen;
    const char* s = getStringArg(L, 1, "match", &slen);
    const char* pattern = getStringArg(L, 2, "match", &plen);

    i32 init = (L->getTop() >= 3) ? static_cast<i32>(getNumberArg(L, 3, "match")) : 1;
    usize initPos = adjustPosition(init, slen);
    if (initPos > slen) initPos = slen;

    const char* p = pattern;
    bool anchor = false;
    if (*p == '^') {
        anchor = true;
        p++;
        plen--;
    }

    MatchState ms;
    prepareMatchState(&ms, L, s, slen, p, plen);

    for (usize i = initPos; i <= slen; i++) {
        ms.level = 0;
        const char* res = lmatch(&ms, s + i, p);
        if (res) {
            return push_captures(&ms, s + i, res);
        }
        if (anchor) break;
    }

    L->pushNil();
    return 1;
}

// =====================================================================
// string.gsub(s, pattern, repl [, n])
// =====================================================================

enum class GsubReplacementKind {
    String,
    Table,
    Function
};

static Value captureToValue(MatchState* ms, i32 i,
                            const char* s, const char* e, LuaState* L) {
    if (i >= ms->level) {
        if (i == 0) {
            return Value(L->getGlobalState().getStringPool().intern(s, static_cast<usize>(e - s)));
        }
        L->error("invalid capture index");
    }

    ptrdiff_t cl = ms->capture[i].len;
    if (cl == CAP_UNFINISHED) {
        L->error("unfinished capture");
    }
    if (cl == CAP_POSITION) {
        return Value(static_cast<f64>(ms->capture[i].init - ms->src_init + 1));
    }
    return Value(L->getGlobalState().getStringPool().intern(ms->capture[i].init,
                                                            static_cast<usize>(cl)));
}

static void addStringReplacement(MatchState* ms, Str& result,
                                 const char* s, const char* e,
                                 const char* repl, usize rlen) {
    for (usize i = 0; i < rlen; i++) {
        if (repl[i] != L_ESC) {
            result.push_back(repl[i]);
        } else {
            i++;
            if (i >= rlen) break;
            if (!std::isdigit(static_cast<unsigned char>(repl[i]))) {
                result.push_back(repl[i]);
            } else if (repl[i] == '0') {
                result.append(s, static_cast<usize>(e - s));
            } else {
                i32 ci = repl[i] - '1';
                if (ci >= ms->level) {
                    if (ci == 0) {
                        result.append(s, static_cast<usize>(e - s));
                    } else {
                        ms->L->error("invalid capture index");
                    }
                } else if (ms->capture[ci].len == CAP_POSITION) {
                    char buffer[32];
                    std::snprintf(buffer, sizeof(buffer), "%.14g",
                                  static_cast<f64>(ms->capture[ci].init - ms->src_init + 1));
                    result.append(buffer);
                } else if (ms->capture[ci].len >= 0) {
                    result.append(ms->capture[ci].init,
                                  static_cast<usize>(ms->capture[ci].len));
                } else {
                    ms->L->error("unfinished capture");
                }
            }
        }
    }
}

static bool addValueReplacement(LuaState* L, Str& result, const Value& value) {
    if (value.isNil() || (value.isBoolean() && !value.asBoolean())) {
        return false;
    }

    if (value.isString()) {
        GCString* str = value.asString();
        result.append(str->c_str(), str->getLength());
        return true;
    }

    if (value.isNumber()) {
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "%.14g", value.asNumber());
        result.append(buffer);
        return true;
    }

    L->error("string.gsub: invalid replacement value");
}

static Value getTableReplacement(MatchState* ms, Table* table,
                                 const char* s, const char* e, LuaState* L) {
    Value key = captureToValue(ms, 0, s, e, L);
    Value result;
    VM::detail::gettable(L, Value(table), key, result);
    return result;
}

static Value getFunctionReplacement(MatchState* ms, const Value& func,
                                    const char* s, const char* e, LuaState* L) {
    usize savedTop = L->getAbsoluteTop();
    try {
        L->pushValue(func);
        i32 nargs = push_captures(ms, s, e);
        RuntimeServices services(L->getGlobalState());
        VM::call(services, L, nargs, 1);
        Value value = L->top();
        L->getStack().setTop(savedTop);
        L->setAbsoluteTop(savedTop);
        return value;
    } catch (...) {
        L->getStack().setTop(savedTop);
        L->setAbsoluteTop(savedTop);
        throw;
    }
}

i32 str_gsub(LuaState* L) {
    if (L->getTop() < 3) {
        L->error("string.gsub: missing arguments");
    }

    usize slen, plen;
    const char* s = getStringArg(L, 1, "gsub", &slen);
    const char* pattern = getStringArg(L, 2, "gsub", &plen);
    Value replValue = L->at(3);
    GsubReplacementKind replKind;
    const char* repl = nullptr;
    usize rlen = 0;

    if (replValue.isTable()) {
        replKind = GsubReplacementKind::Table;
    } else if (replValue.isFunction()) {
        replKind = GsubReplacementKind::Function;
    } else {
        repl = getStringLikeArg(L, 3, "gsub", &rlen);
        replValue = L->at(3);
        replKind = GsubReplacementKind::String;
    }

    i32 maxn = (L->getTop() >= 4) ? static_cast<i32>(getNumberArg(L, 4, "gsub")) : static_cast<i32>(slen + 1);

    const char* p = pattern;
    bool anchor = false;
    if (*p == '^') {
        anchor = true;
        p++;
        plen--;
    }

    MatchState ms;
    prepareMatchState(&ms, L, s, slen, p, plen);

    Str result;
    result.reserve(slen);
    i32 count = 0;
    usize srcPos = 0;

    while (count < maxn) {
        ms.level = 0;
        const char* e = lmatch(&ms, s + srcPos, p);
        if (e) {
            count++;
            bool replaced = true;
            switch (replKind) {
                case GsubReplacementKind::String:
                    addStringReplacement(&ms, result, s + srcPos, e, repl, rlen);
                    break;
                case GsubReplacementKind::Table:
                    replaced = addValueReplacement(
                        L, result, getTableReplacement(&ms, replValue.asTable(), s + srcPos, e, L));
                    break;
                case GsubReplacementKind::Function:
                    replaced = addValueReplacement(
                        L, result, getFunctionReplacement(&ms, replValue, s + srcPos, e, L));
                    break;
            }
            if (!replaced) {
                result.append(s + srcPos, static_cast<usize>(e - (s + srcPos)));
            }
            // If empty match, advance by one
            if (e == s + srcPos) {
                if (srcPos < slen) {
                    result.push_back(s[srcPos]);
                    srcPos++;
                } else {
                    break;
                }
            } else {
                srcPos = static_cast<usize>(e - s);
            }
        } else {
            if (srcPos < slen) {
                result.push_back(s[srcPos]);
                srcPos++;
            } else {
                break;
            }
            if (anchor) break;
        }
    }

    // Append remaining
    if (srcPos < slen) {
        result.append(s + srcPos, slen - srcPos);
    }

    GCString* str = L->getGlobalState().getStringPool().intern(result);
    L->pushString(str);
    L->pushNumber(static_cast<f64>(count));
    return 2;
}

// =====================================================================
// string.gmatch(s, pattern) → iterator function
// =====================================================================

/// Helper: get current C closure from call stack
static Function* getCurrentCClosure(LuaState* L) {
    const CallInfo& ci = L->getCurrentCallInfo();
    Value funcVal = L->getStack()[ci.func];
    return funcVal.isFunction() ? funcVal.asFunction() : nullptr;
}

/// Helper: get closed upvalue value from current closure
static Value getUpval(LuaState* L, usize index) {
    Function* closure = getCurrentCClosure(L);
    if (!closure) L->error("gmatch: internal error");
    Upvalue* uv = closure->getUpvalue(index);
    if (!uv) L->error("gmatch: internal error");
    return uv->getValue(L->getStack());
}

/// Helper: set closed upvalue value on current closure
static void setUpval(LuaState* L, usize index, const Value& val) {
    Function* closure = getCurrentCClosure(L);
    if (!closure) return;
    Upvalue* uv = closure->getUpvalue(index);
    if (!uv) return;
    uv->setValue(L->getStack(), val);
}

/// gmatch iterator function — upvalues: [0]=string, [1]=pattern, [2]=position
static i32 gmatch_aux(LuaState* L) {
    Value sVal   = getUpval(L, 0);
    Value pVal   = getUpval(L, 1);
    Value posVal = getUpval(L, 2);

    if (!sVal.isString() || !pVal.isString()) return 0;

    GCString* subject = sVal.asString();
    GCString* patString = pVal.asString();
    const char* s = subject->c_str();
    usize slen = subject->getLength();
    const char* pattern = patString->c_str();
    usize plen = patString->getLength();
    usize pos = static_cast<usize>(posVal.asNumber());

    MatchState ms;
    prepareMatchState(&ms, L, s, slen, pattern, plen);

    for (usize i = pos; i <= slen; i++) {
        ms.level = 0;
        const char* e = lmatch(&ms, s + i, pattern);
        if (e) {
            // Advance position: if empty match, move forward by 1
            usize newpos = (e == s + i) ? i + 1 : static_cast<usize>(e - s);
            setUpval(L, 2, Value(static_cast<f64>(newpos)));
            return push_captures(&ms, s + i, e);
        }
    }
    return 0;
}

/// Helper: create C closure with closed upvalues (same pattern as iolib)
static Function* createClosureWithUpvalues(
    LuaState* L, CFunction func, const Vec<Value>& upvalues) {
    Function* closure = new Function(func);
    L->getGlobalState().getGC().registerObject(closure);
    for (const Value& v : upvalues) {
        Upvalue* uv = Upvalue::createClosed(v);
        L->getGlobalState().getGC().registerObject(uv);
        closure->addUpvalue(uv);
    }
    return closure;
}

i32 str_gmatch(LuaState* L) {
    if (L->getTop() < 2) {
        L->error("string.gmatch: missing arguments");
    }

    usize slen, plen;
    const char* s = getStringArg(L, 1, "gmatch", &slen);
    const char* p = getStringArg(L, 2, "gmatch", &plen);

    // Strip anchor — gmatch ignores '^'
    const char* pat = p;
    usize patLen = plen;
    if (patLen > 0 && *pat == '^') {
        pat++;
        patLen--;
    }

    auto& pool = L->getGlobalState().getStringPool();

    Vec<Value> upvalues;
    upvalues.push_back(Value(pool.intern(s, slen)));
    upvalues.push_back(Value(pool.intern(pat, patLen)));
    upvalues.push_back(Value(0.0));

    Function* iter = createClosureWithUpvalues(L, gmatch_aux, upvalues);
    L->pushFunction(iter);
    return 1;
}

i32 str_format(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("string.format: missing format string");
    }

    usize fmtLen = 0;
    const char* fmt = getStringArg(L, 1, "format", &fmtLen);

    Str result;
    result.reserve(fmtLen + 32);

    i32 argIdx = 2;
    const char* p = fmt;
    const char* fmtEnd = fmt + fmtLen;

    while (p < fmtEnd) {
        if (*p == '%') {
            p++;
            if (p < fmtEnd && *p == '%') {
                result.push_back('%');
                p++;
                continue;
            }

            if (p >= fmtEnd) {
                formatError(L, '\0');
            }

            Str flags;
            while (p < fmtEnd && isFormatFlag(*p)) {
                if (flags.find(*p) == Str::npos) {
                    flags.push_back(*p);
                }
                p++;
            }

            Str width;
            while (p < fmtEnd && std::isdigit(static_cast<unsigned char>(*p)) != 0) {
                width.push_back(*p);
                p++;
            }

            Str precision;
            if (p < fmtEnd && *p == '.') {
                precision.push_back(*p++);
                while (p < fmtEnd && std::isdigit(static_cast<unsigned char>(*p)) != 0) {
                    precision.push_back(*p);
                    p++;
                }
            }

            if (p >= fmtEnd) {
                formatError(L, '\0');
            }

            char spec = *p;
            if (!isSupportedFormatSpecifier(spec)) {
                formatError(L, spec);
            }

            if (argIdx > L->getTop()) {
                L->error("string.format: not enough arguments");
            }

            if (spec == 'q') {
                usize len = 0;
                const char* arg = getStringLikeArg(L, argIdx++, "format", &len);
                result.append(quoteLuaString(arg, len));
                p++;
                continue;
            }

            Str printfFormat;
            printfFormat.reserve(flags.size() + width.size() + precision.size() + 2);
            printfFormat.push_back('%');
            printfFormat.append(flags);
            printfFormat.append(width);
            printfFormat.append(precision);
            printfFormat.push_back(spec);

            switch (spec) {
                case 's': {
                    usize argLen = 0;
                    const char* arg = getStringLikeArg(L, argIdx++, "format", &argLen);
                    usize outLen = argLen;
                    if (!precision.empty()) {
                        i32 limit = precision.size() == 1
                            ? 0
                            : std::atoi(precision.c_str() + 1);
                        if (limit >= 0) {
                            outLen = std::min(outLen, static_cast<usize>(limit));
                        }
                    }
                    i32 fieldWidth = width.empty() ? 0 : std::atoi(width.c_str());
                    usize padding = fieldWidth > static_cast<i32>(outLen)
                        ? static_cast<usize>(fieldWidth - static_cast<i32>(outLen))
                        : 0;
                    bool leftJustify = flags.find('-') != Str::npos;
                    if (!leftJustify) result.append(padding, ' ');
                    result.append(arg, outLen);
                    if (leftJustify) result.append(padding, ' ');
                    break;
                }
                case 'c': {
                    f64 val = getNumberArg(L, argIdx++, "format");
                    i32 ch = static_cast<i32>(val);
                    appendPrintfFormatted(result, printfFormat, ch);
                    break;
                }
                case 'd':
                case 'i': {
                    f64 val = getNumberArg(L, argIdx++, "format");
                    appendPrintfFormatted(result, printfFormat, static_cast<long long>(val));
                    break;
                }
                case 'u':
                case 'o':
                case 'x':
                case 'X': {
                    f64 val = getNumberArg(L, argIdx++, "format");
                    auto unsignedVal = static_cast<unsigned long long>(static_cast<long long>(val));
                    appendPrintfFormatted(result, printfFormat, unsignedVal);
                    break;
                }
                case 'e':
                case 'E':
                case 'f':
                case 'g':
                case 'G': {
                    f64 val = getNumberArg(L, argIdx++, "format");
                    appendPrintfFormatted(result, printfFormat, val);
                    break;
                }
                default:
                    formatError(L, spec);
            }

            p++;
        } else {
            result.push_back(*p);
            p++;
        }
    }

    GCString* str = L->getGlobalState().getStringPool().intern(result);
    L->pushString(str);
    return 1;
}

static void dumpByte(Str& out, u8 value) {
    out.push_back(static_cast<char>(value));
}

static void dumpU32(Str& out, u32 value) {
    for (i32 i = 0; i < 4; ++i) {
        out.push_back(static_cast<char>((value >> (i * 8)) & 0xffu));
    }
}

static void dumpI32(Str& out, i32 value) {
    dumpU32(out, static_cast<u32>(value));
}

static void dumpU64(Str& out, u64 value) {
    for (i32 i = 0; i < 8; ++i) {
        out.push_back(static_cast<char>((value >> (i * 8)) & 0xffu));
    }
}

static void dumpNumber(Str& out, LuaNumber value) {
    u64 bits = 0;
    static_assert(sizeof(bits) == sizeof(value), "LuaNumber dumping expects 64-bit double");
    std::memcpy(&bits, &value, sizeof(bits));
    dumpU64(out, bits);
}

static void dumpSize(Str& out, usize value) {
    if (value > static_cast<usize>(std::numeric_limits<u32>::max())) {
        throw std::runtime_error("string.dump: chunk too large");
    }
    dumpU32(out, static_cast<u32>(value));
}

static void dumpMaybeString(Str& out, GCString* str) {
    if (str == nullptr) {
        dumpU32(out, std::numeric_limits<u32>::max());
        return;
    }
    dumpSize(out, str->getLength());
    out.append(str->c_str(), str->getLength());
}

static void dumpConstant(Str& out, const Value& value) {
    if (value.isNil()) {
        dumpByte(out, 0);
    } else if (value.isBoolean()) {
        dumpByte(out, 1);
        dumpByte(out, value.asBoolean() ? 1 : 0);
    } else if (value.isNumber()) {
        dumpByte(out, 3);
        dumpNumber(out, value.asNumber());
    } else if (value.isString()) {
        dumpByte(out, 4);
        dumpMaybeString(out, value.asString());
    } else {
        dumpByte(out, 0);
    }
}

static void dumpProto(Str& out, Proto* proto) {
    dumpMaybeString(out, proto->getSource());
    dumpI32(out, proto->getLineDefined());
    dumpI32(out, proto->getLastLineDefined());
    dumpByte(out, proto->getNumParams());
    dumpByte(out, proto->getVarargFlags());
    dumpByte(out, proto->getMaxStackSize());
    dumpByte(out, proto->getNumUpvalues());

    dumpSize(out, proto->getInstructionCount());
    for (Instruction inst : proto->getCode()) {
        dumpU32(out, inst);
    }

    dumpSize(out, proto->getConstantCount());
    for (usize i = 0; i < proto->getConstantCount(); ++i) {
        dumpConstant(out, proto->getConstant(i));
    }

    dumpSize(out, proto->getSubProtoCount());
    for (usize i = 0; i < proto->getSubProtoCount(); ++i) {
        dumpProto(out, proto->getSubProto(i));
    }

    dumpSize(out, proto->getLineInfo().size());
    for (i32 line : proto->getLineInfo()) {
        dumpI32(out, line);
    }

    dumpSize(out, proto->getLocVarCount());
    for (usize i = 0; i < proto->getLocVarCount(); ++i) {
        const LocVar& loc = proto->getLocVar(i);
        dumpMaybeString(out, loc.varname);
        dumpI32(out, loc.startpc);
        dumpI32(out, loc.endpc);
        dumpI32(out, loc.reg);
    }

    dumpSize(out, proto->getUpvalueNameCount());
    for (usize i = 0; i < proto->getUpvalueNameCount(); ++i) {
        dumpMaybeString(out, proto->getUpvalueName(i));
    }
}

i32 str_dump(LuaState* L) {
    if (L->getTop() < 1 || !L->at(1).isFunction() || L->at(1).asFunction()->isCFunction()) {
        L->error("bad argument #1 to 'string.dump' (Lua function expected)");
    }

    Function* func = L->at(1).asFunction();
    Proto* proto = func->getProto();
    if (proto == nullptr) {
        L->error("bad argument #1 to 'string.dump' (Lua function expected)");
    }

    Str chunk;
    chunk.reserve(128 + proto->getInstructionCount() * sizeof(Instruction));
    chunk.append("\x1bLua", 4);
    dumpByte(chunk, 0x51); // Lua 5.1-style version marker.
    dumpByte(chunk, 0);    // Project-local dump format revision.
    dumpByte(chunk, 1);    // Little-endian payload.
    dumpByte(chunk, static_cast<u8>(sizeof(i32)));
    dumpByte(chunk, static_cast<u8>(sizeof(usize)));
    dumpByte(chunk, static_cast<u8>(sizeof(Instruction)));
    dumpByte(chunk, static_cast<u8>(sizeof(LuaNumber)));
    dumpByte(chunk, 0);    // LuaNumber is floating point.
    chunk.append("LC++", 4);
    dumpProto(chunk, proto);

    L->pushString(L->getGlobalState().getStringPool().intern(chunk.data(), chunk.size()));
    return 1;
}

// =====================================================================
// Library Registration
// =====================================================================

void StringLibModule::registerFunctions(LuaState* L) {
    if (!L) {
        return;
    }

    // Create string table
    Table* stringTable = FunctionRegistrar::createLibTable(L, "string");

    // Register functions using FunctionRegistrar
    FunctionRegistrar(L)
        .addGlobal("len", str_len)
        .addGlobal("sub", str_sub)
        .addGlobal("upper", str_upper)
        .addGlobal("lower", str_lower)
        .addGlobal("reverse", str_reverse)
        .addGlobal("rep", str_rep)
        .addGlobal("byte", str_byte)
        .addGlobal("char", str_char)
        .addGlobal("find", str_find)
        .addGlobal("gsub", str_gsub)
        .addGlobal("match", str_match)
        .addGlobal("gmatch", str_gmatch)
        .addGlobal("format", str_format)
        .addGlobal("dump", str_dump)
        .commitToTable(stringTable);

    auto& gs = L->getGlobalState();
    GCString* gmatchKey = gs.getStringPool().intern("gmatch");
    GCString* gfindKey = gs.getStringPool().intern("gfind");
    stringTable->set(Value(gfindKey), stringTable->get(Value(gmatchKey)));

    // Lua 5.1 exposes string methods through the shared string metatable.
    Table* stringMT = gs.getMetatable(ValueType::String);
    if (stringMT == nullptr) {
        stringMT = new Table();
        gs.getGC().registerObject(stringMT);
        gs.setMetatable(ValueType::String, stringMT);
    }

    GCString* indexKey = gs.getStringPool().intern("__index");
    stringMT->set(Value(indexKey), Value(stringTable));
}

void StringLibModule::initialize(LuaState* L) {
    // No additional initialization needed
    (void)L;
}

void openStringLib(LuaState* L) {
    if (!L) {
        return;
    }

    StringLibModule module;
    module.registerFunctions(L);
    module.initialize(L);
}

} // namespace Lua

