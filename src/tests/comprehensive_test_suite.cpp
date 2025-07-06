#include "comprehensive_test_suite.hpp"
#include "../lib/base/base_lib.hpp"
#include "../lib/string_lib.hpp"
#include "../lib/math_lib.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <vector>
#include <future>

namespace Lua {
    namespace Tests {

        // Static member initialization
        i32 ComprehensiveTestSuite::totalTests_ = 0;
        i32 ComprehensiveTestSuite::passedTests_ = 0;
        std::vector<Str> ComprehensiveTestSuite::failedTests_;

        void ComprehensiveTestSuite::runAllTests() {
            std::cout << "=== Lua Standard Library Comprehensive Test Suite ===\n";
            std::cout << "Following development standards with modern C++ and type safety\n\n";

            auto startTime = std::chrono::high_resolution_clock::now();

            try {
                // Reset test counters
                totalTests_ = 0;
                passedTests_ = 0;
                failedTests_.clear();

                // Run test categories
                std::cout << "1. Testing Core Framework Components...\n";
                testCoreFramework();

                std::cout << "\n2. Testing Standard Library Modules...\n";
                testStandardLibraries();

                std::cout << "\n3. Testing Performance Benchmarks...\n";
                testPerformance();

                std::cout << "\n4. Testing Thread Safety...\n";
                testThreadSafety();

                std::cout << "\n5. Testing Integration Scenarios...\n";
                testIntegration();

            } catch (const std::exception& e) {
                std::cout << "Critical test failure: " << e.what() << "\n";
                reportTestResult("Critical", false, e.what());
            }

            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

            // Print summary
            std::cout << "\n=== Test Summary ===\n";
            std::cout << "Total Tests: " << totalTests_ << "\n";
            std::cout << "Passed: " << passedTests_ << "\n";
            std::cout << "Failed: " << (totalTests_ - passedTests_) << "\n";
            std::cout << "Success Rate: " << std::fixed << std::setprecision(1) 
                      << (totalTests_ > 0 ? (f64(passedTests_) / totalTests_ * 100.0) : 0.0) << "%\n";
            std::cout << "Total Time: " << duration.count() << "ms\n";

            if (!failedTests_.empty()) {
                std::cout << "\nFailed Tests:\n";
                for (const auto& test : failedTests_) {
                    std::cout << "  - " << test << "\n";
                }
            }

            std::cout << (passedTests_ == totalTests_ ? "\n✅ All tests passed!" : "\n❌ Some tests failed!") << "\n";
        }

        void ComprehensiveTestSuite::testCoreFramework() {
            testLibContext();
            testLibFuncRegistry();
            testLibraryManager();
        }

        void ComprehensiveTestSuite::testStandardLibraries() {
            testBaseLib();
            testStringLib();
            testMathLib();
        }

        void ComprehensiveTestSuite::testLibContext() {
            std::cout << "  Testing LibContext...\n";

            try {
                // Test basic configuration
                auto context = make_unique<Lib::LibContext>();
                
                // Test configuration setting and getting
                context->setConfig("test_key", 42);
                auto value = context->getConfig<i32>("test_key");
                TEST_ASSERT(value.has_value(), "Config value should be retrievable");
                TEST_ASSERT_EQ(42, value.value(), "Config value should match what was set");

                // Test configuration removal
                context->removeConfig("test_key");
                auto removedValue = context->getConfig<i32>("test_key");
                TEST_ASSERT(!removedValue.has_value(), "Removed config should not be retrievable");

                // Test string configuration
                context->setConfig("string_key", Str("hello"));
                auto stringValue = context->getConfig<Str>("string_key");
                TEST_ASSERT(stringValue.has_value(), "String config should be retrievable");
                TEST_ASSERT(stringValue.value() == "hello", "String config should match");

                reportTestResult("LibContext_BasicConfiguration", true);

            } catch (const std::exception& e) {
                reportTestResult("LibContext_BasicConfiguration", false, e.what());
            }

            try {
                // Test copy constructor and assignment
                auto context1 = make_unique<Lib::LibContext>();
                context1->setConfig("copy_test", 123);
                
                auto context2 = make_unique<Lib::LibContext>(*context1);
                auto copiedValue = context2->getConfig<i32>("copy_test");
                TEST_ASSERT(copiedValue.has_value(), "Copied context should have original values");
                TEST_ASSERT_EQ(123, copiedValue.value(), "Copied value should match original");

                reportTestResult("LibContext_CopySemantics", true);

            } catch (const std::exception& e) {
                reportTestResult("LibContext_CopySemantics", false, e.what());
            }
        }

        void ComprehensiveTestSuite::testLibFuncRegistry() {
            std::cout << "  Testing LibFuncRegistry...\n";

            try {
                auto registry = make_unique<Lib::LibFuncRegistry>();

                // Test function registration
                Lib::FunctionMetadata meta("test_func");
                meta.withDescription("Test function").withArgs(1, 2);
                
                registry->registerFunction(meta, [](State*, i32) -> Value {
                    return Value(42);
                });

                TEST_ASSERT(registry->hasFunction("test_func"), "Registered function should exist");

                // Test metadata retrieval
                const auto* retrievedMeta = registry->getFunctionMetadata("test_func");
                TEST_ASSERT(retrievedMeta != nullptr, "Function metadata should be available");
                TEST_ASSERT(retrievedMeta->name == "test_func", "Metadata name should match");
                TEST_ASSERT(retrievedMeta->description == "Test function", "Metadata description should match");

                reportTestResult("LibFuncRegistry_BasicRegistration", true);

            } catch (const std::exception& e) {
                reportTestResult("LibFuncRegistry_BasicRegistration", false, e.what());
            }

            try {
                auto registry = make_unique<Lib::LibFuncRegistry>();

                // Test batch registration
                std::vector<Lib::FunctionRegistration> functions;
                for (i32 i = 0; i < 10; ++i) {
                    Lib::FunctionMetadata meta("func_" + std::to_string(i));
                    meta.withDescription("Batch function " + std::to_string(i));
                    
                    functions.emplace_back(meta, [i](State*, i32) -> Value {
                        return Value(i);
                    });
                }

                registry->registerFunctions(functions);

                // Verify all functions are registered
                for (i32 i = 0; i < 10; ++i) {
                    Str funcName = "func_" + std::to_string(i);
                    TEST_ASSERT(registry->hasFunction(funcName), "Batch registered function should exist");
                }

                reportTestResult("LibFuncRegistry_BatchRegistration", true);

            } catch (const std::exception& e) {
                reportTestResult("LibFuncRegistry_BatchRegistration", false, e.what());
            }
        }

        void ComprehensiveTestSuite::testLibraryManager() {
            std::cout << "  Testing LibraryManager...\n";

            try {
                auto context = make_ptr<Lib::LibContext>();
                auto manager = make_unique<LibManager>(context);

                // Test module registration
                auto baseLib = make_unique<Lib::BaseLib>();
                manager->registerModule(std::move(baseLib));

                TEST_ASSERT(manager->getModuleStatus("base") != LibManager::ModuleStatus::Failed, 
                           "Module should be registered successfully");

                reportTestResult("LibraryManager_ModuleRegistration", true);

            } catch (const std::exception& e) {
                reportTestResult("LibraryManager_ModuleRegistration", false, e.what());
            }
        }

        void ComprehensiveTestSuite::testBaseLib() {
            std::cout << "  Testing BaseLib...\n";

            try {
                auto baseLib = make_unique<Lib::BaseLib>();
                auto registry = make_unique<Lib::LibFuncRegistry>();
                auto context = make_unique<Lib::LibContext>();

                // Test module properties
                TEST_ASSERT(baseLib->getName() == "base", "BaseLib name should be 'base'");
                TEST_ASSERT(!baseLib->getVersion().empty(), "BaseLib should have version");

                // Test function registration
                baseLib->registerFunctions(*registry, *context);

                const std::vector<StrView> expectedFunctions = {
                    "print", "type", "tostring", "tonumber", "error", "assert"
                };

                for (StrView funcName : expectedFunctions) {
                    TEST_ASSERT(registry->hasFunction(funcName), 
                               Str("BaseLib should register ") + Str(funcName));
                }

                reportTestResult("BaseLib_ModuleProperties", true);

            } catch (const std::exception& e) {
                reportTestResult("BaseLib_ModuleProperties", false, e.what());
            }
        }

        void ComprehensiveTestSuite::testStringLib() {
            std::cout << "  Testing StringLib...\n";

            try {
                auto stringLib = make_unique<StringLib>();
                auto registry = make_unique<Lib::LibFuncRegistry>();
                auto context = make_unique<Lib::LibContext>();

                // Test function registration
                stringLib->registerFunctions(*registry, *context);

                const std::vector<StrView> expectedFunctions = {
                    "len", "sub", "upper", "lower", "reverse",
                    "find", "match", "gsub", "format", "rep"
                };

                for (StrView funcName : expectedFunctions) {
                    TEST_ASSERT(registry->hasFunction(funcName), 
                               Str("StringLib should register ") + Str(funcName));
                }

                reportTestResult("StringLib_FunctionRegistration", true);

            } catch (const std::exception& e) {
                reportTestResult("StringLib_FunctionRegistration", false, e.what());
            }
        }

        void ComprehensiveTestSuite::testMathLib() {
            std::cout << "  Testing MathLib...\n";

            try {
                auto mathLib = make_unique<MathLib>();
                auto registry = make_unique<Lib::LibFuncRegistry>();
                auto context = make_unique<Lib::LibContext>();

                // Test function registration
                mathLib->registerFunctions(*registry, *context);

                const std::vector<StrView> expectedFunctions = {
                    "abs", "floor", "ceil", "sin", "cos", "tan",
                    "sqrt", "pow", "exp", "log", "min", "max"
                };

                for (StrView funcName : expectedFunctions) {
                    TEST_ASSERT(registry->hasFunction(funcName), 
                               Str("MathLib should register ") + Str(funcName));
                }

                reportTestResult("MathLib_FunctionRegistration", true);

            } catch (const std::exception& e) {
                reportTestResult("MathLib_FunctionRegistration", false, e.what());
            }
        }

        void ComprehensiveTestSuite::testPerformance() {
            std::cout << "  Running performance benchmarks...\n";

            // Test function registration performance
            auto regTime = benchmarkFunction("FunctionRegistration", []() {
                auto registry = make_unique<Lib::LibFuncRegistry>();
                auto context = make_unique<Lib::LibContext>();
                auto baseLib = make_unique<Lib::BaseLib>();
                baseLib->registerFunctions(*registry, *context);
            }, 100);

            std::cout << "    Function registration: " << regTime.count() << "ms average\n";
            reportTestResult("Performance_FunctionRegistration", regTime.count() < 10, 
                           "Registration should be fast");

            // Test context configuration performance
            auto configTime = benchmarkFunction("ContextConfiguration", []() {
                auto context = make_unique<Lib::LibContext>();
                for (i32 i = 0; i < 100; ++i) {
                    context->setConfig("key_" + std::to_string(i), i);
                }
            }, 10);

            std::cout << "    Context configuration: " << configTime.count() << "ms average\n";
            reportTestResult("Performance_ContextConfiguration", configTime.count() < 50,
                           "Configuration should be reasonably fast");
        }

        void ComprehensiveTestSuite::testThreadSafety() {
            std::cout << "  Testing thread safety...\n";

            // Test LibContext thread safety
            bool contextSafe = testConcurrentAccess("LibContext_ThreadSafety", []() {
                static auto context = make_ptr<Lib::LibContext>();
                static std::atomic<i32> counter{0};
                
                i32 id = counter++;
                context->setConfig("thread_" + std::to_string(id), id);
                auto value = context->getConfig<i32>("thread_" + std::to_string(id));
                return value.has_value() && value.value() == id;
            });

            reportTestResult("LibContext_ThreadSafety", contextSafe, 
                           "LibContext should be thread-safe");

            // Test LibFuncRegistry thread safety
            bool registrySafe = testConcurrentAccess("LibFuncRegistry_ThreadSafety", []() {
                static auto registry = make_ptr<Lib::LibFuncRegistry>();
                static std::atomic<i32> counter{0};
                
                i32 id = counter++;
                Str funcName = "thread_func_" + std::to_string(id);
                
                Lib::FunctionMetadata meta(funcName);
                registry->registerFunction(meta, [id](State*, i32) -> Value {
                    return Value(id);
                });
                
                return registry->hasFunction(funcName);
            });

            reportTestResult("LibFuncRegistry_ThreadSafety", registrySafe,
                           "LibFuncRegistry should be thread-safe");
        }

        void ComprehensiveTestSuite::testIntegration() {
            std::cout << "  Testing integration scenarios...\n";

            try {
                // Test full library manager with multiple modules
                auto context = make_ptr<Lib::LibContext>();
                auto manager = make_unique<LibManager>(context);

                // Register multiple modules
                manager->registerModule(make_unique<Lib::BaseLib>());
                manager->registerModule(make_unique<StringLib>());
                manager->registerModule(make_unique<MathLib>());

                // Test that all modules are registered
                auto moduleNames = manager->getModuleNames();
                TEST_ASSERT(moduleNames.size() >= 3, "All modules should be registered");

                // Test function availability across modules
                TEST_ASSERT(manager->hasFunction("print"), "BaseLib functions should be available");
                // Note: String and Math functions might be namespaced, so this test may need adjustment

                reportTestResult("Integration_MultiModuleManager", true);

            } catch (const std::exception& e) {
                reportTestResult("Integration_MultiModuleManager", false, e.what());
            }
        }

        template<typename Func>
        std::chrono::milliseconds ComprehensiveTestSuite::benchmarkFunction(const Str& name, Func&& func, i32 iterations) {
            auto start = std::chrono::high_resolution_clock::now();
            
            for (i32 i = 0; i < iterations; ++i) {
                func();
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto total = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            return std::chrono::milliseconds(total.count() / iterations);
        }

        template<typename TestFunc>
        bool ComprehensiveTestSuite::testConcurrentAccess(const Str& testName, TestFunc&& testFunc, i32 threadCount, i32 iterationsPerThread) {
            std::vector<std::future<bool>> futures;
            
            for (i32 i = 0; i < threadCount; ++i) {
                futures.emplace_back(std::async(std::launch::async, [&testFunc, iterationsPerThread]() {
                    try {
                        for (i32 j = 0; j < iterationsPerThread; ++j) {
                            if (!testFunc()) {
                                return false;
                            }
                        }
                        return true;
                    } catch (...) {
                        return false;
                    }
                }));
            }
            
            bool allSucceeded = true;
            for (auto& future : futures) {
                if (!future.get()) {
                    allSucceeded = false;
                }
            }
            
            return allSucceeded;
        }

        void ComprehensiveTestSuite::reportTestResult(const Str& testName, bool passed, const Str& details) {
            totalTests_++;
            if (passed) {
                passedTests_++;
                std::cout << "    ✅ " << testName << "\n";
            } else {
                failedTests_.push_back(testName + (details.empty() ? "" : " (" + details + ")"));
                std::cout << "    ❌ " << testName;
                if (!details.empty()) {
                    std::cout << " - " << details;
                }
                std::cout << "\n";
            }
        }

    } // namespace Tests
} // namespace Lua
