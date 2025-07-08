#include "lib/lua_standard_library.hpp"
#include "vm/state.hpp"
#include "vm/value.hpp"
#include <iostream>
#include <cassert>

/**
 * @brief Comprehensive test for all standard library modules
 * 
 * This test verifies that all standard library modules can be:
 * 1. Compiled successfully
 * 2. Initialized without errors
 * 3. Registered with the Lua state
 * 4. Basic functionality works as expected
 */

namespace Lua {

class StandardLibraryTest {
public:
    static void runAllTests() {
        std::cout << "=== Lua Standard Library Comprehensive Test ===" << std::endl;
        
        testLibraryAvailability();
        testBasicInitialization();
        testConfigurationBasedInitialization();
        testSafeConfiguration();
        testFullConfiguration();
        testVersionInfo();
        testStatistics();
        
        std::cout << "=== All tests passed successfully! ===" << std::endl;
    }

private:
    static void testLibraryAvailability() {
        std::cout << "\n[TEST] Library Availability..." << std::endl;
        
        // Test individual library availability
        assert(StandardLibrary::isLibraryAvailable("base"));
        assert(StandardLibrary::isLibraryAvailable("string"));
        assert(StandardLibrary::isLibraryAvailable("math"));
        assert(StandardLibrary::isLibraryAvailable("table"));
        assert(StandardLibrary::isLibraryAvailable("io"));
        assert(StandardLibrary::isLibraryAvailable("os"));
        assert(StandardLibrary::isLibraryAvailable("debug"));
        
        // Test non-existent library
        assert(!StandardLibrary::isLibraryAvailable("nonexistent"));
        
        // Test available libraries list
        Vec<Str> libraries = StandardLibrary::getAvailableLibraries();
        assert(libraries.size() == 7); // All 7 libraries should be available
        
        std::cout << "[PASS] Library availability test passed" << std::endl;
    }
    
    static void testBasicInitialization() {
        std::cout << "\n[TEST] Basic Initialization..." << std::endl;
        
        // Create a test state
        State state;
        
        try {
            // Test individual library initialization
            StandardLibrary::initializeBase(&state);
            std::cout << "[PASS] Base library initialized" << std::endl;
            
            StandardLibrary::initializeString(&state);
            std::cout << "[PASS] String library initialized" << std::endl;
            
            StandardLibrary::initializeMath(&state);
            std::cout << "[PASS] Math library initialized" << std::endl;
            
            StandardLibrary::initializeTable(&state);
            std::cout << "[PASS] Table library initialized" << std::endl;
            
            StandardLibrary::initializeIO(&state);
            std::cout << "[PASS] IO library initialized" << std::endl;
            
            StandardLibrary::initializeOS(&state);
            std::cout << "[PASS] OS library initialized" << std::endl;
            
            StandardLibrary::initializeDebug(&state);
            std::cout << "[PASS] Debug library initialized" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "[FAIL] Initialization failed: " << e.what() << std::endl;
            assert(false);
        }
        
        std::cout << "[PASS] Basic initialization test passed" << std::endl;
    }
    
    static void testConfigurationBasedInitialization() {
        std::cout << "\n[TEST] Configuration-based Initialization..." << std::endl;
        
        State state;
        
        try {
            // Test core libraries only
            StandardLibrary::initializeCore(&state);
            std::cout << "[PASS] Core libraries initialized" << std::endl;
            
            // Test all libraries
            State fullState;
            StandardLibrary::initializeAll(&fullState);
            std::cout << "[PASS] All libraries initialized" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "[FAIL] Configuration-based initialization failed: " << e.what() << std::endl;
            assert(false);
        }
        
        std::cout << "[PASS] Configuration-based initialization test passed" << std::endl;
    }
    
    static void testSafeConfiguration() {
        std::cout << "\n[TEST] Safe Configuration..." << std::endl;
        
        State state;
        
        try {
            LibraryConfig safeConfig = createSafeConfig();
            
            // Verify safe configuration properties
            assert(safeConfig.enableBase == true);
            assert(safeConfig.enableString == true);
            assert(safeConfig.enableMath == true);
            assert(safeConfig.enableTable == true);
            assert(safeConfig.enableIO == false);
            assert(safeConfig.enableOS == false);
            assert(safeConfig.enableDebug == false);
            assert(safeConfig.restrictedMode == true);
            
            initializeLibrariesWithConfig(&state, safeConfig);
            std::cout << "[PASS] Safe configuration applied successfully" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "[FAIL] Safe configuration test failed: " << e.what() << std::endl;
            assert(false);
        }
        
        std::cout << "[PASS] Safe configuration test passed" << std::endl;
    }
    
    static void testFullConfiguration() {
        std::cout << "\n[TEST] Full Configuration..." << std::endl;
        
        State state;
        
        try {
            LibraryConfig fullConfig = createFullConfig();
            
            // Verify full configuration properties
            assert(fullConfig.enableBase == true);
            assert(fullConfig.enableString == true);
            assert(fullConfig.enableMath == true);
            assert(fullConfig.enableTable == true);
            assert(fullConfig.enableIO == true);
            assert(fullConfig.enableOS == true);
            assert(fullConfig.enableDebug == true);
            assert(fullConfig.restrictedMode == false);
            assert(fullConfig.verboseLogging == true);
            
            initializeLibrariesWithConfig(&state, fullConfig);
            std::cout << "[PASS] Full configuration applied successfully" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "[FAIL] Full configuration test failed: " << e.what() << std::endl;
            assert(false);
        }
        
        std::cout << "[PASS] Full configuration test passed" << std::endl;
    }
    
    static void testVersionInfo() {
        std::cout << "\n[TEST] Version Information..." << std::endl;
        
        try {
            Str versionInfo = StandardLibrary::getVersionInfo();
            assert(!versionInfo.empty());
            
            std::cout << "Version Info:\n" << versionInfo << std::endl;
            std::cout << "[PASS] Version information retrieved successfully" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "[FAIL] Version info test failed: " << e.what() << std::endl;
            assert(false);
        }
        
        std::cout << "[PASS] Version information test passed" << std::endl;
    }
    
    static void testStatistics() {
        std::cout << "\n[TEST] Library Statistics..." << std::endl;
        
        try {
            State state;
            Str statistics = StandardLibrary::getStatistics(&state);
            assert(!statistics.empty());
            
            std::cout << "Library Statistics:\n" << statistics << std::endl;
            std::cout << "[PASS] Statistics retrieved successfully" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "[FAIL] Statistics test failed: " << e.what() << std::endl;
            assert(false);
        }
        
        std::cout << "[PASS] Statistics test passed" << std::endl;
    }
};

} // namespace Lua

int main() {
    try {
        Lua::StandardLibraryTest::runAllTests();
        std::cout << "\n🎉 All standard library tests completed successfully!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "\n❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}
