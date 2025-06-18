#pragma once

#include "../../common/types.hpp"
#include "test_define.hpp"
#include <memory>
#include <string>

namespace Lua {
namespace Tests {
namespace TestFormatting {

// Forward declaration
class TestConfig;

class TestFormatter {
public:
    TestFormatter();
    ~TestFormatter();
    
    // Singleton access
    static TestFormatter& getInstance();
    
    // Level-based formatting
    void printLevelHeader(TestLevel level, const std::string& title, const std::string& description = "");
    void printLevelFooter(TestLevel level, const std::string& message = "");
    
    // Test result formatting
    void printTestResult(const std::string& testName, bool passed);
    
    // Message formatting
    void printInfo(const std::string& message);
    void printWarning(const std::string& message);
    void printError(const std::string& message);
    
    // Backward compatibility methods
    void printSectionHeader(const std::string& title);
    void printSectionFooter(const std::string& message = "");
    void printSimpleSectionHeader(const std::string& title);
    void printSimpleSectionFooter(const std::string& message = "");
    
    // Configuration methods
    void setColorEnabled(bool enabled);
    void setTheme(const std::string& theme);
    TestConfig& getConfig();
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
    
    // Disable copy and assignment
    TestFormatter(const TestFormatter&) = delete;
    TestFormatter& operator=(const TestFormatter&) = delete;
};

} // namespace TestFormatting
} // namespace Tests
} // namespace Lua