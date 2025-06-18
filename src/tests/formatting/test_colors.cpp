#include "test_colors.hpp"
#include "test_config.hpp"
#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#endif

namespace Lua {
namespace Tests {
namespace TestFormatting {

TestColorManager::TestColorManager() {
    initializeColorSupport();
    initializeColorSchemes();
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
#ifdef _MSC_VER
        // Use secure version for MSVC
        char* wt_session = nullptr;
        char* term_program = nullptr;
        size_t len;
        
        _dupenv_s(&wt_session, &len, "WT_SESSION");
        _dupenv_s(&term_program, &len, "TERM_PROGRAM");
        
        if (wt_session || term_program) {
            colorSupported_ = true;
        }
        
        // Free allocated memory
        if (wt_session) free(wt_session);
        if (term_program) free(term_program);
#else
        // Use standard getenv for other compilers (MinGW, etc.)
        const char* wt_session = std::getenv("WT_SESSION");
        const char* term_program = std::getenv("TERM_PROGRAM");
        if (wt_session || term_program) {
            colorSupported_ = true;
        }
#endif
    }
#else
    // Unix-like systems
    if (isatty(fileno(stdout))) {
        const char* term = std::getenv("TERM");
        if (term) {
            std::string termStr(term);
            if (termStr.find("color") != std::string::npos ||
                termStr == "xterm" || termStr == "xterm-256color" ||
                termStr == "screen" || termStr == "tmux" ||
                termStr == "linux") {
                colorSupported_ = true;
            }
        }
    }
#endif
    
    // Check environment variables
#ifdef _MSC_VER
    // Use secure version for MSVC
    char* forceColor = nullptr;
    char* noColor = nullptr;
    size_t len;
    
    _dupenv_s(&forceColor, &len, "FORCE_COLOR");
    if (forceColor && strlen(forceColor) > 0) {
        colorSupported_ = true;
    }
    
    _dupenv_s(&noColor, &len, "NO_COLOR");
    if (noColor && strlen(noColor) > 0) {
        colorSupported_ = false;
    }
    
    // Free allocated memory
    if (forceColor) free(forceColor);
    if (noColor) free(noColor);
#else
    // Use standard getenv for other compilers (MinGW, etc.)
    const char* forceColor = std::getenv("FORCE_COLOR");
    if (forceColor && strlen(forceColor) > 0) {
        colorSupported_ = true;
    }
    
    const char* noColor = std::getenv("NO_COLOR");
    if (noColor && strlen(noColor) > 0) {
        colorSupported_ = false;
    }
#endif
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

} // namespace TestFormatting
} // namespace Tests
} // namespace Lua