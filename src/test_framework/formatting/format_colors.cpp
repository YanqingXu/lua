#include "format_colors.hpp"
#include "format_config.hpp"
#include <iostream>
#include <cstdlib>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#endif

// 安全的环境变量获取函数
inline std::string safe_getenv(const char* name) {
#ifdef _WIN32
    char* buffer = nullptr;
    size_t size = 0;
    errno_t err = _dupenv_s(&buffer, &size, name);
    if (err == 0 && buffer != nullptr) {
        std::string result(buffer);
        free(buffer);
        return result;
    }
    return "";
#else
    const char* value = std::getenv(name);
    return value ? std::string(value) : std::string("");
#endif
}

namespace Lua {
namespace Tests {
namespace TestFormatting {

TestColorManager::TestColorManager() {
    initializeColorCodes();
    initializeColorSupport();
    initializeColorSchemes();
    currentTheme_ = "default";
}

std::string TestColorManager::getColor(ColorType type) const {
    const auto& config = TestConfig::getInstance();
    
    if (!config.isColorEnabled() || !colorSupported_) {
        return "";
    }
    
    const std::string& themeName = config.getTheme();
    auto themeIt = colorSchemes_.find(themeName);
    if (themeIt == colorSchemes_.end()) {
        themeIt = colorSchemes_.find("default");
    }
    
    const auto& scheme = themeIt->second;
    auto colorIt = scheme.find(type);
    if (colorIt != scheme.end()) {
        return colorIt->second;
    }
    
    return "";
}

bool TestColorManager::isColorSupported() const {
    return colorSupported_;
}

void TestColorManager::initializeColorSupport() {
    colorSupported_ = false;
    
#ifdef _WIN32
    // Enable ANSI escape sequences on Windows 10+
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            if (SetConsoleMode(hOut, dwMode)) {
                colorSupported_ = true;
            }
        }
    }
    
    // Fallback: check if output is redirected
    if (!colorSupported_ && _isatty(_fileno(stdout))) {
        // Try to detect Windows Terminal or other modern terminals
        std::string wt_session = safe_getenv("WT_SESSION");
        std::string term_program = safe_getenv("TERM_PROGRAM");
        
        if (!wt_session.empty() || !term_program.empty()) {
            colorSupported_ = true;
        }
    }
#else
    // Unix-like systems
    if (isatty(fileno(stdout))) {
        std::string term = safe_getenv("TERM");
        if (!term.empty()) {
            if (term.find("color") != std::string::npos ||
                term == "xterm" || term == "xterm-256color" ||
                term == "screen" || term == "tmux" ||
                term == "linux") {
                colorSupported_ = true;
            }
        }
    }
#endif
    
    // Check environment variables
    std::string forceColor = safe_getenv("FORCE_COLOR");
    if (!forceColor.empty()) {
        colorSupported_ = true;
    }
    
    std::string noColor = safe_getenv("NO_COLOR");
    if (!noColor.empty()) {
        colorSupported_ = false;
    }
}

void TestColorManager::initializeColorSchemes() {
    // Default color scheme
    ColorScheme defaultScheme;
    defaultScheme[ColorType::RESET] = "\033[0m";
    defaultScheme[ColorType::SUCCESS] = "\033[32m";      // Green
    defaultScheme[ColorType::ERROR_COLOR] = "\033[31m";        // Red
    defaultScheme[ColorType::WARNING] = "\033[33m";      // Yellow
    defaultScheme[ColorType::INFO] = "\033[36m";         // Cyan
    defaultScheme[ColorType::HEADER] = "\033[1;34m";     // Bold Blue
    defaultScheme[ColorType::SUBHEADER] = "\033[1;35m";  // Bold Magenta
    defaultScheme[ColorType::EMPHASIS] = "\033[1m";      // Bold
    defaultScheme[ColorType::DIM] = "\033[2m";           // Dim
    colorSchemes_["default"] = defaultScheme;
    
    // Dark theme (more vibrant colors for dark backgrounds)
    ColorScheme darkScheme;
    darkScheme[ColorType::RESET] = "\033[0m";
    darkScheme[ColorType::SUCCESS] = "\033[92m";      // Bright Green
    darkScheme[ColorType::ERROR_COLOR] = "\033[91m";        // Bright Red
    darkScheme[ColorType::WARNING] = "\033[93m";      // Bright Yellow
    darkScheme[ColorType::INFO] = "\033[96m";         // Bright Cyan
    darkScheme[ColorType::HEADER] = "\033[1;94m";     // Bold Bright Blue
    darkScheme[ColorType::SUBHEADER] = "\033[1;95m";  // Bold Bright Magenta
    darkScheme[ColorType::EMPHASIS] = "\033[1;97m";   // Bold Bright White
    darkScheme[ColorType::DIM] = "\033[2;37m";        // Dim White
    colorSchemes_["dark"] = darkScheme;
    
    // Light theme (subdued colors for light backgrounds)
    ColorScheme lightScheme;
    lightScheme[ColorType::RESET] = "\033[0m";
    lightScheme[ColorType::SUCCESS] = "\033[32m";      // Green
    lightScheme[ColorType::ERROR_COLOR] = "\033[31m";      // Red
    lightScheme[ColorType::WARNING] = "\033[33m";      // Yellow
    lightScheme[ColorType::INFO] = "\033[34m";         // Blue
    lightScheme[ColorType::HEADER] = "\033[1;30m";     // Bold Black
    lightScheme[ColorType::SUBHEADER] = "\033[35m";    // Magenta
    lightScheme[ColorType::EMPHASIS] = "\033[1;30m";   // Bold Black
    lightScheme[ColorType::DIM] = "\033[2;30m";        // Dim Black
    colorSchemes_["light"] = lightScheme;
    
    // Monochrome theme (no colors, only text formatting)
    ColorScheme monoScheme;
    monoScheme[ColorType::RESET] = "\033[0m";
    monoScheme[ColorType::SUCCESS] = "";
    monoScheme[ColorType::ERROR_COLOR] = "";
    monoScheme[ColorType::WARNING] = "";
    monoScheme[ColorType::INFO] = "";
    monoScheme[ColorType::HEADER] = "\033[1m";         // Bold
    monoScheme[ColorType::SUBHEADER] = "\033[4m";      // Underline
    monoScheme[ColorType::EMPHASIS] = "\033[1m";       // Bold
    monoScheme[ColorType::DIM] = "\033[2m";            // Dim
    colorSchemes_["mono"] = monoScheme;
}

Str TestColorManager::colorize(const Str& text, Color color) const {
    if (!colorSupported_) {
        return text;
    }
    
    Str colorCode;
    auto it = colorCodes_.find(color);
    if (it != colorCodes_.end()) {
        colorCode = it->second;
    }
    
    if (colorCode.empty()) {
        return text;
    }
    
    return colorCode + text + colorCodes_.at(Color::RESET);
}

Str TestColorManager::colorize(const Str& text, const Str& colorName) const {
    if (!colorSupported_) {
        return text;
    }
    
    // Map color names to Color enum values
    static const HashMap<Str, Color> colorMap = {
        {"reset", Color::RESET},
        {"black", Color::BLACK},
        {"red", Color::RED},
        {"green", Color::GREEN},
        {"yellow", Color::YELLOW},
        {"blue", Color::BLUE},
        {"magenta", Color::MAGENTA},
        {"cyan", Color::CYAN},
        {"white", Color::WHITE},
        {"bright_black", Color::BRIGHT_BLACK},
        {"bright_red", Color::BRIGHT_RED},
        {"bright_green", Color::BRIGHT_GREEN},
        {"bright_yellow", Color::BRIGHT_YELLOW},
        {"bright_blue", Color::BRIGHT_BLUE},
        {"bright_magenta", Color::BRIGHT_MAGENTA},
        {"bright_cyan", Color::BRIGHT_CYAN},
        {"bright_white", Color::BRIGHT_WHITE}
    };
    
    auto it = colorMap.find(colorName);
    if (it != colorMap.end()) {
        return colorize(text, it->second);
    }
    
    return text;
}

bool TestColorManager::supportsColor() const {
    return colorSupported_;
}

void TestColorManager::detectTerminalCapabilities() {
    initializeColorSupport();
}

void TestColorManager::setTheme(const Str& theme) {
    if (colorSchemes_.find(theme) != colorSchemes_.end()) {
        currentTheme_ = theme;
        TestConfig::getInstance().setTheme(theme);
    }
}

Str TestColorManager::getTheme() const {
    return currentTheme_;
}

Str TestColorManager::success(const Str& text) const {
    return getColor(ColorType::SUCCESS) + text + getColor(ColorType::RESET);
}

Str TestColorManager::error(const Str& text) const {
    return getColor(ColorType::ERROR_COLOR) + text + getColor(ColorType::RESET);
}

Str TestColorManager::warning(const Str& text) const {
    return getColor(ColorType::WARNING) + text + getColor(ColorType::RESET);
}

Str TestColorManager::info(const Str& text) const {
    return getColor(ColorType::INFO) + text + getColor(ColorType::RESET);
}

Str TestColorManager::header(const Str& text, TestLevel level) const {
    ColorType colorType;
    switch (level) {
        case TestLevel::MAIN:
        case TestLevel::MODULE:
            colorType = ColorType::HEADER;
            break;
        case TestLevel::SUITE:
        case TestLevel::GROUP:
            colorType = ColorType::SUBHEADER;
            break;
        case TestLevel::INDIVIDUAL:
        default:
            colorType = ColorType::EMPHASIS;
            break;
    }
    
    return getColor(colorType) + text + getColor(ColorType::RESET);
}

} // namespace TestFormatting
} // namespace Tests
} // namespace Lua
