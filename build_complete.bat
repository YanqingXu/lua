@echo off
REM Complete Lua Interpreter Build Script for Windows
REM Attempts to build with all available core components

echo Building complete Lua interpreter...

REM Create bin directory if it doesn't exist
if not exist "bin" mkdir "bin"

REM Core source files - comprehensive list
set SOURCES=src\main.cpp

REM VM components (core execution engine)
set SOURCES=%SOURCES% src\vm\state.cpp
set SOURCES=%SOURCES% src\vm\value.cpp
set SOURCES=%SOURCES% src\vm\table.cpp
set SOURCES=%SOURCES% src\vm\function.cpp
set SOURCES=%SOURCES% src\vm\vm.cpp
set SOURCES=%SOURCES% src\vm\upvalue.cpp
set SOURCES=%SOURCES% src\vm\state_factory.cpp

REM Lexer
set SOURCES=%SOURCES% src\lexer\lexer.cpp

REM Parser components  
set SOURCES=%SOURCES% src\parser\expression_parser.cpp
set SOURCES=%SOURCES% src\parser\statement_parser.cpp
set SOURCES=%SOURCES% src\parser\parser_utils.cpp
set SOURCES=%SOURCES% src\parser\visitor.cpp
set SOURCES=%SOURCES% src\parser\ast\source_location.cpp

REM Compiler components
set SOURCES=%SOURCES% src\compiler\compiler.cpp
set SOURCES=%SOURCES% src\compiler\expression_compiler.cpp
set SOURCES=%SOURCES% src\compiler\statement_compiler.cpp
set SOURCES=%SOURCES% src\compiler\symbol_table.cpp
set SOURCES=%SOURCES% src\compiler\register_manager.cpp
set SOURCES=%SOURCES% src\compiler\compiler_utils.cpp
set SOURCES=%SOURCES% src\compiler\upvalue_analyzer.cpp

REM GC components
set SOURCES=%SOURCES% src\gc\memory\allocator.cpp
set SOURCES=%SOURCES% src\gc\core\garbage_collector.cpp
set SOURCES=%SOURCES% src\gc\core\string_pool.cpp
set SOURCES=%SOURCES% src\gc\core\gc_string.cpp
set SOURCES=%SOURCES% src\gc\core\gc_ref.cpp
set SOURCES=%SOURCES% src\gc\algorithms\gc_marker.cpp
set SOURCES=%SOURCES% src\gc\algorithms\gc_sweeper.cpp

REM BaseLib (the only fully working standard library)
set SOURCES=%SOURCES% src\lib\base\base_lib.cpp
set SOURCES=%SOURCES% src\lib\base\lib_base_utils.cpp

REM Lib core (framework)
set SOURCES=%SOURCES% src\lib\core\lib_func_registry.cpp
set SOURCES=%SOURCES% src\lib\core\lib_context.cpp
set SOURCES=%SOURCES% src\lib\core\lib_manager.cpp

REM Localization
set SOURCES=%SOURCES% src\localization\localization_manager.cpp

REM Common utilities
if exist "src\common\defines.cpp" set SOURCES=%SOURCES% src\common\defines.cpp

REM Compiler flags
set CXX_FLAGS=-std=c++17 -O2 -I src/

REM Output binary
set OUTPUT=bin\lua_complete.exe

echo.
echo Attempting to build with all core components...
echo.
echo Sources included:
echo %SOURCES%
echo.

g++ %CXX_FLAGS% -o %OUTPUT% %SOURCES%

if %ERRORLEVEL% equ 0 (
    echo.
    echo ========================================
    echo Build successful! Output: %OUTPUT%
    echo ========================================
    echo.
    echo Usage: %OUTPUT% script.lua
    echo.
    echo This Lua interpreter includes:
    echo - Complete VM components
    echo - Lexer and Parser
    echo - Compiler
    echo - Garbage Collector
    echo - BaseLib ^(core functions^)
    echo.
    echo Note: String/Math/Table libraries are not yet included
    echo due to ongoing refactoring.
    echo.
) else (
    echo.
    echo ========================================
    echo Build failed!
    echo ========================================
    echo.
    echo This is expected due to missing implementations.
    echo See compilation_recovery_plan_2025_07_06.md for details.
    echo.
    exit /b 1
)
