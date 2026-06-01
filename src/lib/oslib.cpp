#include "lib/oslib.hpp"
#include "lib/iolib.hpp"
#include "lib/lib_registry.hpp"
#include "core/table.hpp"
#include "core/gc_string.hpp"
#include "vm/state/global_state.hpp"
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cerrno>
#include <cstdio>
#include <clocale>
#include <array>
#include <format>
#include <memory>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

namespace Lua {

// ===================================================================
// Helper Functions
// ===================================================================

static std::string errnoMessage(int err) {
#ifdef _MSC_VER
    char buf[256] = {};
    strerror_s(buf, sizeof(buf), err);
    return std::string(buf);
#else
    return std::string(std::strerror(err));
#endif
}

static i32 pushFileErrorResult(LuaState* L, const char* filename, int err) {
    if (err == 0) {
        err = errno;
    }
    if (err == 0) {
        err = EINVAL;
    }

    std::string message;
    if (filename != nullptr && filename[0] != '\0') {
        message = std::string(filename) + ": " + errnoMessage(err);
    } else {
        message = errnoMessage(err);
    }

    L->pushNil();
    L->pushString(L->getGlobalState().getStringPool().intern(message.c_str()));
    L->pushNumber(static_cast<f64>(err));
    return 3;
}

/**
 * @brief 设置日期表的整数字段
 */
static void setfield(LuaState* L, Table* t, const char* key, i32 value) {
    GCString* keyStr = L->getGlobalState().getStringPool().intern(key);
    t->set(Value(keyStr), Value(static_cast<f64>(value)));
}

/**
 * @brief 设置日期表的布尔字段
 */
static void setboolfield(LuaState* L, Table* t, const char* key, i32 value) {
    if (value < 0) return; // 未定义，不设置
    GCString* keyStr = L->getGlobalState().getStringPool().intern(key);
    t->set(Value(keyStr), Value(value != 0));
}

/**
 * @brief 获取日期表的整数字段
 */
static i32 getfield(LuaState* L, Table* t, const char* key, i32 defaultValue) {
    GCString* keyStr = L->getGlobalState().getStringPool().intern(key);
    Value v = t->get(Value(keyStr));
    if (v.isNumber()) {
        return static_cast<i32>(v.asNumber());
    }
    if (defaultValue < 0) {
        L->error(std::format("field '{}' missing in date table", key).c_str());
    }
    return defaultValue;
}

/**
 * @brief 获取日期表的布尔字段
 */
static i32 getboolfield(LuaState* L, Table* t, const char* key) {
    GCString* keyStr = L->getGlobalState().getStringPool().intern(key);
    Value v = t->get(Value(keyStr));
    if (v.isNil()) return -1;
    return v.asBoolean() ? 1 : 0;
}

// ===================================================================
// OS Function Implementations
// ===================================================================

i32 luaOS_clock(LuaState* L) {
    std::clock_t c = std::clock();
    if (c == static_cast<std::clock_t>(-1)) {
        L->pushNil();
        return 1;
    }
    f64 seconds = static_cast<f64>(c) / CLOCKS_PER_SEC;
    L->pushNumber(seconds);
    return 1;
}

i32 luaOS_difftime(LuaState* L) {
    if (L->getTop() < 2) {
        L->error("difftime: missing arguments");
    }
    if (!L->isNumber(1) || !L->isNumber(2)) {
        L->error("difftime: arguments must be numbers");
    }
    
    std::time_t t2 = static_cast<std::time_t>(L->toNumber(1));
    std::time_t t1 = static_cast<std::time_t>(L->toNumber(2));
    
    f64 diff = std::difftime(t2, t1);
    L->pushNumber(diff);
    return 1;
}

i32 luaOS_execute(LuaState* L) {
    if (L->getTop() < 1) {
        L->pushNumber(static_cast<f64>(std::system(nullptr)));
        return 1;
    }
    
    if (!L->isString(1)) {
        L->error("execute: command must be a string");
    }
    
    const char* command = L->toString(1);
#ifdef _WIN32
    Str wrappedCommand;
    if (command != nullptr && command[0] == '"') {
        wrappedCommand = Str("\"") + command + "\"";
        command = wrappedCommand.c_str();
    }
#endif
    i32 result = std::system(command);
    
    if (result == -1) {
        L->pushNil();
    } else {
        L->pushNumber(static_cast<f64>(result));
    }
    return 1;
}

i32 luaOS_exit(LuaState* L) {
    i32 exitCode = 0;
    if (L->getTop() >= 1 && L->isNumber(1)) {
        exitCode = static_cast<i32>(L->toNumber(1));
    }
    std::exit(exitCode);
}

i32 luaOS_getenv(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("getenv: missing argument");
    }
    if (!L->isString(1)) {
        L->error("getenv: argument must be a string");
    }

    const char* varName = L->toString(1);

#ifdef _WIN32
    char* rawValue = nullptr;
    size_t len = 0;
    errno_t err = _dupenv_s(&rawValue, &len, varName);
    std::unique_ptr<char, decltype(&std::free)> value(rawValue, &std::free);
    if (err == 0 && value != nullptr) {
        GCString* str = L->getGlobalState().getStringPool().intern(value.get());
        L->pushString(str);
    } else {
        L->pushNil();
    }
#else
    const char* value = std::getenv(varName);
    if (value) {
        GCString* str = L->getGlobalState().getStringPool().intern(value);
        L->pushString(str);
    } else {
        L->pushNil();
    }
#endif
    return 1;
}

i32 luaOS_remove(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("remove: missing argument");
    }
    if (!L->isString(1)) {
        L->error("remove: argument must be a string");
    }

    const char* filename = L->toString(1);
    errno = 0;
    if (std::remove(filename) == 0) {
        L->pushBoolean(true);
    } else {
        int err = errno;
#ifdef _WIN32
        errno = 0;
        if (releaseFileHandlesForPath(L, filename) && std::remove(filename) == 0) {
            L->pushBoolean(true);
            return 1;
        }
        if (errno != 0) {
            err = errno;
        }
#endif
        return pushFileErrorResult(L, filename, err);
    }
    return 1;
}

i32 luaOS_rename(LuaState* L) {
    if (L->getTop() < 2) {
        L->error("rename: missing arguments");
    }
    if (!L->isString(1) || !L->isString(2)) {
        L->error("rename: arguments must be strings");
    }

    const char* oldName = L->toString(1);
    const char* newName = L->toString(2);

    errno = 0;
    if (std::rename(oldName, newName) == 0) {
        L->pushBoolean(true);
    } else {
        int err = errno;
#ifdef _WIN32
        errno = 0;
        if (releaseFileHandlesForPath(L, oldName) &&
            std::rename(oldName, newName) == 0) {
            L->pushBoolean(true);
            return 1;
        }
        if (errno != 0) {
            err = errno;
        }
#endif
        return pushFileErrorResult(L, oldName, err);
    }
    return 1;
}

i32 luaOS_setlocale(LuaState* L) {
    i32 nargs = L->getTop();
    const char* locale = nullptr;
    if (nargs >= 1 && !L->isNil(1)) {
        if (!L->isString(1)) {
            L->error("setlocale: argument must be a string");
        }
        locale = L->toString(1);
    }

    i32 category = LC_ALL; // 默认类别
    if (nargs >= 2 && !L->isNil(2)) {
        if (!L->isString(2)) {
            L->error("setlocale: category must be a string");
        }
        const char* catStr = L->toString(2);
        if (std::strcmp(catStr, "all") == 0) category = LC_ALL;
        else if (std::strcmp(catStr, "collate") == 0) category = LC_COLLATE;
        else if (std::strcmp(catStr, "ctype") == 0) category = LC_CTYPE;
        else if (std::strcmp(catStr, "monetary") == 0) category = LC_MONETARY;
        else if (std::strcmp(catStr, "numeric") == 0) category = LC_NUMERIC;
        else if (std::strcmp(catStr, "time") == 0) category = LC_TIME;
        else L->error("setlocale: invalid category");
    }

    const char* result = std::setlocale(category, locale);
    if (result) {
        GCString* str = L->getGlobalState().getStringPool().intern(result);
        L->pushString(str);
    } else {
        L->pushNil();
    }
    return 1;
}

i32 luaOS_tmpname(LuaState* L) {
#ifdef _WIN32
    std::array<char, L_tmpnam> tmpBuffer{};
    errno_t err = tmpnam_s(tmpBuffer.data(), tmpBuffer.size());
    if (err == 0) {
        GCString* str = L->getGlobalState().getStringPool().intern(tmpBuffer.data());
        L->pushString(str);
    } else {
        L->error("tmpname: unable to generate a unique filename");
    }
#else
    std::array<char, L_tmpnam> tmpBuffer{};
    if (std::tmpnam(tmpBuffer.data())) {
        GCString* str = L->getGlobalState().getStringPool().intern(tmpBuffer.data());
        L->pushString(str);
    } else {
        L->error("tmpname: unable to generate a unique filename");
    }
#endif
    return 1;
}

i32 luaOS_time(LuaState* L) {
    std::time_t t;

    if (L->getTop() == 0 || L->isNil(1)) {
        // 无参数，返回当前时间
        t = std::time(nullptr);
    } else {
        // 参数必须是表
        if (!L->isTable(1)) {
            L->error("time: argument must be a table");
        }

        Table* table = L->at(1).asTable();

        // 从表中提取时间字段
        std::tm ts;
        ts.tm_sec = getfield(L, table, "sec", 0);
        ts.tm_min = getfield(L, table, "min", 0);
        ts.tm_hour = getfield(L, table, "hour", 12);
        ts.tm_mday = getfield(L, table, "day", -1);  // 必需字段
        ts.tm_mon = getfield(L, table, "month", -1) - 1;  // 必需字段，Lua使用1-12，C使用0-11
        ts.tm_year = getfield(L, table, "year", -1) - 1900;  // 必需字段，Lua使用实际年份，C使用1900年起
        ts.tm_isdst = getboolfield(L, table, "isdst");

        t = std::mktime(&ts);
    }

    if (t == static_cast<std::time_t>(-1)) {
        L->pushNil();
    } else {
        L->pushNumber(static_cast<f64>(t));
    }
    return 1;
}

i32 luaOS_date(LuaState* L) {
    // 获取格式字符串（默认为"%c"）
    const char* format = "%c";
    if (L->getTop() >= 1 && L->isString(1)) {
        format = L->toString(1);
    }

    // 获取时间戳（默认为当前时间）
    std::time_t t = std::time(nullptr);
    if (L->getTop() >= 2 && L->isNumber(2)) {
        t = static_cast<std::time_t>(L->toNumber(2));
    }

    // 检查是否使用UTC时间
    bool useUTC = false;
    if (*format == '!') {
        useUTC = true;
        format++; // 跳过'!'
    }

    // 获取时间结构
    std::tm* stm;
#ifdef _WIN32
    std::tm tm_buf;
    if (useUTC) {
        errno_t err = gmtime_s(&tm_buf, &t);
        stm = (err == 0) ? &tm_buf : nullptr;
    } else {
        errno_t err = localtime_s(&tm_buf, &t);
        stm = (err == 0) ? &tm_buf : nullptr;
    }
#else
    if (useUTC) {
        stm = std::gmtime(&t);
    } else {
        stm = std::localtime(&t);
    }
#endif

    if (stm == nullptr) {
        L->pushNil();
        return 1;
    }

    // 检查是否返回日期表
    if (std::strcmp(format, "*t") == 0) {
        Table* table = L->getGlobalState().getGC().create<Table>();

        setfield(L, table, "sec", stm->tm_sec);
        setfield(L, table, "min", stm->tm_min);
        setfield(L, table, "hour", stm->tm_hour);
        setfield(L, table, "day", stm->tm_mday);
        setfield(L, table, "month", stm->tm_mon + 1);  // C使用0-11，Lua使用1-12
        setfield(L, table, "year", stm->tm_year + 1900);  // C使用1900年起，Lua使用实际年份
        setfield(L, table, "wday", stm->tm_wday + 1);  // C使用0-6，Lua使用1-7
        setfield(L, table, "yday", stm->tm_yday + 1);  // C使用0-365，Lua使用1-366
        setboolfield(L, table, "isdst", stm->tm_isdst);

        L->pushTable(table);
    } else {
        // 使用strftime格式化
        std::array<char, 256> buffer{};
        std::size_t result = std::strftime(buffer.data(), buffer.size(), format, stm);

        if (result == 0) {
            GCString* str = L->getGlobalState().getStringPool().intern("");
            L->pushString(str);
        } else {
            GCString* str = L->getGlobalState().getStringPool().intern(buffer.data());
            L->pushString(str);
        }
    }

    return 1;
}

// ===================================================================
// Library Registration
// ===================================================================

void openOSLib(LuaState* L) {
    if (!L) return;

    // 创建os表
    Table* osTable = FunctionRegistrar::createLibTable(L, "os");

    // 注册所有OS函数到os表
    FunctionRegistrar(L)
        .addGlobal("clock", luaOS_clock)
        .addGlobal("date", luaOS_date)
        .addGlobal("difftime", luaOS_difftime)
        .addGlobal("execute", luaOS_execute)
        .addGlobal("exit", luaOS_exit)
        .addGlobal("getenv", luaOS_getenv)
        .addGlobal("remove", luaOS_remove)
        .addGlobal("rename", luaOS_rename)
        .addGlobal("setlocale", luaOS_setlocale)
        .addGlobal("time", luaOS_time)
        .addGlobal("tmpname", luaOS_tmpname)
        .commitToTable(osTable);
}

} // namespace Lua
