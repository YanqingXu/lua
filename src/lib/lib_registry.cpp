/**
 * @file lib_registry.cpp
 * @brief Lua函数注册工具实现
 *
 * 提供统一的函数注册实现，支持静态方法和流式接口两种使用方式。
 *
 * @author Lua C++ 项目
 * @date 2025-12-19
 */

#include "lib/lib_registry.hpp"

#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/value.hpp"
#include "vm/state/global_state.hpp"
#include "vm/state/lua_state.hpp"

#include <expected>

namespace Lua {

namespace {

std::unexpected<LibRegistrationError> registrationError(LibRegistrationErrorCode code, StrView operation) {
    return std::unexpected(LibRegistrationError{code, operation});
}

} // namespace

// =====================================================================
// 私有辅助函数
// =====================================================================

std::expected<Function*, LibRegistrationError> FunctionRegistrar::tryCreateClosure(LuaState* L, LibCFunction func) {
    if (!L) {
        return registrationError(LibRegistrationErrorCode::NullState, "createClosure");
    }
    if (!func) {
        return registrationError(LibRegistrationErrorCode::NullFunction, "createClosure");
    }

    Function* closure = L->getGlobalState().getGC().create<Function>(func);
    closure->setEnv(L->getGlobalTable());
    return closure;
}

Function* FunctionRegistrar::createClosure(LuaState* L, LibCFunction func) {
    auto created = tryCreateClosure(L, func);
    return created ? *created : nullptr;
}

// =====================================================================
// 静态方法实现：单个函数注册
// =====================================================================

void FunctionRegistrar::registerGlobal(LuaState* L, const char* name, LibCFunction func) {
    if (!L || !name || !func) {
        return;
    }

    Function* closure = createClosure(L, func);
    if (!closure) {
        return;
    }

    L->setGlobal(name, Value(closure));
}

void FunctionRegistrar::registerToTable(LuaState* L, Table* table, const char* name, LibCFunction func) {
    if (!L || !table || !name || !func) {
        return;
    }

    Function* closure = createClosure(L, func);
    if (!closure) {
        return;
    }

    GCString* key = L->getGlobalState().getStringPool().intern(name);
    table->set(Value(key), Value(closure));
}

// =====================================================================
// 静态方法实现：库表创建
// =====================================================================

Table* FunctionRegistrar::createLibTable(LuaState* L, const char* libName) {
    auto created = tryCreateLibTable(L, libName != nullptr ? StrView(libName) : StrView{});
    return created ? *created : nullptr;
}

std::expected<Table*, LibRegistrationError> FunctionRegistrar::tryCreateLibTable(LuaState* L, StrView libName) {
    if (!L) {
        return registrationError(LibRegistrationErrorCode::NullState, "createLibTable");
    }
    if (libName.empty()) {
        return registrationError(LibRegistrationErrorCode::NullName, "createLibTable");
    }

    Str ownedName(libName);
    Table* table = L->getGlobalState().getGC().create<Table>();
    L->setGlobal(ownedName.c_str(), Value(table));
    return table;
}

} // namespace Lua
