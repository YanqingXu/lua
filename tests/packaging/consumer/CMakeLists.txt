cmake_minimum_required(VERSION 3.20)

project(lua_cpp_package_consumer LANGUAGES C CXX)

find_package(LuaCpp 0.1 CONFIG REQUIRED)

add_executable(lua_cpp_package_consumer main.c)
target_link_libraries(lua_cpp_package_consumer PRIVATE LuaCpp::Lua)

if(TARGET LuaCpp::Shared)
    add_executable(lua_cpp_package_shared_consumer main.c)
    target_link_libraries(lua_cpp_package_shared_consumer PRIVATE LuaCpp::Shared)
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
