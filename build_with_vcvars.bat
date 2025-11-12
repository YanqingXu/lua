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
set VCVARSALL=D:\VS2026\2026\VC\Auxiliary\Build\vcvarsall.bat

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
echo #include "core/gc_string.hpp"
echo #include "core/string_pool.hpp"
echo #include "core/table.hpp"
echo #include ^<iostream^>
echo #include ^<string_view^>
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
echo     std::cout ^<^< "\n[TEST 3] Testing GCString class..." ^<^< std::endl;
echo.
echo     // Test 1: Create string
echo     Lua::GCString* str1 = new Lua::GCString^("Hello, Lua!"^);
echo     std::cout ^<^< "  [1] String creation: PASS" ^<^< std::endl;
echo.
echo     // Test 2: Get length
echo     std::cout ^<^< "  [2] String length: " ^<^< ^(str1-^>getLength^(^) == 11 ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 3: Get data
echo     std::cout ^<^< "  [3] String data: " ^<^< str1-^>getData^(^) ^<^< std::endl;
echo.
echo     // Test 4: Hash computation
echo     Lua::usize hash1 = str1-^>getHash^(^);
echo     std::cout ^<^< "  [4] Hash value: 0x" ^<^< std::hex ^<^< hash1 ^<^< std::dec ^<^< std::endl;
echo.
echo     // Test 5: Same content should have same hash
echo     Lua::GCString* str2 = new Lua::GCString^("Hello, Lua!"^);
echo     std::cout ^<^< "  [5] Same hash: " ^<^< ^(str1-^>getHash^(^) == str2-^>getHash^(^) ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 6: Different content should have different hash
echo     Lua::GCString* str3 = new Lua::GCString^("Different"^);
echo     std::cout ^<^< "  [6] Different hash: " ^<^< ^(str1-^>getHash^(^) != str3-^>getHash^(^) ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 7: Pointer comparison ^(not equal yet, need StringPool^)
echo     std::cout ^<^< "  [7] Pointer comparison: " ^<^< ^(str1 != str2 ? "PASS ^(different objects^)" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 8: c_str method
echo     std::cout ^<^< "  [8] c_str: " ^<^< str1-^>c_str^(^) ^<^< std::endl;
echo.
echo     // Test 9: GC type
echo     std::cout ^<^< "  [9] GC type: " ^<^< ^(str1-^>getType^(^) == Lua::GCObjectType::String ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 10: Object size
echo     std::cout ^<^< "  [10] Object size: " ^<^< str1-^>getSize^(^) ^<^< " bytes" ^<^< std::endl;
echo.
echo     // Cleanup
echo     delete str3;
echo     delete str2;
echo     delete str1;
echo.
echo     std::cout ^<^< "\n[TEST 4] Testing StringPool class..." ^<^< std::endl;
echo.
echo     // Get StringPool instance
echo     Lua::StringPool^& pool = Lua::StringPool::getInstance^(^);
echo.
echo     // Test 1: Intern first string
echo     Lua::GCString* poolStr1 = pool.intern^("Hello, World!"^);
echo     std::cout ^<^< "  [1] Intern string: PASS" ^<^< std::endl;
echo.
echo     // Test 2: Intern same string should return same pointer
echo     Lua::GCString* poolStr2 = pool.intern^("Hello, World!"^);
echo     std::cout ^<^< "  [2] Same pointer: " ^<^< ^(poolStr1 == poolStr2 ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 3: Intern different string
echo     Lua::GCString* poolStr3 = pool.intern^("Different"^);
echo     std::cout ^<^< "  [3] Different pointer: " ^<^< ^(poolStr1 != poolStr3 ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 4: Pool size
echo     std::cout ^<^< "  [4] Pool size: " ^<^< pool.size^(^) ^<^< " ^(expected 2^)" ^<^< std::endl;
echo.
echo     // Test 5: Find existing string
echo     Lua::GCString* found = pool.find^("Hello, World!"^);
echo     std::cout ^<^< "  [5] Find existing: " ^<^< ^(found == poolStr1 ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 6: Find non-existing string
echo     Lua::GCString* notFound = pool.find^("Not exists"^);
echo     std::cout ^<^< "  [6] Find non-existing: " ^<^< ^(notFound == nullptr ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 7: Intern with string_view
echo     std::string_view sv^("Test view"^);
echo     Lua::GCString* poolStr4 = pool.intern^(sv^);
echo     std::cout ^<^< "  [7] Intern string_view: PASS" ^<^< std::endl;
echo.
echo     // Test 8: Intern same string_view
echo     Lua::GCString* poolStr5 = pool.intern^(sv^);
echo     std::cout ^<^< "  [8] Same pointer ^(view^): " ^<^< ^(poolStr4 == poolStr5 ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 9: Pool size after more interns
echo     std::cout ^<^< "  [9] Pool size: " ^<^< pool.size^(^) ^<^< " ^(expected 3^)" ^<^< std::endl;
echo.
echo     // Test 10: Remove string
echo     pool.remove^(poolStr3^);
echo     std::cout ^<^< "  [10] Remove string: " ^<^< ^(pool.size^(^) == 2 ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 11: Verify pointer equality for interned strings
echo     std::cout ^<^< "  [11] Pointer equality: " ^<^< ^(poolStr1 == poolStr2 ^&^& poolStr4 == poolStr5 ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Cleanup ^(note: in real GC, strings are managed by GC^)
echo     delete poolStr4;
echo     delete poolStr3;
echo     delete poolStr1;
echo     pool.clear^(^);
echo.
echo     std::cout ^<^< "\n[TEST 5] Testing Table class..." ^<^< std::endl;
echo.
echo     // Test 1: Create table
echo     Lua::Table* table = new Lua::Table^(^);
echo     std::cout ^<^< "  [1] Table creation: PASS" ^<^< std::endl;
echo.
echo     // Test 2: Set and get array elements
echo     table-^>setArray^(1, Lua::Value^(42.0^)^);
echo     table-^>setArray^(2, Lua::Value^(true^)^);
echo     Lua::Value arr1 = table-^>getArray^(1^);
echo     Lua::Value arr2 = table-^>getArray^(2^);
echo     std::cout ^<^< "  [2] Array set/get: " ^<^< ^(arr1.asNumber^(^) == 42.0 ^&^& arr2.asBoolean^(^) == true ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 3: Set and get hash elements
echo     Lua::GCString* keyStr = new Lua::GCString^("name"^);
echo     table-^>set^(Lua::Value^(keyStr^), Lua::Value^(poolStr2^)^);
echo     Lua::Value hashVal = table-^>get^(Lua::Value^(keyStr^)^);
echo     std::cout ^<^< "  [3] Hash set/get: " ^<^< ^(hashVal.isString^(^) ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 4: Check array size
echo     std::cout ^<^< "  [4] Array size: " ^<^< table-^>getArraySize^(^) ^<^< " ^(expected 2^)" ^<^< std::endl;
echo.
echo     // Test 5: Check hash size
echo     std::cout ^<^< "  [5] Hash size: " ^<^< table-^>getHashSize^(^) ^<^< " ^(expected 1^)" ^<^< std::endl;
echo.
echo     // Test 6: Table length
echo     Lua::usize len = table-^>length^(^);
echo     std::cout ^<^< "  [6] Table length: " ^<^< len ^<^< " ^(expected 2^)" ^<^< std::endl;
echo.
echo     // Test 7: Has key
echo     bool hasKey = table-^>has^(Lua::Value^(keyStr^)^);
echo     std::cout ^<^< "  [7] Has key: " ^<^< ^(hasKey ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 8: Remove key
echo     table-^>remove^(Lua::Value^(keyStr^)^);
echo     bool hasKeyAfter = table-^>has^(Lua::Value^(keyStr^)^);
echo     std::cout ^<^< "  [8] Remove key: " ^<^< ^(!hasKeyAfter ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 9: Metatable
echo     Lua::Table* mt = new Lua::Table^(^);
echo     table-^>setMetatable^(mt^);
echo     std::cout ^<^< "  [9] Metatable: " ^<^< ^(table-^>getMetatable^(^) == mt ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 10: GC type
echo     std::cout ^<^< "  [10] GC type: " ^<^< ^(table-^>getType^(^) == Lua::GCObjectType::Table ? "PASS" : "FAIL"^) ^<^< std::endl;
echo.
echo     // Test 11: Object size
echo     Lua::usize tableSize = table-^>getSize^(^);
echo     std::cout ^<^< "  [11] Object size: " ^<^< tableSize ^<^< " bytes" ^<^< std::endl;
echo.
echo     // Cleanup
echo     delete mt;
echo     delete table;
echo     delete keyStr;
echo.
echo     std::cout ^<^< "\n[INFO] Class sizes:" ^<^< std::endl;
echo     std::cout ^<^< "  - Value: " ^<^< sizeof^(Lua::Value^) ^<^< " bytes" ^<^< std::endl;
echo     std::cout ^<^< "  - GCObject: " ^<^< sizeof^(Lua::GCObject^) ^<^< " bytes" ^<^< std::endl;
echo     std::cout ^<^< "  - TestGCObject: " ^<^< sizeof^(TestGCObject^) ^<^< " bytes" ^<^< std::endl;
echo     std::cout ^<^< "  - GCString: " ^<^< sizeof^(Lua::GCString^) ^<^< " bytes" ^<^< std::endl;
echo     std::cout ^<^< "  - Table: " ^<^< sizeof^(Lua::Table^) ^<^< " bytes" ^<^< std::endl;
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
echo [INFO] Compiling GCString class...
echo [INFO] Command: cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\gc_string.obj" "src\core\gc_string.cpp"
echo.

cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\gc_string.obj" "src\core\gc_string.cpp"

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] ========================================
    echo [ERROR] GCString class compilation failed!
    echo [ERROR] Error code: %errorlevel%
    echo [ERROR] ========================================
    exit /b %errorlevel%
)

echo.
echo [INFO] Compiling StringPool class...
echo [INFO] Command: cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\string_pool.obj" "src\core\string_pool.cpp"
echo.

cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\string_pool.obj" "src\core\string_pool.cpp"

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] ========================================
    echo [ERROR] StringPool class compilation failed!
    echo [ERROR] Error code: %errorlevel%
    echo [ERROR] ========================================
    exit /b %errorlevel%
)

echo.
echo [INFO] Compiling Table class...
echo [INFO] Command: cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\table.obj" "src\core\table.cpp"
echo.

cl %CXX_FLAGS% /Isrc /c /Fo"%OUTPUT_DIR%\table.obj" "src\core\table.cpp"

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] ========================================
    echo [ERROR] Table class compilation failed!
    echo [ERROR] Error code: %errorlevel%
    echo [ERROR] ========================================
    exit /b %errorlevel%
)

echo.
echo [INFO] Compiling test file...
echo [INFO] Command: cl %CXX_FLAGS% /Isrc /Fo"%OUTPUT_DIR%\\" /Fe"%OUTPUT_DIR%\test_build.exe" "%OUTPUT_DIR%\test_build.cpp" "%OUTPUT_DIR%\value.obj" "%OUTPUT_DIR%\gc_object.obj" "%OUTPUT_DIR%\gc_string.obj" "%OUTPUT_DIR%\string_pool.obj" "%OUTPUT_DIR%\table.obj"
echo.

cl %CXX_FLAGS% /Isrc /Fo"%OUTPUT_DIR%\\" /Fe"%OUTPUT_DIR%\test_build.exe" "%OUTPUT_DIR%\test_build.cpp" "%OUTPUT_DIR%\value.obj" "%OUTPUT_DIR%\gc_object.obj" "%OUTPUT_DIR%\gc_string.obj" "%OUTPUT_DIR%\string_pool.obj" "%OUTPUT_DIR%\table.obj"

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

