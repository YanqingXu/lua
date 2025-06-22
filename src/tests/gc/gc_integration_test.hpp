#pragma once

namespace Lua::Tests {

/**
 * @brief GC integration test suite
 * 
 * Tests garbage collection integration with core types and complex reference patterns
 */
class GCIntegrationTestSuite {
public:
    /**
     * @brief Run all GC integration tests
     */
    static void runAllTests();
    
private:
    /**
     * @brief Test GC integration with core types
     * @return true if test passes, false otherwise
     */
    static bool testGCIntegration();
    
    /**
     * @brief Test GC object marking with complex reference patterns
     * @return true if test passes, false otherwise
     */
    static bool testGCMarking();
};

} // namespace Lua::Tests