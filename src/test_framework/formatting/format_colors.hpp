#pragma once
#include "../../common/types.hpp"
#include "format_define.hpp"
#include <string>
#include <unordered_map>
#include <iostream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#endif

namespace Lua {
namespace Tests {
namespace TestFormatting {


// Type alias for color scheme mapping
using ColorScheme = HashMap<ColorType, Str>;

class TestColorManager {
    private:
        HashMap<Color, Str> colorCodes_;
        HashMap<Str, ColorScheme> colorSchemes_;
        bool terminalSupportsColor_;
        bool colorSupported_;
        Str currentTheme_;
        
        void initializeColorCodes() {
            colorCodes_[Color::RESET] = "\033[0m";
            colorCodes_[Color::BLACK] = "\033[30m";
            colorCodes_[Color::RED] = "\033[31m";
            colorCodes_[Color::GREEN] = "\033[32m";
            colorCodes_[Color::YELLOW] = "\033[33m";
            colorCodes_[Color::BLUE] = "\033[34m";
            colorCodes_[Color::MAGENTA] = "\033[35m";
            colorCodes_[Color::CYAN] = "\033[36m";
            colorCodes_[Color::WHITE] = "\033[37m";
            colorCodes_[Color::BRIGHT_BLACK] = "\033[90m";
            colorCodes_[Color::BRIGHT_RED] = "\033[91m";
            colorCodes_[Color::BRIGHT_GREEN] = "\033[92m";
            colorCodes_[Color::BRIGHT_YELLOW] = "\033[93m";
            colorCodes_[Color::BRIGHT_BLUE] = "\033[94m";
            colorCodes_[Color::BRIGHT_MAGENTA] = "\033[95m";
            colorCodes_[Color::BRIGHT_CYAN] = "\033[96m";
            colorCodes_[Color::BRIGHT_WHITE] = "\033[97m";
        }
        
    public:
        TestColorManager();
        
        Str colorize(const Str& text, Color color) const;
        Str colorize(const Str& text, const Str& colorName) const;
        bool supportsColor() const;
        bool isColorSupported() const;
        
        void initializeColorSchemes();
        void initializeColorSupport();
        
        void detectTerminalCapabilities();
        
        void setTheme(const Str& theme);
        Str getTheme() const;
        
        // Custom theme support
        void addCustomTheme(const Str& name, const ColorScheme& scheme);
        bool hasTheme(const Str& theme) const;
        Vec<Str> getAvailableThemes() const;
        
        // Get color code for ColorType
        Str getColor(ColorType type) const;
        
        // 便捷方法
        Str success(const Str& text) const;
        Str error(const Str& text) const;
        Str warning(const Str& text) const;
        Str info(const Str& text) const;
        
        Str header(const Str& text, TestLevel level) const;
    };

} // namespace TestFormatting
} // namespace Tests
} // namespace Lua