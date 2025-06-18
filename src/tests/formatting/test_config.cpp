#include "test_config.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <iostream>

namespace Lua {
namespace Tests {
namespace TestFormatting {

TestConfig* TestConfig::instance_ = nullptr;

TestConfig::TestConfig() {
    initializeDefaults();
}

TestConfig& TestConfig::getInstance() {
    if (!instance_) {
        instance_ = new TestConfig();
    }
    return *instance_;
}

void TestConfig::initializeDefaults() {
    // Initialize default level configurations
    levelConfigs_[TestLevel::MAIN] = LevelConfig("=", "=", 80, 0, true, true, true);
    levelConfigs_[TestLevel::SUITE] = LevelConfig("-", "-", 70, 2, true, true, false);
    levelConfigs_[TestLevel::GROUP] = LevelConfig(".", ".", 60, 4, false, false, false);
    levelConfigs_[TestLevel::INDIVIDUAL] = LevelConfig(" ", " ", 50, 6, false, false, false);
    
    // Load from environment
    loadFromEnvironment();
}

bool TestConfig::isColorEnabled() const {
    return colorEnabled_;
}

void TestConfig::setColorEnabled(bool enabled) {
    colorEnabled_ = enabled;
}



void TestConfig::setTheme(const Str& theme) {
    theme_ = theme;
}

const LevelConfig& TestConfig::getLevelConfig(TestLevel level) const {
    auto it = levelConfigs_.find(level);
    if (it != levelConfigs_.end()) {
        return it->second;
    }
    static LevelConfig defaultConfig;
    return defaultConfig;
}

void TestConfig::setLevelConfig(TestLevel level, const LevelConfig& config) {
    levelConfigs_[level] = config;
}

const Str& TestConfig::getTheme() const {
    return theme_;
}

bool TestConfig::loadFromFile(const Str& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    Str line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Parse key=value pairs
        size_t equalPos = line.find('=');
        if (equalPos != Str::npos) {
            Str key = trim(line.substr(0, equalPos));
            Str value = trim(line.substr(equalPos + 1));
            
            if (key == "colorEnabled") {
                colorEnabled_ = (value == "true" || value == "1" || value == "yes");
            } else if (key == "theme") {
                theme_ = value;
            }
        }
    }
    
    return true;
}

bool TestConfig::saveToFile(const Str& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    file << "# Test Format Configuration File\n";
    file << "# Generated automatically\n\n";
    file << "colorEnabled=" << (colorEnabled_ ? "true" : "false") << "\n";
    file << "theme=" << theme_ << "\n";
    
    return true;
}

void TestConfig::loadFromEnvironment() {
#ifdef _MSC_VER
    // Use secure version for MSVC
    char* noColor = nullptr;
    char* forceColorFlag = nullptr;
    char* testTheme = nullptr;
    size_t len;
    
    _dupenv_s(&noColor, &len, "NO_COLOR");
    if (noColor && Str(noColor) != "") {
        colorEnabled_ = false;
    }
    
    _dupenv_s(&forceColorFlag, &len, "FORCE_COLOR");
    if (forceColorFlag && Str(forceColorFlag) == "1") {
        colorEnabled_ = true;
    }
    
    _dupenv_s(&testTheme, &len, "TEST_THEME");
    if (testTheme) {
        theme_ = Str(testTheme);
    }
    
    // Free allocated memory
    if (noColor) free(noColor);
    if (forceColorFlag) free(forceColorFlag);
    if (testTheme) free(testTheme);
#else
    // Use standard getenv for other compilers (MinGW, etc.)
    const char* noColor = std::getenv("NO_COLOR");
    if (noColor && Str(noColor) != "") {
        colorEnabled_ = false;
    }
    
    const char* forceColorFlag = std::getenv("FORCE_COLOR");
    if (forceColorFlag && Str(forceColorFlag) == "1") {
        colorEnabled_ = true;
    }
    
    const char* testTheme = std::getenv("TEST_THEME");
    if (testTheme) {
        theme_ = Str(testTheme);
    }
#endif
}
}
}
}// namespace Lua
