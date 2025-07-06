@echo off
REM Lua Interpreter Project Code Standards Check Script (English Version - ASCII)
REM Usage: check_standards_en.bat [file_path]

setlocal enabledelayedexpansion

echo [CHECK] Lua Interpreter Project Code Standards Check
echo ========================================

set check_passed=0
set check_failed=0

REM Check if file exists
if "%~1"=="" (
    echo Usage: %0 ^<file_path^>
    echo Example: %0 src\lib\base_lib.cpp
    exit /b 1
)

if not exist "%~1" (
    echo [ERROR] File not found: %~1
    exit /b 1
)

set target_file=%~1

echo Checking file: %target_file%
echo ----------------------------------------

REM Check type system usage
echo.
echo [INFO] Checking type system compliance...

findstr /C:"std::string" /C:"int " /C:"double " /C:"float " "%target_file%" >nul 2>&1
if !errorlevel! equ 0 (
    echo [FAIL] Found prohibited type usage:
    findstr /N /C:"std::string" /C:"int " /C:"double " /C:"float " "%target_file%"
    set /a check_failed+=1
) else (
    echo [PASS] Type system usage check passed
    set /a check_passed+=1
)

REM Check if types.hpp is included
findstr /C:"types.hpp" "%target_file%" >nul 2>&1
if !errorlevel! neq 0 (
    echo [FAIL] Missing types.hpp include
    set /a check_failed+=1
) else (
    echo [PASS] types.hpp is included
)

REM Check comment language (simplified - Windows cmd limitations)
echo.
echo [INFO] Checking comment language compliance...
echo [WARN] Windows version cannot detect Chinese characters automatically
echo [WARN] Please manually verify all comments are in English

REM Check modern C++ features
echo.
echo [INFO] Checking modern C++ features usage...

findstr /C:" new " /C:" delete " "%target_file%" >nul 2>&1
if !errorlevel! equ 0 (
    echo [FAIL] Found raw pointer memory management:
    findstr /N /C:" new " /C:" delete " "%target_file%"
    set /a check_failed+=1
) else (
    echo [PASS] No raw pointer memory management found
    set /a check_passed+=1
)

findstr /C:"std::unique_ptr" /C:"std::shared_ptr" "%target_file%" >nul 2>&1
if !errorlevel! equ 0 (
    echo [PASS] Found smart pointer usage
    set /a check_passed+=1
)

findstr /C:"auto " "%target_file%" >nul 2>&1
if !errorlevel! equ 0 (
    echo [PASS] Found auto type deduction usage
)

REM Check documentation comments
echo.
echo [INFO] Checking documentation completeness...

findstr /C:"/**" "%target_file%" >nul 2>&1
if !errorlevel! equ 0 (
    echo [PASS] Found documentation comments
    set /a check_passed+=1
) else (
    echo [WARN] No documentation comments found
    echo [WARN] Please add comments for public interfaces
)

REM Display check results
echo.
echo ========================================
echo [SUMMARY] Check Results Summary
echo ========================================
echo Passed items: !check_passed!
echo Failed items: !check_failed!

if !check_failed! equ 0 (
    echo [SUCCESS] All basic checks passed!
    echo.
    echo Complete standards document: DEVELOPMENT_STANDARDS.md
    echo.
    echo Recommended tools for complete checking:
    echo   - clang-tidy: Static code analysis
    echo   - clang-format: Code formatting
    echo   - cppcheck: Additional static checks
    exit /b 0
) else (
    echo [FAILED] Found !check_failed! standards violations!
    echo.
    echo Please fix the issues and recheck.
    echo.
    echo Standards document: DEVELOPMENT_STANDARDS.md
    echo.
    echo Fix suggestions:
    echo   1. Use types defined in types.hpp
    echo   2. Change all comments to English
    echo   3. Use smart pointers instead of raw pointers
    echo   4. Add documentation comments for public interfaces
    echo.
    echo Type mapping examples:
    echo   std::string  -^>  Str
    echo   int         -^>  i32
    echo   double      -^>  f64
    echo   float       -^>  f32
    exit /b 1
)

endlocal
