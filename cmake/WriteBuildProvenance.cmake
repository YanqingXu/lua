include_guard(GLOBAL)

function(_lua_cpp_json_escape output_variable value)
    string(REPLACE "\\" "\\\\" _escaped "${value}")
    string(REPLACE "\"" "\\\"" _escaped "${_escaped}")
    string(REPLACE "\r" "\\r" _escaped "${_escaped}")
    string(REPLACE "\n" "\\n" _escaped "${_escaped}")
    string(REPLACE "\t" "\\t" _escaped "${_escaped}")
    set(${output_variable} "${_escaped}" PARENT_SCOPE)
endfunction()

function(lua_cpp_write_build_provenance)
    set(_options)
    set(_one_value_arguments OUTPUT SOURCE_DIR BINARY_DIR GIT_EXECUTABLE)
    cmake_parse_arguments(
        LUA_CPP_PROVENANCE
        "${_options}"
        "${_one_value_arguments}"
        ""
        ${ARGN}
    )
    if(LUA_CPP_PROVENANCE_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "Unexpected build provenance arguments: "
            "${LUA_CPP_PROVENANCE_UNPARSED_ARGUMENTS}"
        )
    endif()
    foreach(_required IN ITEMS OUTPUT SOURCE_DIR BINARY_DIR)
        if("${LUA_CPP_PROVENANCE_${_required}}" STREQUAL "")
            message(FATAL_ERROR "${_required} is required for build provenance")
        endif()
    endforeach()
    if("${CMAKE_SYSTEM_NAME}" STREQUAL "")
        message(FATAL_ERROR
            "CMAKE_SYSTEM_NAME is required for build provenance target identity"
        )
    endif()
    if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "")
        message(FATAL_ERROR
            "CMAKE_SYSTEM_PROCESSOR is required for build provenance target identity"
        )
    endif()
    if(NOT DEFINED CMAKE_SIZEOF_VOID_P
       OR NOT "${CMAKE_SIZEOF_VOID_P}" MATCHES "^[1-9][0-9]*$")
        message(FATAL_ERROR
            "CMAKE_SIZEOF_VOID_P must be a positive integer for build provenance target identity"
        )
    endif()

    file(REAL_PATH "${LUA_CPP_PROVENANCE_SOURCE_DIR}" _source_directory)
    file(REAL_PATH "${LUA_CPP_PROVENANCE_BINARY_DIR}" _binary_directory)
    file(TO_CMAKE_PATH "${_source_directory}" _source_directory)
    file(TO_CMAKE_PATH "${_binary_directory}" _binary_directory)

    set(_source_git_status "unavailable")
    set(_source_git_sha "unavailable")
    if(NOT "${LUA_CPP_PROVENANCE_GIT_EXECUTABLE}" STREQUAL ""
       AND NOT "${LUA_CPP_PROVENANCE_GIT_EXECUTABLE}" MATCHES "-NOTFOUND$")
        execute_process(
            COMMAND
                "${LUA_CPP_PROVENANCE_GIT_EXECUTABLE}"
                -C "${LUA_CPP_PROVENANCE_SOURCE_DIR}"
                rev-parse --verify HEAD
            RESULT_VARIABLE _git_result
            OUTPUT_VARIABLE _git_output
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        string(LENGTH "${_git_output}" _git_output_length)
        if(_git_result EQUAL 0
           AND _git_output_length EQUAL 40
           AND _git_output MATCHES "^[0-9A-Fa-f]+$")
            string(TOLOWER "${_git_output}" _source_git_sha)
            set(_source_git_status "exact")
        endif()
    endif()

    string(TIMESTAMP _configured_at "%Y-%m-%dT%H:%M:%SZ" UTC)
    _lua_cpp_json_escape(_source_directory_json "${_source_directory}")
    _lua_cpp_json_escape(_binary_directory_json "${_binary_directory}")
    _lua_cpp_json_escape(_generator_json "${CMAKE_GENERATOR}")
    _lua_cpp_json_escape(_generator_platform_json "${CMAKE_GENERATOR_PLATFORM}")
    _lua_cpp_json_escape(_generator_toolset_json "${CMAKE_GENERATOR_TOOLSET}")
    _lua_cpp_json_escape(_system_name_json "${CMAKE_SYSTEM_NAME}")
    _lua_cpp_json_escape(_system_processor_json "${CMAKE_SYSTEM_PROCESSOR}")
    set(_pointer_size_json "${CMAKE_SIZEOF_VOID_P}")
    _lua_cpp_json_escape(_cmake_version_json "${CMAKE_VERSION}")
    _lua_cpp_json_escape(_build_type_json "${CMAKE_BUILD_TYPE}")
    set(_configuration_types_json "")
    foreach(_configuration_type IN LISTS CMAKE_CONFIGURATION_TYPES)
        _lua_cpp_json_escape(_configuration_type_json "${_configuration_type}")
        if(NOT _configuration_types_json STREQUAL "")
            string(APPEND _configuration_types_json ", ")
        endif()
        string(APPEND _configuration_types_json "\"${_configuration_type_json}\"")
    endforeach()

    get_filename_component(_output_directory "${LUA_CPP_PROVENANCE_OUTPUT}" DIRECTORY)
    file(MAKE_DIRECTORY "${_output_directory}")
    configure_file(
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/BuildProvenance.json.in"
        "${LUA_CPP_PROVENANCE_OUTPUT}"
        @ONLY
        NEWLINE_STYLE UNIX
    )
endfunction()
