#include "test_framework.hpp"

#include "vm/lua_state.hpp"
#include "vm/stack.hpp"
#include "core/function.hpp"

namespace LuaTest {

LuaStdLibTestContext::LuaStdLibTestContext(StdLibOpenFunction openFunc)
    : state_(Lua::LuaState::newState()) {
    if (state_ && openFunc) {
        openFunc(state_);
    }
}

LuaStdLibTestContext::~LuaStdLibTestContext() {
    if (state_) {
        delete state_;
        state_ = nullptr;
    }
}

void LuaStdLibTestContext::clearStack() const {
    if (state_) {
        state_->getStack().clear();
    }
}

Lua::Value LuaStdLibTestContext::getGlobal(const char* name) const {
    if (!state_ || !name) {
        return Lua::Value();
    }
    return state_->getGlobal(name);
}

bool LuaStdLibTestContext::ensureGlobalFunction(const char* name, TestSuite& suite, const std::string& message) const {
    bool ok = getGlobal(name).isFunction();
    suite.addResult(TestResult(message, ok, ok ? "" : (std::string("missing function: ") + (name ? name : "<null>"))));
    return ok;
}

int LuaStdLibTestContext::invoke(const char* name, const std::function<void(Lua::LuaState*)>& pushArgs) const {
    if (!state_ || !name) {
        return -1;
    }

    Lua::Value func = getGlobal(name);
    if (!func.isFunction()) {
        return -1;
    }

    state_->getStack().clear();
    state_->pushFunction(func.asFunction());

    if (pushArgs) {
        pushArgs(state_);
    }

    return func.asFunction()->getCFunction()(state_);
}

} // namespace LuaTest
