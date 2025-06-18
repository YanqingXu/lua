#include "format_strategies.hpp"
#include "test_colors.hpp"
#include <iostream>

namespace Lua {
namespace Tests {
namespace TestFormatting {

// MainFormatStrategy Implementation
void MainFormatStrategy::printHeader(const Str& title, const Str& description, const TestColorManager& colorManager) {
    Str headerColor = colorManager.getColor(ColorType::HEADER);
    Str emphasisColor = colorManager.getColor(ColorType::EMPHASIS);
    Str resetColor = colorManager.getColor(ColorType::RESET);
    
    std::cout << "\n" << headerColor << Str(80, '=') << resetColor << std::endl;
    std::cout << headerColor << "  " << emphasisColor << title << resetColor << std::endl;
    if (!description.empty()) {
        std::cout << headerColor << "  " << description << resetColor << std::endl;
    }
    std::cout << headerColor << Str(80, '=') << resetColor << std::endl;
}

void MainFormatStrategy::printFooter(const Str& message, const TestColorManager& colorManager) {
    Str headerColor = colorManager.getColor(ColorType::HEADER);
    Str resetColor = colorManager.getColor(ColorType::RESET);
    
    std::cout << headerColor << Str(80, '=') << resetColor << std::endl;
    if (!message.empty()) {
        std::cout << headerColor << "  " << message << resetColor << std::endl;
        std::cout << headerColor << Str(80, '=') << resetColor << std::endl;
    }
    std::cout << std::endl;
}

// ModuleFormatStrategy Implementation
void ModuleFormatStrategy::printHeader(const Str& title, const Str& description, const TestColorManager& colorManager) {
    Str headerColor = colorManager.getColor(ColorType::HEADER);
    Str emphasisColor = colorManager.getColor(ColorType::EMPHASIS);
    Str resetColor = colorManager.getColor(ColorType::RESET);
    
    std::cout << "\n" << headerColor << "+" << Str(70, '-') << "+" << resetColor << std::endl;
    std::cout << headerColor << "| " << emphasisColor << title << Str(70 - title.length() - 1, ' ') << headerColor << "|" << resetColor << std::endl;
    if (!description.empty()) {
        std::cout << headerColor << "| " << description << Str(70 - description.length() - 1, ' ') << "|" << resetColor << std::endl;
    }
    std::cout << headerColor << "+" << Str(70, '-') << "+" << resetColor << std::endl;
}

void ModuleFormatStrategy::printFooter(const Str& message, const TestColorManager& colorManager) {
    Str headerColor = colorManager.getColor(ColorType::HEADER);
    Str resetColor = colorManager.getColor(ColorType::RESET);
    
    if (!message.empty()) {
        std::cout << headerColor << "+- " << message << resetColor << std::endl;
    }
    std::cout << std::endl;
}

// SuiteFormatStrategy Implementation
void SuiteFormatStrategy::printHeader(const Str& title, const Str& description, const TestColorManager& colorManager) {
    Str subheaderColor = colorManager.getColor(ColorType::SUBHEADER);
    Str emphasisColor = colorManager.getColor(ColorType::EMPHASIS);
    Str resetColor = colorManager.getColor(ColorType::RESET);
    
    std::cout << "\n" << subheaderColor << "  +" << Str(60, '-') << "+" << resetColor << std::endl;
    std::cout << subheaderColor << "  | " << emphasisColor << title << Str(60 - title.length() - 1, ' ') << subheaderColor << "|" << resetColor << std::endl;
    if (!description.empty()) {
        std::cout << subheaderColor << "  | " << description << Str(60 - description.length() - 1, ' ') << "|" << resetColor << std::endl;
    }
    std::cout << subheaderColor << "  +" << Str(60, '-') << "+" << resetColor << std::endl;
}

void SuiteFormatStrategy::printFooter(const Str& message, const TestColorManager& colorManager) {
    Str subheaderColor = colorManager.getColor(ColorType::SUBHEADER);
    Str resetColor = colorManager.getColor(ColorType::RESET);
    
    if (!message.empty()) {
        std::cout << subheaderColor << "  +- " << message << resetColor << std::endl;
    }
    std::cout << std::endl;
}

// GroupFormatStrategy Implementation
void GroupFormatStrategy::printHeader(const Str& title, const Str& description, const TestColorManager& colorManager) {
    Str infoColor = colorManager.getColor(ColorType::INFO);
    Str emphasisColor = colorManager.getColor(ColorType::EMPHASIS);
    Str resetColor = colorManager.getColor(ColorType::RESET);
    
    std::cout << "\n" << infoColor << "+- " << emphasisColor << title << resetColor;
    if (!description.empty()) {
        std::cout << infoColor << " - " << description << resetColor;
    }
    std::cout << std::endl;
}

void GroupFormatStrategy::printFooter(const Str& message, const TestColorManager& colorManager) {
    Str infoColor = colorManager.getColor(ColorType::INFO);
    Str resetColor = colorManager.getColor(ColorType::RESET);
    
    std::cout << infoColor << "+-";
    if (!message.empty()) {
        std::cout << " " << message;
    }
    std::cout << resetColor << std::endl;
}

// IndividualFormatStrategy Implementation
void IndividualFormatStrategy::printHeader(const Str& title, const Str& description, const TestColorManager& colorManager) {
    Str dimColor = colorManager.getColor(ColorType::DIM);
    Str resetColor = colorManager.getColor(ColorType::RESET);
    
    std::cout << dimColor << "  - " << title;
    if (!description.empty()) {
        std::cout << " - " << description;
    }
    std::cout << resetColor << std::endl;
}

void IndividualFormatStrategy::printFooter(const Str& message, const TestColorManager& colorManager) {
    Str dimColor = colorManager.getColor(ColorType::DIM);
    Str resetColor = colorManager.getColor(ColorType::RESET);
    
    if (!message.empty()) {
        std::cout << dimColor << "    " << message << resetColor << std::endl;
    }
}

} // namespace TestFormatting
} // namespace Tests
} // namespace Lua