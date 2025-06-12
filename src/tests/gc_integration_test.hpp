#pragma once

namespace Lua {
namespace Tests {

    /**
     * Test class for GC integration functionality
     * Tests garbage collection integration with core types and complex reference patterns
     */
    class GCIntegrationTest {
    public:
        /**
         * Run all GC integration tests
         */
        static bool runAllTests();
        
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

} // namespace Tests
} // namespace Lua