/**
 * @file value_serializer.cpp
 * @brief Value → JSON 字符串序列化实现
 */

#include "debug/value_serializer.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/function.hpp"
#include "core/userdata.hpp"
#include <cmath>
#include <format>

namespace Lua {
namespace Trace {

// =========================================================================
// 内部工具
// =========================================================================

namespace {

/**
 * @brief 将指针格式化为十六进制 ID 字符串
 */
Str ptrToHex(const void* p) {
    return std::format(
        "0x{:x}",
        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(p))
    );
}

} // anonymous namespace

// =========================================================================
// 公开接口实现
// =========================================================================

const char* getValueTypeName(ValueType type) {
    switch (type) {
        case ValueType::Nil:            return "nil";
        case ValueType::Boolean:        return "boolean";
        case ValueType::LightUserdata:  return "lightuserdata";
        case ValueType::Number:         return "number";
        case ValueType::String:         return "string";
        case ValueType::Table:          return "table";
        case ValueType::Function:       return "function";
        case ValueType::Userdata:       return "userdata";
        case ValueType::Thread:         return "thread";
        default:                        return "unknown";
    }
}

Str jsonEscape(StrView s) {
    Str out;
    out.reserve(s.size() + 8);
    for (char ch : s) {
        switch (ch) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20) {
                    out += std::format("\\u{:04x}", static_cast<unsigned>(static_cast<unsigned char>(ch)));
                } else {
                    out += ch;
                }
                break;
        }
    }
    return out;
}

Str serializeValue(const Value& v) {
    switch (v.getType()) {
        case ValueType::Nil:
            return "null";

        case ValueType::Boolean:
            return v.asBoolean() ? "true" : "false";

        case ValueType::Number: {
            f64 n = v.asNumber();
            if (std::isnan(n))   return "\"NaN\"";
            if (std::isinf(n))   return n > 0 ? "\"Infinity\"" : "\"-Infinity\"";
            // 整数优化：如果是整数值，不带小数点
            if (n == std::floor(n) && std::abs(n) < 1e15) {
                return std::format("{:.0f}", n);
            }
            return std::format("{:.14g}", n);
        }

        case ValueType::String: {
            GCString* s = v.asString();
            if (!s) return "\"\"";
            return "\"" + jsonEscape(s->c_str()) + "\"";
        }

        case ValueType::Table:
            return "\"table:" + ptrToHex(v.asTable()) + "\"";

        case ValueType::Function:
            return "\"function:" + ptrToHex(v.asFunction()) + "\"";

        case ValueType::Userdata:
            return "\"userdata:" + ptrToHex(v.asUserdata()) + "\"";

        case ValueType::Thread:
            return "\"thread:" + ptrToHex(v.asThread()) + "\"";

        case ValueType::LightUserdata:
            return "\"lightuserdata:" + ptrToHex(v.asLightUserdata()) + "\"";

        default:
            return "null";
    }
}

Str serializeRegisters(const Value* base, i32 maxStack, Proto* proto, i32 pc) {
    Str out = "[";

    for (i32 i = 0; i < maxStack; ++i) {
        if (i > 0) out += ",";

        // 获取局部变量名
        const char* name = nullptr;
        if (proto) {
            name = proto->getLocalName(i + 1, pc);  // localNumber 从 1 开始
        }

        const Value& val = base[i];

        out += "{\"slot\":";
        out += std::to_string(i);
        out += ",\"name\":";
        if (name) {
            out += "\"" + jsonEscape(name) + "\"";
        } else {
            out += "null";
        }
        out += ",\"value\":";
        out += serializeValue(val);
        out += ",\"type\":\"";
        out += getValueTypeName(val.getType());
        out += "\"}";
    }

    out += "]";
    return out;
}

} // namespace Trace
} // namespace Lua
