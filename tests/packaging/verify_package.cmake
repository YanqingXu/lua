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
