#include "test_closure.hpp"
#include "../../test_utils.hpp"
#include "closure_basic_test.hpp"
#include "closure_advanced_test.hpp"
#include "closure_memory_test.hpp"
#include "closure_performance_test.hpp"
#include "closure_error_test.hpp"

namespace Lua {
namespace Tests {

void ClosureTestSuite::runAllTests() {
    setupTestEnvironment();
    

    
    cleanupTestEnvironment();
}

void ClosureTestSuite::setupTestEnvironment() {
    std::cout << "Setting up closure test environment..." << std::endl;
}

void ClosureTestSuite::cleanupTestEnvironment() {
    std::cout << "Cleaning up closure test environment..." << std::endl;
}

} // namespace Tests
} // namespace Lua