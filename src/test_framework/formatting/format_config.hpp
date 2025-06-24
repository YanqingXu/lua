#pragma once
#include "../../common/types.hpp"
#include "format_define.hpp"
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include <cctype>

namespace Lua {
namespace Tests {
namespace TestFormatting {
    struct LevelConfig {
        Str headerChar = "=";
        Str footerChar = "=";
        i32 width = 80;
        i32 indent = 0;
        bool showTimestamp = true;
        bool showStatistics = true;
        bool useDoubleLines = false;
        
        LevelConfig() = default;
        LevelConfig(const Str& hChar, const Str& fChar, i32 w, i32 ind, 
                   bool timestamp = true, bool stats = true, bool doubleLine = false)
            : headerChar(hChar), footerChar(fChar), width(w), indent(ind), 
              showTimestamp(timestamp), showStatistics(stats), useDoubleLines(doubleLine) {}
    };

    class TestConfig {
    private:
        HashMap<TestLevel, LevelConfig> levelConfigs_;
        bool colorEnabled_ = true;
        Str theme_ = "default";
        
        static TestConfig* instance_;
        
        TestConfig();

        // Remove left space
        static void ltrim(Str &s) {
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
        }

        // Remove right space
        static void rtrim(Str &s) {
            s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(), s.end());
        }

        // Remove both left and right space
        static void trim(Str &s) {
            ltrim(s);
            rtrim(s);
        }

        static Str trim(const Str& s) {
            Str trimmed = s;
            trim(trimmed);
            return trimmed;
        }

    public:
        static TestConfig& getInstance();
        
        const LevelConfig& getLevelConfig(TestLevel level) const;
        
        void setLevelConfig(TestLevel level, const LevelConfig& config);
        
        bool isColorEnabled() const;
        void setColorEnabled(bool enabled);
        
        const Str& getTheme() const;
        void setTheme(const Str& theme);
        
        bool loadFromFile(const Str& filename);
        bool saveToFile(const Str& filename) const;

        void loadFromEnvironment();
        void initializeDefaults();
    };

} // namespace TestFormatting
} // namespace Tests
} // namespace Lua