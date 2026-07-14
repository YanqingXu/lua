/**
 * @file test_framework.hpp
 * @brief Lua 项目测试框架适配层
 *
 * 基于独立的 test_framework 库，添加 Lua 项目特定的测试工具。
 * 通过命名空间别名保持与原有代码的兼容性。
 *
 * @author Lua C++ Project
 * @date 2025-11-14
 */

#ifndef LUA_TEST_FRAMEWORK_HPP
#define LUA_TEST_FRAMEWORK_HPP

// 引入独立的通用测试框架
#include "test_framework/test_framework.hpp"

#include "core/value.hpp"
#include "gc/garbage_collector.hpp"
#include <utility>
#include <vector>

namespace Lua {
class LuaState;
}

// ============================================================================
// 兼容性适配：将 TestFramework 命名空间映射到 LuaTest
// 确保现有测试代码无需修改即可编译
// ============================================================================
namespace LuaTest {

// 类型别名 - 保持与原有代码的兼容性
using TestResult = TestFramework::TestResult;
using TestSuite = TestFramework::TestSuite;
using TestRegistry = TestFramework::TestRegistry;

// ============================================================================
// Lua 项目特有的测试工具
// ============================================================================

using StdLibOpenFunction = void (*)(Lua::LuaState*);

class ScopedGCRoots {
public:
    explicit ScopedGCRoots(Lua::LuaState* state);
    ~ScopedGCRoots();

    ScopedGCRoots(const ScopedGCRoots&) = delete;
    ScopedGCRoots& operator=(const ScopedGCRoots&) = delete;
    ScopedGCRoots(ScopedGCRoots&&) = delete;
    ScopedGCRoots& operator=(ScopedGCRoots&&) = delete;

    template <typename T, typename... Args> T* create(Args&&... args) {
        T* object = gc_.createRoot<T>(std::forward<Args>(args)...);
        try {
            roots_.push_back(object);
        } catch (...) {
            gc_.removeRoot(object);
            gc_.destroyManagedObject(object);
            throw;
        }
        return object;
    }

    // Acquire a root only for an unrooted object owned by this scope's
    // collector. An already-rooted object's existing owner remains
    // responsible for keeping that root alive.
    void protect(Lua::GCObject* object);

private:
    Lua::GarbageCollector& gc_;
    std::vector<Lua::GCObject*> roots_;
};

class LuaStdLibTestContext {
public:
    explicit LuaStdLibTestContext(StdLibOpenFunction openFunc = nullptr);
    ~LuaStdLibTestContext();

    LuaStdLibTestContext(const LuaStdLibTestContext&) = delete;
    LuaStdLibTestContext& operator=(const LuaStdLibTestContext&) = delete;
    LuaStdLibTestContext(LuaStdLibTestContext&&) = delete;
    LuaStdLibTestContext& operator=(LuaStdLibTestContext&&) = delete;

    Lua::LuaState* getState() const {
        return state_;
    }

    void clearStack() const;

    Lua::Value getGlobal(const char* name) const;

    bool ensureGlobalFunction(const char* name, TestSuite& suite, const std::string& message) const;

    int invoke(const char* name, const std::function<void(Lua::LuaState*)>& pushArgs) const;

private:
    Lua::LuaState* state_;
};

// 兼容性断言宏 - 委托到通用框架的宏
#define ASSERT_TRUE(suite, condition, testName)                                                                        \
    do {                                                                                                               \
        bool bool_result = (condition);                                                                                \
        suite.addResult(LuaTest::TestResult(testName, bool_result, bool_result ? "" : "Expected true"));               \
    } while (0)

#define ASSERT_FALSE(suite, condition, testName)                                                                       \
    do {                                                                                                               \
        bool bool_result = !(condition);                                                                               \
        suite.addResult(LuaTest::TestResult(testName, bool_result, bool_result ? "" : "Expected false"));              \
    } while (0)

#define ASSERT_EQ(suite, expected, actual, testName)                                                                   \
    do {                                                                                                               \
        bool bool_result = ((expected) == (actual));                                                                   \
        suite.addResult(LuaTest::TestResult(testName, bool_result, bool_result ? "" : "Values not equal"));            \
    } while (0)

} // namespace LuaTest

#endif // LUA_TEST_FRAMEWORK_HPP
