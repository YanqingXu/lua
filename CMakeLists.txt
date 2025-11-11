# =====================================================================
# Lua C++ 解释器 CMake 配置文件
# =====================================================================
#
# 功能：
# - 配置 C++17 标准
# - 设置编译选项和警告级别
# - 组织源文件和头文件
# - 配置测试框架（Google Test）
# - 支持多种构建类型（Debug/Release）
#
# 使用方法：
#   mkdir build && cd build
#   cmake ..
#   cmake --build .
#
# =====================================================================

cmake_minimum_required(VERSION 3.15)

# =====================================================================
# 项目信息
# =====================================================================

project(LuaCppInterpreter
    VERSION 0.1.0
    DESCRIPTION "Lua 5.1.5 Interpreter implemented in Modern C++"
    LANGUAGES CXX
)

# =====================================================================
# C++ 标准设置
# =====================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")

# =====================================================================
# 编译选项
# =====================================================================

# MSVC 编译器选项
if(MSVC)
    add_compile_options(
        /W4                 # 警告级别4
        /WX-                # 警告不视为错误（开发阶段）
        /permissive-        # 严格标准符合性
        /Zc:__cplusplus     # 正确设置 __cplusplus 宏
        /EHsc               # 异常处理模型
        /utf-8              # 源文件和执行字符集都使用 UTF-8
    )
    
    # Debug 模式选项
    add_compile_options($<$<CONFIG:Debug>:/Od>)      # 禁用优化
    add_compile_options($<$<CONFIG:Debug>:/Zi>)      # 生成调试信息
    add_compile_options($<$<CONFIG:Debug>:/MDd>)     # 多线程调试 DLL
    
    # Release 模式选项
    add_compile_options($<$<CONFIG:Release>:/O2>)    # 最大优化
    add_compile_options($<$<CONFIG:Release>:/MD>)    # 多线程 DLL
    add_compile_options($<$<CONFIG:Release>:/DNDEBUG>) # 定义 NDEBUG
    
    message(STATUS "Compiler: MSVC")
endif()

# GCC/Clang 编译器选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall               # 启用所有警告
        -Wextra             # 启用额外警告
        -Wpedantic          # 严格标准符合性
        -Wno-unused-parameter  # 忽略未使用参数警告
    )
    
    # Debug 模式选项
    add_compile_options($<$<CONFIG:Debug>:-O0>)      # 禁用优化
    add_compile_options($<$<CONFIG:Debug>:-g>)       # 生成调试信息
    
    # Release 模式选项
    add_compile_options($<$<CONFIG:Release>:-O3>)    # 最大优化
    add_compile_options($<$<CONFIG:Release>:-DNDEBUG>) # 定义 NDEBUG
    
    message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

# =====================================================================
# 包含目录
# =====================================================================

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/compiler
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lib
)

# =====================================================================
# 源文件
# =====================================================================

# 公共模块
set(COMMON_HEADERS
    src/common/types.hpp
    src/common/config.hpp
    src/common/macros.hpp
)

# 核心模块（待实现）
set(CORE_HEADERS
    # src/core/value.hpp
    # src/core/gc_object.hpp
)

set(CORE_SOURCES
    # src/core/value.cpp
    # src/core/gc_object.cpp
)

# GC 模块（待实现）
set(GC_HEADERS
    # src/gc/garbage_collector.hpp
)

set(GC_SOURCES
    # src/gc/garbage_collector.cpp
)

# VM 模块（待实现）
set(VM_HEADERS
    # src/vm/lua_state.hpp
)

set(VM_SOURCES
    # src/vm/lua_state.cpp
)

# 编译器模块（待实现）
set(COMPILER_HEADERS
    # src/compiler/lexer.hpp
)

set(COMPILER_SOURCES
    # src/compiler/lexer.cpp
)

# 标准库模块（待实现）
set(LIB_HEADERS
    # src/lib/base_lib.hpp
)

set(LIB_SOURCES
    # src/lib/base_lib.cpp
)

# 所有头文件
set(ALL_HEADERS
    ${COMMON_HEADERS}
    ${CORE_HEADERS}
    ${GC_HEADERS}
    ${VM_HEADERS}
    ${COMPILER_HEADERS}
    ${LIB_HEADERS}
)

# 所有源文件
set(ALL_SOURCES
    ${CORE_SOURCES}
    ${GC_SOURCES}
    ${VM_SOURCES}
    ${COMPILER_SOURCES}
    ${LIB_SOURCES}
)

# =====================================================================
# 库目标（暂时注释，等有源文件后启用）
# =====================================================================

# add_library(lua_core STATIC
#     ${ALL_SOURCES}
#     ${ALL_HEADERS}
# )

# =====================================================================
# 测试可执行文件
# =====================================================================

# 创建一个简单的测试程序来验证编译
add_executable(test_build
    ${CMAKE_CURRENT_BINARY_DIR}/test_build.cpp
)

# 生成测试源文件
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/test_build.cpp
"#include \"common/types.hpp\"
#include \"common/config.hpp\"
#include \"common/macros.hpp\"
#include <iostream>

int main() {
    Lua::LUA_LOG_INFO(\"Lua C++ Interpreter - Build Test\");
    Lua::LUA_LOG_INFO(\"Version: \" << Lua::LUA_VERSION);
    
    std::cout << \"\\n[INFO] Configuration:\" << std::endl;
    std::cout << \"  - C++ Standard: C++17\" << std::endl;
    std::cout << \"  - Debug Mode: \" << (Lua::DEBUG_MODE ? \"Yes\" : \"No\") << std::endl;
    std::cout << \"  - Platform: \";
    if (Lua::IS_WINDOWS) std::cout << \"Windows\";
    else if (Lua::IS_LINUX) std::cout << \"Linux\";
    else if (Lua::IS_MACOS) std::cout << \"macOS\";
    else std::cout << \"Unknown\";
    std::cout << std::endl;
    std::cout << \"  - Architecture: \" << (Lua::IS_64BIT ? \"64-bit\" : \"32-bit\") << std::endl;
    
    std::cout << \"\\n[INFO] Type Sizes:\" << std::endl;
    std::cout << \"  - LuaInteger: \" << sizeof(Lua::LuaInteger) << \" bytes\" << std::endl;
    std::cout << \"  - LuaNumber: \" << sizeof(Lua::LuaNumber) << \" bytes\" << std::endl;
    std::cout << \"  - ValueType: \" << sizeof(Lua::ValueType) << \" bytes\" << std::endl;
    
    std::cout << \"\\n[SUCCESS] Build test passed!\" << std::endl;
    return 0;
}
")

# =====================================================================
# 测试框架（Google Test）- 可选
# =====================================================================

option(BUILD_TESTS "Build unit tests" OFF)

if(BUILD_TESTS)
    # 查找 Google Test
    find_package(GTest)
    
    if(GTest_FOUND)
        enable_testing()
        
        # 添加测试可执行文件
        # add_executable(unit_tests
        #     tests/unit/test_value.cpp
        # )
        
        # target_link_libraries(unit_tests
        #     lua_core
        #     GTest::GTest
        #     GTest::Main
        # )
        
        # include(GoogleTest)
        # gtest_discover_tests(unit_tests)
        
        message(STATUS "Google Test found, unit tests enabled")
    else()
        message(WARNING "Google Test not found, unit tests disabled")
    endif()
endif()

# =====================================================================
# 安装配置
# =====================================================================

# install(TARGETS lua_core
#     ARCHIVE DESTINATION lib
#     LIBRARY DESTINATION lib
#     RUNTIME DESTINATION bin
# )

# install(DIRECTORY src/
#     DESTINATION include/lua
#     FILES_MATCHING PATTERN "*.hpp"
# )

# =====================================================================
# 输出信息
# =====================================================================

message(STATUS "")
message(STATUS "========================================")
message(STATUS "Lua C++ Interpreter Configuration")
message(STATUS "========================================")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build Tests: ${BUILD_TESTS}")
message(STATUS "========================================")
message(STATUS "")

