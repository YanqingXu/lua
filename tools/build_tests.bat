@echo off
REM =====================================================================
REM Lua C++ Interpreter - Unit Test Build Script
REM =====================================================================
REM
REM This script builds and runs the unit test suite.
REM It uses the same MSVC environment as the main build script.
REM
REM Usage:
REM   build_tests.bat [debug|release]
REM
REM Examples:
REM   build_tests.bat          - Build and run tests (Debug)
REM   build_tests.bat debug    - Build and run tests (Debug)
REM   build_tests.bat release  - Build and run tests (Release)
REM
REM =====================================================================

setlocal

REM Change to script directory (lua/tools/) then go to lua/
cd /d "%~dp0"
cd ..

REM Parse build type
set BUILD_TYPE=Debug
if "%1"=="release" set BUILD_TYPE=Release
if "%1"=="Release" set BUILD_TYPE=Release
if "%1"=="RELEASE" set BUILD_TYPE=Release

echo.
echo [INFO] ========================================
echo [INFO] Lua C++ Unit Test Build Script
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
    set OUTPUT_DIR=build\test_debug
    set CXX_FLAGS=/std:c++17 /EHsc /nologo /Od /Zi /MDd /DDEBUG /D_DEBUG /W3 /D_CRT_SECURE_NO_WARNINGS /utf-8
) else (
    set OUTPUT_DIR=build\test_release
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
echo [INFO] Compiling Core Classes...
echo [INFO] ========================================
echo.

REM Compile core classes (reuse from main build if possible)
set CORE_OBJS=
for %%F in (value gc_object gc_string string_pool table function userdata upvalue metatable) do (
    echo [INFO] Compiling %%F...
    cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\%%F.obj" "src\core\%%F.cpp" >nul 2>&1
    if %errorlevel% neq 0 (
        echo [ERROR] Failed to compile %%F
        exit /b %errorlevel%
    )
    set CORE_OBJS=!CORE_OBJS! "%OUTPUT_DIR%\%%F.obj"
)

echo [INFO] Compiling GC...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\garbage_collector.obj" "src\gc\garbage_collector.cpp" >nul 2>&1

echo [INFO] Compiling VM classes...
for %%F in (global_state stack lua_state) do (
    cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\%%F.obj" "src\vm\%%F.cpp" >nul 2>&1
)

echo [INFO] Compiling Compiler classes...
for %%F in (lexer ast parser opcode codegen) do (
    cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\%%F.obj" "src\compiler\%%F.cpp" >nul 2>&1
)

echo [INFO] Compiling VM...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\vm.obj" "src\vm\vm.cpp" >nul 2>&1

echo [INFO] Compiling Libraries...
for %%F in (lib_registry lib_manager baselib) do (
    cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\%%F.obj" "src\lib\%%F.cpp" >nul 2>&1
)

echo.
echo [INFO] ========================================
echo [INFO] Compiling Test Files...
echo [INFO] ========================================
echo.

REM Compile test framework
echo [INFO] Compiling test_framework...
cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_framework.obj" "tests\unit\framework\test_framework.cpp" >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Failed to compile test_framework
    exit /b %errorlevel%
)

REM Compile core tests
for %%F in (test_value test_gc_string test_table test_function) do (
    echo [INFO] Compiling %%F...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\%%F.obj" "tests\unit\core\%%F.cpp" >nul 2>&1
    if %errorlevel% neq 0 (
        echo [ERROR] Failed to compile %%F
        exit /b %errorlevel%
    )
)

REM Compile gc tests
echo [INFO] Compiling test_gc...
cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_gc.obj" "tests\unit\gc\test_gc.cpp" >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Failed to compile test_gc
    exit /b %errorlevel%
)



REM Compile vm tests
echo [INFO] Compiling test_vm_core...
cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_vm_core.obj" "tests\unit\vm\test_vm_core.cpp" >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Failed to compile test_vm_core
    exit /b %errorlevel%
)

REM Compile compiler tests
for %%F in (test_binary_unary_expr test_function_codegen test_syntax_sugar test_lua_functions test_indexed_access test_method_call test_storevar test_parser_recursion test_parser_error_recovery test_lexer_number test_lexer_lookahead) do (
    echo [INFO] Compiling %%F...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\%%F.obj" "tests\unit\compiler\%%F.cpp"
    if %errorlevel% neq 0 (
        echo [ERROR] Failed to compile %%F
        exit /b %errorlevel%
    )
)

REM Compile stdlib tests
echo [INFO] Compiling test_baselib...
cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_baselib.obj" "tests\unit\stdlib\test_baselib.cpp" >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Failed to compile test_baselib
    exit /b %errorlevel%
)

REM Compile metamethod tests
echo [INFO] Compiling test_metamethod_arith...
cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_metamethod_arith.obj" "tests\unit\metamethod\test_metamethod_arith.cpp" >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Failed to compile test_metamethod_arith
    exit /b %errorlevel%
)

echo [INFO] Compiling test_metamethod_complete...
cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_metamethod_complete.obj" "tests\unit\metamethod\test_metamethod_complete.cpp" >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Failed to compile test_metamethod_complete
    exit /b %errorlevel%
)

echo [INFO] Compiling test_runner...
cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_runner.obj" "tests\unit\framework\test_runner.cpp" >nul 2>&1

echo.
echo [INFO] ========================================
echo [INFO] Linking Test Executable...
echo [INFO] ========================================
echo.

REM Link all object files
echo [INFO] Linking test_runner.exe...
cl %CXX_FLAGS% /Fe"%OUTPUT_DIR%\test_runner.exe" ^
    "%OUTPUT_DIR%\test_runner.obj" ^
    "%OUTPUT_DIR%\test_framework.obj" ^
    "%OUTPUT_DIR%\test_value.obj" ^
    "%OUTPUT_DIR%\test_gc_string.obj" ^
    "%OUTPUT_DIR%\test_table.obj" ^
    "%OUTPUT_DIR%\test_vm_core.obj" ^
    "%OUTPUT_DIR%\test_function.obj" ^
    "%OUTPUT_DIR%\test_gc.obj" ^
    "%OUTPUT_DIR%\test_binary_unary_expr.obj" ^
    "%OUTPUT_DIR%\test_function_codegen.obj" ^
    "%OUTPUT_DIR%\test_syntax_sugar.obj" ^
    "%OUTPUT_DIR%\test_indexed_access.obj" ^
    "%OUTPUT_DIR%\test_method_call.obj" ^
    "%OUTPUT_DIR%\test_storevar.obj" ^
    "%OUTPUT_DIR%\test_parser_recursion.obj" ^
    "%OUTPUT_DIR%\test_parser_error_recovery.obj" ^
    "%OUTPUT_DIR%\test_lexer_number.obj" ^
    "%OUTPUT_DIR%\test_lexer_lookahead.obj" ^
    "%OUTPUT_DIR%\test_baselib.obj" ^
    "%OUTPUT_DIR%\test_lua_functions.obj" ^
    "%OUTPUT_DIR%\test_metamethod_arith.obj" ^
    "%OUTPUT_DIR%\test_metamethod_complete.obj" ^
    "%OUTPUT_DIR%\value.obj" ^
    "%OUTPUT_DIR%\gc_object.obj" ^
    "%OUTPUT_DIR%\gc_string.obj" ^
    "%OUTPUT_DIR%\string_pool.obj" ^
    "%OUTPUT_DIR%\table.obj" ^
    "%OUTPUT_DIR%\function.obj" ^
    "%OUTPUT_DIR%\userdata.obj" ^
    "%OUTPUT_DIR%\upvalue.obj" ^
    "%OUTPUT_DIR%\metatable.obj" ^
    "%OUTPUT_DIR%\garbage_collector.obj" ^
    "%OUTPUT_DIR%\global_state.obj" ^
    "%OUTPUT_DIR%\stack.obj" ^
    "%OUTPUT_DIR%\lua_state.obj" ^
    "%OUTPUT_DIR%\lexer.obj" ^
    "%OUTPUT_DIR%\ast.obj" ^
    "%OUTPUT_DIR%\parser.obj" ^
    "%OUTPUT_DIR%\opcode.obj" ^
    "%OUTPUT_DIR%\codegen.obj" ^
    "%OUTPUT_DIR%\vm.obj" ^
    "%OUTPUT_DIR%\lib_registry.obj" ^
    "%OUTPUT_DIR%\lib_manager.obj" ^
    "%OUTPUT_DIR%\baselib.obj"

if %errorlevel% neq 0 (
    echo [ERROR] Linking failed!
    exit /b %errorlevel%
)

echo [INFO] Build successful!
echo.

echo [INFO] ========================================
echo [INFO] Running Tests...
echo [INFO] ========================================
echo.

"%OUTPUT_DIR%\test_runner.exe"

set TEST_RESULT=%errorlevel%

echo.
echo [INFO] ========================================
echo [INFO] Test Execution Complete
echo [INFO] ========================================
echo.

if %TEST_RESULT% neq 0 (
    echo [ERROR] Some tests failed!
    echo [ERROR] Exit code: %TEST_RESULT%
    exit /b %TEST_RESULT%
)

echo [SUCCESS] All tests passed!
echo.

endlocal
exit /b 0