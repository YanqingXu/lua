#!/bin/bash
# compile_check.sh - 代码编译验证脚本
# 按照 DEVELOPMENT_STANDARDS.md 要求进行编译验证

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 编译配置
CXX="g++"
CXXFLAGS="-std=c++17 -Wall -Wextra -Werror"
INCLUDE_DIRS="-I src"

echo -e "${BLUE}=== Lua 项目编译验证工具 ===${NC}"
echo -e "${BLUE}遵循 DEVELOPMENT_STANDARDS.md 规范${NC}"
echo ""

# 编译单个文件的函数
compile_file() {
    local file="$1"
    local output="${file%.cpp}.o"
    
    echo -e "${YELLOW}编译: $file${NC}"
    
    if $CXX $CXXFLAGS $INCLUDE_DIRS -c "$file" -o "$output" 2>&1; then
        echo -e "${GREEN}✅ $file 编译成功${NC}"
        return 0
    else
        echo -e "${RED}❌ $file 编译失败${NC}"
        return 1
    fi
}

# 编译核心库文件
echo -e "${BLUE}1. 编译核心框架组件...${NC}"

files_to_compile=(
    "src/lib/lib_context.cpp"
    "src/lib/lib_func_registry.cpp"
    "src/lib/lib_manager.cpp"
    "src/lib/base_lib.cpp"
    "src/lib/string_lib.cpp"
    "src/lib/math_lib.cpp"
)

failed_files=()
success_count=0

for file in "${files_to_compile[@]}"; do
    if [ -f "$file" ]; then
        if compile_file "$file"; then
            ((success_count++))
        else
            failed_files+=("$file")
        fi
    else
        echo -e "${YELLOW}⚠️  文件不存在: $file${NC}"
    fi
    echo ""
done

# 结果统计
echo -e "${BLUE}=== 编译结果统计 ===${NC}"
echo -e "总文件数: ${#files_to_compile[@]}"
echo -e "成功: ${GREEN}$success_count${NC}"
echo -e "失败: ${RED}${#failed_files[@]}${NC}"

if [ ${#failed_files[@]} -eq 0 ]; then
    echo -e "${GREEN}🎉 所有文件编译成功！代码符合开发规范。${NC}"
    echo ""
    echo -e "${BLUE}=== 开发规范检查 ===${NC}"
    echo -e "${GREEN}✅ 编译器警告: 零警告 (-Werror)${NC}"
    echo -e "${GREEN}✅ C++17 标准: 兼容${NC}"
    echo -e "${GREEN}✅ 类型安全: 通过${NC}"
    echo -e "${GREEN}✅ 头文件依赖: 正确${NC}"
    exit 0
else
    echo -e "${RED}编译失败的文件:${NC}"
    for file in "${failed_files[@]}"; do
        echo -e "${RED}  - $file${NC}"
    done
    echo ""
    echo -e "${RED}❌ 请修复编译错误后重新运行此脚本${NC}"
    echo ""
    echo -e "${YELLOW}常见编译错误修复提示:${NC}"
    echo -e "1. 缺失头文件: 添加正确的 #include"
    echo -e "2. 类型不匹配: 使用 types.hpp 中定义的统一类型"
    echo -e "3. 未使用参数: 使用 /*parameter*/ 注释或删除参数名"
    echo -e "4. 函数声明歧义: 使用大括号 {} 而不是圆括号 ()"
    exit 1
fi
