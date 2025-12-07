@echo off
REM =====================================================================
REM Build C++ bytecode dumper (bytecode_main.exe)
REM =====================================================================

setlocal

REM Change to script directory (lua/tools/) then go to lua/
cd /d "%~dp0"
cd ..

set BUILD_TYPE=Debug
if /I "%1"=="release" set BUILD_TYPE=Release

echo [INFO] Building C++ bytecode dumper (%BUILD_TYPE%)

set VCVARSALL=D:\VS2026\2026\VC\Auxiliary\Build\vcvarsall.bat
if not exist "%VCVARSALL%" (
    echo [ERROR] Cannot find vcvarsall.bat: %VCVARSALL%
    endlocal
    exit /b 1
)

call "%VCVARSALL%" x64 >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Failed to set up MSVC environment
    endlocal
    exit /b %errorlevel%
)

if "%BUILD_TYPE%"=="Debug" (
    set OUTPUT_DIR=build\debug_bytecode
    set CXX_FLAGS=/std:c++17 /EHsc /nologo /Od /Zi /MDd /DDEBUG /D_DEBUG /W3 /D_CRT_SECURE_NO_WARNINGS /utf-8
) else (
    set OUTPUT_DIR=build\release_bytecode
    set CXX_FLAGS=/std:c++17 /EHsc /nologo /O2 /MD /DNDEBUG /W3 /D_CRT_SECURE_NO_WARNINGS /utf-8
)

if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

REM Create temp directory for .obj files
set OBJ_DIR=tmp
if not exist "%OBJ_DIR%" mkdir "%OBJ_DIR%"

echo [INFO] Compiling and linking bytecode_main.exe ...

cl %CXX_FLAGS% /Isrc ^
  /Fo"%OBJ_DIR%\\" ^
  src\core\value.cpp ^
  src\core\gc_object.cpp ^
  src\core\gc_string.cpp ^
  src\core\string_pool.cpp ^
  src\core\table.cpp ^
  src\core\function.cpp ^
  src\core\userdata.cpp ^
  src\core\upvalue.cpp ^
  src\core\metatable.cpp ^
  src\gc\garbage_collector.cpp ^
  src\vm\global_state.cpp ^
  src\vm\stack.cpp ^
  src\vm\lua_state.cpp ^
  src\vm\vm.cpp ^
  src\lib\lib_registry.cpp ^
  src\lib\lib_manager.cpp ^
  src\lib\baselib.cpp ^
  src\compiler\lexer.cpp ^
  src\compiler\ast.cpp ^
  src\compiler\parser.cpp ^
  src\compiler\opcode.cpp ^
  src\compiler\codegen.cpp ^
  src\compiler\bytecode_printer.cpp ^
  src\bytecode_main.cpp ^
  /Fe"%OUTPUT_DIR%\bytecode_main.exe"

if errorlevel 1 (
    echo [ERROR] Build failed
    endlocal
    exit /b %errorlevel%
)

echo [OK] Built %OUTPUT_DIR%\bytecode_main.exe

echo.
endlocal
exit /b 0

