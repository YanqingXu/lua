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

echo [INFO] Compiling Upvalue class...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\upvalue.obj" "src\core\upvalue.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling Userdata class...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\userdata.obj" "src\core\userdata.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling Metatable class...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\metatable.obj" "src\core\metatable.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling LuaState class...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\lua_state.obj" "src\vm\lua_state.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling Lexer class...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\lexer.obj" "src\compiler\lexer.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling AST class...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\ast.obj" "src\compiler\ast.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling Parser class...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\parser.obj" "src\compiler\parser.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling OpCode class...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\opcode.obj" "src\compiler\opcode.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling CodeGenerator class...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\codegen.obj" "src\compiler\codegen.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling VM class...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\vm.obj" "src\vm\vm.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling BaseLib...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\baselib.obj" "src\lib\baselib.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo.
echo [INFO] ========================================
echo [INFO] Compiling Test Files...
echo [INFO] ========================================
echo.

echo [INFO] Compiling test_value...
cl %CXX_FLAGS% /Isrc /Itests\unit /c /Fo"%OUTPUT_DIR%\test_value.obj" "tests\unit\test_value.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling test_gc_string...
cl %CXX_FLAGS% /Isrc /Itests\unit /c /Fo"%OUTPUT_DIR%\test_gc_string.obj" "tests\unit\test_gc_string.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling test_table...
cl %CXX_FLAGS% /Isrc /Itests\unit /c /Fo"%OUTPUT_DIR%\test_table.obj" "tests\unit\test_table.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling test_vm_core...
cl %CXX_FLAGS% /Isrc /Itests\unit /c /Fo"%OUTPUT_DIR%\test_vm_core.obj" "tests\unit\test_vm_core.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling test_function...
cl %CXX_FLAGS% /Isrc /Itests\unit /c /Fo"%OUTPUT_DIR%\test_function.obj" "tests\unit\test_function.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling test_gc...
cl %CXX_FLAGS% /Isrc /Itests\unit /c /Fo"%OUTPUT_DIR%\test_gc.obj" "tests\unit\test_gc.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling test_binary_unary_expr...
cl %CXX_FLAGS% /Isrc /Itests\unit /c /Fo"%OUTPUT_DIR%\test_binary_unary_expr.obj" "tests\unit\test_binary_unary_expr.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling test_function_codegen...
cl %CXX_FLAGS% /Isrc /Itests\unit /c /Fo"%OUTPUT_DIR%\test_function_codegen.obj" "tests\unit\test_function_codegen.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling test_baselib...
cl %CXX_FLAGS% /Isrc /Itests\unit /c /Fo"%OUTPUT_DIR%\test_baselib.obj" "tests\unit\test_baselib.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling test_lua_functions...
cl %CXX_FLAGS% /Isrc /Itests\unit /c /Fo"%OUTPUT_DIR%\test_lua_functions.obj" "tests\unit\test_lua_functions.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling test_metamethod_arith...
cl %CXX_FLAGS% /Isrc /Itests\unit /c /Fo"%OUTPUT_DIR%\test_metamethod_arith.obj" "tests\unit\test_metamethod_arith.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo.
echo [INFO] Compiling main.cpp...
cl %CXX_FLAGS% /Isrc /Itests\unit /c /Fo"%OUTPUT_DIR%\main.obj" "src\main.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo.
echo [INFO] Linking executable...
cl %CXX_FLAGS% /Fe"%OUTPUT_DIR%\main.exe" ^
    "%OUTPUT_DIR%\main.obj" ^
    "%OUTPUT_DIR%\test_value.obj" ^
    "%OUTPUT_DIR%\test_gc_string.obj" ^
    "%OUTPUT_DIR%\test_table.obj" ^
    "%OUTPUT_DIR%\test_vm_core.obj" ^
    "%OUTPUT_DIR%\test_function.obj" ^
    "%OUTPUT_DIR%\test_gc.obj" ^
    "%OUTPUT_DIR%\test_binary_unary_expr.obj" ^
    "%OUTPUT_DIR%\test_function_codegen.obj" ^
    "%OUTPUT_DIR%\test_baselib.obj" ^
    "%OUTPUT_DIR%\test_lua_functions.obj" ^
    "%OUTPUT_DIR%\test_metamethod_arith.obj" ^
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
    "%OUTPUT_DIR%\baselib.obj"

if %errorlevel% neq 0 (
    echo [ERROR] Linking failed!
    exit /b %errorlevel%
)

echo.
echo [INFO] ========================================
echo [INFO] Compilation successful!
echo [INFO] ========================================
echo.
echo [INFO] Executable: %OUTPUT_DIR%\main.exe
echo.

echo [INFO] ========================================
echo [INFO] Running test program...
echo [INFO] ========================================
echo.

"%OUTPUT_DIR%\main.exe"

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

