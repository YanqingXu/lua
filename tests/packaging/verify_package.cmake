if(NOT DEFINED PROJECT_BUILD_DIR OR NOT DEFINED PROJECT_SOURCE_DIR)
    message(FATAL_ERROR "PROJECT_BUILD_DIR and PROJECT_SOURCE_DIR are required")
endif()

if(NOT DEFINED TEST_CONFIGURATION OR TEST_CONFIGURATION STREQUAL "")
    set(TEST_CONFIGURATION Release)
endif()

set(prefix "${PROJECT_BUILD_DIR}/package-test/${TEST_CONFIGURATION}/prefix")
set(consumer_build "${PROJECT_BUILD_DIR}/package-test/${TEST_CONFIGURATION}/consumer")

execute_process(
    COMMAND "${CMAKE_COMMAND}" --install "${PROJECT_BUILD_DIR}" --config "${TEST_CONFIGURATION}" --prefix "${prefix}"
    RESULT_VARIABLE install_result
    OUTPUT_VARIABLE install_output
    ERROR_VARIABLE install_error
)
if(NOT install_result EQUAL 0)
    message(FATAL_ERROR "SDK install failed:\n${install_output}\n${install_error}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        -S "${PROJECT_SOURCE_DIR}/tests/packaging/consumer"
        -B "${consumer_build}"
        -DCMAKE_BUILD_TYPE=${TEST_CONFIGURATION}
        -DCMAKE_PREFIX_PATH=${prefix}
    RESULT_VARIABLE configure_result
    OUTPUT_VARIABLE configure_output
    ERROR_VARIABLE configure_error
)
if(NOT configure_result EQUAL 0)
    message(FATAL_ERROR "Installed SDK consumer configure failed:\n${configure_output}\n${configure_error}")
endif()

set(consumer_cache "${consumer_build}/CMakeCache.txt")
if(EXISTS "${consumer_cache}")
    file(READ "${consumer_cache}" consumer_cache_contents)
    if(consumer_cache_contents MATCHES "CMAKE_GENERATOR:INTERNAL=Visual Studio")
        set(consumer_projects
            "${consumer_build}/lua_cpp_package_consumer.vcxproj"
        )
        set(installed_targets "${prefix}/lib/cmake/LuaCpp/LuaCppTargets.cmake")
        if(EXISTS "${installed_targets}")
            file(READ "${installed_targets}" installed_targets_contents)
            if(installed_targets_contents MATCHES "LuaCpp::Shared")
                list(APPEND consumer_projects
                    "${consumer_build}/lua_cpp_package_shared_consumer.vcxproj"
                )
            endif()
        endif()

        file(TO_CMAKE_PATH "${prefix}/include" package_include_directory)
        foreach(consumer_project IN LISTS consumer_projects)
            if(NOT EXISTS "${consumer_project}")
                message(FATAL_ERROR "Expected installed SDK consumer project is missing: ${consumer_project}")
            endif()

            file(READ "${consumer_project}" consumer_project_contents)
            if(consumer_project_contents MATCHES "/external:I")
                message(FATAL_ERROR
                    "Installed SDK include directory is still emitted as /external:I in ${consumer_project}"
                )
            endif()

            set(in_compile_settings FALSE)
            set(found_compile_include FALSE)
            file(STRINGS "${consumer_project}" consumer_project_lines)
            foreach(consumer_project_line IN LISTS consumer_project_lines)
                if(consumer_project_line MATCHES "<ClCompile>")
                    set(in_compile_settings TRUE)
                elseif(consumer_project_line MATCHES "</ClCompile>")
                    set(in_compile_settings FALSE)
                elseif(in_compile_settings)
                    string(REPLACE "\\" "/" normalized_project_line "${consumer_project_line}")
                    string(FIND
                        "${normalized_project_line}"
                        "${package_include_directory}"
                        package_include_index
                    )
                    if(NOT package_include_index EQUAL -1)
                        set(found_compile_include TRUE)
                    endif()
                endif()
            endforeach()
            if(NOT found_compile_include)
                message(FATAL_ERROR
                    "Installed SDK include directory is not a normal compiler include in ${consumer_project}"
                )
            endif()
        endforeach()
    endif()
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${consumer_build}" --config "${TEST_CONFIGURATION}"
    RESULT_VARIABLE build_result
    OUTPUT_VARIABLE build_output
    ERROR_VARIABLE build_error
)
if(NOT build_result EQUAL 0)
    message(FATAL_ERROR "Installed SDK consumer build failed:\n${build_output}\n${build_error}")
endif()

execute_process(
    COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${consumer_build}" -C "${TEST_CONFIGURATION}" --output-on-failure
    RESULT_VARIABLE test_result
    OUTPUT_VARIABLE test_output
    ERROR_VARIABLE test_error
)
if(NOT test_result EQUAL 0)
    message(FATAL_ERROR "Installed SDK consumer test failed:\n${test_output}\n${test_error}")
endif()
