cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED PROJECT_SOURCE_DIR OR NOT DEFINED EVIDENCE_OUTPUT)
    message(FATAL_ERROR "PROJECT_SOURCE_DIR and EVIDENCE_OUTPUT are required")
endif()

# This script-mode fixture exercises the release-only CMake decisions without
# pretending to be remote platform evidence. Production evidence is generated
# only by the real configured compiler and host in CMakeLists.txt.
set(MINGW FALSE)
if(DEFINED TEST_MINGW AND TEST_MINGW)
    set(MINGW TRUE)
endif()
set(CMAKE_CROSSCOMPILING FALSE)
set(CMAKE_SIZEOF_VOID_P 8)
set(CMAKE_HOST_SYSTEM_NAME "Windows")
set(CMAKE_HOST_SYSTEM_VERSION "10.0.20348")
set(CMAKE_SYSTEM_NAME "Windows")
set(CMAKE_SYSTEM_PROCESSOR "AMD64")
set(CMAKE_GENERATOR "Visual Studio 17 2022")
set(CMAKE_CXX_COMPILER_ID "MSVC")
set(CMAKE_CXX_COMPILER_VERSION "19.44.35214.0")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
if(DEFINED TEST_MSVC_RUNTIME)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "${TEST_MSVC_RUNTIME}")
endif()

include("${PROJECT_SOURCE_DIR}/cmake/LuaCppPlatformBaseline.cmake")
lua_cpp_apply_platform_baseline(
    POLICY "${PROJECT_SOURCE_DIR}/docs/release/platform-baseline.json"
    EVIDENCE "${EVIDENCE_OUTPUT}"
    RELEASE_RID "windows-x64"
    RELEASE_RUNNER "windows-2022"
)
