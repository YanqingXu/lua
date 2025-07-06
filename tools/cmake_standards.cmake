# Lua 解释器项目 - 代码规范相关的CMake配置
# 将此内容添加到主CMakeLists.txt中

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译器警告设置 - 强制零警告
if(MSVC)
    # Visual Studio
    add_compile_options(/W4 /WX)
    add_compile_options(/wd4100)  # 未使用参数警告可选择性忽略
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    # GCC/Clang
    add_compile_options(-Wall -Wextra -Werror)
    add_compile_options(-Wno-unused-parameter)  # 可选择性忽略
endif()

# Debug构建增加额外检查
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # 添加AddressSanitizer
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
        
        # 添加其他调试标志
        add_compile_options(-g3 -O0)
    endif()
endif()

# Release构建优化
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -DNDEBUG)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options(-flto)  # Link Time Optimization
        add_link_options(-flto)
    endif()
endif()

# 代码质量检查工具集成
find_program(CLANG_TIDY_EXE 
    NAMES "clang-tidy"
    DOC "Path to clang-tidy executable"
)

if(CLANG_TIDY_EXE)
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
    
    # 创建clang-tidy目标
    set(CMAKE_CXX_CLANG_TIDY 
        ${CLANG_TIDY_EXE};
        -checks=-*,readability-*,performance-*,modernize-*,bugprone-*;
        -header-filter=.*
    )
else()
    message(WARNING "clang-tidy not found")
endif()

find_program(CLANG_FORMAT_EXE
    NAMES "clang-format"
    DOC "Path to clang-format executable"
)

if(CLANG_FORMAT_EXE)
    message(STATUS "clang-format found: ${CLANG_FORMAT_EXE}")
    
    # 获取所有源文件
    file(GLOB_RECURSE ALL_SOURCE_FILES
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/src/*.hpp
        ${CMAKE_SOURCE_DIR}/src/*.h
    )
    
    # 创建格式化目标
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i ${ALL_SOURCE_FILES}
        COMMENT "Formatting code with clang-format"
    )
    
    # 创建格式检查目标
    add_custom_target(format-check
        COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror ${ALL_SOURCE_FILES}
        COMMENT "Checking code format with clang-format"
    )
else()
    message(WARNING "clang-format not found")
endif()

# cppcheck 静态分析
find_program(CPPCHECK_EXE
    NAMES "cppcheck"
    DOC "Path to cppcheck executable"
)

if(CPPCHECK_EXE)
    message(STATUS "cppcheck found: ${CPPCHECK_EXE}")
    
    add_custom_target(cppcheck
        COMMAND ${CPPCHECK_EXE}
            --enable=all
            --error-exitcode=1
            --suppress=missingIncludeSystem
            --suppress=unmatchedSuppression
            --quiet
            ${CMAKE_SOURCE_DIR}/src
        COMMENT "Running cppcheck static analysis"
    )
else()
    message(WARNING "cppcheck not found")
endif()

# 创建代码质量检查组合目标
add_custom_target(check-all
    DEPENDS format-check cppcheck
    COMMENT "Running all code quality checks"
)

# 函数：为目标启用严格编译检查
function(enable_strict_compilation target_name)
    # 确保目标使用项目编译标志
    target_compile_features(${target_name} PRIVATE cxx_std_17)
    
    # 添加包含目录
    target_include_directories(${target_name} PRIVATE 
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/common
    )
    
    # 强制包含types.hpp
    target_compile_definitions(${target_name} PRIVATE 
        LUA_ENFORCE_TYPE_SYSTEM=1
    )
    
    # 设置目标属性
    set_target_properties(${target_name} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
    
    # 如果启用了clang-tidy，为此目标设置
    if(CLANG_TIDY_EXE)
        set_target_properties(${target_name} PROPERTIES
            CXX_CLANG_TIDY "${CMAKE_CXX_CLANG_TIDY}"
        )
    endif()
endfunction()

# 示例使用方法：
# add_library(base_lib src/lib/base_lib.cpp)
# enable_strict_compilation(base_lib)

# 测试配置
if(BUILD_TESTING)
    enable_testing()
    
    # 查找Google Test
    find_package(GTest QUIET)
    if(GTest_FOUND)
        message(STATUS "Google Test found")
        
        # 为所有测试启用严格编译
        function(add_strict_test test_name)
            add_executable(${test_name} ${ARGN})
            target_link_libraries(${test_name} GTest::gtest GTest::gtest_main)
            enable_strict_compilation(${test_name})
            add_test(NAME ${test_name} COMMAND ${test_name})
        endfunction()
    else()
        message(WARNING "Google Test not found - tests will not be built")
    endif()
endif()

# 打印配置摘要
message(STATUS "=== Lua 解释器代码规范配置 ===")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
if(CLANG_TIDY_EXE)
    message(STATUS "Static Analysis: Enabled (clang-tidy)")
endif()
if(CLANG_FORMAT_EXE)
    message(STATUS "Code Formatting: Enabled (clang-format)")
endif()
if(CPPCHECK_EXE)
    message(STATUS "Additional Checks: Enabled (cppcheck)")
endif()
message(STATUS "========================================")
