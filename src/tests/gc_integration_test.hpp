#pragma once

namespace Lua {
namespace Tests {

    /**
     * @brief Test GC integration with core types
     * @return true if test passes, false otherwise
     */
    bool testGCIntegration();
    
    /**
     * @brief Test GC object marking with complex reference patterns
     * @return true if test passes, false otherwise
     */
    bool testGCMarking();
    
    /**
     * @brief Run all GC integration tests
     * @return true if all tests pass, false otherwise
     */
    bool runGCIntegrationTests();

} // namespace Tests
} // namespace Lua