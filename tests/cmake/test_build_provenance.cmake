foreach(_required IN ITEMS
        LUA_CPP_SOURCE_DIR
        LUA_CPP_BINARY_DIR
        LUA_CPP_PROVENANCE_FILE
        LUA_CPP_GIT_EXECUTABLE
        LUA_CPP_EXPECTED_SYSTEM_NAME
        LUA_CPP_EXPECTED_SYSTEM_PROCESSOR
        LUA_CPP_EXPECTED_POINTER_SIZE)
    if(NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
        message(FATAL_ERROR "${_required} is required")
    endif()
endforeach()

if(NOT EXISTS "${LUA_CPP_PROVENANCE_FILE}")
    message(FATAL_ERROR "Configured build provenance file is missing")
endif()

file(READ "${LUA_CPP_PROVENANCE_FILE}" _provenance)
foreach(_field IN ITEMS
        schema
        source_git_status
        source_git_sha
        source_directory
        binary_directory
        generator
        generator_platform
        generator_toolset
        system_name
        system_processor
        pointer_size
        cmake_version
        build_type
        configuration_types
        configured_at)
    string(JSON _value ERROR_VARIABLE _json_error GET "${_provenance}" "${_field}")
    if(NOT "${_json_error}" STREQUAL "NOTFOUND")
        message(FATAL_ERROR "Build provenance field ${_field} is invalid: ${_json_error}")
    endif()
endforeach()

string(JSON _schema GET "${_provenance}" schema)
if(NOT "${_schema}" STREQUAL "lua-cpp.build-provenance/v2")
    message(FATAL_ERROR "Build provenance schema mismatch")
endif()

file(REAL_PATH "${LUA_CPP_SOURCE_DIR}" _expected_source)
file(REAL_PATH "${LUA_CPP_BINARY_DIR}" _expected_binary)
file(TO_CMAKE_PATH "${_expected_source}" _expected_source)
file(TO_CMAKE_PATH "${_expected_binary}" _expected_binary)
string(JSON _actual_source GET "${_provenance}" source_directory)
string(JSON _actual_binary GET "${_provenance}" binary_directory)
if(NOT "${_actual_source}" STREQUAL "${_expected_source}")
    message(FATAL_ERROR "Build provenance source directory mismatch")
endif()
if(NOT "${_actual_binary}" STREQUAL "${_expected_binary}")
    message(FATAL_ERROR "Build provenance binary directory mismatch")
endif()

execute_process(
    COMMAND "${LUA_CPP_GIT_EXECUTABLE}" -C "${LUA_CPP_SOURCE_DIR}" rev-parse --verify HEAD
    RESULT_VARIABLE _git_result
    OUTPUT_VARIABLE _expected_sha
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(NOT _git_result EQUAL 0)
    message(FATAL_ERROR "Unable to resolve the contract fixture Git HEAD")
endif()
string(TOLOWER "${_expected_sha}" _expected_sha)
string(JSON _git_status GET "${_provenance}" source_git_status)
string(JSON _actual_sha GET "${_provenance}" source_git_sha)
if(NOT "${_git_status}" STREQUAL "exact"
   OR NOT "${_actual_sha}" STREQUAL "${_expected_sha}")
    message(FATAL_ERROR "Build provenance exact Git SHA mismatch")
endif()

string(JSON _generator GET "${_provenance}" generator)
string(JSON _generator_platform GET "${_provenance}" generator_platform)
string(JSON _system_name GET "${_provenance}" system_name)
string(JSON _system_processor GET "${_provenance}" system_processor)
string(JSON _pointer_size GET "${_provenance}" pointer_size)
string(JSON _system_name_kind TYPE "${_provenance}" system_name)
string(JSON _system_processor_kind TYPE "${_provenance}" system_processor)
string(JSON _pointer_size_kind TYPE "${_provenance}" pointer_size)
string(JSON _cmake_version GET "${_provenance}" cmake_version)
string(JSON _build_type GET "${_provenance}" build_type)
string(JSON _configuration_type_kind TYPE "${_provenance}" configuration_types)
string(JSON _configuration_type_count LENGTH "${_provenance}" configuration_types)
string(JSON _configured_at GET "${_provenance}" configured_at)
if("${_generator}" STREQUAL ""
   OR NOT "${_system_name_kind}" STREQUAL "STRING"
   OR NOT "${_system_processor_kind}" STREQUAL "STRING"
   OR NOT "${_pointer_size_kind}" STREQUAL "NUMBER"
   OR NOT "${_system_name}" STREQUAL "${LUA_CPP_EXPECTED_SYSTEM_NAME}"
   OR NOT "${_system_processor}" STREQUAL "${LUA_CPP_EXPECTED_SYSTEM_PROCESSOR}"
   OR NOT "${_pointer_size}" STREQUAL "${LUA_CPP_EXPECTED_POINTER_SIZE}"
   OR NOT "${_generator_platform}" STREQUAL
       "${LUA_CPP_EXPECTED_GENERATOR_PLATFORM}"
   OR NOT "${_configuration_type_kind}" STREQUAL "ARRAY"
   OR NOT "${_cmake_version}" MATCHES "^[0-9]+\\.[0-9]+"
   OR NOT "${_configured_at}" MATCHES
       "^[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]T[0-9][0-9]:[0-9][0-9]:[0-9][0-9]Z$")
    message(FATAL_ERROR "Build provenance configure identity is incomplete")
endif()
if(_configuration_type_count GREATER 0)
    if(NOT "${_build_type}" STREQUAL "")
        message(FATAL_ERROR "Multi-config provenance must have an empty build_type")
    endif()
else()
    if("${_build_type}" STREQUAL "")
        message(FATAL_ERROR "Single-config provenance must name its build_type")
    endif()
endif()

message(STATUS "Build provenance CMake contract passed")
