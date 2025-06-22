# Lua测试框架CMake配置文件
cmake_minimum_required(VERSION 3.12)

# 项目信息
project(LuaTestFramework 
    VERSION 2.0.0
    DESCRIPTION "Modern C++ Testing Framework for Lua Interpreter"
    LANGUAGES CXX
)

# C++标准设置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# 构建类型设置
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 调试模式下的额外选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG)
    if(NOT MSVC)
        add_compile_options(-g -O0)
    endif()
else()
    add_compile_definitions(NDEBUG)
    if(NOT MSVC)
        add_compile_options(-O2)
    endif()
endif()

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 测试框架库
add_library(lua_test_framework INTERFACE)

# 设置接口包含目录
target_include_directories(lua_test_framework INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# 设置接口编译特性
target_compile_features(lua_test_framework INTERFACE cxx_std_17)

# 查找测试文件
file(GLOB_RECURSE TEST_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/../tests/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../tests/*.hpp"
)

# 示例测试可执行文件
add_executable(example_tests
    examples/example_test.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/example_main.cpp
)

target_link_libraries(example_tests PRIVATE lua_test_framework)

# 主测试可执行文件（如果存在）
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../tests/test_main.cpp")
    add_executable(main_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/test_main.cpp
        ${TEST_SOURCES}
    )
    
    target_link_libraries(main_tests PRIVATE lua_test_framework)
    
    # 添加测试目标
    enable_testing()
    add_test(NAME MainTests COMMAND main_tests)
endif()

# 新测试框架主测试（如果存在）
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../tests/test_main_new.hpp")
    # 创建新测试主文件
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/test_main_template.cpp.in
        ${CMAKE_BINARY_DIR}/test_main_new.cpp
        @ONLY
    )
    
    add_executable(new_tests
        ${CMAKE_BINARY_DIR}/test_main_new.cpp
    )
    
    target_link_libraries(new_tests PRIVATE lua_test_framework)
    
    # 添加新测试目标
    add_test(NAME NewFrameworkTests COMMAND new_tests)
endif()

# 安装配置
install(TARGETS lua_test_framework
    EXPORT LuaTestFrameworkTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# 安装头文件
install(DIRECTORY 
    core/
    formatting/
    tools/
    examples/
    DESTINATION include/test_framework
    FILES_MATCHING PATTERN "*.hpp"
)

install(FILES
    test_framework.hpp
    DESTINATION include/test_framework
)

# 安装文档
install(FILES
    README.md
    USAGE_GUIDE.md
    MIGRATION_GUIDE.md
    DESTINATION share/doc/lua_test_framework
)

# 导出目标
install(EXPORT LuaTestFrameworkTargets
    FILE LuaTestFrameworkTargets.cmake
    NAMESPACE LuaTestFramework::
    DESTINATION lib/cmake/LuaTestFramework
)

# 创建配置文件
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/LuaTestFrameworkConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/LuaTestFrameworkConfig.cmake
    INSTALL_DESTINATION lib/cmake/LuaTestFramework
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/LuaTestFrameworkConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/LuaTestFrameworkConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/LuaTestFrameworkConfigVersion.cmake
    DESTINATION lib/cmake/LuaTestFramework
)

# 自定义目标
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS main_tests new_tests example_tests
    COMMENT "Running all tests"
)

add_custom_target(run_example
    COMMAND example_tests
    DEPENDS example_tests
    COMMENT "Running example tests"
)

# 代码覆盖率（如果支持）
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT MSVC)
    option(ENABLE_COVERAGE "Enable code coverage" OFF)
    
    if(ENABLE_COVERAGE)
        target_compile_options(lua_test_framework INTERFACE --coverage)
        target_link_options(lua_test_framework INTERFACE --coverage)
        
        find_program(GCOV_PATH gcov)
        find_program(LCOV_PATH lcov)
        find_program(GENHTML_PATH genhtml)
        
        if(GCOV_PATH AND LCOV_PATH AND GENHTML_PATH)
            add_custom_target(coverage
                COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage.info
                COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' --output-file coverage.info
                COMMAND ${LCOV_PATH} --list coverage.info
                COMMAND ${GENHTML_PATH} -o coverage coverage.info
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Generating code coverage report"
            )
        endif()
    endif()
endif()

# 打印配置信息
message(STATUS "Lua Test Framework Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "  Debug Mode: Enabled")
else()
    message(STATUS "  Release Mode: Enabled")
endif()

if(ENABLE_COVERAGE)
    message(STATUS "  Code Coverage: Enabled")
else()
    message(STATUS "  Code Coverage: Disabled")
endif()