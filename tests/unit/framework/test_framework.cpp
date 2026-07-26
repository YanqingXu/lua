#include "test_framework.hpp"

#include "vm/state/lua_state.hpp"
#include "vm/state/stack.hpp"
#include "core/function.hpp"
#include <stdexcept>
#include <sstream>

namespace LuaTest {

ScopedGCRoots::ScopedGCRoots(Lua::LuaState* state) : gc_(state->getGlobalState().getGC()) {}

ScopedGCRoots::~ScopedGCRoots() {
    for (auto it = roots_.rbegin(); it != roots_.rend(); ++it) {
        gc_.removeRoot(*it);
    }
}

void ScopedGCRoots::protect(Lua::GCObject* object) {
    if (object == nullptr) {
        return;
    }
    if (object->getOwnerCollector() != &gc_) {
        throw std::invalid_argument("ScopedGCRoots cannot protect an object owned by another collector");
    }
    if (gc_.isRoot(object)) {
        return;
    }

    try {
        gc_.addRoot(object);
        roots_.push_back(object);
    } catch (...) {
        gc_.removeRoot(object);
        throw;
    }
}

LuaStdLibTestContext::LuaStdLibTestContext(StdLibOpenFunction openFunc) : state_(Lua::LuaState::newState()) {
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
        state_->setAbsoluteTop(0); // 同步 LuaState::top_
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

    // 清空栈并只压入参数（不压入函数对象）
    // 这符合 Lua 5.1 C 函数调用约定：参数从索引 1 开始
    state_->getStack().clear();
    state_->setAbsoluteTop(0); // 同步 LuaState::top_

    if (pushArgs) {
        pushArgs(state_);
    }

    // 调用 C 函数
    return func.asFunction()->getCFunction()(state_);
}

namespace {

void testSkipAccountingContract(TestSuite& suite) {
    TestSuite probe("skip-accounting-probe");
    probe.addResult(TestResult("pass", true));
    probe.addResult(TestResult("failure", false, "intentional probe failure"));
    probe.addResult(TestResult::expectedSkip("expected", "missing optional environment capability"));
    probe.addResult(TestResult::unexpectedSkip("unexpected", "unregistered skip"));

    ASSERT_EQ(suite, 1, probe.getPassCount(), "skip accounting keeps ordinary passes separate");
    ASSERT_EQ(suite, 1, probe.getFailCount(), "skip accounting keeps assertion failures separate");
    ASSERT_EQ(suite, 1, probe.getExpectedSkipCount(), "expected skips are counted explicitly");
    ASSERT_EQ(suite, 1, probe.getUnexpectedSkipCount(), "unexpected skips are counted explicitly");
    ASSERT_EQ(suite, 2, probe.getBlockingCount(), "unexpected skips fail the enclosing test run");
    ASSERT_EQ(suite, 4, probe.getTotalCount(), "all outcomes contribute to the result total");

    std::ostringstream junit;
    probe.writeJUnitXml(junit);
    const std::string xml = junit.str();
    ASSERT_TRUE(suite,
                xml.find("<testsuite name=\"skip-accounting-probe\" tests=\"4\" failures=\"2\" skipped=\"1\">") !=
                    std::string::npos,
                "JUnit totals classify four outcomes");
    ASSERT_TRUE(suite, xml.find("name=\"pass\" />") != std::string::npos, "JUnit classifies a pass");
    ASSERT_TRUE(suite, xml.find("name=\"failure\">") != std::string::npos, "JUnit classifies an assertion failure");
    ASSERT_TRUE(suite,
                xml.find("name=\"expected\">") != std::string::npos &&
                    xml.find("<skipped message=\"missing optional environment capability\" />") != std::string::npos,
                "JUnit classifies an expected skip as skipped");
    ASSERT_TRUE(suite,
                xml.find("name=\"unexpected\">") != std::string::npos &&
                    xml.find("<failure type=\"unexpected-skip\" message=\"unregistered skip\">") != std::string::npos,
                "JUnit classifies an unexpected skip as a failure");
}

} // namespace

void registerTestFrameworkContractTests() {
    TestRegistry::getInstance().registerTest("Test Framework Contract", "Skip accounting is fail-closed",
                                             testSkipAccountingContract);
}

} // namespace LuaTest
