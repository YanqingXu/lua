@echo off
REM Minimal Lua Interpreter Build Script for Windows
REM Builds only the core components that are known to work

echo Building minimal Lua interpreter...

REM Create bin directory if it doesn't exist
if not exist "bin" mkdir "bin"

REM Source files that are known to compile (only existing ones)
set SOURCES=src\main_simple.cpp

REM Check and add existing source files
if exist "src\vm\state.cpp" set SOURCES=%SOURCES% src\vm\state.cpp
if exist "src\vm\value.cpp" set SOURCES=%SOURCES% src\vm\value.cpp
if exist "src\vm\table.cpp" set SOURCES=%SOURCES% src\vm\table.cpp
if exist "src\vm\function.cpp" set SOURCES=%SOURCES% src\vm\function.cpp
if exist "src\lexer\lexer.cpp" set SOURCES=%SOURCES% src\lexer\lexer.cpp
if exist "src\parser\parser.cpp" set SOURCES=%SOURCES% src\parser\parser.cpp
if exist "src\compiler\symbol_table.cpp" set SOURCES=%SOURCES% src\compiler\symbol_table.cpp
if exist "src\lib\base\base_lib.cpp" set SOURCES=%SOURCES% src\lib\base\base_lib.cpp
if exist "src\lib\base\lib_base_utils.cpp" set SOURCES=%SOURCES% src\lib\base\lib_base_utils.cpp
if exist "src\lib\core\lib_func_registry.cpp" set SOURCES=%SOURCES% src\lib\core\lib_func_registry.cpp
if exist "src\lib\core\lib_context.cpp" set SOURCES=%SOURCES% src\lib\core\lib_context.cpp
if exist "src\lib\core\lib_manager.cpp" set SOURCES=%SOURCES% src\lib\core\lib_manager.cpp
if exist "src\gc\gc.cpp" set SOURCES=%SOURCES% src\gc\gc.cpp
if exist "src\common\defines.cpp" set SOURCES=%SOURCES% src\common\defines.cpp

REM Compiler flags
set CXX_FLAGS=-std=c++17 -Wall -Wextra -O2 -I src/

REM Output binary
set OUTPUT=bin\lua_minimal.exe

echo Compiling sources: %SOURCES%
g++ %CXX_FLAGS% -o %OUTPUT% %SOURCES%

if %ERRORLEVEL% equ 0 (
    echo Build successful! Output: %OUTPUT%
    echo Usage: %OUTPUT% script.lua
) else (
    echo Build failed!
    exit /b 1
)
