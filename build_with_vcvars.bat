@echo off
REM =====================================================================
REM Lua C++ Interpreter Build Script (using vcvarsall.bat)
REM =====================================================================
REM
REM Usage:
REM   build_with_vcvars.bat [debug|release]
REM
REM Examples:
REM   build_with_vcvars.bat          - Build Debug version
REM   build_with_vcvars.bat debug    - Build Debug version
REM   build_with_vcvars.bat release  - Build Release version
REM
REM =====================================================================

setlocal

REM Parse build type
set BUILD_TYPE=Debug
if "%1"=="release" set BUILD_TYPE=Release
if "%1"=="Release" set BUILD_TYPE=Release
if "%1"=="RELEASE" set BUILD_TYPE=Release

echo.
echo [INFO] ========================================
echo [INFO] Lua C++ Interpreter Build Script
echo [INFO] ========================================
echo [INFO] Build type: %BUILD_TYPE%
echo.

REM Set up MSVC environment using vcvarsall.bat
set VCVARSALL=D:\VS2026\Insiders\VC\Auxiliary\Build\vcvarsall.bat

if not exist "%VCVARSALL%" (
    echo [ERROR] Cannot find vcvarsall.bat: %VCVARSALL%
    echo [ERROR] Please check Visual Studio installation path
    exit /b 1
)

echo [INFO] Setting up MSVC environment...
echo [INFO] Calling: "%VCVARSALL%" x64
echo.

call "%VCVARSALL%" x64

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Failed to set up MSVC environment
    echo [ERROR] Error code: %errorlevel%
    exit /b %errorlevel%
)

echo.
echo [INFO] MSVC environment set up successfully
echo.

REM Set output directory and compile flags
if "%BUILD_TYPE%"=="Debug" (
    set OUTPUT_DIR=build\debug
    set CXX_FLAGS=/std:c++17 /EHsc /nologo /Od /Zi /MDd /DDEBUG /D_DEBUG /W3 /D_CRT_SECURE_NO_WARNINGS /utf-8
) else (
    set OUTPUT_DIR=build\release
    set CXX_FLAGS=/std:c++17 /EHsc /nologo /O2 /MD /DNDEBUG /W3 /D_CRT_SECURE_NO_WARNINGS /utf-8
)

echo [INFO] Compile flags: %CXX_FLAGS%
echo [INFO] Output directory: %OUTPUT_DIR%
echo.

REM Create output directory
if not exist "%OUTPUT_DIR%" (
    echo [INFO] Creating output directory: %OUTPUT_DIR%
    mkdir "%OUTPUT_DIR%"
)

echo [INFO] Creating test source file...

REM Create test source file
(
echo #include "common/types.hpp"
echo #include "common/config.hpp"
echo #include "common/macros.hpp"
echo #include ^<iostream^>
echo.
echo int main^(^) {
echo     std::cout ^<^< "[INFO] Lua C++ Interpreter - Build Test" ^<^< std::endl;
echo     std::cout ^<^< "[INFO] Version: " ^<^< Lua::LUA_VERSION ^<^< std::endl;
echo     std::cout ^<^< "[INFO] Build Type: %BUILD_TYPE%" ^<^< std::endl;
echo     std::cout ^<^< "[INFO] Debug Mode: " ^<^< ^(Lua::DEBUG_MODE ? "Yes" : "No"^) ^<^< std::endl;
echo     std::cout ^<^< "\n[INFO] Configuration:" ^<^< std::endl;
echo     std::cout ^<^< "  - C++ Standard: C++17" ^<^< std::endl;
echo     std::cout ^<^< "  - Platform: ";
echo     if ^(Lua::IS_WINDOWS^) std::cout ^<^< "Windows";
echo     else if ^(Lua::IS_LINUX^) std::cout ^<^< "Linux";
echo     else if ^(Lua::IS_MACOS^) std::cout ^<^< "macOS";
echo     else std::cout ^<^< "Unknown";
echo     std::cout ^<^< std::endl;
echo     std::cout ^<^< "  - Architecture: " ^<^< ^(Lua::IS_64BIT ? "64-bit" : "32-bit"^) ^<^< std::endl;
echo     std::cout ^<^< "\n[INFO] Type Sizes:" ^<^< std::endl;
echo     std::cout ^<^< "  - LuaInteger: " ^<^< sizeof^(Lua::LuaInteger^) ^<^< " bytes" ^<^< std::endl;
echo     std::cout ^<^< "  - LuaNumber: " ^<^< sizeof^(Lua::LuaNumber^) ^<^< " bytes" ^<^< std::endl;
echo     std::cout ^<^< "  - ValueType: " ^<^< sizeof^(Lua::ValueType^) ^<^< " bytes" ^<^< std::endl;
echo     std::cout ^<^< "\n[SUCCESS] Build test passed!" ^<^< std::endl;
echo     return 0;
echo }
) > "%OUTPUT_DIR%\test_build.cpp"

echo.
echo [INFO] ========================================
echo [INFO] Starting compilation...
echo [INFO] ========================================
echo.

echo [INFO] Compiling test file...
echo [INFO] Command: cl %CXX_FLAGS% /Isrc /Isrc/common /Fo"%OUTPUT_DIR%\\" /Fe"%OUTPUT_DIR%\test_build.exe" "%OUTPUT_DIR%\test_build.cpp"
echo.

cl %CXX_FLAGS% /Isrc /Isrc/common /Fo"%OUTPUT_DIR%\\" /Fe"%OUTPUT_DIR%\test_build.exe" "%OUTPUT_DIR%\test_build.cpp"

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] ========================================
    echo [ERROR] Compilation failed!
    echo [ERROR] Error code: %errorlevel%
    echo [ERROR] ========================================
    exit /b %errorlevel%
)

echo.
echo [INFO] ========================================
echo [INFO] Compilation successful!
echo [INFO] ========================================
echo.
echo [INFO] Executable: %OUTPUT_DIR%\test_build.exe
echo.

echo [INFO] ========================================
echo [INFO] Running test program...
echo [INFO] ========================================
echo.

"%OUTPUT_DIR%\test_build.exe"

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Test program failed!
    echo [ERROR] Error code: %errorlevel%
    exit /b %errorlevel%
)

echo.
echo [INFO] ========================================
echo [INFO] Build complete!
echo [INFO] ========================================
echo.
echo [INFO] Build type: %BUILD_TYPE%
echo [INFO] Output directory: %OUTPUT_DIR%
echo [INFO] Executable: %OUTPUT_DIR%\test_build.exe
echo.

endlocal
exit /b 0

