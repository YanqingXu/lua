#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
check_naming_simple.py - 简化版测试文件命名规范检查工具

这是一个轻量级的命名规范检查脚本，基于 README.md 中的原始代码。
"""

import os
import re
from pathlib import Path

def check_test_files(directory):
    """检查测试文件命名规范"""
    issues = []
    
    if not os.path.exists(directory):
        print(f"错误: 目录 '{directory}' 不存在")
        return issues
    
    for root, dirs, files in os.walk(directory):
        # 跳过隐藏目录、构建目录和formatting目录（测试框架组件）
        dirs[:] = [d for d in dirs if not d.startswith('.') and d not in ['build', 'Debug', 'Release', 'formatting']]
        
        for file in files:
            if file.endswith(('.hpp', '.cpp')):
                if not check_naming_convention(file, root):
                    relative_path = os.path.relpath(os.path.join(root, file), directory)
                    issues.append(relative_path)
    
    return issues

def check_naming_convention(filename, path):
    """检查单个文件的命名规范"""
    # 主测试文件规范: test_{模块名}_{子模块名}.hpp
    if filename.startswith('test_'):
        return re.match(r'^test_[a-z]+(_[a-z]+)*\.hpp$', filename) is not None
    
    # 子模块测试文件规范: {模块名}_{功能名}_test.hpp/cpp
    if filename.endswith('_test.hpp'):
        return re.match(r'^[a-z]+(_[a-z]+)*_test\.hpp$', filename) is not None
    
    if filename.endswith('_test.cpp'):
        return re.match(r'^[a-z]+(_[a-z]+)*_test\.cpp$', filename) is not None
    
    # 其他文件检查是否包含大写字母或特殊字符
    if re.search(r'[A-Z]', filename) or re.search(r'[^a-z0-9_.]', filename):
        return False
    
    return True

def main():
    """主函数"""
    import sys
    
    # 获取检查目录
    directory = sys.argv[1] if len(sys.argv) > 1 else "."
    
    print(f"检查目录: {directory}")
    print("-" * 40)
    
    # 执行检查
    issues = check_test_files(directory)
    
    # 输出结果
    if issues:
        print(f"\n❌ 发现 {len(issues)} 个命名规范问题:")
        for issue in issues:
            print(f"  - {issue}")
        print("\n💡 命名规范:")
        print("  • 主测试文件: test_{模块名}_{子模块名}.hpp")
        print("  • 子模块测试文件: {模块名}_{功能名}_test.hpp/cpp")
        print("  • 只使用小写字母、数字和下划线")
        return 1
    else:
        print("✅ 所有文件都符合命名规范")
        return 0

if __name__ == "__main__":
    try:
        exit_code = main()
        exit(exit_code)
    except KeyboardInterrupt:
        print("\n检查被用户中断")
        exit(1)
    except Exception as e:
        print(f"错误: {e}")
        exit(1)