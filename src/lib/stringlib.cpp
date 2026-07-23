/**
 * @file stringlib.cpp
 * @brief Lua 字符串库实现
 *
 * 使用现代 C++ 实现 Lua 5.1 字符串库函数，并遵循 mathlib.cpp 与 baselib.cpp 的既有模式。
 *
 * @author Lua C++ 项目
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
#include <format>
#include <limits>

namespace Lua {

// =====================================================================
// 辅助函数
// =====================================================================

/**
 * @brief 从栈中获取字符串参数
 * @param L Lua 状态指针
 * @param idx 参数索引（从 1 开始）
 * @param funcName 函数名称（用于错误消息）
 * @return 字符串指针与长度
 */
static inline const char* getStringArg(LuaState* L, i32 idx, const char* funcName, usize* len = nullptr) {
    const char* str = L->toString(idx);
    if (str == nullptr) {
        L->error(std::format("bad argument #{} to 'string.{}' (string expected)", idx, funcName).c_str());
    }
    if (len) {
        const Value& v = L->at(idx);
        *len = v.isString() ? v.asString()->getLength() : std::strlen(str);
    }
    return str;
}

/**
 * @brief 从栈中获取数值参数
 * @param L Lua 状态指针
 * @param idx 参数索引（从 1 开始）
 * @param funcName 函数名称（用于错误消息）
 * @return 数值
 */
static inline f64 getNumberArg(LuaState* L, i32 idx, const char* funcName) {
    const Value& value = L->at(idx);
    if (value.isNumber()) {
        return value.asNumber();
    }

    if (value.isString()) {
        GCString* str = value.asString();
        LuaNumber number = 0.0;
        L->consumeNativeWork(str->getLength());
        if (luaStringToNumber(str->view(), number, L->getGlobalState().getAllocator())) {
            return number;
        }
    }

    { L->error(std::format("bad argument #{} to 'string.{}' (number expected)", idx, funcName).c_str()); }
}

static i32 getIntegerArg(LuaState* L, i32 idx, const char* funcName,
                         IntegerConversion mode = IntegerConversion::Truncate) {
    const auto converted = checkedLuaInteger(getNumberArg(L, idx, funcName), mode);
    if (!converted) {
        const char* detail = converted.error() == IntegerConversionError::NotFinite     ? "finite number expected"
                             : converted.error() == IntegerConversionError::NotIntegral ? "integer expected"
                                                                                        : "number out of range";
        L->error(std::format("bad argument #{} to 'string.{}' ({})", idx, funcName, detail).c_str());
    }
    return *converted;
}

static inline const char* getStringLikeArg(LuaState* L, i32 idx, const char* funcName, usize* len = nullptr) {
    const char* str = L->toString(idx);
    if (str == nullptr) {
        L->error(std::format("bad argument #{} to 'string.{}' (string expected)", idx, funcName).c_str());
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
    L->error(std::format("invalid option '%{}' to 'format'", message).c_str());
}

[[noreturn]] static void formatError(LuaState* L, char specifier) {
    const Str message = specifier == '\0' ? "%" : Str(1, specifier);
    formatError(L, message.c_str());
}

static usize stringOutputLimit(LuaState* L) {
    const ResourcePolicy& policy = L->getGlobalState().getResourcePolicy();
    return std::min(policy.maxStringBytes, policy.maxOutputBytes);
}

static void ensureStringOutput(LuaState* L, usize current, usize addition, const char* functionName) {
    const usize limit = stringOutputLimit(L);
    if (addition > limit || current > limit - addition) {
        L->error(std::format("string.{}: result exceeds resource limit", functionName).c_str());
    }
    L->consumeNativeWork(addition == 0 ? 1 : addition);
}

template <typename String>
static void appendStringOutput(LuaState* L, String& output, const char* bytes, usize count, const char* functionName) {
    ensureStringOutput(L, output.size(), count, functionName);
    output.append(bytes, count);
}

template <typename String>
static void pushStringOutput(LuaState* L, String& output, char byte, const char* functionName) {
    ensureStringOutput(L, output.size(), 1, functionName);
    output.push_back(byte);
}

template <typename String, typename Format, typename T>
static void appendPrintfFormatted(LuaState* L, String& out, const Format& format, T value) {
    i32 required = std::snprintf(nullptr, 0, format.c_str(), value);
    if (required < 0) {
        throw std::runtime_error("string.format: snprintf failed");
    }

    ensureStringOutput(L, out.size(), static_cast<usize>(required), "format");
    LuaString buffer(LuaStdAllocator<char>(L->getGlobalState().getAllocator()));
    buffer.resize(static_cast<usize>(required) + 1, '\0');
    std::snprintf(buffer.data(), buffer.size(), format.c_str(), value);
    out.append(buffer.data(), static_cast<usize>(required));
}

static LuaString quoteLuaString(LuaState* L, const char* str, usize len) {
    const usize limit = stringOutputLimit(L);
    if (limit < 2 || len > (limit - 2) / 4) {
        L->error("string.format: result exceeds resource limit");
    }
    L->consumeNativeWork(len == 0 ? 1 : len);
    LuaString quoted(LuaStdAllocator<char>(L->getGlobalState().getAllocator()));
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
            quoted.append("\\000", 4);
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
 * @brief 调整字符串位置（Lua 使用从 1 开始的索引，并支持负索引）
 * @param pos Lua 位置（从 1 开始，负值表示从末尾起算）
 * @param len 字符串长度
 * @return 调整后的从 0 开始位置
 */
static inline usize adjustPosition(i32 pos, usize len) {
    if (pos > 0) {
        return static_cast<usize>(pos - 1); // 将从 1 开始的索引转换为从 0 开始
    } else if (pos < 0) {
        // 负索引：从末尾起算
        i32 adjusted = static_cast<i32>(len) + pos + 1;
        return adjusted > 0 ? static_cast<usize>(adjusted - 1) : 0;
    } else {
        // Lua 将 pos == 0 视为 1
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
// 基本字符串函数
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
    i32 start = getIntegerArg(L, 2, "sub");
    i32 end = (L->getTop() >= 3) ? getIntegerArg(L, 3, "sub") : static_cast<i32>(len);

    i32 startPos = luaStringPosition(start, len);
    i32 endPos = luaStringPosition(end, len);

    if (startPos < 1)
        startPos = 1;
    if (endPos > static_cast<i32>(len))
        endPos = static_cast<i32>(len);

    if (startPos > endPos) {
        L->pushString(L->getGlobalState().getStringPool().intern(""));
        return 1;
    }

    /** @brief 提取子字符串。 */
    usize startIndex = static_cast<usize>(startPos - 1);
    usize subLen = static_cast<usize>(endPos - startPos + 1);
    ensureStringOutput(L, 0, subLen, "sub");
    LuaString result(s + startIndex, s + startIndex + subLen,
                     LuaStdAllocator<char>(L->getGlobalState().getAllocator()));

    GCString* str = L->getGlobalState().getStringPool().intern(result.data(), result.size());
    L->pushString(str);
    return 1;
}

i32 str_upper(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("string.upper: missing argument");
    }

    usize len;
    const char* s = getStringArg(L, 1, "upper", &len);
    ensureStringOutput(L, 0, len, "upper");
    LuaString result(s, s + len, LuaStdAllocator<char>(L->getGlobalState().getAllocator()));
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

    GCString* str = L->getGlobalState().getStringPool().intern(result.data(), result.size());
    L->pushString(str);
    return 1;
}

i32 str_lower(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("string.lower: missing argument");
    }

    usize len;
    const char* s = getStringArg(L, 1, "lower", &len);
    ensureStringOutput(L, 0, len, "lower");
    LuaString result(s, s + len, LuaStdAllocator<char>(L->getGlobalState().getAllocator()));
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    GCString* str = L->getGlobalState().getStringPool().intern(result.data(), result.size());
    L->pushString(str);
    return 1;
}

i32 str_reverse(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("string.reverse: missing argument");
    }

    usize len;
    const char* s = getStringArg(L, 1, "reverse", &len);
    ensureStringOutput(L, 0, len, "reverse");
    LuaString result(s, s + len, LuaStdAllocator<char>(L->getGlobalState().getAllocator()));
    std::reverse(result.begin(), result.end());

    GCString* str = L->getGlobalState().getStringPool().intern(result.data(), result.size());
    L->pushString(str);
    return 1;
}

i32 str_rep(LuaState* L) {
    if (L->getTop() < 2) {
        L->error("string.rep: missing arguments");
    }

    usize len;
    const char* s = getStringArg(L, 1, "rep", &len);
    i32 n = getIntegerArg(L, 2, "rep");

    if (n <= 0) {
        L->pushString(L->getGlobalState().getStringPool().intern(""));
        return 1;
    }

    const ResourcePolicy& resources = L->getGlobalState().getResourcePolicy();
    const usize outputLimit = std::min(resources.maxStringBytes, resources.maxOutputBytes);
    if (len != 0 && static_cast<usize>(n) > outputLimit / len) {
        L->error("string.rep: result exceeds resource limit");
    }

    if (len == 0) {
        L->consumeNativeWork();
        L->pushString(L->getGlobalState().getStringPool().intern(""));
        return 1;
    }
    const usize outputSize = len * static_cast<usize>(n);

    // 构建重复字符串
    ensureStringOutput(L, 0, outputSize, "rep");
    LuaString result(LuaStdAllocator<char>(L->getGlobalState().getAllocator()));
    result.reserve(outputSize);
    for (i32 i = 0; i < n; i++) {
        result.append(s, len);
    }

    GCString* str = L->getGlobalState().getStringPool().intern(result.data(), result.size());
    L->pushString(str);
    return 1;
}

i32 str_byte(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("string.byte: missing argument");
    }

    usize len;
    const char* s = getStringArg(L, 1, "byte", &len);

    i32 start = (L->getTop() >= 2) ? getIntegerArg(L, 2, "byte") : 1;
    i32 startPos = luaStringPosition(start, len);
    i32 endPos = (L->getTop() >= 3) ? luaStringPosition(getIntegerArg(L, 3, "byte"), len) : startPos;

    if (startPos <= 0)
        startPos = 1;
    if (endPos > static_cast<i32>(len))
        endPos = static_cast<i32>(len);

    if (startPos > endPos) {
        return 0; // 不返回值
    }

    const usize returnCount = static_cast<usize>(endPos - startPos + 1);
    if (returnCount > L->getGlobalState().getResourcePolicy().maxReturnValues) {
        L->error("string.byte: return value limit exceeded");
    }
    if (returnCount > std::numeric_limits<usize>::max() - L->getAbsoluteTop()) {
        throw StackOverflowError("stack overflow: resource stack slot limit exceeded");
    }
    L->getStack().checkLimit(L->getAbsoluteTop() + returnCount);
    L->consumeNativeWork(returnCount);

    /** @brief 压入各字节的数值。 */
    i32 count = 0;
    for (i32 i = startPos; i <= endPos; i++) {
        L->pushNumber(static_cast<f64>(static_cast<unsigned char>(s[i - 1])));
        count++;
    }

    return count;
}

i32 str_char(LuaState* L) {
    i32 n = L->getTop();

    ensureStringOutput(L, 0, static_cast<usize>(n), "char");
    LuaString result(LuaStdAllocator<char>(L->getGlobalState().getAllocator()));
    result.reserve(n);

    for (i32 i = 1; i <= n; i++) {
        i32 c = getIntegerArg(L, i, "char");

        if (c < 0 || c > 255) {
            L->error(std::format("bad argument #{} to 'string.char' (value out of range)", i).c_str());
        }

        result.push_back(static_cast<char>(c));
    }

    GCString* str = L->getGlobalState().getStringPool().intern(result.data(), result.size());
    L->pushString(str);
    return 1;
}

// =====================================================================
// Lua 5.1 模式匹配引擎
// =====================================================================

/** @brief 纯文本搜索，供 plain=true 的 string.find 使用。 */
static i32 plainFind(LuaState* L, const char* s, usize slen, const char* pattern, usize plen, usize init) {
    if (plen == 0)
        return static_cast<i32>(init);
    if (init + plen > slen)
        return -1;
    usize steps = 0;
    const usize stepLimit = L->getGlobalState().getResourcePolicy().maxPatternSteps;
    for (usize i = init; i <= slen - plen; i++) {
        if (++steps > stepLimit) {
            L->error("string.find: pattern step limit exceeded");
        }
        L->consumeNativeWork(plen == 0 ? 1 : plen);
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
    usize steps;
    usize stepLimit;
    MatchCapture capture[LUA_MAXCAPTURES];
};

static void patternStep(MatchState* ms, usize units = 1) {
    if (units > ms->stepLimit || ms->steps > ms->stepLimit - units) {
        ms->L->error("string pattern step limit exceeded");
    }
    ms->steps += units;
    ms->L->consumeNativeWork(units);
}

struct PatternCursor {
    const char* current = nullptr;
    const char* end = nullptr;
};

using MatchResult = Opt<const char*>;

/** @brief 模式匹配辅助函数的前置声明。 */
static const char* lmatch(MatchState* ms, const char* s, const char* p);

static MatchResult tryMatch(MatchState* ms, PatternCursor source, const char* pattern) {
    patternStep(ms);
    if (source.current == nullptr || source.current > source.end) {
        return std::nullopt;
    }

    if (const char* result = lmatch(ms, source.current, pattern)) {
        return result;
    }
    return std::nullopt;
}

static i32 matchclass(i32 c, i32 cl) {
    i32 res;
    i32 lcl = std::tolower(cl);
    switch (lcl) {
    case 'a':
        res = std::isalpha(c);
        break;
    case 'c':
        res = std::iscntrl(c);
        break;
    case 'd':
        res = std::isdigit(c);
        break;
    case 'l':
        res = std::islower(c);
        break;
    case 'p':
        res = std::ispunct(c);
        break;
    case 's':
        res = std::isspace(c);
        break;
    case 'u':
        res = std::isupper(c);
        break;
    case 'w':
        res = std::isalnum(c);
        break;
    case 'x':
        res = std::isxdigit(c);
        break;
    case 'z':
        res = (c == '\0');
        break;
    default:
        return (cl == c) ? 1 : 0;
    }
    if (std::isupper(cl))
        res = !res;
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
        if (p < p_end && *p == '^')
            p++;
        do {
            if (p >= p_end)
                return p;
            if (*p == L_ESC && p + 1 < p_end)
                p++;
            p++;
        } while (p < p_end && *p != ']');
        return (p < p_end) ? p + 1 : p;
    default:
        return p;
    }
}

static i32 singlematch(i32 c, const char* p, const char* ep) {
    switch (*p) {
    case '.':
        return 1;
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
                if (static_cast<unsigned char>(*(p - 2)) <= c && c <= static_cast<unsigned char>(*p))
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
    if (p >= ms->p_end - 1)
        return nullptr;
    if (*s != *p)
        return nullptr;
    i32 b = *p;
    i32 e = *(p + 1);
    i32 cont = 1;
    while (++s < ms->src_end) {
        patternStep(ms);
        if (*s == e) {
            if (--cont == 0)
                return s + 1;
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
    if (l < 0)
        return nullptr;
    usize len = static_cast<usize>(ms->capture[l].len);
    if (static_cast<usize>(ms->src_end - s) >= len && std::memcmp(ms->capture[l].init, s, len) == 0)
        return s + len;
    return nullptr;
}

static const char* max_expand(MatchState* ms, const char* s, const char* p, const char* ep) {
    i32 i = 0;
    while (s + i < ms->src_end && singlematch(static_cast<unsigned char>(*(s + i)), p, ep)) {
        patternStep(ms);
        i++;
    }
    while (i >= 0) {
        patternStep(ms);
        const char* res = lmatch(ms, s + i, ep + 1);
        if (res)
            return res;
        i--;
    }
    return nullptr;
}

static const char* min_expand(MatchState* ms, const char* s, const char* p, const char* ep) {
    for (;;) {
        patternStep(ms);
        const char* res = lmatch(ms, s, ep + 1);
        if (res)
            return res;
        if (s < ms->src_end && singlematch(static_cast<unsigned char>(*s), p, ep))
            s++;
        else
            return nullptr;
    }
}

static const char* start_capture(MatchState* ms, const char* s, const char* p, ptrdiff_t what) {
    i32 level = ms->level;
    if (level >= LUA_MAXCAPTURES)
        return nullptr;
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
    patternStep(ms);
    if (p >= ms->p_end)
        return s;
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
            if (!s)
                return nullptr;
            p += 4;
            goto init;
        }
        case 'f': {
            const char* ep2;
            i32 previous;
            i32 current;
            p += 2;
            if (*p != '[')
                return nullptr;
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
                if (!s)
                    return nullptr;
                p += 2;
                goto init;
            }
            goto dflt;
        }
    }
    default:
    dflt: {
        const char* ep = classend(p, ms->p_end);
        i32 m = (s < ms->src_end) && singlematch(static_cast<unsigned char>(*s), p, ep);
        if (ep < ms->p_end) {
            switch (*ep) {
            case '?': {
                if (m) {
                    const char* res = lmatch(ms, s + 1, ep + 1);
                    if (res)
                        return res;
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
        if (!m)
            return nullptr;
        s++;
        p = ep;
        goto init;
    }
    }
}

/** @brief 压入一个捕获结果；没有捕获时压入完整匹配。 */
static void push_onecapture(MatchState* ms, i32 i, const char* s, const char* e) {
    LuaState* L = ms->L;
    if (i >= ms->level) {
        if (i == 0) {
            L->pushString(L->getGlobalState().getStringPool().intern(s, static_cast<usize>(e - s)));
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
            L->pushString(L->getGlobalState().getStringPool().intern(ms->capture[i].init, static_cast<usize>(cl)));
        }
    }
}

/** @brief 返回要压入的捕获数量，至少为 1。 */
static i32 push_captures(MatchState* ms, const char* s, const char* e) {
    i32 nlevels = (ms->level == 0) ? 1 : ms->level;
    ms->L->getStack().checkLimit(ms->L->getAbsoluteTop() + static_cast<usize>(nlevels));
    for (i32 i = 0; i < nlevels; i++)
        push_onecapture(ms, i, s, e);
    return nlevels;
}

/** @brief 为字符串 s 上的模式 p 准备 MatchState。 */
static void prepareMatchState(MatchState* ms, LuaState* L, const char* s, usize slen, const char* p, usize plen) {
    ms->L = L;
    ms->src_init = s;
    ms->src_end = s + slen;
    ms->p_end = p + plen;
    ms->level = 0;
    ms->steps = 0;
    ms->stepLimit = L->getGlobalState().getResourcePolicy().maxPatternSteps;
}

// =====================================================================
/** @brief 字符串查找函数。 */
// =====================================================================

i32 str_find(LuaState* L) {
    if (L->getTop() < 2) {
        L->error("string.find: missing arguments");
    }

    usize slen, plen;
    const char* s = getStringArg(L, 1, "find", &slen);
    const char* pattern = getStringArg(L, 2, "find", &plen);

    i32 init = (L->getTop() >= 3) ? getIntegerArg(L, 3, "find") : 1;
    bool plain = (L->getTop() >= 4) ? L->toBoolean(4) : false;

    usize initPos = adjustPosition(init, slen);
    if (initPos > slen)
        initPos = slen;

    if (plain) {
        i32 pos = plainFind(L, s, slen, pattern, plen, initPos);
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
        MatchResult res = tryMatch(&ms, PatternCursor{s + i, s + slen}, p);
        if (res.has_value()) {
            L->pushNumber(static_cast<f64>(i + 1));
            L->pushNumber(static_cast<f64>(res.value() - s));
            /** @brief 在起止位置之后压入捕获结果。 */
            for (i32 c = 0; c < ms.level; c++)
                push_onecapture(&ms, c, s + i, res.value());
            return 2 + ms.level;
        }
        if (anchor)
            break;
    }

    L->pushNil();
    return 1;
}

// =====================================================================
/** @brief 字符串匹配函数。 */
// =====================================================================

i32 str_match(LuaState* L) {
    if (L->getTop() < 2) {
        L->error("string.match: missing arguments");
    }

    usize slen, plen;
    const char* s = getStringArg(L, 1, "match", &slen);
    const char* pattern = getStringArg(L, 2, "match", &plen);

    i32 init = (L->getTop() >= 3) ? getIntegerArg(L, 3, "match") : 1;
    usize initPos = adjustPosition(init, slen);
    if (initPos > slen)
        initPos = slen;

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
        MatchResult res = tryMatch(&ms, PatternCursor{s + i, s + slen}, p);
        if (res.has_value()) {
            return push_captures(&ms, s + i, res.value());
        }
        if (anchor)
            break;
    }

    L->pushNil();
    return 1;
}

// =====================================================================
/** @brief 字符串全局替换函数。 */
// =====================================================================

enum class GsubReplacementKind { String, Table, Function };

static Value captureToValue(MatchState* ms, i32 i, const char* s, const char* e, LuaState* L) {
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
    return Value(L->getGlobalState().getStringPool().intern(ms->capture[i].init, static_cast<usize>(cl)));
}

static void addStringReplacement(MatchState* ms, LuaString& result, const char* s, const char* e, const char* repl,
                                 usize rlen) {
    for (usize i = 0; i < rlen; i++) {
        if (repl[i] != L_ESC) {
            pushStringOutput(ms->L, result, repl[i], "gsub");
        } else {
            i++;
            if (i >= rlen)
                break;
            if (!std::isdigit(static_cast<unsigned char>(repl[i]))) {
                pushStringOutput(ms->L, result, repl[i], "gsub");
            } else if (repl[i] == '0') {
                appendStringOutput(ms->L, result, s, static_cast<usize>(e - s), "gsub");
            } else {
                i32 ci = repl[i] - '1';
                if (ci >= ms->level) {
                    if (ci == 0) {
                        appendStringOutput(ms->L, result, s, static_cast<usize>(e - s), "gsub");
                    } else {
                        ms->L->error("invalid capture index");
                    }
                } else if (ms->capture[ci].len == CAP_POSITION) {
                    const Str position = luaNumberToString(static_cast<f64>(ms->capture[ci].init - ms->src_init + 1));
                    appendStringOutput(ms->L, result, position.data(), position.size(), "gsub");
                } else if (ms->capture[ci].len >= 0) {
                    appendStringOutput(ms->L, result, ms->capture[ci].init, static_cast<usize>(ms->capture[ci].len),
                                       "gsub");
                } else {
                    ms->L->error("unfinished capture");
                }
            }
        }
    }
}

static bool addValueReplacement(LuaState* L, LuaString& result, const Value& value) {
    if (value.isNil() || (value.isBoolean() && !value.asBoolean())) {
        return false;
    }

    if (value.isString()) {
        GCString* str = value.asString();
        appendStringOutput(L, result, str->c_str(), str->getLength(), "gsub");
        return true;
    }

    if (value.isNumber()) {
        const Str number = luaNumberToString(value.asNumber());
        appendStringOutput(L, result, number.data(), number.size(), "gsub");
        return true;
    }

    L->error("string.gsub: invalid replacement value");
}

static Value getTableReplacement(MatchState* ms, Table* table, const char* s, const char* e, LuaState* L) {
    Value key = captureToValue(ms, 0, s, e, L);
    Value result;
    VM::detail::gettable(L, Value(table), key, result);
    return result;
}

static Value getFunctionReplacement(MatchState* ms, const Value& func, const char* s, const char* e, LuaState* L) {
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

    i32 maxn = (L->getTop() >= 4) ? getIntegerArg(L, 4, "gsub") : static_cast<i32>(slen + 1);

    const char* p = pattern;
    bool anchor = false;
    if (*p == '^') {
        anchor = true;
        p++;
        plen--;
    }

    MatchState ms;
    prepareMatchState(&ms, L, s, slen, p, plen);

    LuaString result(LuaStdAllocator<char>(L->getGlobalState().getAllocator()));
    result.reserve(std::min(slen, stringOutputLimit(L)));
    i32 count = 0;
    usize srcPos = 0;

    while (count < maxn) {
        ms.level = 0;
        MatchResult e = tryMatch(&ms, PatternCursor{s + srcPos, s + slen}, p);
        if (e.has_value()) {
            const char* matchEnd = e.value();
            count++;
            bool replaced = true;
            switch (replKind) {
            case GsubReplacementKind::String:
                addStringReplacement(&ms, result, s + srcPos, matchEnd, repl, rlen);
                break;
            case GsubReplacementKind::Table:
                replaced = addValueReplacement(L, result,
                                               getTableReplacement(&ms, replValue.asTable(), s + srcPos, matchEnd, L));
                break;
            case GsubReplacementKind::Function:
                replaced =
                    addValueReplacement(L, result, getFunctionReplacement(&ms, replValue, s + srcPos, matchEnd, L));
                break;
            }
            if (!replaced) {
                appendStringOutput(L, result, s + srcPos, static_cast<usize>(matchEnd - (s + srcPos)), "gsub");
            }
            // 若为空匹配，则前进一个位置
            if (matchEnd == s + srcPos) {
                if (srcPos < slen) {
                    pushStringOutput(L, result, s[srcPos], "gsub");
                    srcPos++;
                } else {
                    break;
                }
            } else {
                srcPos = static_cast<usize>(matchEnd - s);
            }
        } else {
            if (srcPos < slen) {
                pushStringOutput(L, result, s[srcPos], "gsub");
                srcPos++;
            } else {
                break;
            }
            if (anchor)
                break;
        }
    }

    // 追加剩余内容
    if (srcPos < slen) {
        appendStringOutput(L, result, s + srcPos, slen - srcPos, "gsub");
    }

    GCString* str = L->getGlobalState().getStringPool().intern(result.data(), result.size());
    L->pushString(str);
    L->pushNumber(static_cast<f64>(count));
    return 2;
}

// =====================================================================
// string.gmatch(s, pattern) → 迭代器函数
// =====================================================================

/** @brief 从调用栈获取当前 C 闭包。 */
static Function* getCurrentCClosure(LuaState* L) {
    const CallInfo& ci = L->getCurrentCallInfo();
    Value funcVal = L->getStack()[ci.func];
    return funcVal.isFunction() ? funcVal.asFunction() : nullptr;
}

/** @brief 从当前闭包获取已关闭上值的值。 */
static Value getUpval(LuaState* L, usize index) {
    Function* closure = getCurrentCClosure(L);
    if (!closure)
        L->error("gmatch: internal error");
    Upvalue* uv = closure->getUpvalue(index);
    if (!uv)
        L->error("gmatch: internal error");
    return uv->getValue(L->getStack());
}

/** @brief 设置当前闭包中已关闭上值的值。 */
static void setUpval(LuaState* L, usize index, const Value& val) {
    Function* closure = getCurrentCClosure(L);
    if (!closure)
        return;
    Upvalue* uv = closure->getUpvalue(index);
    if (!uv)
        return;
    uv->setValue(L->getStack(), val);
}

/** @brief gmatch 迭代器函数；上值：[0]=字符串，[1]=模式，[2]=位置。 */
static i32 gmatch_aux(LuaState* L) {
    Value sVal = getUpval(L, 0);
    Value pVal = getUpval(L, 1);
    Value posVal = getUpval(L, 2);

    if (!sVal.isString() || !pVal.isString())
        return 0;

    GCString* subject = sVal.asString();
    GCString* patString = pVal.asString();
    const char* s = subject->c_str();
    usize slen = subject->getLength();
    const char* pattern = patString->c_str();
    usize plen = patString->getLength();
    const auto convertedPosition = checkedLuaInteger(posVal.asNumber(), IntegerConversion::Exact);
    if (!convertedPosition.has_value() || *convertedPosition < 0) {
        return 0;
    }
    usize pos = static_cast<usize>(*convertedPosition);

    MatchState ms;
    prepareMatchState(&ms, L, s, slen, pattern, plen);

    for (usize i = pos; i <= slen; i++) {
        ms.level = 0;
        MatchResult e = tryMatch(&ms, PatternCursor{s + i, s + slen}, pattern);
        if (e.has_value()) {
            const char* matchEnd = e.value();
            // 推进位置：若为空匹配，则向前移动 1
            usize newpos = (matchEnd == s + i) ? i + 1 : static_cast<usize>(matchEnd - s);
            setUpval(L, 2, Value(static_cast<f64>(newpos)));
            return push_captures(&ms, s + i, matchEnd);
        }
    }
    return 0;
}

/** @brief 创建带已关闭上值的 C 闭包，模式与 iolib 相同。 */
static Function* createClosureWithUpvalues(LuaState* L, CFunction func, const LuaVector<Value>& upvalues) {
    Function* closure = L->getGlobalState().getGC().create<Function>(func);
    for (const Value& v : upvalues) {
        Upvalue* uv = L->getGlobalState().getGC().create<Upvalue>(v);
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

    /** @brief 去除锚点，因为 gmatch 会忽略起始锚点。 */
    const char* pat = p;
    usize patLen = plen;
    if (patLen > 0 && *pat == '^') {
        pat++;
        patLen--;
    }

    auto& pool = L->getGlobalState().getStringPool();

    LuaVector<Value> upvalues(LuaStdAllocator<Value>(L->getGlobalState().getAllocator()));
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

    const LuaStdAllocator<char> stringAllocator(L->getGlobalState().getAllocator());
    LuaString result(stringAllocator);
    if (fmtLen > stringOutputLimit(L)) {
        L->error("string.format: result exceeds resource limit");
    }
    result.reserve(fmtLen);

    i32 argIdx = 2;
    const char* p = fmt;
    const char* fmtEnd = fmt + fmtLen;

    while (p < fmtEnd) {
        if (*p == '%') {
            p++;
            if (p < fmtEnd && *p == '%') {
                pushStringOutput(L, result, '%', "format");
                p++;
                continue;
            }

            if (p >= fmtEnd) {
                formatError(L, '\0');
            }

            LuaString flags(stringAllocator);
            while (p < fmtEnd && isFormatFlag(*p)) {
                if (std::find(flags.begin(), flags.end(), *p) == flags.end()) {
                    flags.push_back(*p);
                }
                p++;
            }

            LuaString width(stringAllocator);
            while (p < fmtEnd && std::isdigit(static_cast<unsigned char>(*p)) != 0) {
                width.push_back(*p);
                p++;
            }

            LuaString precision(stringAllocator);
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
                const LuaString quoted = quoteLuaString(L, arg, len);
                appendStringOutput(L, result, quoted.data(), quoted.size(), "format");
                p++;
                continue;
            }

            LuaString printfFormat(stringAllocator);
            printfFormat.reserve(flags.size() + width.size() + precision.size() + 2);
            printfFormat.push_back('%');
            printfFormat.append(flags.data(), flags.size());
            printfFormat.append(width.data(), width.size());
            printfFormat.append(precision.data(), precision.size());
            printfFormat.push_back(spec);

            switch (spec) {
            case 's': {
                usize argLen = 0;
                const char* arg = getStringLikeArg(L, argIdx++, "format", &argLen);
                usize outLen = argLen;
                if (!precision.empty()) {
                    i32 limit = precision.size() == 1 ? 0 : std::atoi(precision.c_str() + 1);
                    if (limit >= 0) {
                        outLen = std::min(outLen, static_cast<usize>(limit));
                    }
                }
                i32 fieldWidth = width.empty() ? 0 : std::atoi(width.c_str());
                usize padding = fieldWidth > static_cast<i32>(outLen)
                                    ? static_cast<usize>(fieldWidth - static_cast<i32>(outLen))
                                    : 0;
                bool leftJustify = std::find(flags.begin(), flags.end(), '-') != flags.end();
                ensureStringOutput(L, result.size(), padding + outLen, "format");
                if (!leftJustify)
                    result.append(padding, ' ');
                result.append(arg, outLen);
                if (leftJustify)
                    result.append(padding, ' ');
                break;
            }
            case 'c': {
                i32 ch = getIntegerArg(L, argIdx++, "format");
                appendPrintfFormatted(L, result, printfFormat, ch);
                break;
            }
            case 'd':
            case 'i': {
                i32 value = getIntegerArg(L, argIdx++, "format");
                appendPrintfFormatted(L, result, printfFormat, value);
                break;
            }
            case 'u':
            case 'o':
            case 'x':
            case 'X': {
                u32 unsignedVal = static_cast<u32>(getIntegerArg(L, argIdx++, "format"));
                appendPrintfFormatted(L, result, printfFormat, unsignedVal);
                break;
            }
            case 'e':
            case 'E':
            case 'f':
            case 'g':
            case 'G': {
                f64 val = getNumberArg(L, argIdx++, "format");
                appendPrintfFormatted(L, result, printfFormat, val);
                break;
            }
            default:
                formatError(L, spec);
            }

            p++;
        } else {
            pushStringOutput(L, result, *p, "format");
            p++;
        }
    }

    GCString* str = L->getGlobalState().getStringPool().intern(result.data(), result.size());
    L->pushString(str);
    return 1;
}

class BoundedDumpWriter {
public:
    BoundedDumpWriter(LuaState* state, LuaString& output) : state_(state), output_(output) {
        const ResourcePolicy& policy = state_->getGlobalState().getResourcePolicy();
        limit_ = std::min({policy.maxStringBytes, policy.maxOutputBytes, policy.maxProtoBytes});
        maxDepth_ = state_->getGlobalState().getCompilationPolicy().maxNesting;
    }

    void bytes(const char* data, usize count) {
        if (count > limit_ || output_.size() > limit_ - count) {
            state_->error("string.dump: result exceeds resource limit");
        }
        state_->consumeNativeWork(count == 0 ? 1 : count);
        output_.append(data, count);
    }

    void byte(u8 value) {
        const char encoded = static_cast<char>(value);
        bytes(&encoded, 1);
    }

    void u32Value(u32 value) {
        char encoded[4];
        for (i32 i = 0; i < 4; ++i) {
            encoded[i] = static_cast<char>((value >> (i * 8)) & 0xffu);
        }
        bytes(encoded, sizeof(encoded));
    }

    void i32Value(i32 value) {
        u32Value(static_cast<u32>(value));
    }

    void u64Value(u64 value) {
        char encoded[8];
        for (i32 i = 0; i < 8; ++i) {
            encoded[i] = static_cast<char>((value >> (i * 8)) & 0xffu);
        }
        bytes(encoded, sizeof(encoded));
    }

    void number(LuaNumber value) {
        u64 bits = 0;
        static_assert(sizeof(bits) == sizeof(value), "LuaNumber dumping expects 64-bit double");
        std::memcpy(&bits, &value, sizeof(bits));
        u64Value(bits);
    }

    void size(usize value) {
        if (value > static_cast<usize>(std::numeric_limits<u32>::max())) {
            state_->error("string.dump: chunk too large");
        }
        u32Value(static_cast<u32>(value));
    }

    void maybeString(GCString* string) {
        if (string == nullptr) {
            u32Value(std::numeric_limits<u32>::max());
            return;
        }
        size(string->getLength());
        bytes(string->c_str(), string->getLength());
    }

    void constant(const Value& value) {
        if (value.isNil()) {
            byte(0);
        } else if (value.isBoolean()) {
            byte(1);
            byte(value.asBoolean() ? 1 : 0);
        } else if (value.isNumber()) {
            byte(3);
            number(value.asNumber());
        } else if (value.isString()) {
            byte(4);
            maybeString(value.asString());
        } else {
            byte(0);
        }
    }

    void proto(Proto* function, usize depth = 1) {
        if (function == nullptr || depth > maxDepth_) {
            state_->error("string.dump: Proto nesting limit exceeded");
        }
        maybeString(function->getSource());
        i32Value(function->getLineDefined());
        i32Value(function->getLastLineDefined());
        byte(function->getNumParams());
        byte(function->getVarargFlags());
        byte(function->getMaxStackSize());
        byte(function->getNumUpvalues());

        size(function->getInstructionCount());
        for (Instruction instruction : function->getCode())
            u32Value(instruction);
        size(function->getConstantCount());
        for (usize i = 0; i < function->getConstantCount(); ++i)
            constant(function->getConstant(i));
        size(function->getSubProtoCount());
        for (usize i = 0; i < function->getSubProtoCount(); ++i)
            proto(function->getSubProto(i), depth + 1);
        size(function->getLineInfo().size());
        for (i32 line : function->getLineInfo())
            i32Value(line);
        size(function->getLocVarCount());
        for (usize i = 0; i < function->getLocVarCount(); ++i) {
            const LocVar& local = function->getLocVar(i);
            maybeString(local.varname);
            i32Value(local.startpc);
            i32Value(local.endpc);
            i32Value(local.reg);
        }
        size(function->getUpvalueNameCount());
        for (usize i = 0; i < function->getUpvalueNameCount(); ++i)
            maybeString(function->getUpvalueName(i));
    }

private:
    LuaState* state_;
    LuaString& output_;
    usize limit_ = 0;
    usize maxDepth_ = 0;
};

i32 str_dump(LuaState* L) {
    if (L->getTop() < 1 || !L->at(1).isFunction() || L->at(1).asFunction()->isCFunction()) {
        L->error("bad argument #1 to 'string.dump' (Lua function expected)");
    }

    Function* func = L->at(1).asFunction();
    Proto* proto = func->getProto();
    if (proto == nullptr) {
        L->error("bad argument #1 to 'string.dump' (Lua function expected)");
    }

    LuaString chunk(LuaStdAllocator<char>(L->getGlobalState().getAllocator()));
    BoundedDumpWriter writer(L, chunk);
    writer.bytes("\x1bLua", 4);
    writer.byte(0x51); // Lua 5.1 版本标记
    writer.byte(0);    // 项目内部 Lua 5.1 载荷
    writer.byte(1);    // 小端序载荷
    writer.byte(static_cast<u8>(sizeof(i32)));
    writer.byte(static_cast<u8>(sizeof(usize)));
    writer.byte(static_cast<u8>(sizeof(Instruction)));
    writer.byte(static_cast<u8>(sizeof(LuaNumber)));
    writer.byte(0); // LuaNumber 为浮点数
    writer.bytes("LC++", 4);
    writer.proto(proto);

    L->pushString(L->getGlobalState().getStringPool().intern(chunk.data(), chunk.size()));
    return 1;
}

// =====================================================================
// 库注册
// =====================================================================

void StringLibModule::registerFunctions(LuaState* L) {
    if (!L) {
        return;
    }

    // 创建字符串表
    Table* stringTable = FunctionRegistrar::createLibTable(L, "string");

    // 使用 FunctionRegistrar 注册函数
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

    // Lua 5.1 通过共享字符串元表公开字符串方法
    Table* stringMT = gs.getMetatable(ValueType::String);
    if (stringMT == nullptr) {
        stringMT = gs.getGC().create<Table>();
        gs.setMetatable(ValueType::String, stringMT);
    }

    GCString* indexKey = gs.getStringPool().intern("__index");
    stringMT->set(Value(indexKey), Value(stringTable));
}

void StringLibModule::initialize(LuaState* L) {
    // 无需额外初始化
    (void)L;
}

void openStringLib(LuaState* L) {
    if (!L) {
        return;
    }

    L->requireStandardLibrary("string");
    StringLibModule module;
    module.registerFunctions(L);
    module.initialize(L);
}

} // namespace Lua
