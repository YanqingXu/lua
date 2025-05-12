cmake_minimum_required(VERSION 3.15)
project(modern_cpp_lua)

# 添加主项目库
add_library(lua_lib
  src/lexer/lexer.cpp
  src/parser/parser.cpp
  src/compiler/compiler.cpp
  src/vm/value.cpp
  src/vm/table.cpp
  src/vm/state.cpp
  src/vm/function.cpp
  src/vm/vm.cpp
  src/lib/base_lib.cpp
)

target_include_directories(lua_lib PUBLIC ${CMAKE_SOURCE_DIR}/src)

# 添加主可执行文件
add_executable(lua src/main.cpp)
target_link_libraries(lua PRIVATE lua_lib)

# 下载并构建 Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0  # 使用最新版本
)
FetchContent_MakeAvailable(googletest)

# 启用测试
enable_testing()

# 测试可执行文件
add_executable(lua_tests 
  tests/lexer_tests.cpp
  tests/parser_tests.cpp
  tests/value_tests.cpp
  tests/table_tests.cpp
  tests/state_tests.cpp
  tests/function_tests.cpp
  tests/vm_tests.cpp
  tests/integration_tests.cpp
  tests/main_test.cpp
  tests/benchmark_tests.cpp
  tests/compatibility_tests.cpp
)

# 链接Google Test和项目库
target_link_libraries(lua_tests PRIVATE 
  gtest
  gtest_main
  lua_lib  # 假设你的主要项目代码被组织为这个库
)

# 包含目录
target_include_directories(lua_tests PRIVATE 
  ${CMAKE_SOURCE_DIR}/src
)

# 添加测试
include(GoogleTest)
gtest_discover_tests(lua_tests)
