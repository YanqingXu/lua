@echo off
echo ========================================
echo Simple Error Format Test
echo ========================================

REM Set up MSVC environment
set MSVC_PATH="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build"
if exist %MSVC_PATH%\vcvars64.bat (
    call %MSVC_PATH%\vcvars64.bat
) else (
    echo Warning: MSVC 2022 not found, trying 2019...
    set MSVC_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build"
    if exist %MSVC_PATH%\vcvars64.bat (
        call %MSVC_PATH%\vcvars64.bat
    ) else (
        echo Error: Could not find MSVC environment
        echo Please ensure Visual Studio is installed
        pause
        exit /b 1
    )
)

echo.
echo Building Simple Error Format Test...
echo ====================================

REM Create build directory
if not exist "build" mkdir build

REM Compile the simple test
cl /EHsc /std:c++17 tests\simple_error_format_test.cpp /Fe:build\simple_error_test.exe

if %ERRORLEVEL% neq 0 (
    echo.
    echo ❌ Build failed!
    echo Check the compilation errors above.
    pause
    exit /b 1
)

echo.
echo ✅ Build successful!
echo.
echo Running Simple Error Format Test...
echo ===================================

REM Run the test
build\simple_error_test.exe

echo.
echo ===================================
echo Test execution completed.
echo ===================================

pause
