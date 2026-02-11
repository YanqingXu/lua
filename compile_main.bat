@echo off
REM Compile main.cpp using Visual Studio 2026 MSVC compiler
REM This script sets up the MSVC environment and compiles the file

echo ========================================
echo Compiling main.cpp with MSVC
echo ========================================
echo.

REM Step 1: Setup MSVC environment
echo [1/3] Setting up MSVC environment...
call "D:\VS2026\2026\VC\Auxiliary\Build\vcvarsall.bat" x64
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Failed to setup MSVC environment
    exit /b 1
)
echo [OK] MSVC environment ready
echo.

REM Step 2: Compile main.cpp
echo [2/3] Compiling src\main.cpp...
cl /std:c++17 /W3 /WX /EHsc /nologo /c /Isrc src\main.cpp /Fo:src\main.obj
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Compilation failed
    exit /b 1
)
echo [OK] Compilation successful
echo.

REM Step 3: Verify output
echo [3/3] Verifying output...
if exist src\main.obj (
    echo [OK] main.obj created successfully
    dir src\main.obj | findstr "main.obj"
    echo.
    echo ========================================
    echo ✅ main.cpp compiled successfully
    echo ========================================
) else (
    echo [ERROR] main.obj not found
    exit /b 1
)

exit /b 0

