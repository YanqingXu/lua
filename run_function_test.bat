@echo off
setlocal

echo ========================================
echo Building and Running Function Test
echo ========================================

REM Setup MSVC environment
call "D:\VS2026\2026\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

REM Compile test
cl /std:c++17 /EHsc /nologo /Od /Zi /MDd /DDEBUG /D_DEBUG /W3 /D_CRT_SECURE_NO_WARNINGS /utf-8 /Isrc /Fo"build\debug\" /Fe"build\debug\test_function_codegen.exe" "tests\unit\test_function_codegen.cpp" build\debug\value.obj build\debug\gc_object.obj build\debug\gc_string.obj build\debug\string_pool.obj build\debug\table.obj build\debug\function.obj build\debug\userdata.obj build\debug\garbage_collector.obj build\debug\upvalue.obj build\debug\global_state.obj build\debug\stack.obj build\debug\lua_state.obj build\debug\lexer.obj build\debug\ast.obj build\debug\parser.obj build\debug\opcode.obj build\debug\codegen.obj build\debug\vm.obj

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Compilation failed!
    exit /b 1
)

echo.
echo ========================================
echo Running Test
echo ========================================
build\debug\test_function_codegen.exe

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Test failed!
    exit /b 1
)

echo.
echo [SUCCESS] Test completed successfully!

