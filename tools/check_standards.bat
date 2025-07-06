@echo off
REM Lua 解释器项目代码规范检查脚本 (Windows版本)
REM 使用方法: check_standards.bat [文件路径]

REM 设置控制台编码为UTF-8以支持中文显示
chcp 65001 >nul 2>&1

setlocal enabledelayedexpansion

echo [INFO] Lua 解释器项目代码规范检查
echo ========================================

set check_passed=0
set check_failed=0

REM 检查文件是否存在
if "%~1"=="" (
    echo 使用方法: %0 ^<文件路径^>
    echo 示例: %0 src\lib\base_lib.cpp
    exit /b 1
)

if not exist "%~1" (
    echo [ERROR] 文件不存在: %~1
    exit /b 1
)

set target_file=%~1

echo 检查文件: %target_file%
echo ----------------------------------------

REM 检查类型系统使用
echo.
echo [INFO] 检查类型系统使用规范...

findstr /C:"std::string" /C:"int " /C:"double " /C:"float " "%target_file%" >nul 2>&1
if !errorlevel! equ 0 (
    echo [ERROR] 发现禁用类型使用
    findstr /N /C:"std::string" /C:"int " /C:"double " /C:"float " "%target_file%"
    set /a check_failed+=1
) else (
    echo [PASS] 类型系统使用检查通过
    set /a check_passed+=1
)

REM 检查是否包含 types.hpp
findstr /C:"types.hpp" "%target_file%" >nul 2>&1
if !errorlevel! neq 0 (
    echo [ERROR] 缺少 types.hpp 包含
    set /a check_failed+=1
)

REM 检查注释语言 (简化版 - Windows cmd限制)
echo.
echo [INFO] 检查注释语言规范...
echo [WARN] Windows版本无法直接检测中文字符，请手动确认注释为英文

REM 检查现代C++特性
echo.
echo [INFO] 检查现代C++特性使用...

findstr /C:" new " /C:" delete " "%target_file%" >nul 2>&1
if !errorlevel! equ 0 (
    echo [ERROR] 发现裸指针内存管理
    findstr /N /C:" new " /C:" delete " "%target_file%"
    set /a check_failed+=1
) else (
    echo [PASS] 未发现裸指针内存管理
    set /a check_passed+=1
)

findstr /C:"std::unique_ptr" /C:"std::shared_ptr" "%target_file%" >nul 2>&1
if !errorlevel! equ 0 (
    echo [PASS] 发现使用智能指针
    set /a check_passed+=1
)

findstr /C:"auto " "%target_file%" >nul 2>&1
if !errorlevel! equ 0 (
    echo [PASS] 发现使用 auto 类型推导
)

REM 检查文档注释
echo.
echo [INFO] 检查文档注释完整性...

findstr /C:"/**" "%target_file%" >nul 2>&1
if !errorlevel! equ 0 (
    echo [PASS] 发现文档注释
    set /a check_passed+=1
) else (
    echo [WARN] 未发现文档注释，请为公共接口添加注释
)

REM 显示检查结果
echo.
echo ========================================
echo [SUMMARY] 检查结果汇总
echo ========================================
echo 通过项目: !check_passed!
echo 失败项目: !check_failed!

if !check_failed! equ 0 (
    echo [SUCCESS] 基础检查通过
    echo.
    echo [INFO] 完整规范文档: DEVELOPMENT_STANDARDS.md
    echo [INFO] 建议使用以下工具进行完整检查:
    echo   - clang-tidy: 静态代码分析
    echo   - clang-format: 代码格式化
    echo   - cppcheck: 额外的静态检查
    exit /b 0
) else (
    echo [FAILED] 存在 !check_failed! 项规范违规，请修复后重新检查
    echo.
    echo [INFO] 规范文档: DEVELOPMENT_STANDARDS.md
    echo [FIX] 修复建议:
    echo   1. 使用 types.hpp 中定义的类型
    echo   2. 所有注释改为英文
    echo   3. 使用智能指针替代裸指针
    echo   4. 为公共接口添加文档注释
    exit /b 1
)

endlocal
