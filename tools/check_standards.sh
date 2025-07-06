#!/bin/bash

# Lua 解释器项目代码规范检查脚本
# 使用方法: ./check_standards.sh [文件路径]

set -e

echo "🔍 Lua 解释器项目代码规范检查"
echo "========================================"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 检查函数
check_passed=0
check_failed=0

print_status() {
    if [ $1 -eq 0 ]; then
        echo -e "${GREEN}✅ $2${NC}"
        ((check_passed++))
    else
        echo -e "${RED}❌ $2${NC}"
        ((check_failed++))
    fi
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ️  $1${NC}"
}

# 检查文件是否存在
check_file_exists() {
    local file="$1"
    if [ ! -f "$file" ]; then
        echo -e "${RED}❌ 文件不存在: $file${NC}"
        exit 1
    fi
}

# 检查类型系统使用
check_type_system() {
    local file="$1"
    local forbidden_types=("std::string" "int " "double " "float " "unsigned int" "long ")
    local type_violations=0
    
    echo
    print_info "检查类型系统使用规范..."
    
    for type in "${forbidden_types[@]}"; do
        if grep -q "$type" "$file"; then
            echo -e "${RED}  - 发现禁用类型: $type${NC}"
            grep -n "$type" "$file" | head -3
            ((type_violations++))
        fi
    done
    
    # 检查是否包含 types.hpp
    if ! grep -q "#include.*types\.hpp" "$file"; then
        echo -e "${RED}  - 缺少 types.hpp 包含${NC}"
        ((type_violations++))
    fi
    
    print_status $type_violations "类型系统使用检查"
}

# 检查注释语言
check_comment_language() {
    local file="$1"
    local chinese_comments=0
    
    echo
    print_info "检查注释语言规范..."
    
    # 检查中文字符
    if grep -P "[\x{4e00}-\x{9fff}]" "$file" > /dev/null 2>&1; then
        echo -e "${RED}  - 发现中文注释:${NC}"
        grep -n -P "[\x{4e00}-\x{9fff}]" "$file" | head -3
        ((chinese_comments++))
    fi
    
    print_status $chinese_comments "注释语言检查"
}

# 检查命名规范
check_naming_convention() {
    local file="$1"
    local naming_violations=0
    
    echo
    print_info "检查命名规范..."
    
    # 检查类名是否使用 PascalCase (简单检查)
    if grep -q "class [a-z]" "$file"; then
        echo -e "${RED}  - 发现小写开头的类名${NC}"
        grep -n "class [a-z]" "$file" | head -3
        ((naming_violations++))
    fi
    
    # 检查函数名是否使用 camelCase (检查大写开头的非类函数)
    if grep -P "^\s*[A-Z][a-zA-Z]*\s*\(" "$file" > /dev/null 2>&1; then
        print_warning "可能存在 PascalCase 函数名，请确认是否为类构造函数"
    fi
    
    print_status $naming_violations "命名规范检查"
}

# 检查现代C++特性
check_modern_cpp() {
    local file="$1"
    local cpp_violations=0
    
    echo
    print_info "检查现代C++特性使用..."
    
    # 检查裸指针 new/delete
    if grep -q " new " "$file" || grep -q " delete " "$file"; then
        echo -e "${RED}  - 发现裸指针内存管理${NC}"
        grep -n -E "(new |delete )" "$file" | head -3
        ((cpp_violations++))
    fi
    
    # 检查是否使用 auto
    if grep -q "auto " "$file"; then
        print_info "✅ 发现使用 auto 类型推导"
    fi
    
    # 检查是否使用智能指针
    if grep -q "std::unique_ptr\|std::shared_ptr" "$file"; then
        print_info "✅ 发现使用智能指针"
    fi
    
    print_status $cpp_violations "现代C++特性检查"
}

# 检查文档注释
check_documentation() {
    local file="$1"
    local doc_violations=0
    
    echo
    print_info "检查文档注释完整性..."
    
    # 检查公共类是否有文档注释
    if grep -q "^class " "$file"; then
        if ! grep -B5 -A5 "^class " "$file" | grep -q "/\*\*"; then
            echo -e "${RED}  - 公共类缺少文档注释${NC}"
            ((doc_violations++))
        fi
    fi
    
    # 检查公共函数是否有文档注释
    public_functions=$(grep -n "public:" "$file" -A 20 | grep -c "^\s*[a-zA-Z].*(" || true)
    documented_functions=$(grep -n "public:" "$file" -A 20 | grep -B2 -A2 "/\*\*" | grep -c "^\s*[a-zA-Z].*(" || true)
    
    if [ $public_functions -gt 0 ] && [ $documented_functions -lt $((public_functions / 2)) ]; then
        echo -e "${RED}  - 公共函数文档注释覆盖率较低${NC}"
        ((doc_violations++))
    fi
    
    print_status $doc_violations "文档注释检查"
}

# 检查线程安全
check_thread_safety() {
    local file="$1"
    local thread_violations=0
    
    echo
    print_info "检查线程安全相关..."
    
    # 检查是否有共享数据但缺少互斥保护
    if grep -q "static.*=" "$file" && ! grep -q "mutex\|atomic" "$file"; then
        print_warning "发现静态变量但未见mutex保护，请确认线程安全"
    fi
    
    # 检查是否使用了线程安全的智能指针
    if grep -q "std::shared_ptr" "$file"; then
        print_info "✅ 使用 shared_ptr (引用计数线程安全)"
    fi
    
    print_status $thread_violations "线程安全检查"
}

# 主检查函数
perform_checks() {
    local file="$1"
    
    echo -e "${BLUE}检查文件: $file${NC}"
    echo "----------------------------------------"
    
    check_file_exists "$file"
    check_type_system "$file"
    check_comment_language "$file"
    check_naming_convention "$file"
    check_modern_cpp "$file"
    check_documentation "$file"
    check_thread_safety "$file"
}

# 显示检查结果
show_summary() {
    echo
    echo "========================================"
    echo "📊 检查结果汇总"
    echo "========================================"
    echo -e "通过项目: ${GREEN}$check_passed${NC}"
    echo -e "失败项目: ${RED}$check_failed${NC}"
    
    if [ $check_failed -eq 0 ]; then
        echo -e "${GREEN}🎉 所有检查通过！代码符合规范要求。${NC}"
        exit 0
    else
        echo -e "${RED}💥 存在 $check_failed 项规范违规，请修复后重新检查。${NC}"
        echo
        echo "📚 规范文档: DEVELOPMENT_STANDARDS.md"
        echo "🛠️  修复建议:"
        echo "  1. 使用 types.hpp 中定义的类型"
        echo "  2. 所有注释改为英文"
        echo "  3. 使用智能指针替代裸指针"
        echo "  4. 为公共接口添加文档注释"
        exit 1
    fi
}

# 主程序
main() {
    if [ $# -eq 0 ]; then
        echo "使用方法: $0 <文件路径>"
        echo "示例: $0 src/lib/base_lib.cpp"
        exit 1
    fi
    
    local file="$1"
    perform_checks "$file"
    show_summary
}

# 如果直接运行此脚本
if [ "${BASH_SOURCE[0]}" == "${0}" ]; then
    main "$@"
fi
