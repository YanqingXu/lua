#pragma once
#include "../../common/types.hpp"
#include "format_define.hpp"
#include <memory>

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
    void printLevelHeader(TestLevel level, const Str& title, const Str& description = "");
    void printLevelFooter(TestLevel level, const Str& message = "");
    
    // Test result formatting
    void printTestResult(const Str& testName, bool passed);
    
    // Message formatting
    void printInfo(const Str& message);
    void printWarning(const Str& message);
    void printError(const Str& message);
    
    // Backward compatibility methods
    void printSectionHeader(const Str& title);
    void printSectionFooter(const Str& message = "");
    void printSimpleSectionHeader(const Str& title);
    void printSimpleSectionFooter(const Str& message = "");
    
    // Timestamp and statistics formatting
    void printTimestamp(const Str& label = "");
    void printStatistics(int passed, int failed, int total, double duration = 0.0);
    void printProgressBar(int current, int total, int width = 50);
    
    // Configuration methods
    void setColorEnabled(bool enabled);
    void setTheme(const Str& theme);
    TestConfig& getConfig();
    
private:
    class Impl;
    UPtr<Impl> pImpl;
    
    // Disable copy and assignment
    TestFormatter(const TestFormatter&) = delete;
    TestFormatter& operator=(const TestFormatter&) = delete;
};

} // namespace TestFormatting
} // namespace Tests
} // namespace Lua