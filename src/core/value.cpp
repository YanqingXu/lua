/**
 * @file value.cpp
 * @brief Value类的实现文件
 */

#include "core/value.hpp"
#include <sstream>
#include <iomanip>

namespace Lua {

/**
 * @brief 获取值的字符串表示（用于调试）
 *
 * 实现说明：
 * 这个方法主要用于调试和日志输出，提供值的可读表示。
 * 对于GC对象，只显示类型和指针地址，不访问对象内容。
 */
std::string Value::toString() const {
    std::ostringstream oss;

    switch (getType()) {
        case ValueType::Nil:
            oss << "nil";
            break;

        case ValueType::Boolean:
            oss << (asBoolean() ? "true" : "false");
            break;

        case ValueType::Number:
            oss << std::fixed << std::setprecision(6) << asNumber();
            break;

        case ValueType::LightUserdata:
            oss << "lightuserdata: 0x" << std::hex << asLightUserdata();
            break;

        case ValueType::String:
            oss << "string: 0x" << std::hex << asString();
            break;

        case ValueType::Table:
            oss << "table: 0x" << std::hex << asTable();
            break;

        case ValueType::Function:
            oss << "function: 0x" << std::hex << asFunction();
            break;

        case ValueType::Userdata:
            oss << "userdata: 0x" << std::hex << asUserdata();
            break;

        case ValueType::Thread:
            oss << "thread: 0x" << std::hex << asThread();
            break;

        default:
            oss << "unknown";
            break;
    }

    return oss.str();
}

} // namespace Lua

