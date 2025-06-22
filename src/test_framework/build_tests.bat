@echo off
REM Lua测试框架构建脚本
REM 用于编译和运行测试

setlocal enabledelayedexpansion

REM 设置颜色
set "GREEN=[32m"
set "RED=[31m"
set "YELLOW=[33m"
set "BLUE=[34m"
set "RESET=[0m"

echo %BLUE%========================================%RESET%
echo %BLUE%    Lua Test Framework Build Script    %RESET%
echo %BLUE%========================================%RESET%
echo.

REM 检查参数
if "%1"=="" (
    echo %YELLOW%Usage: build_tests.bat [command]%RESET%
    echo.
    echo Available commands:
    echo   build     - Build all tests
    echo   run       - Build and run all tests
    echo   clean     - Clean build artifacts
    echo   example   - Build and run example tests
    echo   help      - Show this help message
    echo.
    goto :end
)

REM 设置路径
set "PROJECT_ROOT=%~dp0..\..\.."
set "TEST_FRAMEWORK_DIR=%~dp0"
set "TESTS_DIR=%PROJECT_ROOT%\src\tests"
set "BUILD_DIR=%PROJECT_ROOT%\build\tests"
set "SRC_DIR=%PROJECT_ROOT%\src"

REM 创建构建目录
if not exist "%BUILD_DIR%" (
    echo %BLUE%Creating build directory...%RESET%
    mkdir "%BUILD_DIR%"
)

REM 处理命令
if "%1"=="help" goto :help
if "%1"=="clean" goto :clean
if "%1"=="build" goto :build
if "%1"=="run" goto :run
if "%1"=="example" goto :example

echo %RED%Unknown command: %1%RESET%
goto :help

:help
echo %YELLOW%Lua Test Framework Build Script%RESET%
echo.
echo Available commands:
echo   %GREEN%build%RESET%     - Build all tests
echo   %GREEN%run%RESET%       - Build and run all tests
echo   %GREEN%clean%RESET%     - Clean build artifacts
echo   %GREEN%example%RESET%   - Build and run example tests
echo   %GREEN%help%RESET%      - Show this help message
echo.
echo Examples:
echo   build_tests.bat build
echo   build_tests.bat run
echo   build_tests.bat example
goto :end

:clean
echo %YELLOW%Cleaning build artifacts...%RESET%
if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%"
    echo %GREEN%Build directory cleaned.%RESET%
) else (
    echo %YELLOW%Build directory does not exist.%RESET%
)
goto :end

:build
echo %BLUE%Building tests...%RESET%
echo.

REM 检查编译器
where g++ >nul 2>&1
if errorlevel 1 (
    echo %RED%Error: g++ compiler not found in PATH%RESET%
    echo Please install MinGW-w64 or add g++ to your PATH
    goto :end
)

REM 编译参数
set "COMPILE_FLAGS=-std=c++17 -Wall -Wextra -I\"%SRC_DIR%\" -O2"
set "DEBUG_FLAGS=-g -DDEBUG"
set "RELEASE_FLAGS=-DNDEBUG"

REM 选择构建模式
if "%2"=="debug" (
    set "BUILD_FLAGS=%DEBUG_FLAGS%"
    echo %YELLOW%Building in DEBUG mode...%RESET%
) else (
    set "BUILD_FLAGS=%RELEASE_FLAGS%"
    echo %YELLOW%Building in RELEASE mode...%RESET%
)

REM 编译主测试文件
echo %BLUE%Compiling main test suite...%RESET%
g++ %COMPILE_FLAGS% %BUILD_FLAGS% -o "%BUILD_DIR%\test_main.exe" "%TESTS_DIR%\test_main.cpp" 2>&1
if errorlevel 1 (
    echo %RED%Error: Failed to compile main test suite%RESET%
    goto :end
)

echo %GREEN%Build completed successfully!%RESET%
echo Executable: %BUILD_DIR%\test_main.exe
goto :end

:run
echo %BLUE%Building and running tests...%RESET%
echo.

call :build
if errorlevel 1 goto :end

echo.
echo %BLUE%Running tests...%RESET%
echo %BLUE%========================================%RESET%

cd /d "%BUILD_DIR%"
test_main.exe
set "TEST_RESULT=%errorlevel%"

echo.
echo %BLUE%========================================%RESET%
if %TEST_RESULT%==0 (
    echo %GREEN%All tests completed successfully!%RESET%
) else (
    echo %RED%Some tests failed (exit code: %TEST_RESULT%)%RESET%
)
goto :end

:example
echo %BLUE%Building and running example tests...%RESET%
echo.

REM 创建示例测试文件
set "EXAMPLE_FILE=%BUILD_DIR%\example_test.cpp"

echo #include "test_framework/test_framework.hpp" > "%EXAMPLE_FILE%"
echo #include "test_framework/examples/example_test.hpp" >> "%EXAMPLE_FILE%"
echo. >> "%EXAMPLE_FILE%"
echo int main() { >> "%EXAMPLE_FILE%"
echo     RUN_ALL_TESTS(Lua::TestFramework::Examples::ExampleTestSuite); >> "%EXAMPLE_FILE%"
echo     return 0; >> "%EXAMPLE_FILE%"
echo } >> "%EXAMPLE_FILE%"

REM 编译示例
echo %BLUE%Compiling example tests...%RESET%
g++ %COMPILE_FLAGS% %RELEASE_FLAGS% -o "%BUILD_DIR%\example_test.exe" "%EXAMPLE_FILE%" 2>&1
if errorlevel 1 (
    echo %RED%Error: Failed to compile example tests%RESET%
    goto :end
)

echo %GREEN%Example build completed!%RESET%
echo.
echo %BLUE%Running example tests...%RESET%
echo %BLUE%========================================%RESET%

cd /d "%BUILD_DIR%"
example_test.exe
set "TEST_RESULT=%errorlevel%"

echo.
echo %BLUE%========================================%RESET%
if %TEST_RESULT%==0 (
    echo %GREEN%Example tests completed successfully!%RESET%
) else (
    echo %RED%Example tests failed (exit code: %TEST_RESULT%)%RESET%
)
goto :end

:end
echo.
echo %BLUE%Script completed.%RESET%
pause