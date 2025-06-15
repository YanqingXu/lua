#ifndef PLUGIN_INTEGRATION_TEST_HPP
#define PLUGIN_INTEGRATION_TEST_HPP

#include "../../common/types.hpp"
#include "../../lib/plugin/plugin.hpp"
#include "../../vm/state.hpp"
#include "../../lib/lib_manager.hpp"

namespace Lua {
namespace Tests {

/**
 * @brief 插件系统集成测试
 * 
 * 测试插件系统与Lua解释器核心组件的深度集成，包括：
 * - 插件与虚拟机状态的交互
 * - 插件与标准库的协同工作
 * - 插件生命周期管理
 * - 插件间通信和依赖
 * - 安全沙箱验证
 * - 性能和资源监控
 * - 错误处理和恢复
 */
class PluginIntegrationTest {
public:
    static void runAllTests();
    
private:
    // 核心集成测试
    static void testPluginVMIntegration();
    static void testPluginLibManagerIntegration();
    static void testPluginStateManagement();
    
    // 生命周期测试
    static void testPluginLifecycle();
    static void testPluginDependencyResolution();
    static void testPluginHotReload();
    
    // 功能集成测试
    static void testPluginLuaFunctionRegistration();
    static void testPluginConfigurationIntegration();
    static void testPluginCommunication();
    
    // 安全和性能测试
    static void testPluginSandboxIntegration();
    static void testPluginResourceMonitoring();
    static void testPluginErrorHandling();
    
    // 高级集成测试
    static void testMultiplePluginsCoexistence();
    static void testPluginSystemShutdown();
    static void testPluginCompatibilityChecks();
    
    // 辅助方法
    static void printTestHeader(const Str& testName);
    static void printTestResult(const Str& testName, bool passed);
    static void assertCondition(bool condition, const Str& message);
    
    // 测试环境创建
    static UPtr<State> createTestState();
    static UPtr<LibManager> createTestLibManager(State* state);
    static UPtr<PluginManager> createTestPluginSystem(State* state, LibManager* libManager);
};

} // namespace Tests
} // namespace Lua

#endif // PLUGIN_INTEGRATION_TEST_HPP