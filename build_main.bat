@echo off
REM =====================================================================
REM Lua C++ Interpreter Build Script for main.cpp
REM =====================================================================

setlocal

REM Parse build type
set BUILD_TYPE=Debug
if "%1"=="release" set BUILD_TYPE=Release
if "%1"=="Release" set BUILD_TYPE=Release
if "%1"=="RELEASE" set BUILD_TYPE=Release

echo.
echo [INFO] ========================================
echo [INFO] Lua C++ Interpreter Build Script (main.cpp)
echo [INFO] ========================================
echo [INFO] Build type: %BUILD_TYPE%
echo.

REM Set up MSVC environment using vcvarsall.bat
set VCVARSALL=D:\VS2026\2026\VC\Auxiliary\Build\vcvarsall.bat

if not exist "%VCVARSALL%" (
    echo [ERROR] Cannot find vcvarsall.bat: %VCVARSALL%
    echo [ERROR] Please check Visual Studio installation path
    exit /b 1
)

echo [INFO] Setting up MSVC environment...
call "%VCVARSALL%" x64 >nul 2>&1

if %errorlevel% neq 0 (
    echo [ERROR] Failed to set up MSVC environment
    exit /b %errorlevel%
)

echo [INFO] MSVC environment set up successfully
echo.

REM Set output directory and compile flags
if "%BUILD_TYPE%"=="Debug" (
    set OUTPUT_DIR=build\debug_main
    set CXX_FLAGS=/std:c++17 /EHsc /nologo /Od /Zi /MDd /DDEBUG /D_DEBUG /W3 /D_CRT_SECURE_NO_WARNINGS /utf-8
) else (
    set OUTPUT_DIR=build\release_main
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

echo [INFO] ========================================
echo [INFO] Starting compilation...
echo [INFO] ========================================
echo.

REM Compile all source files
echo [INFO] Compiling Value class...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\value.obj" "src\core\value.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling GCObject class...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\gc_object.obj" "src\core\gc_object.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling GCString class...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\gc_string.obj" "src\core\gc_string.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling StringPool class...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\string_pool.obj" "src\core\string_pool.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling Table class...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\table.obj" "src\core\table.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling Function class...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\function.obj" "src\core\function.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling GarbageCollector class...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\garbage_collector.obj" "src\gc\garbage_collector.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling GlobalState class...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\global_state.obj" "src\vm\global_state.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling Stack class...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\stack.obj" "src\vm\stack.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling LuaState class...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\lua_state.obj" "src\vm\lua_state.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling main.cpp...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\main.obj" "src\main.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo.
echo [INFO] Linking executable...
cl %CXX_FLAGS% /Fe"%OUTPUT_DIR%\lua_test.exe" ^
    "%OUTPUT_DIR%\main.obj" ^
    "%OUTPUT_DIR%\value.obj" ^
    "%OUTPUT_DIR%\gc_object.obj" ^
    "%OUTPUT_DIR%\gc_string.obj" ^
    "%OUTPUT_DIR%\string_pool.obj" ^
    "%OUTPUT_DIR%\table.obj" ^
    "%OUTPUT_DIR%\function.obj" ^
    "%OUTPUT_DIR%\garbage_collector.obj" ^
    "%OUTPUT_DIR%\global_state.obj" ^
    "%OUTPUT_DIR%\stack.obj" ^
    "%OUTPUT_DIR%\lua_state.obj"

if %errorlevel% neq 0 (
    echo [ERROR] Linking failed!
    exit /b %errorlevel%
)

echo.
echo [INFO] ========================================
echo [INFO] Compilation successful!
echo [INFO] ========================================
echo.
echo [INFO] Executable: %OUTPUT_DIR%\lua_test.exe
echo.

echo [INFO] ========================================
echo [INFO] Running test program...
echo [INFO] ========================================
echo.

"%OUTPUT_DIR%\lua_test.exe"

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Test program failed!
    exit /b %errorlevel%
)

echo.
echo [INFO] ========================================
echo [INFO] Build complete!
echo [INFO] ========================================
echo.

endlocal
exit /b 0

