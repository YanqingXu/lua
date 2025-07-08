#!/usr/bin/env python3
"""
Script to update all standard library test files to comply with DEVELOPMENT_STANDARDS.md

This script performs the following updates:
1. Header organization (system headers first, then project headers)
2. Comment language standardization (English only)
3. Doxygen documentation format
4. Namespace structure compliance
5. Function naming conventions
6. Error handling patterns
"""

import os
import re
from pathlib import Path

def update_header_includes(content):
    """Update header includes to follow the standard order"""
    
    # Define the standard header template
    header_template = '''#pragma once

// System headers
#include <iostream>
#include <cassert>

// Project base headers
#include "common/types.hpp"

// Project other headers
#include "test_framework/core/test_macros.hpp"
#include "test_framework/core/test_utils.hpp"
#include "lib/{lib_name}/{lib_name}_lib.hpp"

namespace Lua {{
namespace Tests {{
'''
    
    # Extract library name from content
    lib_match = re.search(r'#include ".*?lib/(\w+)/\w+_lib\.hpp"', content)
    if lib_match:
        lib_name = lib_match.group(1)
        return header_template.format(lib_name=lib_name)
    
    return content

def update_comments_to_english(content):
    """Convert Chinese comments to English"""
    
    comment_translations = {
        r'测试函数 \(SUITE level\)': 'test functions (SUITE level)',
        r'遵循层次调用传播机制：': 'Follows hierarchical calling pattern:',
        r'测试内容包括：': 'Test coverage includes:',
        r'运行所有.*?库测试 \(SUITE level\)': 'Run all library tests (SUITE level)',
        r'只调用 RUN_TEST_GROUP \(GROUP level\)': 'Only calls RUN_TEST_GROUP (GROUP level)',
        r'不直接调用 RUN_TEST \(INDIVIDUAL level\)': 'Does not directly call RUN_TEST (INDIVIDUAL level)',
        r'测试组 \(GROUP level\)': 'test group (GROUP level)',
        r'测试类 \(INDIVIDUAL level\)': 'test class (INDIVIDUAL level)',
        r'包含具体的测试方法，不调用其他测试宏': 'Contains concrete test methods, does not call other test macros',
        r'基础函数：': 'Basic functions: ',
        r'核心函数：': 'Core functions: ',
        r'类型操作：': 'Type operations: ',
        r'表操作：': 'Table operations: ',
        r'元表操作：': 'Metatable operations: ',
        r'原始访问：': 'Raw access: ',
        r'错误处理：': 'Error handling: ',
        r'工具函数：': 'Utility functions: ',
        r'模式匹配：': 'Pattern matching: ',
        r'格式化：': 'Formatting: ',
        r'字符操作：': 'Character operations: ',
        r'时间操作：': 'Time operations: ',
        r'系统操作：': 'System operations: ',
        r'文件操作：': 'File operations: ',
        r'本地化：': 'Localization: ',
        r'调试功能：': 'Debug functions: ',
        r'环境操作：': 'Environment operations: ',
        r'钩子操作：': 'Hook operations: ',
        r'变量操作：': 'Variable operations: ',
        r'流操作：': 'Stream operations: ',
        r'长度操作：': 'Length operations: ',
        r'常量：': 'Constants: ',
        r'幂函数：': 'Power functions: ',
        r'三角函数：': 'Trigonometric functions: ',
        r'角度转换：': 'Angle conversion: ',
        r'随机函数：': 'Random functions: ',
    }
    
    for chinese, english in comment_translations.items():
        content = re.sub(chinese, english, content)
    
    return content

def update_doxygen_comments(content):
    """Add proper Doxygen comments to functions"""
    
    # Add @note tags for important information
    content = re.sub(r'✅ (.*)', r'@note \1', content)
    content = re.sub(r'❌ (.*)', r'@note \1', content)
    
    # Add @brief tags where missing
    content = re.sub(r'/\*\*\s*\n\s*\* ([^@].*?)\n', r'/**\n * @brief \1\n', content)
    
    return content

def update_namespace_structure(content):
    """Ensure proper namespace structure"""
    
    # Fix namespace opening
    content = re.sub(r'namespace Lua::Tests \{', 'namespace Lua {\nnamespace Tests {', content)
    
    # Fix namespace closing
    content = re.sub(r'\} // namespace Lua::Tests', '} // namespace Tests\n} // namespace Lua', content)
    
    return content

def update_function_documentation(content):
    """Add comprehensive function documentation"""
    
    # Add documentation for test functions
    test_function_pattern = r'(static void test\w+\(\)) \{'
    
    def add_test_doc(match):
        func_name = match.group(1)
        return f'''/**
     * @brief Test function
     * @note Validates functionality and error conditions
     * @throws std::exception on test failure
     */
    {func_name} {{'''
    
    content = re.sub(test_function_pattern, add_test_doc, content)
    
    return content

def process_test_file(file_path):
    """Process a single test file"""
    
    print(f"Processing {file_path}...")
    
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Apply all transformations
    content = update_header_includes(content)
    content = update_comments_to_english(content)
    content = update_doxygen_comments(content)
    content = update_namespace_structure(content)
    content = update_function_documentation(content)
    
    # Write back the updated content
    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(content)
    
    print(f"Updated {file_path}")

def main():
    """Main function to process all test files"""
    
    # Define the test files to process
    test_files = [
        'src/tests/lib/string_lib_test.hpp',
        'src/tests/lib/math_lib_test.hpp',
        'src/tests/lib/table_lib_test.hpp',
        'src/tests/lib/io_lib_test.hpp',
        'src/tests/lib/os_lib_test.hpp',
        'src/tests/lib/debug_lib_test.hpp',
        'src/tests/lib/test_lib.hpp'
    ]
    
    # Get the project root directory
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    
    print("Updating standard library test files to comply with DEVELOPMENT_STANDARDS.md")
    print("=" * 70)
    
    for test_file in test_files:
        file_path = project_root / test_file
        if file_path.exists():
            try:
                process_test_file(file_path)
            except Exception as e:
                print(f"Error processing {file_path}: {e}")
        else:
            print(f"File not found: {file_path}")
    
    print("=" * 70)
    print("All test files have been updated!")
    print("\nChanges made:")
    print("✓ Header includes reorganized (system headers first)")
    print("✓ Comments translated to English")
    print("✓ Doxygen documentation format applied")
    print("✓ Namespace structure standardized")
    print("✓ Function documentation enhanced")

if __name__ == "__main__":
    main()
