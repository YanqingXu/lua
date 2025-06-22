#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
check_naming.py - 测试文件命名规范检查工具

此脚本用于检查 Lua 项目中测试文件的命名是否符合规范。

命名规范:
1. 主测试文件: test_{模块名}_{子模块名}.hpp
2. 子模块测试文件: {模块名}_{功能名}_test.hpp/.cpp
3. 所有文件名使用小写字母和下划线

使用方法:
    python check_naming.py [测试目录路径]
    
示例:
    python check_naming.py src/tests
    python check_naming.py  # 默认检查 src/tests
"""

import os
import re
import sys
import argparse
from pathlib import Path
from typing import List, Tuple, Dict

class NamingChecker:
    """测试文件命名规范检查器"""
    
    def __init__(self):
        # 主测试文件命名规范 (test_模块名_子模块名.hpp)
        # 修改：强制要求必须包含模块名和子模块名，不允许只有一个下划线
        self.main_test_pattern = re.compile(r'^test_[a-z]+_[a-z]+(_[a-z]+)*\.hpp$')
        
        # 子模块测试文件命名规范 (模块名_功能名_test.hpp/cpp)
        self.sub_test_hpp_pattern = re.compile(r'^[a-z]+(_[a-z]+)*_test\.hpp$')
        self.sub_test_cpp_pattern = re.compile(r'^[a-z]+(_[a-z]+)*_test\.cpp$')
        
        # 统计信息
        self.stats = {
            'total_files': 0,
            'main_test_files': 0,
            'sub_test_files': 0,
            'other_files': 0,
            'issues': 0
        }
    
    def check_test_files(self, directory: str) -> List[Dict[str, str]]:
        """检查测试文件命名规范
        
        Args:
            directory: 要检查的目录路径
            
        Returns:
            包含问题文件信息的列表
        """
        if not os.path.exists(directory):
            print(f"❌ 错误: 目录 '{directory}' 不存在")
            return []
        
        issues = []
        
        print(f"🔍 正在检查目录: {directory}")
        print("-" * 50)
        
        for root, dirs, files in os.walk(directory):
            # 跳过隐藏目录、构建目录和formatting目录（测试框架组件）
            dirs[:] = [d for d in dirs if not d.startswith('.') and d not in ['build', 'Debug', 'Release', 'formatting']]
            
            for file in files:
                if file.endswith(('.hpp', '.cpp')):
                    self.stats['total_files'] += 1
                    file_path = os.path.join(root, file)
                    relative_path = os.path.relpath(file_path, directory)
                    
                    issue = self.check_naming_convention(file, root, relative_path)
                    if issue:
                        issues.append(issue)
                        self.stats['issues'] += 1
        
        return issues
    
    def check_naming_convention(self, filename: str, path: str, relative_path: str) -> Dict[str, str]:
        """检查单个文件的命名规范
        
        Args:
            filename: 文件名
            path: 文件所在目录的绝对路径
            relative_path: 相对于检查根目录的路径
            
        Returns:
            如果有问题返回问题信息字典，否则返回None
        """
        # 主测试文件检查
        if filename.startswith('test_'):
            self.stats['main_test_files'] += 1
            if filename.endswith('.hpp'):
                if self.main_test_pattern.match(filename):
                    print(f"✅ 主测试文件: {relative_path}")
                    return None
                else:
                    return {
                        'file': relative_path,
                        'type': '主测试文件',
                        'issue': '命名格式不正确',
                        'expected': 'test_{模块名}_{子模块名}.hpp',
                        'actual': filename
                    }
            else:
                return {
                    'file': relative_path,
                    'type': '主测试文件',
                    'issue': '文件扩展名错误',
                    'expected': '.hpp',
                    'actual': os.path.splitext(filename)[1]
                }
        
        # 子模块测试文件检查
        elif filename.endswith('_test.hpp') or filename.endswith('_test.cpp'):
            self.stats['sub_test_files'] += 1
            
            if filename.endswith('.hpp'):
                pattern = self.sub_test_hpp_pattern
            else:
                pattern = self.sub_test_cpp_pattern
            
            if pattern.match(filename):
                print(f"✅ 子模块测试文件: {relative_path}")
                return None
            else:
                return {
                    'file': relative_path,
                    'type': '子模块测试文件',
                    'issue': '命名格式不正确',
                    'expected': '{模块名}_{功能名}_test.hpp/cpp',
                    'actual': filename
                }
        
        # 其他测试相关文件
        else:
            self.stats['other_files'] += 1
            # 检查是否包含大写字母或特殊字符
            if re.search(r'[A-Z]', filename) or re.search(r'[^a-z0-9_.]', filename):
                return {
                    'file': relative_path,
                    'type': '其他文件',
                    'issue': '文件名包含大写字母或特殊字符',
                    'expected': '只使用小写字母、数字和下划线',
                    'actual': filename
                }
            
            print(f"ℹ️  其他文件: {relative_path}")
        
        return None
    
    def print_issues(self, issues: List[Dict[str, str]]):
        """打印问题报告"""
        if not issues:
            print("\n🎉 恭喜! 所有文件都符合命名规范")
            return
        
        print(f"\n❌ 发现 {len(issues)} 个命名规范问题:")
        print("=" * 60)
        
        # 按问题类型分组
        issues_by_type = {}
        for issue in issues:
            issue_type = issue['type']
            if issue_type not in issues_by_type:
                issues_by_type[issue_type] = []
            issues_by_type[issue_type].append(issue)
        
        for issue_type, type_issues in issues_by_type.items():
            print(f"\n📁 {issue_type} ({len(type_issues)} 个问题):")
            for issue in type_issues:
                print(f"  ❌ {issue['file']}")
                print(f"     问题: {issue['issue']}")
                print(f"     期望: {issue['expected']}")
                print(f"     实际: {issue['actual']}")
                print()
    
    def print_statistics(self):
        """打印统计信息"""
        print("\n📊 统计信息:")
        print("-" * 30)
        print(f"总文件数: {self.stats['total_files']}")
        print(f"主测试文件: {self.stats['main_test_files']}")
        print(f"子模块测试文件: {self.stats['sub_test_files']}")
        print(f"其他文件: {self.stats['other_files']}")
        print(f"问题文件: {self.stats['issues']}")
        
        if self.stats['total_files'] > 0:
            compliance_rate = ((self.stats['total_files'] - self.stats['issues']) / self.stats['total_files']) * 100
            print(f"规范符合率: {compliance_rate:.1f}%")
    
    def suggest_fixes(self, issues: List[Dict[str, str]]):
        """提供修复建议"""
        if not issues:
            return
        
        print("\n💡 修复建议:")
        print("-" * 30)
        
        for issue in issues:
            filename = os.path.basename(issue['file'])
            
            if issue['type'] == '主测试文件':
                if 'test_' in filename and not filename.endswith('.hpp'):
                    suggested = filename.replace('.cpp', '.hpp')
                    print(f"• {filename} → {suggested}")
                elif not re.match(r'^test_[a-z_]+$', filename.replace('.hpp', '')):
                    print(f"• {filename} → 使用格式: test_模块名_子模块名.hpp")
            
            elif issue['type'] == '子模块测试文件':
                if not filename.endswith('_test.hpp') and not filename.endswith('_test.cpp'):
                    base_name = os.path.splitext(filename)[0]
                    ext = '.hpp' if 'hpp' in issue['file'] else '.cpp'
                    suggested = f"{base_name}_test{ext}"
                    print(f"• {filename} → {suggested}")
            
            elif issue['type'] == '其他文件':
                suggested = re.sub(r'[A-Z]', lambda m: m.group().lower(), filename)
                suggested = re.sub(r'[^a-z0-9_.]', '_', suggested)
                if suggested != filename:
                    print(f"• {filename} → {suggested}")

def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        description='检查测试文件命名规范',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  %(prog)s                    # 检查默认目录 src/tests
  %(prog)s src/tests          # 检查指定目录
  %(prog)s --verbose src/tests # 详细输出模式
        """
    )
    
    parser.add_argument(
        'directory',
        nargs='?',
        default='.',
        help='要检查的测试目录路径 (默认: 当前目录)'
    )
    
    parser.add_argument(
        '-v', '--verbose',
        action='store_true',
        help='显示详细输出'
    )
    
    parser.add_argument(
        '--stats-only',
        action='store_true',
        help='只显示统计信息'
    )
    
    parser.add_argument(
        '--suggest-fixes',
        action='store_true',
        help='显示修复建议'
    )
    
    args = parser.parse_args()
    
    # 创建检查器实例
    checker = NamingChecker()
    
    # 执行检查
    issues = checker.check_test_files(args.directory)
    
    # 输出结果
    if not args.stats_only:
        checker.print_issues(issues)
    
    if args.suggest_fixes and issues:
        checker.suggest_fixes(issues)
    
    checker.print_statistics()
    
    # 返回适当的退出码
    return 1 if issues else 0

if __name__ == "__main__":
    try:
        exit_code = main()
        sys.exit(exit_code)
    except KeyboardInterrupt:
        print("\n\n⚠️  检查被用户中断")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ 发生错误: {e}")
        sys.exit(1)