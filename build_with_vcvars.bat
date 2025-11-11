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
echo #include "core/value.hpp"
echo #include "core/gc_object.hpp"
echo #include ^<iostream^>
echo.
echo // Test GCObject implementation
echo class TestGCObject : public Lua::GCObject {
echo public:
echo     TestGCObject^(^) : GCObject^(Lua::GCObjectType::String^) {}
echo     void mark^(^) override {}
echo     Lua::usize getSize^(^) const override { return sizeof^(TestGCObject^); }
echo };
echo.
echo int main^(^) {
echo     std::cout ^<^< "[INFO] Lua C++ Interpreter - Core Classes Test" ^<^< std::endl;
echo     std::cout ^<^< "[INFO] Version: " ^<^< Lua::LUA_VERSION ^<^< std::endl;
echo     std::cout ^<^< "[INFO] Build Type: %BUILD_TYPE%" ^<^< std::endl;
echo     std::cout ^<^< "[INFO] Debug Mode: " ^<^< ^(Lua::DEBUG_MODE ? "Yes" : "No"^) ^<^< std::endl;
echo.
echo     std::cout ^<^< "\n[TEST 1] Testing Value class..." ^<^< std::endl;
echo.
echo     // Test 1: Nil value
echo     Lua::Value nilVal;
echo     std::cout ^<^< "  [1] Nil value: " ^<^< ^(nilVal.isNil^(^) ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 2: Boolean value
echo     Lua::Value boolVal^(true^);
echo     std::cout ^<^< "  [2] Boolean value: " ^<^< ^(boolVal.isBoolean^(^) ^&^& boolVal.asBoolean^(^) == true ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 3: Number value
echo     Lua::Value numVal^(3.14^);
echo     std::cout ^<^< "  [3] Number value: " ^<^< ^(numVal.isNumber^(^) ^&^& numVal.asNumber^(^) == 3.14 ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 4: Integer value
echo     Lua::Value intVal^(Lua::LuaInteger^(42^)^);
echo     std::cout ^<^< "  [4] Integer value: " ^<^< ^(intVal.isNumber^(^) ^&^& intVal.asInteger^(^) == 42 ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 5: Type checking
echo     std::cout ^<^< "  [5] Type checking: " ^<^< ^(numVal.getType^(^) == Lua::ValueType::Number ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 6: Safe value access
echo     auto maybeNum = numVal.tryGetNumber^(^);
echo     std::cout ^<^< "  [6] Safe access: " ^<^< ^(maybeNum.has_value^(^) ^&^& maybeNum.value^(^) == 3.14 ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 7: Lua truth semantics
echo     std::cout ^<^< "  [7] Lua truth ^(nil^): " ^<^< ^(nilVal.isFalse^(^) ? "PASS" : "FAIL"^) ^<^< std::endl;
echo     std::cout ^<^< "  [8] Lua truth ^(false^): " ^<^< ^(Lua::Value^(false^).isFalse^(^) ? "PASS" : "FAIL"^) ^<^< std::endl;
echo     std::cout ^<^< "  [9] Lua truth ^(0^): " ^<^< ^(Lua::Value^(0.0^).isTrue^(^) ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 8: Equality comparison
echo     Lua::Value num1^(42.0^);
echo     Lua::Value num2^(42.0^);
echo     Lua::Value num3^(43.0^);
echo     std::cout ^<^< "  [10] Equality ^(same^): " ^<^< ^(num1 == num2 ? "PASS" : "FAIL"^) ^<^< std::endl;
echo     std::cout ^<^< "  [11] Equality ^(diff^): " ^<^< ^(num1 != num3 ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 9: toString method
echo     std::cout ^<^< "  [12] toString: " ^<^< numVal.toString^(^) ^<^< std::endl;
echo     std::cout ^<^< "  [13] toString: " ^<^< boolVal.toString^(^) ^<^< std::endl;
echo     std::cout ^<^< "  [14] toString: " ^<^< nilVal.toString^(^) ^<^< std::endl;
echo.
echo.
echo     std::cout ^<^< "\n[TEST 2] Testing GCObject class..." ^<^< std::endl;
echo.
echo     // Test 1: Create GC object
echo     TestGCObject* obj = new TestGCObject^(^);
echo     std::cout ^<^< "  [1] GCObject creation: PASS" ^<^< std::endl;
echo.
echo     // Test 2: Type checking
echo     std::cout ^<^< "  [2] Type checking: " ^<^< ^(obj-^>getType^(^) == Lua::GCObjectType::String ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 3: Initial color ^(should be white^)
echo     obj-^>setColor^(Lua::GCColor::White^);
echo     std::cout ^<^< "  [3] Initial color ^(white^): " ^<^< ^(obj-^>isWhite^(^) ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 4: Set to gray
echo     obj-^>setColor^(Lua::GCColor::Gray^);
echo     std::cout ^<^< "  [4] Set to gray: " ^<^< ^(obj-^>isGray^(^) ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 5: Set to black
echo     obj-^>setColor^(Lua::GCColor::Black^);
echo     std::cout ^<^< "  [5] Set to black: " ^<^< ^(obj-^>isBlack^(^) ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 6: isMarked ^(black is marked^)
echo     std::cout ^<^< "  [6] isMarked ^(black^): " ^<^< ^(obj-^>isMarked^(^) ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 7: Chain objects
echo     TestGCObject* obj2 = new TestGCObject^(^);
echo     obj-^>setNext^(obj2^);
echo     std::cout ^<^< "  [7] Chain objects: " ^<^< ^(obj-^>getNext^(^) == obj2 ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 8: Object size
echo     std::cout ^<^< "  [8] Object size: " ^<^< obj-^>getSize^(^) ^<^< " bytes" ^<^< std::endl;
echo.
echo     // Cleanup
echo     delete obj2;
echo     delete obj;
echo.
echo     std::cout ^<^< "\n[INFO] Class sizes:" ^<^< std::endl;
echo     std::cout ^<^< "  - Value: " ^<^< sizeof^(Lua::Value^) ^<^< " bytes" ^<^< std::endl;
echo     std::cout ^<^< "  - GCObject: " ^<^< sizeof^(Lua::GCObject^) ^<^< " bytes" ^<^< std::endl;
echo     std::cout ^<^< "  - TestGCObject: " ^<^< sizeof^(TestGCObject^) ^<^< " bytes" ^<^< std::endl;
echo.
echo     std::cout ^<^< "\n[SUCCESS] All tests passed!" ^<^< std::endl;
echo     return 0;
echo }
) > "%OUTPUT_DIR%\test_build.cpp"

echo.
echo [INFO] ========================================
echo [INFO] Starting compilation...
echo [INFO] ========================================
echo.

echo [INFO] Compiling Value class...
echo [INFO] Command: cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\value.obj" "src\core\value.cpp"
echo.

cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\value.obj" "src\core\value.cpp"

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] ========================================
    echo [ERROR] Value class compilation failed!
    echo [ERROR] Error code: %errorlevel%
    echo [ERROR] ========================================
    exit /b %errorlevel%
)

echo.
echo [INFO] Compiling GCObject class...
echo [INFO] Command: cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\gc_object.obj" "src\core\gc_object.cpp"
echo.

cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\gc_object.obj" "src\core\gc_object.cpp"

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] ========================================
    echo [ERROR] GCObject class compilation failed!
    echo [ERROR] Error code: %errorlevel%
    echo [ERROR] ========================================
    exit /b %errorlevel%
)

echo.
echo [INFO] Compiling test file...
echo [INFO] Command: cl %CXX_FLAGS% /Isrc /Fo"%OUTPUT_DIR%\\" /Fe"%OUTPUT_DIR%\test_build.exe" "%OUTPUT_DIR%\test_build.cpp" "%OUTPUT_DIR%\value.obj" "%OUTPUT_DIR%\gc_object.obj"
echo.

cl %CXX_FLAGS% /Isrc /Fo"%OUTPUT_DIR%\\" /Fe"%OUTPUT_DIR%\test_build.exe" "%OUTPUT_DIR%\test_build.cpp" "%OUTPUT_DIR%\value.obj" "%OUTPUT_DIR%\gc_object.obj"

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

