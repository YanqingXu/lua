#include "format_strategies.hpp"
#include "test_colors.hpp"
#include <iostream>

namespace Lua {
namespace Tests {
namespace TestFormatting {

// MainFormatStrategy Implementation
void MainFormatStrategy::printHeader(const std::string& title, const std::string& description, const TestColorManager& colorManager) {
    std::string headerColor = colorManager.getColor(ColorType::HEADER);
    std::string emphasisColor = colorManager.getColor(ColorType::EMPHASIS);
    std::string resetColor = colorManager.getColor(ColorType::RESET);
    
    std::cout << "\n" << headerColor << std::string(80, '=') << resetColor << std::endl;
    std::cout << headerColor << "  " << emphasisColor << title << resetColor << std::endl;
    if (!description.empty()) {
        std::cout << headerColor << "  " << description << resetColor << std::endl;
    }
    std::cout << headerColor << std::string(80, '=') << resetColor << std::endl;
}

void MainFormatStrategy::printFooter(const std::string& message, const TestColorManager& colorManager) {
    std::string headerColor = colorManager.getColor(ColorType::HEADER);
    std::string resetColor = colorManager.getColor(ColorType::RESET);
    
    std::cout << headerColor << std::string(80, '=') << resetColor << std::endl;
    if (!message.empty()) {
        std::cout << headerColor << "  " << message << resetColor << std::endl;
        std::cout << headerColor << std::string(80, '=') << resetColor << std::endl;
    }
    std::cout << std::endl;
}

// SuiteFormatStrategy Implementation
void SuiteFormatStrategy::printHeader(const std::string& title, const std::string& description, const TestColorManager& colorManager) {
    std::string subheaderColor = colorManager.getColor(ColorType::SUBHEADER);
    std::string emphasisColor = colorManager.getColor(ColorType::EMPHASIS);
    std::string resetColor = colorManager.getColor(ColorType::RESET);
    
    std::cout << "\n" << subheaderColor << std::string(60, '-') << resetColor << std::endl;
    std::cout << subheaderColor << " " << emphasisColor << title << resetColor << std::endl;
    if (!description.empty()) {
        std::cout << subheaderColor << " " << description << resetColor << std::endl;
    }
    std::cout << subheaderColor << std::string(60, '-') << resetColor << std::endl;
}

void SuiteFormatStrategy::printFooter(const std::string& message, const TestColorManager& colorManager) {
    std::string subheaderColor = colorManager.getColor(ColorType::SUBHEADER);
    std::string resetColor = colorManager.getColor(ColorType::RESET);
    
    if (!message.empty()) {
        std::cout << subheaderColor << " " << message << resetColor << std::endl;
    }
    std::cout << subheaderColor << std::string(60, '-') << resetColor << std::endl;
}

// GroupFormatStrategy Implementation
void GroupFormatStrategy::printHeader(const std::string& title, const std::string& description, const TestColorManager& colorManager) {
    std::string infoColor = colorManager.getColor(ColorType::INFO);
    std::string emphasisColor = colorManager.getColor(ColorType::EMPHASIS);
    std::string resetColor = colorManager.getColor(ColorType::RESET);
    
    std::cout << "\n" << infoColor << "┌─ " << emphasisColor << title << resetColor;
    if (!description.empty()) {
        std::cout << infoColor << " - " << description << resetColor;
    }
    std::cout << std::endl;
}

void GroupFormatStrategy::printFooter(const std::string& message, const TestColorManager& colorManager) {
    std::string infoColor = colorManager.getColor(ColorType::INFO);
    std::string resetColor = colorManager.getColor(ColorType::RESET);
    
    std::cout << infoColor << "└─";
    if (!message.empty()) {
        std::cout << " " << message;
    }
    std::cout << resetColor << std::endl;
}

// IndividualFormatStrategy Implementation
void IndividualFormatStrategy::printHeader(const std::string& title, const std::string& description, const TestColorManager& colorManager) {
    std::string dimColor = colorManager.getColor(ColorType::DIM);
    std::string resetColor = colorManager.getColor(ColorType::RESET);
    
    std::cout << dimColor << "  - " << title;
    if (!description.empty()) {
        std::cout << " - " << description;
    }
    std::cout << resetColor << std::endl;
}

void IndividualFormatStrategy::printFooter(const std::string& message, const TestColorManager& colorManager) {
    std::string dimColor = colorManager.getColor(ColorType::DIM);
    std::string resetColor = colorManager.getColor(ColorType::RESET);
    
    if (!message.empty()) {
        std::cout << dimColor << "    " << message << resetColor << std::endl;
    }
}

} // namespace TestFormatting
} // namespace Tests
} // namespace Lua