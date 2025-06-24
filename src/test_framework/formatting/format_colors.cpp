#include "format_colors.hpp"
#include "format_config.hpp"
#include <iostream>
#include <cstdlib>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#endif

namespace Lua {
namespace Tests {
namespace TestFormatting {

// 使用format_config.cpp中定义的safe_getenv函数
extern Str safe_getenv(const char* name);

TestColorManager::TestColorManager() {
    initializeColorCodes();
    initializeColorSupport();
    initializeColorSchemes();
    currentTheme_ = "default";
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
        Str wt_session = safe_getenv("WT_SESSION");
        Str term_program = safe_getenv("TERM_PROGRAM");
        
        if (!wt_session.empty() || !term_program.empty()) {
            colorSupported_ = true;
        }
    }
#else
    // Unix-like systems
    if (isatty(fileno(stdout))) {
        Str term = safe_getenv("TERM");
        if (!term.empty()) {
            if (term.find("color") != Str::npos ||
                term == "xterm" || term == "xterm-256color" ||
                term == "screen" || term == "tmux" ||
                term == "linux") {
                colorSupported_ = true;
            }
        }
    }
#endif
    
    // Check environment variables
    Str forceColor = safe_getenv("FORCE_COLOR");
    if (!forceColor.empty()) {
        colorSupported_ = true;
    }
    
    Str noColor = safe_getenv("NO_COLOR");
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
    
    // High contrast theme (for accessibility)
    ColorScheme highContrastScheme;
    highContrastScheme[ColorType::RESET] = "\033[0m";
    highContrastScheme[ColorType::SUCCESS] = "\033[1;42;30m";    // Bold Black on Green
    highContrastScheme[ColorType::ERROR_COLOR] = "\033[1;41;37m";      // Bold White on Red
    highContrastScheme[ColorType::WARNING] = "\033[1;43;30m";    // Bold Black on Yellow
    highContrastScheme[ColorType::INFO] = "\033[1;46;30m";       // Bold Black on Cyan
    highContrastScheme[ColorType::HEADER] = "\033[1;44;37m";     // Bold White on Blue
    highContrastScheme[ColorType::SUBHEADER] = "\033[1;45;37m";  // Bold White on Magenta
    highContrastScheme[ColorType::EMPHASIS] = "\033[1;47;30m";   // Bold Black on White
    highContrastScheme[ColorType::DIM] = "\033[2;37m";          // Dim White
    colorSchemes_["high-contrast"] = highContrastScheme;
    
    // Pastel theme (soft colors)
    ColorScheme pastelScheme;
    pastelScheme[ColorType::RESET] = "\033[0m";
    pastelScheme[ColorType::SUCCESS] = "\033[38;5;120m";     // Light Green
    pastelScheme[ColorType::ERROR_COLOR] = "\033[38;5;210m";       // Light Red
    pastelScheme[ColorType::WARNING] = "\033[38;5;222m";     // Light Yellow
    pastelScheme[ColorType::INFO] = "\033[38;5;117m";        // Light Blue
    pastelScheme[ColorType::HEADER] = "\033[1;38;5;105m";    // Bold Light Purple
    pastelScheme[ColorType::SUBHEADER] = "\033[38;5;183m";   // Light Pink
    pastelScheme[ColorType::EMPHASIS] = "\033[1;38;5;189m";  // Bold Light Cyan
    pastelScheme[ColorType::DIM] = "\033[2;38;5;250m";       // Dim Light Gray
    colorSchemes_["pastel"] = pastelScheme;
    
    // Neon theme (bright, vibrant colors)
    ColorScheme neonScheme;
    neonScheme[ColorType::RESET] = "\033[0m";
    neonScheme[ColorType::SUCCESS] = "\033[1;38;5;46m";      // Bright Neon Green
    neonScheme[ColorType::ERROR_COLOR] = "\033[1;38;5;196m";       // Bright Neon Red
    neonScheme[ColorType::WARNING] = "\033[1;38;5;226m";     // Bright Neon Yellow
    neonScheme[ColorType::INFO] = "\033[1;38;5;51m";         // Bright Neon Cyan
    neonScheme[ColorType::HEADER] = "\033[1;38;5;21m";       // Bright Neon Blue
    neonScheme[ColorType::SUBHEADER] = "\033[1;38;5;201m";   // Bright Neon Magenta
    neonScheme[ColorType::EMPHASIS] = "\033[1;38;5;15m";     // Bright White
    neonScheme[ColorType::DIM] = "\033[2;38;5;8m";           // Dim Gray
    colorSchemes_["neon"] = neonScheme;
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

Str TestColorManager::getColor(ColorType type) const {
    if (!colorSupported_) {
        return "";
    }
    
    auto schemeIt = colorSchemes_.find(currentTheme_);
    if (schemeIt == colorSchemes_.end()) {
        // Fallback to default theme
        schemeIt = colorSchemes_.find("default");
        if (schemeIt == colorSchemes_.end()) {
            return "";
        }
    }
    
    const auto& scheme = schemeIt->second;
    auto colorIt = scheme.find(type);
    if (colorIt != scheme.end()) {
        return colorIt->second;
    }
    
    return "";
}

bool TestColorManager::isColorSupported() const {
    return colorSupported_;
}

void TestColorManager::addCustomTheme(const Str& name, const ColorScheme& scheme) {
    colorSchemes_[name] = scheme;
}

bool TestColorManager::hasTheme(const Str& theme) const {
    return colorSchemes_.find(theme) != colorSchemes_.end();
}

Vec<Str> TestColorManager::getAvailableThemes() const {
    Vec<Str> themes;
    for (const auto& pair : colorSchemes_) {
        themes.push_back(pair.first);
    }
    return themes;
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
