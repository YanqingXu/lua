@echo off
echo ========================================
echo Enhanced Error Reporting Test Suite
echo ========================================

REM Set up environment
set MSVC_PATH="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build"
if exist %MSVC_PATH%\vcvars64.bat (
    call %MSVC_PATH%\vcvars64.bat
) else (
    echo Warning: MSVC environment not found, trying alternative paths...
    set MSVC_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build"
    if exist %MSVC_PATH%\vcvars64.bat (
        call %MSVC_PATH%\vcvars64.bat
    ) else (
        echo Error: Could not find MSVC environment
        pause
        exit /b 1
    )
)

echo.
echo Building Enhanced Error Reporting Test...
echo ========================================

REM Create build directory
if not exist "build" mkdir build
cd build

REM Compile the test program
cl /EHsc /std:c++17 /I.. ^
   ..\tests\enhanced_error_reporting_validation.cpp ^
   ..\src\parser\error_formatter.cpp ^
   ..\src\parser\enhanced_error_reporter.cpp ^
   ..\src\localization\localization_manager.cpp ^
   ..\src\parser\enhanced_parser.cpp ^
   ..\src\parser\parser.cpp ^
   ..\src\lexer\lexer.cpp ^
   ..\src\vm\*.cpp ^
   ..\src\common\*.cpp ^
   /Fe:error_reporting_test.exe

if %ERRORLEVEL% neq 0 (
    echo.
    echo ❌ Build failed!
    cd ..
    pause
    exit /b 1
)

echo.
echo ✅ Build successful!
echo.
echo Running Enhanced Error Reporting Tests...
echo ========================================

REM Run the test
error_reporting_test.exe

echo.
echo ========================================
echo Test execution completed.
echo ========================================

cd ..
pause
