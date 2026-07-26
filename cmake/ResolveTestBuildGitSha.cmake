include_guard(GLOBAL)

# Resolve the immutable build identity embedded in lua_test.
#
# An explicit override supports source archives and other Git-less builds. When
# no override is supplied, Git is mandatory and every failure is fatal: a test
# binary with ambiguous provenance must never be produced.
function(lua_resolve_test_build_git_sha output_variable)
    set(one_value_arguments OVERRIDE SOURCE_DIR GIT_EXECUTABLE)
    cmake_parse_arguments(
        LUA_TEST_SHA
        ""
        "${one_value_arguments}"
        ""
        ${ARGN}
    )

    if(LUA_TEST_SHA_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "Unexpected arguments while resolving the lua_test build Git SHA: "
            "${LUA_TEST_SHA_UNPARSED_ARGUMENTS}"
        )
    endif()

    if(NOT "${LUA_TEST_SHA_OVERRIDE}" STREQUAL "")
        set(_lua_test_sha_candidate "${LUA_TEST_SHA_OVERRIDE}")
        set(_lua_test_sha_source "LUA_TEST_BUILD_GIT_SHA cache override")
    else()
        if("${LUA_TEST_SHA_GIT_EXECUTABLE}" STREQUAL "")
            message(FATAL_ERROR
                "LUA_CPP_BUILD_TESTS requires an exact build Git SHA, but Git was not found. "
                "Install Git or configure -DLUA_TEST_BUILD_GIT_SHA=<40-hex-sha>."
            )
        endif()
        if("${LUA_TEST_SHA_SOURCE_DIR}" STREQUAL "")
            message(FATAL_ERROR "SOURCE_DIR is required when resolving the lua_test SHA from Git.")
        endif()

        execute_process(
            COMMAND
                "${LUA_TEST_SHA_GIT_EXECUTABLE}"
                -C "${LUA_TEST_SHA_SOURCE_DIR}"
                rev-parse --verify HEAD
            RESULT_VARIABLE _lua_test_git_result
            OUTPUT_VARIABLE _lua_test_sha_candidate
            ERROR_VARIABLE _lua_test_git_error
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_STRIP_TRAILING_WHITESPACE
        )
        if(NOT "${_lua_test_git_result}" STREQUAL "0")
            if("${_lua_test_git_error}" STREQUAL "")
                set(_lua_test_git_error "<no stderr>")
            endif()
            message(FATAL_ERROR
                "Unable to resolve the lua_test build SHA from Git HEAD "
                "(exit ${_lua_test_git_result}): ${_lua_test_git_error}. "
                "Fix the Git checkout or configure "
                "-DLUA_TEST_BUILD_GIT_SHA=<40-hex-sha>."
            )
        endif()
        set(_lua_test_sha_source "Git HEAD")
    endif()

    string(LENGTH "${_lua_test_sha_candidate}" _lua_test_sha_length)
    if(NOT _lua_test_sha_length EQUAL 40
       OR NOT _lua_test_sha_candidate MATCHES "^[0-9A-Fa-f]+$")
        message(FATAL_ERROR
            "The lua_test build SHA from ${_lua_test_sha_source} must be exactly "
            "40 hexadecimal characters; got '${_lua_test_sha_candidate}'."
        )
    endif()

    string(TOLOWER "${_lua_test_sha_candidate}" _lua_test_sha_normalized)
    set(${output_variable} "${_lua_test_sha_normalized}" PARENT_SCOPE)
endfunction()
