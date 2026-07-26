cmake_minimum_required(VERSION 3.20)

project(lua_cpp_package_consumer LANGUAGES C CXX)

if(NOT DEFINED LUA_CPP_PACKAGE_ROOT OR LUA_CPP_PACKAGE_ROOT STREQUAL "")
    message(FATAL_ERROR
        "LUA_CPP_PACKAGE_ROOT is required; the consumer must bind to one extracted package"
    )
endif()
file(REAL_PATH "${LUA_CPP_PACKAGE_ROOT}" _lua_cpp_package_root)
set(_lua_cpp_config_directory
    "${_lua_cpp_package_root}/lib/cmake/LuaCpp"
)
if(NOT EXISTS "${_lua_cpp_config_directory}/LuaCppConfig.cmake")
    message(FATAL_ERROR
        "The extracted package has no LuaCppConfig.cmake: ${_lua_cpp_config_directory}"
    )
endif()
set(LuaCpp_DIR "${_lua_cpp_config_directory}" CACHE PATH
    "Exact extracted LuaCpp package config directory" FORCE
)
find_package(LuaCpp 0.1 CONFIG REQUIRED
    PATHS "${_lua_cpp_config_directory}"
    NO_DEFAULT_PATH
)
file(REAL_PATH "${LuaCpp_DIR}" _lua_cpp_resolved_config_directory)
file(REAL_PATH "${_lua_cpp_config_directory}" _lua_cpp_expected_config_directory)
if(NOT _lua_cpp_resolved_config_directory STREQUAL _lua_cpp_expected_config_directory)
    message(FATAL_ERROR
        "LuaCpp resolved outside the extracted package: "
        "${_lua_cpp_resolved_config_directory}"
    )
endif()
unset(_lua_cpp_expected_config_directory)
unset(_lua_cpp_resolved_config_directory)
unset(_lua_cpp_config_directory)
unset(_lua_cpp_package_root)

set(_lua_cpp_imported_targets LuaCpp::Lua)
if(TARGET LuaCpp::Shared)
    list(APPEND _lua_cpp_imported_targets LuaCpp::Shared)
endif()
foreach(_lua_cpp_imported_target IN LISTS _lua_cpp_imported_targets)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.23")
        get_target_property(
            _lua_cpp_imported_no_system
            "${_lua_cpp_imported_target}"
            IMPORTED_NO_SYSTEM
        )
        if(NOT _lua_cpp_imported_no_system)
            message(FATAL_ERROR
                "${_lua_cpp_imported_target} must preserve non-system package include precedence"
            )
        endif()
    else()
        get_target_property(
            _lua_cpp_imported_include_directories
            "${_lua_cpp_imported_target}"
            INTERFACE_INCLUDE_DIRECTORIES
        )
        if(_lua_cpp_imported_include_directories)
            message(FATAL_ERROR
                "${_lua_cpp_imported_target} must delegate includes to the CMake 3.20 compatibility provider"
            )
        endif()
        get_target_property(
            _lua_cpp_imported_link_libraries
            "${_lua_cpp_imported_target}"
            INTERFACE_LINK_LIBRARIES
        )
        if(NOT _lua_cpp_imported_link_libraries MATCHES "_LuaCpp_non_system_headers_")
            message(FATAL_ERROR
                "${_lua_cpp_imported_target} is missing its CMake 3.20 compatibility provider"
            )
        endif()
    endif()
endforeach()
unset(_lua_cpp_imported_link_libraries)
unset(_lua_cpp_imported_include_directories)
unset(_lua_cpp_imported_no_system)
unset(_lua_cpp_imported_target)
unset(_lua_cpp_imported_targets)

add_executable(lua_cpp_package_consumer main.c)
set_property(TARGET lua_cpp_package_consumer PROPERTY LINKER_LANGUAGE CXX)
target_link_libraries(lua_cpp_package_consumer PRIVATE LuaCpp::Lua)
if(MSVC)
    target_compile_options(lua_cpp_package_consumer PRIVATE /W4 /WX)
else()
    target_compile_options(lua_cpp_package_consumer PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(TARGET LuaCpp::Shared)
    add_executable(lua_cpp_package_shared_consumer main.c)
    set_property(TARGET lua_cpp_package_shared_consumer PROPERTY LINKER_LANGUAGE CXX)
    target_link_libraries(lua_cpp_package_shared_consumer PRIVATE LuaCpp::Shared)
    if(MSVC)
        target_compile_options(lua_cpp_package_shared_consumer PRIVATE /W4 /WX)
    else()
        target_compile_options(lua_cpp_package_shared_consumer PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_custom_command(TARGET lua_cpp_package_shared_consumer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:LuaCpp::Shared>
            $<TARGET_FILE_DIR:lua_cpp_package_shared_consumer>
    )
endif()

enable_testing()
add_test(NAME installed_lua_cpp_consumer COMMAND lua_cpp_package_consumer)
if(TARGET lua_cpp_package_shared_consumer)
    add_test(NAME installed_lua_cpp_shared_consumer COMMAND lua_cpp_package_shared_consumer)
endif()
