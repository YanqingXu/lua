# Plugin Examples CMake Configuration
cmake_minimum_required(VERSION 3.12)

# Example plugin implementation
set(EXAMPLE_SOURCES
    example_plugin.cpp
)

set(EXAMPLE_HEADERS
    example_plugin.hpp
)

# Create example plugin library
add_library(lua_example_plugins STATIC ${EXAMPLE_SOURCES} ${EXAMPLE_HEADERS})

# Include directories
target_include_directories(lua_example_plugins PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
)

# Link with plugin system
target_link_libraries(lua_example_plugins
    lua_plugin_system
    lua_lib_manager
)

# Compiler features
target_compile_features(lua_example_plugins PUBLIC cxx_std_17)

# Demo application
add_executable(plugin_demo
    demo.cpp
)

target_link_libraries(plugin_demo
    lua_example_plugins
    lua_plugin_system
    lua_lib_manager
)

# Install rules
install(TARGETS lua_example_plugins plugin_demo
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${EXAMPLE_HEADERS}
    DESTINATION include/lua/plugin/examples
)