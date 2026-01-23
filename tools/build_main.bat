@echo off
REM =====================================================================
REM Lua C++ Interpreter Build Script for main.cpp
REM =====================================================================
REM
REM This script builds main.cpp in two modes:
REM   1. Test Mode (default): Compiles with ENABLE_TESTS, includes test files
REM   2. Interpreter Mode: Compiles without tests, standalone interpreter
REM
REM Usage:
REM   build_main.bat [mode] [build_type]
REM
REM Modes:
REM   test       - Build with tests (default)
REM   interpreter - Build standalone interpreter
REM
REM Build Types:
REM   debug      - Debug build (default)
REM   release    - Release build
REM
REM Examples:
REM   build_main.bat                    - Test mode, Debug
REM   build_main.bat test debug         - Test mode, Debug
REM   build_main.bat interpreter        - Interpreter mode, Debug
REM   build_main.bat interpreter release - Interpreter mode, Release
REM   build_main.bat test release       - Test mode, Release
REM
REM =====================================================================

setlocal enabledelayedexpansion

REM Change to script directory (lua/tools/) then go to lua/
cd /d "%~dp0"
cd ..

REM Parse mode (test or interpreter)
set BUILD_MODE=test
if "%1"=="interpreter" set BUILD_MODE=interpreter
if "%1"=="Interpreter" set BUILD_MODE=interpreter
if "%1"=="INTERPRETER" set BUILD_MODE=interpreter

REM Parse build type (debug or release)
set BUILD_TYPE=Debug
if "%1"=="release" set BUILD_TYPE=Release
if "%1"=="Release" set BUILD_TYPE=Release
if "%1"=="RELEASE" set BUILD_TYPE=Release
if "%2"=="release" set BUILD_TYPE=Release
if "%2"=="Release" set BUILD_TYPE=Release
if "%2"=="RELEASE" set BUILD_TYPE=Release

echo.
echo [INFO] ========================================
echo [INFO] Lua C++ Interpreter Build Script
echo [INFO] ========================================
echo [INFO] Build mode: %BUILD_MODE%
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

REM Set output directory and compile flags based on mode and build type
if "%BUILD_MODE%"=="test" (
    if "%BUILD_TYPE%"=="Debug" (
        set OUTPUT_DIR=build\test_main_debug
        set CXX_FLAGS=/std:c++17 /EHsc /nologo /Od /Zi /MDd /DDEBUG /D_DEBUG /DENABLE_TESTS /W3 /D_CRT_SECURE_NO_WARNINGS /utf-8
        set EXE_NAME=main_test.exe
    ) else (
        set OUTPUT_DIR=build\test_main_release
        set CXX_FLAGS=/std:c++17 /EHsc /nologo /O2 /MD /DNDEBUG /DENABLE_TESTS /W3 /D_CRT_SECURE_NO_WARNINGS /utf-8
        set EXE_NAME=main_test.exe
    )
) else (
    if "%BUILD_TYPE%"=="Debug" (
        set OUTPUT_DIR=build\interpreter_debug
        set CXX_FLAGS=/std:c++17 /EHsc /nologo /Od /Zi /MDd /DDEBUG /D_DEBUG /W3 /D_CRT_SECURE_NO_WARNINGS /utf-8
        set EXE_NAME=lua.exe
    ) else (
        set OUTPUT_DIR=build\interpreter_release
        set CXX_FLAGS=/std:c++17 /EHsc /nologo /O2 /MD /DNDEBUG /W3 /D_CRT_SECURE_NO_WARNINGS /utf-8
        set EXE_NAME=lua.exe
    )
)

echo [INFO] Compile flags: %CXX_FLAGS%
echo [INFO] Output directory: %OUTPUT_DIR%
echo [INFO] Executable name: %EXE_NAME%
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

echo [INFO] Compiling BytecodePrinter class...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\bytecode_printer.obj" "src\compiler\bytecode_printer.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling VM class...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\vm.obj" "src\vm\vm.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling LibRegistry...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\lib_registry.obj" "src\lib\lib_registry.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling LibManager...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\lib_manager.obj" "src\lib\lib_manager.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling BaseLib...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\baselib.obj" "src\lib\baselib.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling MathLib...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\mathlib.obj" "src\lib\mathlib.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling IOLib...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\iolib.obj" "src\lib\iolib.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling StringLib...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\stringlib.obj" "src\lib\stringlib.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling TableLib...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\tablelib.obj" "src\lib\tablelib.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling DynamicBuffer class...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\dynamic_buffer.obj" "src\io\dynamic_buffer.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

echo [INFO] Compiling InputStream class...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\input_stream.obj" "src\io\input_stream.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

REM Compile test files only in test mode
if "%BUILD_MODE%"=="test" (
    echo.
    echo [INFO] ========================================
    echo [INFO] Compiling Test Files...
    echo [INFO] ========================================
    echo.

    echo [INFO] Compiling test_framework...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_framework.obj" "tests\unit\framework\test_framework.cpp" >nul 2>&1
    if %errorlevel% neq 0 (
        echo [ERROR] Failed to compile test_framework
        exit /b %errorlevel%
    )

    echo [INFO] Compiling test_value...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_value.obj" "tests\unit\core\test_value.cpp" >nul 2>&1
    if %errorlevel% neq 0 exit /b %errorlevel%

    echo [INFO] Compiling test_gc_string...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_gc_string.obj" "tests\unit\core\test_gc_string.cpp" >nul 2>&1
    if %errorlevel% neq 0 exit /b %errorlevel%

    echo [INFO] Compiling test_table...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_table.obj" "tests\unit\core\test_table.cpp" >nul 2>&1
    if %errorlevel% neq 0 exit /b %errorlevel%

    echo [INFO] Compiling test_vm_core...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_vm_core.obj" "tests\unit\vm\test_vm_core.cpp" >nul 2>&1
    if %errorlevel% neq 0 exit /b %errorlevel%

    echo [INFO] Compiling test_function...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_function.obj" "tests\unit\core\test_function.cpp" >nul 2>&1
    if %errorlevel% neq 0 exit /b %errorlevel%

    echo [INFO] Compiling test_gc...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_gc.obj" "tests\unit\gc\test_gc.cpp" >nul 2>&1
    if %errorlevel% neq 0 exit /b %errorlevel%

    echo [INFO] Compiling test_binary_unary_expr...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_binary_unary_expr.obj" "tests\unit\compiler\test_binary_unary_expr.cpp" >nul 2>&1
    if %errorlevel% neq 0 exit /b %errorlevel%

    echo [INFO] Compiling test_function_codegen...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_function_codegen.obj" "tests\unit\compiler\test_function_codegen.cpp" >nul 2>&1
    if %errorlevel% neq 0 exit /b %errorlevel%

    echo [INFO] Compiling test_syntax_sugar...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_syntax_sugar.obj" "tests\unit\compiler\test_syntax_sugar.cpp" >nul 2>&1
    if %errorlevel% neq 0 exit /b %errorlevel%

    echo [INFO] Compiling test_baselib...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_baselib.obj" "tests\unit\stdlib\test_baselib.cpp" >nul 2>&1
    if %errorlevel% neq 0 exit /b %errorlevel%

    echo [INFO] Compiling test_stringlib...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_stringlib.obj" "tests\unit\stdlib\test_stringlib.cpp" >nul 2>&1
    if %errorlevel% neq 0 exit /b %errorlevel%

    echo [INFO] Compiling test_tablelib...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_tablelib.obj" "tests\unit\stdlib\test_tablelib.cpp" >nul 2>&1
    if %errorlevel% neq 0 exit /b %errorlevel%

    echo [INFO] Compiling test_lua_functions...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_lua_functions.obj" "tests\unit\compiler\test_lua_functions.cpp" >nul 2>&1
    if %errorlevel% neq 0 exit /b %errorlevel%

    echo [INFO] Compiling test_metamethod_arith...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_metamethod_arith.obj" "tests\unit\metamethod\test_metamethod_arith.cpp" >nul 2>&1
    if %errorlevel% neq 0 exit /b %errorlevel%

    echo [INFO] Compiling test_metamethod_complete...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_metamethod_complete.obj" "tests\unit\metamethod\test_metamethod_complete.cpp" >nul 2>&1
    if %errorlevel% neq 0 exit /b %errorlevel%

    echo [INFO] Compiling test_function_call...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_function_call.obj" "tests\unit\vm\test_function_call.cpp" >nul 2>&1
    if %errorlevel% neq 0 exit /b %errorlevel%

    echo [INFO] Compiling test_lexer_number...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_lexer_number.obj" "tests\unit\compiler\test_lexer_number.cpp" >nul 2>&1
    if %errorlevel% neq 0 exit /b %errorlevel%

    echo [INFO] Compiling test_lexer_lookahead...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_lexer_lookahead.obj" "tests\unit\compiler\test_lexer_lookahead.cpp" >nul 2>&1
    if %errorlevel% neq 0 exit /b %errorlevel%

    echo [INFO] Compiling test_parser_recursion...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_parser_recursion.obj" "tests\unit\compiler\test_parser_recursion.cpp" >nul 2>&1
    if %errorlevel% neq 0 exit /b %errorlevel%

    echo [INFO] Compiling test_parser_error_recovery...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_parser_error_recovery.obj" "tests\unit\compiler\test_parser_error_recovery.cpp" >nul 2>&1
    if %errorlevel% neq 0 exit /b %errorlevel%

    echo [INFO] Compiling test_parser_memory_pool...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_parser_memory_pool.obj" "tests\unit\compiler\test_parser_memory_pool.cpp" >nul 2>&1
    if %errorlevel% neq 0 exit /b %errorlevel%

    echo [INFO] Compiling test_dynamic_buffer...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_dynamic_buffer.obj" "tests\unit\io\test_dynamic_buffer.cpp" >nul 2>&1
    if %errorlevel% neq 0 exit /b %errorlevel%

    echo [INFO] Compiling test_input_stream_string...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_input_stream_string.obj" "tests\unit\io\test_input_stream_string.cpp" >nul 2>&1
    if %errorlevel% neq 0 exit /b %errorlevel%

    echo [INFO] Compiling test_input_stream_stream...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_input_stream_stream.obj" "tests\unit\io\test_input_stream_stream.cpp"
    if %errorlevel% neq 0 (
        echo [ERROR] Compilation of test_input_stream_stream.cpp failed
        exit /b %errorlevel%
    )

    echo [INFO] Compiling test_input_stream_file...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\test_input_stream_file.obj" "tests\unit\io\test_input_stream_file.cpp"
    if %errorlevel% neq 0 (
        echo [ERROR] Compilation of test_input_stream_file.cpp failed
        exit /b %errorlevel%
    )
)

echo.
echo [INFO] ========================================
echo [INFO] Compiling main.cpp...
echo [INFO] ========================================
echo.

REM Compile repl.cpp for both modes
echo [INFO] Compiling repl.cpp...
cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\repl.obj" "src\repl.cpp"
if %errorlevel% neq 0 exit /b %errorlevel%

if "%BUILD_MODE%"=="test" (
    echo [INFO] Compiling main.cpp with test support...
    cl %CXX_FLAGS% /Isrc /Itests\unit\framework /c /Fo"%OUTPUT_DIR%\main.obj" "src\main.cpp"
) else (
    echo [INFO] Compiling main.cpp as standalone interpreter...
    cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\main.obj" "src\main.cpp"
)
if %errorlevel% neq 0 exit /b %errorlevel%

echo.
echo [INFO] ========================================
echo [INFO] Linking executable...
echo [INFO] ========================================
echo.

REM Build link command based on mode
if "%BUILD_MODE%"=="test" (
    echo [INFO] Linking %EXE_NAME% with test support...
    cl %CXX_FLAGS% /Fe"%OUTPUT_DIR%\%EXE_NAME%" ^
        "%OUTPUT_DIR%\main.obj" ^
        "%OUTPUT_DIR%\repl.obj" ^
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
        "%OUTPUT_DIR%\test_baselib.obj" ^
        "%OUTPUT_DIR%\test_stringlib.obj" ^
        "%OUTPUT_DIR%\test_tablelib.obj" ^
        "%OUTPUT_DIR%\test_lua_functions.obj" ^
        "%OUTPUT_DIR%\test_metamethod_arith.obj" ^
        "%OUTPUT_DIR%\test_metamethod_complete.obj" ^
        "%OUTPUT_DIR%\test_function_call.obj" ^
        "%OUTPUT_DIR%\test_lexer_number.obj" ^
        "%OUTPUT_DIR%\test_lexer_lookahead.obj" ^
        "%OUTPUT_DIR%\test_parser_recursion.obj" ^
        "%OUTPUT_DIR%\test_parser_error_recovery.obj" ^
        "%OUTPUT_DIR%\test_parser_memory_pool.obj" ^
        "%OUTPUT_DIR%\test_dynamic_buffer.obj" ^
        "%OUTPUT_DIR%\test_input_stream_string.obj" ^
        "%OUTPUT_DIR%\test_input_stream_stream.obj" ^
        "%OUTPUT_DIR%\test_input_stream_file.obj" ^
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
        "%OUTPUT_DIR%\baselib.obj" ^
        "%OUTPUT_DIR%\mathlib.obj" ^
        "%OUTPUT_DIR%\iolib.obj" ^
        "%OUTPUT_DIR%\stringlib.obj" ^
        "%OUTPUT_DIR%\tablelib.obj" ^
        "%OUTPUT_DIR%\dynamic_buffer.obj" ^
        "%OUTPUT_DIR%\input_stream.obj"
) else (
    echo [INFO] Linking %EXE_NAME% as standalone interpreter...
    cl %CXX_FLAGS% /Fe"%OUTPUT_DIR%\%EXE_NAME%" ^
        "%OUTPUT_DIR%\main.obj" ^
        "%OUTPUT_DIR%\repl.obj" ^
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
        "%OUTPUT_DIR%\bytecode_printer.obj" ^
        "%OUTPUT_DIR%\vm.obj" ^
        "%OUTPUT_DIR%\lib_registry.obj" ^
        "%OUTPUT_DIR%\lib_manager.obj" ^
        "%OUTPUT_DIR%\baselib.obj" ^
        "%OUTPUT_DIR%\mathlib.obj" ^
        "%OUTPUT_DIR%\iolib.obj" ^
        "%OUTPUT_DIR%\stringlib.obj" ^
        "%OUTPUT_DIR%\tablelib.obj" ^
        "%OUTPUT_DIR%\dynamic_buffer.obj" ^
        "%OUTPUT_DIR%\input_stream.obj"
)

if %errorlevel% neq 0 (
    echo [ERROR] Linking failed!
    exit /b %errorlevel%
)

echo.
echo [INFO] ========================================
echo [INFO] Build Successful!
echo [INFO] ========================================
echo.
echo [INFO] Executable: %OUTPUT_DIR%\%EXE_NAME%
echo.


REM Run the executable based on mode
if "%BUILD_MODE%"=="test" (
    echo [INFO] ========================================
    echo [INFO] Running Tests...
    echo [INFO] ========================================
    echo.

    "%OUTPUT_DIR%\%EXE_NAME%"

    if %errorlevel% neq 0 (
        echo.
        echo [ERROR] Tests failed!
        exit /b %errorlevel%
    )

    echo.
    echo [INFO] ========================================
    echo [INFO] All Tests Passed!
    echo [INFO] ========================================
    echo.
) else (
    echo [INFO] ========================================
    echo [INFO] Interpreter Build Complete
    echo [INFO] ========================================
    echo.
    echo [INFO] You can now run the interpreter:
    echo [INFO]   %OUTPUT_DIR%\%EXE_NAME% -v        (show version)
    echo [INFO]   %OUTPUT_DIR%\%EXE_NAME% -h        (show help)
    echo [INFO]   %OUTPUT_DIR%\%EXE_NAME% script.lua (run script)
    echo.
)

echo [INFO] ========================================
echo [INFO] Build Complete!
echo [INFO] ========================================
echo.

endlocal
exit /b 0