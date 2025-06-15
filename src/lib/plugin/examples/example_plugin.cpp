#include "example_plugin.hpp"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace Lua {

// ExamplePlugin Implementation
ExamplePlugin::ExamplePlugin() : state_(PluginState::Unloaded), context_(nullptr) {
    metadata_.name = "ExamplePlugin";
    metadata_.version = {1, 0, 0};
    metadata_.author = "Lua Plugin System";
    metadata_.description = "A simple example plugin demonstrating basic functionality";
    metadata_.category = "Example";
    metadata_.tags = {"example", "demo", "basic"};
    metadata_.autoLoad = false;
}

StrView ExamplePlugin::getName() const {
    return metadata_.name;
}

void ExamplePlugin::registerFunctions(FunctionRegistry& registry) {
    registry.registerFunction("example_hello", luaHello);
    registry.registerFunction("example_add", luaAdd);
    registry.registerFunction("example_info", luaGetInfo);
}

bool ExamplePlugin::onLoad(PluginContext* context) {
    context_ = context;
    state_ = PluginState::Loaded;
    
    if (context_) {
        context_->logInfo("ExamplePlugin loaded successfully");
        
        // Register additional functions with context
        context_->registerFunction("example_context_test", [this](lua_State* L) {
            return this->luaContextTest(L);
        });
    }
    
    return true;
}

bool ExamplePlugin::onUnload(PluginContext* context) {
    if (context_) {
        context_->logInfo("ExamplePlugin unloading");
        context_->unregisterFunction("example_context_test");
    }
    
    state_ = PluginState::Unloaded;
    context_ = nullptr;
    return true;
}

bool ExamplePlugin::onEnable(PluginContext* context) {
    state_ = PluginState::Enabled;
    if (context_) {
        context_->logInfo("ExamplePlugin enabled");
    }
    return true;
}

bool ExamplePlugin::onDisable(PluginContext* context) {
    state_ = PluginState::Disabled;
    if (context_) {
        context_->logInfo("ExamplePlugin disabled");
    }
    return true;
}

PluginState ExamplePlugin::getState() const {
    return state_;
}

void ExamplePlugin::setState(PluginState state) {
    state_ = state;
}

const PluginMetadata& ExamplePlugin::getMetadata() const {
    return metadata_;
}

bool ExamplePlugin::configure(const HashMap<Str, Str>& config) {
    configuration_ = config;
    
    if (context_) {
        context_->logInfo("ExamplePlugin configured with " + std::to_string(config.size()) + " settings");
    }
    
    return true;
}

HashMap<Str, Str> ExamplePlugin::getConfiguration() const {
    return configuration_;
}

bool ExamplePlugin::handleMessage(StrView message, const HashMap<Str, Str>& data) {
    if (context_) {
        context_->logInfo("ExamplePlugin received message: " + std::string(message));
    }
    
    if (message == "ping") {
        // Respond to ping
        return true;
    }
    
    return false;
}

// Lua function implementations
int ExamplePlugin::luaHello(lua_State* L) {
    const char* name = luaL_optstring(L, 1, "World");
    std::string greeting = "Hello, " + std::string(name) + "! From ExamplePlugin.";
    lua_pushstring(L, greeting.c_str());
    return 1;
}

int ExamplePlugin::luaAdd(lua_State* L) {
    double a = luaL_checknumber(L, 1);
    double b = luaL_checknumber(L, 2);
    lua_pushnumber(L, a + b);
    return 1;
}

int ExamplePlugin::luaGetInfo(lua_State* L) {
    lua_newtable(L);
    
    lua_pushstring(L, "ExamplePlugin");
    lua_setfield(L, -2, "name");
    
    lua_pushstring(L, "1.0.0");
    lua_setfield(L, -2, "version");
    
    lua_pushstring(L, "A simple example plugin");
    lua_setfield(L, -2, "description");
    
    return 1;
}

int ExamplePlugin::luaContextTest(lua_State* L) {
    if (!context_) {
        return luaL_error(L, "Plugin context not available");
    }
    
    // Test context functionality
    lua_newtable(L);
    
    lua_pushstring(L, context_->getPluginName().data());
    lua_setfield(L, -2, "plugin_name");
    
    lua_pushboolean(L, context_->getLuaState() == L);
    lua_setfield(L, -2, "lua_state_match");
    
    return 1;
}

// MathPlugin Implementation
MathPlugin::MathPlugin() : state_(PluginState::Unloaded), context_(nullptr) {
    metadata_.name = "MathPlugin";
    metadata_.version = {1, 0, 0};
    metadata_.author = "Lua Plugin System";
    metadata_.description = "Mathematical functions plugin";
    metadata_.category = "Math";
    metadata_.tags = {"math", "calculation", "utility"};
    metadata_.autoLoad = false;
}

StrView MathPlugin::getName() const {
    return metadata_.name;
}

void MathPlugin::registerFunctions(FunctionRegistry& registry) {
    registry.registerFunction("factorial", luaFactorial);
    registry.registerFunction("fibonacci", luaFibonacci);
    registry.registerFunction("is_prime", luaIsPrime);
    registry.registerFunction("gcd", luaGcd);
    registry.registerFunction("lcm", luaLcm);
    registry.registerFunction("power", luaPower);
}

bool MathPlugin::onLoad(PluginContext* context) {
    context_ = context;
    state_ = PluginState::Loaded;
    
    if (context_) {
        context_->logInfo("MathPlugin loaded successfully");
    }
    
    return true;
}

bool MathPlugin::onUnload(PluginContext* context) {
    if (context_) {
        context_->logInfo("MathPlugin unloading");
    }
    
    state_ = PluginState::Unloaded;
    context_ = nullptr;
    return true;
}

bool MathPlugin::onEnable(PluginContext* context) {
    state_ = PluginState::Enabled;
    if (context_) {
        context_->logInfo("MathPlugin enabled");
    }
    return true;
}

bool MathPlugin::onDisable(PluginContext* context) {
    state_ = PluginState::Disabled;
    if (context_) {
        context_->logInfo("MathPlugin disabled");
    }
    return true;
}

PluginState MathPlugin::getState() const {
    return state_;
}

void MathPlugin::setState(PluginState state) {
    state_ = state;
}

const PluginMetadata& MathPlugin::getMetadata() const {
    return metadata_;
}

bool MathPlugin::configure(const HashMap<Str, Str>& config) {
    configuration_ = config;
    return true;
}

HashMap<Str, Str> MathPlugin::getConfiguration() const {
    return configuration_;
}

bool MathPlugin::handleMessage(StrView message, const HashMap<Str, Str>& data) {
    if (context_) {
        context_->logInfo("MathPlugin received message: " + std::string(message));
    }
    return false;
}

// Math function implementations
int MathPlugin::luaFactorial(lua_State* L) {
    lua_Integer n = luaL_checkinteger(L, 1);
    if (n < 0) {
        return luaL_error(L, "factorial: negative number");
    }
    
    lua_Integer result = 1;
    for (lua_Integer i = 2; i <= n; ++i) {
        result *= i;
        if (result < 0) { // Overflow check
            return luaL_error(L, "factorial: result too large");
        }
    }
    
    lua_pushinteger(L, result);
    return 1;
}

int MathPlugin::luaFibonacci(lua_State* L) {
    lua_Integer n = luaL_checkinteger(L, 1);
    if (n < 0) {
        return luaL_error(L, "fibonacci: negative number");
    }
    
    if (n <= 1) {
        lua_pushinteger(L, n);
        return 1;
    }
    
    lua_Integer a = 0, b = 1;
    for (lua_Integer i = 2; i <= n; ++i) {
        lua_Integer temp = a + b;
        a = b;
        b = temp;
    }
    
    lua_pushinteger(L, b);
    return 1;
}

int MathPlugin::luaIsPrime(lua_State* L) {
    lua_Integer n = luaL_checkinteger(L, 1);
    if (n < 2) {
        lua_pushboolean(L, 0);
        return 1;
    }
    
    if (n == 2) {
        lua_pushboolean(L, 1);
        return 1;
    }
    
    if (n % 2 == 0) {
        lua_pushboolean(L, 0);
        return 1;
    }
    
    for (lua_Integer i = 3; i * i <= n; i += 2) {
        if (n % i == 0) {
            lua_pushboolean(L, 0);
            return 1;
        }
    }
    
    lua_pushboolean(L, 1);
    return 1;
}

int MathPlugin::luaGcd(lua_State* L) {
    lua_Integer a = luaL_checkinteger(L, 1);
    lua_Integer b = luaL_checkinteger(L, 2);
    
    a = std::abs(a);
    b = std::abs(b);
    
    while (b != 0) {
        lua_Integer temp = b;
        b = a % b;
        a = temp;
    }
    
    lua_pushinteger(L, a);
    return 1;
}

int MathPlugin::luaLcm(lua_State* L) {
    lua_Integer a = luaL_checkinteger(L, 1);
    lua_Integer b = luaL_checkinteger(L, 2);
    
    if (a == 0 || b == 0) {
        lua_pushinteger(L, 0);
        return 1;
    }
    
    a = std::abs(a);
    b = std::abs(b);
    
    // Calculate GCD first
    lua_Integer gcd_val = a;
    lua_Integer temp_b = b;
    while (temp_b != 0) {
        lua_Integer temp = temp_b;
        temp_b = gcd_val % temp_b;
        gcd_val = temp;
    }
    
    lua_Integer result = (a / gcd_val) * b;
    lua_pushinteger(L, result);
    return 1;
}

int MathPlugin::luaPower(lua_State* L) {
    lua_Number base = luaL_checknumber(L, 1);
    lua_Number exponent = luaL_checknumber(L, 2);
    
    lua_Number result = std::pow(base, exponent);
    lua_pushnumber(L, result);
    return 1;
}

// StringPlugin Implementation
StringPlugin::StringPlugin() : state_(PluginState::Unloaded), context_(nullptr) {
    metadata_.name = "StringPlugin";
    metadata_.version = {1, 0, 0};
    metadata_.author = "Lua Plugin System";
    metadata_.description = "String manipulation functions plugin";
    metadata_.category = "String";
    metadata_.tags = {"string", "text", "utility"};
    metadata_.autoLoad = false;
}

StrView StringPlugin::getName() const {
    return metadata_.name;
}

void StringPlugin::registerFunctions(FunctionRegistry& registry) {
    registry.registerFunction("str_reverse", luaReverse);
    registry.registerFunction("str_capitalize", luaCapitalize);
    registry.registerFunction("str_count", luaCount);
    registry.registerFunction("str_split", luaSplit);
    registry.registerFunction("str_join", luaJoin);
    registry.registerFunction("str_trim", luaTrim);
}

bool StringPlugin::onLoad(PluginContext* context) {
    context_ = context;
    state_ = PluginState::Loaded;
    
    if (context_) {
        context_->logInfo("StringPlugin loaded successfully");
    }
    
    return true;
}

bool StringPlugin::onUnload(PluginContext* context) {
    if (context_) {
        context_->logInfo("StringPlugin unloading");
    }
    
    state_ = PluginState::Unloaded;
    context_ = nullptr;
    return true;
}

bool StringPlugin::onEnable(PluginContext* context) {
    state_ = PluginState::Enabled;
    if (context_) {
        context_->logInfo("StringPlugin enabled");
    }
    return true;
}

bool StringPlugin::onDisable(PluginContext* context) {
    state_ = PluginState::Disabled;
    if (context_) {
        context_->logInfo("StringPlugin disabled");
    }
    return true;
}

PluginState StringPlugin::getState() const {
    return state_;
}

void StringPlugin::setState(PluginState state) {
    state_ = state;
}

const PluginMetadata& StringPlugin::getMetadata() const {
    return metadata_;
}

bool StringPlugin::configure(const HashMap<Str, Str>& config) {
    configuration_ = config;
    return true;
}

HashMap<Str, Str> StringPlugin::getConfiguration() const {
    return configuration_;
}

bool StringPlugin::handleMessage(StrView message, const HashMap<Str, Str>& data) {
    if (context_) {
        context_->logInfo("StringPlugin received message: " + std::string(message));
    }
    return false;
}

// String function implementations
int StringPlugin::luaReverse(lua_State* L) {
    size_t len;
    const char* str = luaL_checklstring(L, 1, &len);
    
    std::string result(str, len);
    std::reverse(result.begin(), result.end());
    
    lua_pushlstring(L, result.c_str(), result.length());
    return 1;
}

int StringPlugin::luaCapitalize(lua_State* L) {
    size_t len;
    const char* str = luaL_checklstring(L, 1, &len);
    
    std::string result(str, len);
    if (!result.empty()) {
        result[0] = std::toupper(result[0]);
        for (size_t i = 1; i < result.length(); ++i) {
            result[i] = std::tolower(result[i]);
        }
    }
    
    lua_pushlstring(L, result.c_str(), result.length());
    return 1;
}

int StringPlugin::luaCount(lua_State* L) {
    size_t str_len, pattern_len;
    const char* str = luaL_checklstring(L, 1, &str_len);
    const char* pattern = luaL_checklstring(L, 2, &pattern_len);
    
    if (pattern_len == 0) {
        lua_pushinteger(L, 0);
        return 1;
    }
    
    std::string text(str, str_len);
    std::string search(pattern, pattern_len);
    
    size_t count = 0;
    size_t pos = 0;
    while ((pos = text.find(search, pos)) != std::string::npos) {
        ++count;
        pos += pattern_len;
    }
    
    lua_pushinteger(L, count);
    return 1;
}

int StringPlugin::luaSplit(lua_State* L) {
    size_t str_len, delim_len;
    const char* str = luaL_checklstring(L, 1, &str_len);
    const char* delimiter = luaL_checklstring(L, 2, &delim_len);
    
    std::string text(str, str_len);
    std::string delim(delimiter, delim_len);
    
    lua_newtable(L);
    
    if (delim.empty()) {
        lua_pushstring(L, str);
        lua_rawseti(L, -2, 1);
        return 1;
    }
    
    size_t start = 0;
    size_t end = 0;
    int index = 1;
    
    while ((end = text.find(delim, start)) != std::string::npos) {
        std::string token = text.substr(start, end - start);
        lua_pushlstring(L, token.c_str(), token.length());
        lua_rawseti(L, -2, index++);
        start = end + delim.length();
    }
    
    // Add the last token
    std::string token = text.substr(start);
    lua_pushlstring(L, token.c_str(), token.length());
    lua_rawseti(L, -2, index);
    
    return 1;
}

int StringPlugin::luaJoin(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    size_t delim_len;
    const char* delimiter = luaL_checklstring(L, 2, &delim_len);
    
    std::string delim(delimiter, delim_len);
    std::string result;
    
    lua_len(L, 1);
    int len = lua_tointeger(L, -1);
    lua_pop(L, 1);
    
    for (int i = 1; i <= len; ++i) {
        lua_rawgeti(L, 1, i);
        if (lua_isstring(L, -1)) {
            if (i > 1) {
                result += delim;
            }
            result += lua_tostring(L, -1);
        }
        lua_pop(L, 1);
    }
    
    lua_pushlstring(L, result.c_str(), result.length());
    return 1;
}

int StringPlugin::luaTrim(lua_State* L) {
    size_t len;
    const char* str = luaL_checklstring(L, 1, &len);
    
    std::string text(str, len);
    
    // Trim leading whitespace
    text.erase(text.begin(), std::find_if(text.begin(), text.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    
    // Trim trailing whitespace
    text.erase(std::find_if(text.rbegin(), text.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), text.end());
    
    lua_pushlstring(L, text.c_str(), text.length());
    return 1;
}

// Factory implementations
UPtr<IPlugin> ExamplePluginFactory::createPlugin() {
    return std::make_unique<ExamplePlugin>();
}

StrView ExamplePluginFactory::getPluginName() const {
    return "ExamplePlugin";
}

PluginVersion ExamplePluginFactory::getPluginVersion() const {
    return {1, 0, 0};
}

UPtr<IPlugin> MathPluginFactory::createPlugin() {
    return std::make_unique<MathPlugin>();
}

StrView MathPluginFactory::getPluginName() const {
    return "MathPlugin";
}

PluginVersion MathPluginFactory::getPluginVersion() const {
    return {1, 0, 0};
}

UPtr<IPlugin> StringPluginFactory::createPlugin() {
    return std::make_unique<StringPlugin>();
}

StrView StringPluginFactory::getPluginName() const {
    return "StringPlugin";
}

PluginVersion StringPluginFactory::getPluginVersion() const {
    return {1, 0, 0};
}

// Demo and utility functions
void demonstratePluginUsage(PluginSystem* system) {
    if (!system) {
        std::cout << "Plugin system not available\n";
        return;
    }
    
    std::cout << "=== Plugin System Demo ===\n";
    
    // Show system state
    auto diagnostics = system->getDiagnostics();
    std::cout << "System diagnostics:\n";
    for (const auto& [key, value] : diagnostics) {
        std::cout << "  " << key << ": " << value << "\n";
    }
    
    // Load example plugins
    std::cout << "\nLoading example plugins...\n";
    
    if (system->loadPlugin("ExamplePlugin")) {
        std::cout << "  ExamplePlugin loaded successfully\n";
    } else {
        std::cout << "  Failed to load ExamplePlugin: " << system->getLastError() << "\n";
    }
    
    if (system->loadPlugin("MathPlugin")) {
        std::cout << "  MathPlugin loaded successfully\n";
    } else {
        std::cout << "  Failed to load MathPlugin: " << system->getLastError() << "\n";
    }
    
    if (system->loadPlugin("StringPlugin")) {
        std::cout << "  StringPlugin loaded successfully\n";
    } else {
        std::cout << "  Failed to load StringPlugin: " << system->getLastError() << "\n";
    }
    
    // Show loaded plugins
    auto loaded = system->getLoadedPlugins();
    std::cout << "\nLoaded plugins (" << loaded.size() << "):";
    for (const auto& name : loaded) {
        std::cout << " " << name;
    }
    std::cout << "\n";
    
    // Show statistics
    auto stats = system->getPluginStatistics();
    std::cout << "\nPlugin statistics:\n";
    std::cout << "  Total plugins: " << stats.totalPlugins << "\n";
    std::cout << "  Loaded plugins: " << stats.loadedPlugins << "\n";
    std::cout << "  Enabled plugins: " << stats.enabledPlugins << "\n";
}

void demonstratePluginCommunication(PluginSystem* system) {
    if (!system) return;
    
    std::cout << "\n=== Plugin Communication Demo ===\n";
    
    auto examplePlugin = system->getPlugin("ExamplePlugin");
    auto mathPlugin = system->getPlugin("MathPlugin");
    
    if (examplePlugin && mathPlugin) {
        std::cout << "Testing plugin communication...\n";
        
        // Send message from example to math plugin
        HashMap<Str, Str> data = {{"sender", "ExamplePlugin"}, {"test", "true"}};
        bool result = examplePlugin->handleMessage("ping", data);
        std::cout << "  Message handling result: " << (result ? "success" : "failed") << "\n";
    }
}

void demonstrateConfigurationManagement(PluginSystem* system) {
    if (!system) return;
    
    std::cout << "\n=== Configuration Management Demo ===\n";
    
    // Set some configuration values
    system->setConfigValue("debug_mode", "true");
    system->setConfigValue("log_level", "info");
    system->setConfigValue("max_plugins", "10");
    
    // Read configuration values
    std::cout << "Configuration values:\n";
    std::cout << "  debug_mode: " << system->getConfigValue("debug_mode", "false") << "\n";
    std::cout << "  log_level: " << system->getConfigValue("log_level", "error") << "\n";
    std::cout << "  max_plugins: " << system->getConfigValue("max_plugins", "5") << "\n";
    std::cout << "  unknown_key: " << system->getConfigValue("unknown_key", "default") << "\n";
}

void demonstrateLifecycleManagement(PluginSystem* system) {
    if (!system) return;
    
    std::cout << "\n=== Lifecycle Management Demo ===\n";
    
    auto plugin = system->getPlugin("ExamplePlugin");
    if (plugin) {
        std::cout << "ExamplePlugin current state: " << static_cast<int>(plugin->getState()) << "\n";
        
        // Enable plugin
        if (system->enablePlugin("ExamplePlugin")) {
            std::cout << "  Plugin enabled successfully\n";
            std::cout << "  New state: " << static_cast<int>(plugin->getState()) << "\n";
        }
        
        // Disable plugin
        if (system->disablePlugin("ExamplePlugin")) {
            std::cout << "  Plugin disabled successfully\n";
            std::cout << "  New state: " << static_cast<int>(plugin->getState()) << "\n";
        }
        
        // Re-enable plugin
        if (system->enablePlugin("ExamplePlugin")) {
            std::cout << "  Plugin re-enabled successfully\n";
            std::cout << "  Final state: " << static_cast<int>(plugin->getState()) << "\n";
        }
    }
}

void demonstrateErrorHandling(PluginSystem* system) {
    if (!system) return;
    
    std::cout << "\n=== Error Handling Demo ===\n";
    
    // Try to load a non-existent plugin
    if (!system->loadPlugin("NonExistentPlugin")) {
        std::cout << "Expected error loading non-existent plugin: " << system->getLastError() << "\n";
    }
    
    // Show error history
    auto errors = system->getErrorHistory();
    if (!errors.empty()) {
        std::cout << "Error history (" << errors.size() << " entries):\n";
        for (size_t i = 0; i < std::min(errors.size(), size_t(3)); ++i) {
            std::cout << "  " << (i + 1) << ". " << errors[i] << "\n";
        }
    }
    
    // Clear errors
    system->clearErrors();
    std::cout << "Errors cleared. New error count: " << system->getErrorHistory().size() << "\n";
}

void demonstratePerformanceMonitoring(PluginSystem* system) {
    if (!system) return;
    
    std::cout << "\n=== Performance Monitoring Demo ===\n";
    
    auto perfStats = system->getPerformanceStatistics();
    if (perfStats.empty()) {
        std::cout << "No performance statistics available\n";
        return;
    }
    
    for (const auto& [pluginName, metrics] : perfStats) {
        std::cout << "Plugin: " << pluginName << "\n";
        for (const auto& [metric, value] : metrics) {
            std::cout << "  " << metric << ": " << value << "\n";
        }
    }
}

} // namespace Lua