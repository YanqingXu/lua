@echo off
chcp 65001 >nul 2>&1
setlocal enabledelayedexpansion

REM Test file naming convention check script
REM Usage: Double-click to run or execute in command line

echo ========================================
echo    Lua Project Test File Naming Check
echo ========================================
echo.

REM Check if Python is installed
python --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Python not found, please install Python 3.6+
    echo.
    echo Download: https://www.python.org/downloads/
    pause
    exit /b 1
)

REM Check if script files exist
if not exist "check_naming.py" (
    if not exist "check_naming_simple.py" (
        echo [ERROR] Check script file not found
        echo Please ensure check_naming.py or check_naming_simple.py is in current directory
        pause
        exit /b 1
    ) else (
        set SCRIPT=check_naming_simple.py
    )
) else (
    set SCRIPT=check_naming.py
)

echo [INFO] Using script: !SCRIPT!
echo.

REM Check if test directory exists
if not exist "." (
    echo [ERROR] Current directory does not exist
    echo Please ensure you are in the tests directory
    pause
    exit /b 1
)

REM Run check script
echo Starting check...
echo.
python "!SCRIPT!" .

set EXIT_CODE=!errorlevel!

echo.
echo ========================================
if !EXIT_CODE! equ 0 (
    echo [SUCCESS] Check completed - All files follow naming convention
) else (
    echo [FAILED] Check completed - Found naming issues
)
echo ========================================
echo.
echo Press any key to exit...
pause >nul

exit /b !EXIT_CODE!