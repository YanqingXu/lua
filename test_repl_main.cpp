#include <iostream>
#include <sstream>
#include <string>
#include "src/repl.cpp"

// 简单的REPL测试函数
void testREPLComponents() {
    std::cout << "=== 测试REPL组件 ===" << std::endl;
    
    // 测试不完整语句检测器
    Lua::IncompleteStatementDetector detector;
    
    // 测试完整语句
    std::cout << "测试完整语句:" << std::endl;
    std::cout << "  'x = 1' -> " << (detector.isComplete("x = 1") ? "完整" : "不完整") << std::endl;
    std::cout << "  'print(\"hello\")' -> " << (detector.isComplete("print(\"hello\")") ? "完整" : "不完整") << std::endl;
    
    // 测试不完整语句
    std::cout << "\n测试不完整语句:" << std::endl;
    std::cout << "  'function test()' -> " << (detector.isComplete("function test()") ? "完整" : "不完整") << std::endl;
    if (!detector.isComplete("function test()")) {
        std::cout << "    原因: " << detector.getIncompleteReason() << std::endl;
    }
    
    std::cout << "  'if x > 0 then' -> " << (detector.isComplete("if x > 0 then") ? "完整" : "不完整") << std::endl;
    if (!detector.isComplete("if x > 0 then")) {
        std::cout << "    原因: " << detector.getIncompleteReason() << std::endl;
    }
    
    std::cout << "  't = {' -> " << (detector.isComplete("t = {") ? "完整" : "不完整") << std::endl;
    if (!detector.isComplete("t = {")) {
        std::cout << "    原因: " << detector.getIncompleteReason() << std::endl;
    }
    
    std::cout << "  'print(' -> " << (detector.isComplete("print(") ? "完整" : "不完整") << std::endl;
    if (!detector.isComplete("print(")) {
        std::cout << "    原因: " << detector.getIncompleteReason() << std::endl;
    }
    
    // 测试表达式检测
    std::cout << "\n测试表达式检测:" << std::endl;
    std::cout << "  '1 + 2' -> " << (isPureExpression("1 + 2") ? "表达式" : "语句") << std::endl;
    std::cout << "  'x = 1' -> " << (isPureExpression("x = 1") ? "表达式" : "语句") << std::endl;
    std::cout << "  'math.sin(3.14)' -> " << (isPureExpression("math.sin(3.14)") ? "表达式" : "语句") << std::endl;
    std::cout << "  'local x = 1' -> " << (isPureExpression("local x = 1") ? "表达式" : "语句") << std::endl;
    
    // 测试值格式化
    std::cout << "\n测试值格式化:" << std::endl;
    std::cout << "  nil -> " << formatValue(Lua::Value(nullptr)) << std::endl;
    std::cout << "  true -> " << formatValue(Lua::Value(true)) << std::endl;
    std::cout << "  false -> " << formatValue(Lua::Value(false)) << std::endl;
    std::cout << "  42 -> " << formatValue(Lua::Value(42)) << std::endl;
    std::cout << "  3.14 -> " << formatValue(Lua::Value(3.14)) << std::endl;
    std::cout << "  \"hello\" -> " << formatValue(Lua::Value("hello")) << std::endl;
    
    std::cout << "\n=== REPL组件测试完成 ===" << std::endl;
}

int main() {
    std::cout << "REPL功能测试程序" << std::endl;
    std::cout << "=================" << std::endl;
    
    try {
        testREPLComponents();
        
        std::cout << "\n要启动交互式REPL吗? (y/n): ";
        char choice;
        std::cin >> choice;
        
        if (choice == 'y' || choice == 'Y') {
            std::cout << "\n启动REPL..." << std::endl;
            run_repl();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
