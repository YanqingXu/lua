# Plugin System CMake Configuration
cmake_minimum_required(VERSION 3.12)

# Plugin system library
set(PLUGIN_SOURCES
    plugin.cpp
    plugin_manager.cpp
    plugin_loader.cpp
    plugin_registry.cpp
    plugin_context.cpp
    plugin_sandbox.cpp
)

set(PLUGIN_HEADERS
    plugin.hpp
    plugin_interface.hpp
    plugin_manager.hpp
    plugin_loader.hpp
    plugin_registry.hpp
    plugin_context.hpp
    plugin_sandbox.hpp
)

# Create plugin system library
add_library(lua_plugin_system STATIC ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})

# Include directories
target_include_directories(lua_plugin_system PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

# Link with Lua and other dependencies
target_link_libraries(lua_plugin_system
    lua_lib_manager
    ${CMAKE_DL_LIBS}  # For dynamic loading
)

# Compiler features
target_compile_features(lua_plugin_system PUBLIC cxx_std_17)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(lua_plugin_system PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
endif()

# Example plugins (optional)
option(BUILD_PLUGIN_EXAMPLES "Build plugin examples" ON)

if(BUILD_PLUGIN_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests (optional)
option(BUILD_PLUGIN_TESTS "Build plugin tests" OFF)

if(BUILD_PLUGIN_TESTS)
    add_subdirectory(tests)
endif()

# Install rules
install(TARGETS lua_plugin_system
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${PLUGIN_HEADERS}
    DESTINATION include/lua/plugin
)