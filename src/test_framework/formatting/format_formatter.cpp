#include "format_formatter.hpp"
#include "format_config.hpp"
#include "format_colors.hpp"
#include "format_strategies.hpp"
#include <iostream>
#include <memory>
#include <map>

namespace Lua {
namespace Tests {
namespace TestFormatting {

class TestFormatter::Impl {
public:
    std::map<TestLevel, std::unique_ptr<IFormatStrategy>> strategies;
    TestColorManager colorManager;
    
    Impl() {
        // Initialize strategies for each test level
        strategies[TestLevel::MAIN] = std::make_unique<MainFormatStrategy>();
        strategies[TestLevel::MODULE] = std::make_unique<ModuleFormatStrategy>();
        strategies[TestLevel::SUITE] = std::make_unique<SuiteFormatStrategy>();
        strategies[TestLevel::GROUP] = std::make_unique<GroupFormatStrategy>();
        strategies[TestLevel::INDIVIDUAL] = std::make_unique<IndividualFormatStrategy>();
    }
    
    IFormatStrategy* getStrategy(TestLevel level) {
        auto it = strategies.find(level);
        return (it != strategies.end()) ? it->second.get() : strategies[TestLevel::INDIVIDUAL].get();
    }
};

TestFormatter::TestFormatter() : pImpl(std::make_unique<Impl>()) {}

TestFormatter::~TestFormatter() = default;

TestFormatter& TestFormatter::getInstance() {
    static TestFormatter instance;
    return instance;
}

void TestFormatter::printLevelHeader(TestLevel level, const std::string& title, const std::string& description) {
    auto* strategy = pImpl->getStrategy(level);
    if (strategy) {
        strategy->printHeader(title, description, pImpl->colorManager);
    }
}

void TestFormatter::printLevelFooter(TestLevel level, const std::string& message) {
    auto* strategy = pImpl->getStrategy(level);
    if (strategy) {
        strategy->printFooter(message, pImpl->colorManager);
    }
}

void TestFormatter::printTestResult(const std::string& testName, bool passed) {
    const auto& config = TestConfig::getInstance();
    
    if (config.isColorEnabled()) {
        if (passed) {
            std::cout << pImpl->colorManager.getColor(ColorType::SUCCESS) 
                     << "[PASS] " << testName 
                     << pImpl->colorManager.getColor(ColorType::RESET) << std::endl;
        } else {
            std::cout << pImpl->colorManager.getColor(ColorType::ERROR_COLOR) 
                     << "[FAIL] " << testName 
                     << pImpl->colorManager.getColor(ColorType::RESET) << std::endl;
        }
    } else {
        std::cout << (passed ? "[PASS] " : "[FAIL] ") << testName << std::endl;
    }
}

void TestFormatter::printInfo(const std::string& message) {
    const auto& config = TestConfig::getInstance();
    
    if (config.isColorEnabled()) {
        std::cout << pImpl->colorManager.getColor(ColorType::INFO) 
                 << "[INFO] " << message 
                 << pImpl->colorManager.getColor(ColorType::RESET) << std::endl;
    } else {
        std::cout << "[INFO] " << message << std::endl;
    }
}

void TestFormatter::printWarning(const std::string& message) {
    const auto& config = TestConfig::getInstance();
    
    if (config.isColorEnabled()) {
        std::cout << pImpl->colorManager.getColor(ColorType::WARNING) 
                 << "[WARN] " << message 
                 << pImpl->colorManager.getColor(ColorType::RESET) << std::endl;
    } else {
        std::cout << "[WARN] " << message << std::endl;
    }
}

void TestFormatter::printError(const std::string& message) {
    const auto& config = TestConfig::getInstance();
    
    if (config.isColorEnabled()) {
        std::cout << pImpl->colorManager.getColor(ColorType::ERROR_COLOR) 
                 << "[ERROR] " << message 
                 << pImpl->colorManager.getColor(ColorType::RESET) << std::endl;
    } else {
        std::cout << "[ERROR] " << message << std::endl;
    }
}

void TestFormatter::printSectionHeader(const std::string& title) {
    printLevelHeader(TestLevel::SUITE, title);
}

void TestFormatter::printSectionFooter(const std::string& message) {
    printLevelFooter(TestLevel::SUITE, message);
}

void TestFormatter::printSimpleSectionHeader(const std::string& title) {
    printLevelHeader(TestLevel::GROUP, title);
}

void TestFormatter::printSimpleSectionFooter(const std::string& message) {
    printLevelFooter(TestLevel::GROUP, message);
}

void TestFormatter::setColorEnabled(bool enabled) {
    TestConfig::getInstance().setColorEnabled(enabled);
}

void TestFormatter::setTheme(const std::string& theme) {
    TestConfig::getInstance().setTheme(theme);
}

TestConfig& TestFormatter::getConfig() {
    return TestConfig::getInstance();
}

} // namespace TestFormatting
} // namespace Tests
} // namespace Lua