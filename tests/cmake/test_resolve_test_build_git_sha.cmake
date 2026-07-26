if(NOT DEFINED LUA_CPP_SOURCE_DIR OR LUA_CPP_SOURCE_DIR STREQUAL "")
    message(FATAL_ERROR "LUA_CPP_SOURCE_DIR is required")
endif()

if(LUA_CPP_SHA_PROBE_CHILD)
    include("${LUA_CPP_SOURCE_DIR}/cmake/ResolveTestBuildGitSha.cmake")
    lua_resolve_test_build_git_sha(
        _probe_sha
        OVERRIDE "${PROBE_OVERRIDE}"
        SOURCE_DIR "${LUA_CPP_SOURCE_DIR}"
        GIT_EXECUTABLE "${PROBE_GIT_EXECUTABLE}"
    )
    message(STATUS "Resolved probe SHA: ${_probe_sha}")
    return()
endif()

function(run_sha_probe result_variable output_variable error_variable)
    execute_process(
        COMMAND
            "${CMAKE_COMMAND}"
            "-DLUA_CPP_SOURCE_DIR=${LUA_CPP_SOURCE_DIR}"
            -DLUA_CPP_SHA_PROBE_CHILD=ON
            ${ARGN}
            -P "${CMAKE_CURRENT_LIST_FILE}"
        RESULT_VARIABLE _probe_result
        OUTPUT_VARIABLE _probe_output
        ERROR_VARIABLE _probe_error
    )
    set(${result_variable} "${_probe_result}" PARENT_SCOPE)
    set(${output_variable} "${_probe_output}" PARENT_SCOPE)
    set(${error_variable} "${_probe_error}" PARENT_SCOPE)
endfunction()

set(_valid_upper_sha "ABCDEF0123456789ABCDEF0123456789ABCDEF01")
set(_valid_lower_sha "abcdef0123456789abcdef0123456789abcdef01")

run_sha_probe(
    _result
    _output
    _error
    "-DPROBE_OVERRIDE=${_valid_upper_sha}"
    -DPROBE_GIT_EXECUTABLE=
)
if(NOT "${_result}" STREQUAL "0")
    message(FATAL_ERROR
        "A valid explicit SHA override was rejected.\nstdout:\n${_output}\nstderr:\n${_error}"
    )
endif()
if(NOT _output MATCHES "Resolved probe SHA: ${_valid_lower_sha}")
    message(FATAL_ERROR
        "The valid SHA override was not normalized to lowercase.\nstdout:\n${_output}"
    )
endif()

run_sha_probe(
    _result
    _output
    _error
    -DPROBE_OVERRIDE=unknown
    -DPROBE_GIT_EXECUTABLE=
)
if("${_result}" STREQUAL "0")
    message(FATAL_ERROR "The invalid 'unknown' SHA override unexpectedly succeeded")
endif()
if(NOT "${_output}${_error}" MATCHES "exactly[ \n]+40 hexadecimal")
    message(FATAL_ERROR
        "The invalid override failure was not explicit.\nstdout:\n${_output}\nstderr:\n${_error}"
    )
endif()

run_sha_probe(
    _result
    _output
    _error
    -DPROBE_OVERRIDE=
    -DPROBE_GIT_EXECUTABLE=
)
if("${_result}" STREQUAL "0")
    message(FATAL_ERROR "Automatic SHA resolution unexpectedly succeeded without Git")
endif()
if(NOT "${_output}${_error}" MATCHES "Git was not found")
    message(FATAL_ERROR
        "The missing-Git failure was not explicit.\nstdout:\n${_output}\nstderr:\n${_error}"
    )
endif()

run_sha_probe(
    _result
    _output
    _error
    -DPROBE_OVERRIDE=
    "-DPROBE_GIT_EXECUTABLE=${CMAKE_COMMAND}"
)
if("${_result}" STREQUAL "0")
    message(FATAL_ERROR "Automatic SHA resolution unexpectedly ignored a Git failure")
endif()
if(NOT "${_output}${_error}" MATCHES "Unable to resolve the lua_test build SHA")
    message(FATAL_ERROR
        "The Git execution failure was not explicit.\nstdout:\n${_output}\nstderr:\n${_error}"
    )
endif()

message(STATUS "Build SHA CMake resolution probes passed")
