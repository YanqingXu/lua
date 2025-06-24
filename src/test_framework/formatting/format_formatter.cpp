#include "format_formatter.hpp"
#include "format_config.hpp"
#include "format_colors.hpp"
#include "format_strategies.hpp"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>

namespace Lua {
namespace Tests {
namespace TestFormatting {

class TestFormatter::Impl {
public:
    HashMap<TestLevel, UPtr<IFormatStrategy>> strategies;
    TestColorManager colorManager;
    
    Impl() {
        // Initialize strategies for each test level
        strategies[TestLevel::MAIN] = make_unique<MainFormatStrategy>();
        strategies[TestLevel::MODULE] = make_unique<ModuleFormatStrategy>();
        strategies[TestLevel::SUITE] = make_unique<SuiteFormatStrategy>();
        strategies[TestLevel::GROUP] = make_unique<GroupFormatStrategy>();
        strategies[TestLevel::INDIVIDUAL] = make_unique<IndividualFormatStrategy>();
    }
    
    IFormatStrategy* getStrategy(TestLevel level) {
        auto it = strategies.find(level);
        return (it != strategies.end()) ? it->second.get() : strategies[TestLevel::INDIVIDUAL].get();
    }
};

TestFormatter::TestFormatter() : pImpl(make_unique<Impl>()) {}

TestFormatter::~TestFormatter() = default;

TestFormatter& TestFormatter::getInstance() {
    static TestFormatter instance;
    return instance;
}

void TestFormatter::printLevelHeader(TestLevel level, const Str& title, const Str& description) {
    auto* strategy = pImpl->getStrategy(level);
    if (strategy) {
        strategy->printHeader(title, description, pImpl->colorManager);
    }
}

void TestFormatter::printLevelFooter(TestLevel level, const Str& message) {
    auto* strategy = pImpl->getStrategy(level);
    if (strategy) {
        strategy->printFooter(message, pImpl->colorManager);
    }
}

void TestFormatter::printTestResult(const Str& testName, bool passed) {
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

void TestFormatter::printInfo(const Str& message) {
    const auto& config = TestConfig::getInstance();
    
    if (config.isColorEnabled()) {
        std::cout << pImpl->colorManager.getColor(ColorType::INFO) 
                 << "[INFO] " << message 
                 << pImpl->colorManager.getColor(ColorType::RESET) << std::endl;
    } else {
        std::cout << "[INFO] " << message << std::endl;
    }
}

void TestFormatter::printWarning(const Str& message) {
    const auto& config = TestConfig::getInstance();
    
    if (config.isColorEnabled()) {
        std::cout << pImpl->colorManager.getColor(ColorType::WARNING) 
                 << "[WARN] " << message 
                 << pImpl->colorManager.getColor(ColorType::RESET) << std::endl;
    } else {
        std::cout << "[WARN] " << message << std::endl;
    }
}

void TestFormatter::printError(const Str& message) {
    const auto& config = TestConfig::getInstance();
    
    if (config.isColorEnabled()) {
        std::cout << pImpl->colorManager.getColor(ColorType::ERROR_COLOR) 
                 << "[ERROR] " << message 
                 << pImpl->colorManager.getColor(ColorType::RESET) << std::endl;
    } else {
        std::cout << "[ERROR] " << message << std::endl;
    }
}

void TestFormatter::printSectionHeader(const Str& title) {
    printLevelHeader(TestLevel::SUITE, title);
}

void TestFormatter::printSectionFooter(const Str& message) {
    printLevelFooter(TestLevel::SUITE, message);
}

void TestFormatter::printSimpleSectionHeader(const Str& title) {
    printLevelHeader(TestLevel::GROUP, title);
}

void TestFormatter::printSimpleSectionFooter(const Str& message) {
    printLevelFooter(TestLevel::GROUP, message);
}

void TestFormatter::setColorEnabled(bool enabled) {
    TestConfig::getInstance().setColorEnabled(enabled);
}

void TestFormatter::setTheme(const Str& theme) {
    TestConfig::getInstance().setTheme(theme);
}

void TestFormatter::printTimestamp(const Str& label) {
    const auto& config = TestConfig::getInstance();
    
    // Get current time
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time_t);
    std::tm* tm = &tm_buf;
#else
    std::tm* tm = std::localtime(&time_t);
#endif
    
    if (config.isColorEnabled()) {
        std::cout << pImpl->colorManager.getColor(ColorType::DIM)
                 << "[" << std::put_time(tm, "%Y-%m-%d %H:%M:%S")
                 << "." << std::setfill('0') << std::setw(3) << ms.count()
                 << "]";
        if (!label.empty()) {
            std::cout << " " << label;
        }
        std::cout << pImpl->colorManager.getColor(ColorType::RESET) << std::endl;
    } else {
        std::cout << "[" << std::put_time(tm, "%Y-%m-%d %H:%M:%S")
                 << "." << std::setfill('0') << std::setw(3) << ms.count()
                 << "]";
        if (!label.empty()) {
            std::cout << " " << label;
        }
        std::cout << std::endl;
    }
}

void TestFormatter::printStatistics(i32 passed, i32 failed, i32 total, f64 duration) {
    const auto& config = TestConfig::getInstance();
    
    if (config.isColorEnabled()) {
        std::cout << pImpl->colorManager.getColor(ColorType::INFO)
                 << "Statistics: ";
        std::cout << pImpl->colorManager.getColor(ColorType::SUCCESS)
                 << passed << " passed";
        if (failed > 0) {
            std::cout << pImpl->colorManager.getColor(ColorType::INFO) << ", "
                     << pImpl->colorManager.getColor(ColorType::ERROR_COLOR)
                     << failed << " failed";
        }
        std::cout << pImpl->colorManager.getColor(ColorType::INFO)
                 << ", " << total << " total";
        if (duration > 0.0) {
            std::cout << " (" << std::fixed << std::setprecision(3) 
                     << duration << "s)";
        }
        std::cout << pImpl->colorManager.getColor(ColorType::RESET) << std::endl;
    } else {
        std::cout << "Statistics: " << passed << " passed";
        if (failed > 0) {
            std::cout << ", " << failed << " failed";
        }
        std::cout << ", " << total << " total";
        if (duration > 0.0) {
            std::cout << " (" << std::fixed << std::setprecision(3) 
                     << duration << "s)";
        }
        std::cout << std::endl;
    }
}

void TestFormatter::printProgressBar(i32 current, i32 total, i32 width) {
    if (total <= 0) return;
    
    const auto& config = TestConfig::getInstance();
    double progress = static_cast<double>(current) / total;
    i32 filled = static_cast<i32>(progress * width);
    
    if (config.isColorEnabled()) {
        std::cout << pImpl->colorManager.getColor(ColorType::INFO)
                 << "[";
        std::cout << pImpl->colorManager.getColor(ColorType::SUCCESS)
                 << Str(filled, '=')
                 << pImpl->colorManager.getColor(ColorType::DIM)
                 << Str(width - filled, '-')
                 << pImpl->colorManager.getColor(ColorType::INFO)
                 << "] "
                 << pImpl->colorManager.getColor(ColorType::EMPHASIS)
                 << std::setw(3) << static_cast<i32>(progress * 100) << "%"
                 << pImpl->colorManager.getColor(ColorType::INFO)
                 << " (" << current << "/" << total << ")"
                 << pImpl->colorManager.getColor(ColorType::RESET)
                 << "\r" << std::flush;
    } else {
        std::cout << "[" << Str(filled, '=')
                 << Str(width - filled, '-')
                 << "] " << std::setw(3) << static_cast<i32>(progress * 100) << "%"
                 << " (" << current << "/" << total << ")"
                 << "\r" << std::flush;
    }
}

TestConfig& TestFormatter::getConfig() {
    return TestConfig::getInstance();
}

} // namespace TestFormatting
} // namespace Tests
} // namespace Lua