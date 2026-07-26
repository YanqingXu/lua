include_guard(GLOBAL)

function(_lua_cpp_platform_json_escape output_variable value)
    string(REPLACE "\\" "\\\\" _escaped "${value}")
    string(REPLACE "\"" "\\\"" _escaped "${_escaped}")
    string(REPLACE "\r" "\\r" _escaped "${_escaped}")
    string(REPLACE "\n" "\\n" _escaped "${_escaped}")
    string(REPLACE "\t" "\\t" _escaped "${_escaped}")
    set(${output_variable} "${_escaped}" PARENT_SCOPE)
endfunction()

function(_lua_cpp_platform_policy_get output_variable policy_json rid section field)
    if("${section}" STREQUAL "")
        string(
            JSON
            _value
            ERROR_VARIABLE _json_error
            GET
            "${policy_json}"
            releaseRids
            "${rid}"
            "${field}"
        )
    else()
        string(
            JSON
            _value
            ERROR_VARIABLE _json_error
            GET
            "${policy_json}"
            releaseRids
            "${rid}"
            "${section}"
            "${field}"
        )
    endif()
    if(NOT _json_error STREQUAL "NOTFOUND")
        message(FATAL_ERROR
            "Invalid platform baseline field releaseRids.${rid}.${section}.${field}: "
            "${_json_error}"
        )
    endif()
    set(${output_variable} "${_value}" PARENT_SCOPE)
endfunction()

function(_lua_cpp_platform_read_linux_identity output_id output_version output_libc)
    if(NOT EXISTS "/etc/os-release")
        message(FATAL_ERROR "Linux release packaging requires /etc/os-release")
    endif()
    file(STRINGS "/etc/os-release" _identity_lines
        REGEX "^(ID|VERSION_ID)="
    )
    set(_distribution_id "")
    set(_distribution_version "")
    foreach(_identity_line IN LISTS _identity_lines)
        if(_identity_line MATCHES "^ID=(.*)$")
            set(_distribution_id "${CMAKE_MATCH_1}")
            string(REGEX REPLACE "^\"(.*)\"$" "\\1"
                _distribution_id "${_distribution_id}"
            )
        elseif(_identity_line MATCHES "^VERSION_ID=(.*)$")
            set(_distribution_version "${CMAKE_MATCH_1}")
            string(REGEX REPLACE "^\"(.*)\"$" "\\1"
                _distribution_version "${_distribution_version}"
            )
        endif()
    endforeach()
    if(_distribution_id STREQUAL "" OR _distribution_version STREQUAL "")
        message(FATAL_ERROR
            "Linux release packaging could not resolve ID and VERSION_ID from /etc/os-release"
        )
    endif()

    execute_process(
        COMMAND getconf GNU_LIBC_VERSION
        RESULT_VARIABLE _getconf_result
        OUTPUT_VARIABLE _getconf_output
        ERROR_VARIABLE _getconf_error
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT _getconf_result EQUAL 0
       OR NOT _getconf_output MATCHES "^glibc ([0-9]+(\\.[0-9]+)+)$")
        message(FATAL_ERROR
            "Linux release packaging requires an exact glibc identity; "
            "getconf returned '${_getconf_output}' (${_getconf_error})"
        )
    endif()
    set(${output_id} "${_distribution_id}" PARENT_SCOPE)
    set(${output_version} "${_distribution_version}" PARENT_SCOPE)
    set(${output_libc} "${CMAKE_MATCH_1}" PARENT_SCOPE)
endfunction()

function(lua_cpp_apply_platform_baseline)
    set(_options)
    set(_one_value_arguments POLICY EVIDENCE RELEASE_RID RELEASE_RUNNER)
    cmake_parse_arguments(
        LUA_CPP_PLATFORM
        "${_options}"
        "${_one_value_arguments}"
        ""
        ${ARGN}
    )
    if(LUA_CPP_PLATFORM_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "Unexpected platform baseline arguments: "
            "${LUA_CPP_PLATFORM_UNPARSED_ARGUMENTS}"
        )
    endif()
    foreach(_required_argument IN ITEMS POLICY EVIDENCE)
        if("${LUA_CPP_PLATFORM_${_required_argument}}" STREQUAL "")
            message(FATAL_ERROR
                "${_required_argument} is required for the platform baseline"
            )
        endif()
    endforeach()

    if(MINGW)
        message(FATAL_ERROR
            "MinGW is not supported by the LuaCpp 0.1.x Windows contract. "
            "Use an x64 MSVC v143 toolchain; MinGW must not configure and fail later at link time."
        )
    endif()

    if(NOT EXISTS "${LUA_CPP_PLATFORM_POLICY}")
        message(FATAL_ERROR
            "Platform baseline policy is missing: ${LUA_CPP_PLATFORM_POLICY}"
        )
    endif()
    file(READ "${LUA_CPP_PLATFORM_POLICY}" _policy_json)
    string(
        JSON
        _policy_schema
        ERROR_VARIABLE _policy_schema_error
        GET
        "${_policy_json}"
        schema
    )
    if(NOT _policy_schema_error STREQUAL "NOTFOUND"
       OR NOT _policy_schema STREQUAL "lua-cpp.platform-baseline/v1")
        message(FATAL_ERROR
            "Platform baseline policy must use lua-cpp.platform-baseline/v1"
        )
    endif()

    if("${LUA_CPP_PLATFORM_RELEASE_RID}" STREQUAL "")
        return()
    endif()
    set(_supported_rids windows-x64 linux-x64 macos-arm64)
    if(NOT LUA_CPP_PLATFORM_RELEASE_RID IN_LIST _supported_rids)
        message(FATAL_ERROR
            "Unsupported release RID '${LUA_CPP_PLATFORM_RELEASE_RID}'; "
            "expected windows-x64, linux-x64, or macos-arm64"
        )
    endif()
    if("${LUA_CPP_PLATFORM_RELEASE_RUNNER}" STREQUAL "")
        message(FATAL_ERROR
            "LUA_CPP_RELEASE_RUNNER is required for a release RID"
        )
    endif()
    if(CMAKE_CROSSCOMPILING)
        message(FATAL_ERROR
            "0.1.x release packages must be built and consumed on their native target"
        )
    endif()
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "0.1.x release packages are 64-bit only")
    endif()

    set(_rid "${LUA_CPP_PLATFORM_RELEASE_RID}")
    _lua_cpp_platform_policy_get(_expected_architecture
        "${_policy_json}" "${_rid}" "" "architecture"
    )
    _lua_cpp_platform_policy_get(_expected_pointer_size
        "${_policy_json}" "${_rid}" "" "pointerSize"
    )
    _lua_cpp_platform_policy_get(_expected_runner
        "${_policy_json}" "${_rid}" "builder" "runner"
    )
    _lua_cpp_platform_policy_get(_expected_host_system
        "${_policy_json}" "${_rid}" "builder" "hostSystem"
    )
    _lua_cpp_platform_policy_get(_expected_generator
        "${_policy_json}" "${_rid}" "builder" "generator"
    )
    _lua_cpp_platform_policy_get(_expected_compiler_id
        "${_policy_json}" "${_rid}" "builder" "compilerId"
    )
    _lua_cpp_platform_policy_get(_compiler_minimum
        "${_policy_json}" "${_rid}" "builder" "compilerVersionMinimum"
    )
    _lua_cpp_platform_policy_get(_compiler_maximum
        "${_policy_json}" "${_rid}" "builder" "compilerVersionMaximumExclusive"
    )

    if(NOT "${LUA_CPP_PLATFORM_RELEASE_RUNNER}" STREQUAL "${_expected_runner}")
        message(FATAL_ERROR
            "Release runner '${LUA_CPP_PLATFORM_RELEASE_RUNNER}' does not match "
            "${_rid} baseline '${_expected_runner}'"
        )
    endif()
    if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL _expected_host_system
       OR NOT CMAKE_SYSTEM_NAME STREQUAL _expected_host_system)
        message(FATAL_ERROR
            "${_rid} requires native ${_expected_host_system}; host/target are "
            "${CMAKE_HOST_SYSTEM_NAME}/${CMAKE_SYSTEM_NAME}"
        )
    endif()
    if(NOT CMAKE_GENERATOR STREQUAL _expected_generator)
        message(FATAL_ERROR
            "${_rid} release generator '${CMAKE_GENERATOR}' does not match "
            "pinned baseline '${_expected_generator}'"
        )
    endif()
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL _expected_compiler_id)
        message(FATAL_ERROR
            "${_rid} compiler '${CMAKE_CXX_COMPILER_ID}' does not match "
            "pinned baseline '${_expected_compiler_id}'"
        )
    endif()
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS _compiler_minimum
       OR NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS _compiler_maximum)
        message(FATAL_ERROR
            "${_rid} compiler version ${CMAKE_CXX_COMPILER_VERSION} is outside "
            "[${_compiler_minimum}, ${_compiler_maximum})"
        )
    endif()
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL _expected_pointer_size)
        message(FATAL_ERROR
            "${_rid} pointer width does not match the platform policy"
        )
    endif()

    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _normalized_processor)
    if(_rid STREQUAL "macos-arm64")
        set(_accepted_processors aarch64 arm64)
    else()
        set(_accepted_processors amd64 x64 x86_64 x86-64)
    endif()
    if(NOT _normalized_processor IN_LIST _accepted_processors)
        message(FATAL_ERROR
            "${_rid} target processor '${CMAKE_SYSTEM_PROCESSOR}' is not "
            "${_expected_architecture}"
        )
    endif()

    set(_host_version "${CMAKE_HOST_SYSTEM_VERSION}")
    set(_distribution_id "")
    set(_distribution_version "")
    set(_msvc_runtime "")
    set(_libc "")
    set(_libc_version "")
    set(_deployment_target "")

    if(_rid STREQUAL "windows-x64")
        _lua_cpp_platform_policy_get(_host_minimum
            "${_policy_json}" "${_rid}" "builder" "hostVersionMinimum"
        )
        _lua_cpp_platform_policy_get(_msvc_runtime
            "${_policy_json}" "${_rid}" "builder" "msvcRuntime"
        )
        if(CMAKE_HOST_SYSTEM_VERSION VERSION_LESS _host_minimum)
            message(FATAL_ERROR
                "Windows release host ${CMAKE_HOST_SYSTEM_VERSION} is below "
                "${_host_minimum}"
            )
        endif()
        if(NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY
           OR "${CMAKE_MSVC_RUNTIME_LIBRARY}" STREQUAL "")
            set(
                CMAKE_MSVC_RUNTIME_LIBRARY
                "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
                CACHE STRING
                "LuaCpp requires the dynamic MSVC runtime"
                FORCE
            )
        elseif(NOT CMAKE_MSVC_RUNTIME_LIBRARY
               STREQUAL "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
               AND NOT CMAKE_MSVC_RUNTIME_LIBRARY STREQUAL "MultiThreadedDLL")
            message(FATAL_ERROR
                "Windows release packages require dynamic UCRT/MSVC v143; "
                "CMAKE_MSVC_RUNTIME_LIBRARY='${CMAKE_MSVC_RUNTIME_LIBRARY}'"
            )
        endif()
    elseif(_rid STREQUAL "linux-x64")
        _lua_cpp_platform_policy_get(_expected_distribution_id
            "${_policy_json}" "${_rid}" "builder" "distributionId"
        )
        _lua_cpp_platform_policy_get(_expected_distribution_version
            "${_policy_json}" "${_rid}" "builder" "distributionVersion"
        )
        _lua_cpp_platform_policy_get(_libc
            "${_policy_json}" "${_rid}" "minimumRuntime" "libc"
        )
        _lua_cpp_platform_policy_get(_expected_libc_version
            "${_policy_json}" "${_rid}" "minimumRuntime" "libcVersion"
        )
        _lua_cpp_platform_read_linux_identity(
            _distribution_id
            _distribution_version
            _libc_version
        )
        if(NOT _distribution_id STREQUAL _expected_distribution_id
           OR NOT _distribution_version STREQUAL _expected_distribution_version)
            message(FATAL_ERROR
                "Linux release host ${_distribution_id} ${_distribution_version} "
                "does not match ${_expected_distribution_id} "
                "${_expected_distribution_version}"
            )
        endif()
        if(NOT _libc_version STREQUAL _expected_libc_version)
            message(FATAL_ERROR
                "Linux release glibc ${_libc_version} does not match "
                "baseline ${_expected_libc_version}"
            )
        endif()
    else()
        _lua_cpp_platform_policy_get(_host_minimum
            "${_policy_json}" "${_rid}" "builder" "hostVersionMinimum"
        )
        _lua_cpp_platform_policy_get(_deployment_target
            "${_policy_json}" "${_rid}" "minimumRuntime" "deploymentTarget"
        )
        execute_process(
            COMMAND sw_vers -productVersion
            RESULT_VARIABLE _sw_vers_result
            OUTPUT_VARIABLE _host_version
            ERROR_VARIABLE _sw_vers_error
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(NOT _sw_vers_result EQUAL 0)
            message(FATAL_ERROR
                "Cannot resolve macOS host version: ${_sw_vers_error}"
            )
        endif()
        if(_host_version VERSION_LESS _host_minimum)
            message(FATAL_ERROR
                "macOS release host ${_host_version} is below ${_host_minimum}"
            )
        endif()
        if(NOT CMAKE_OSX_DEPLOYMENT_TARGET STREQUAL _deployment_target)
            message(FATAL_ERROR
                "macOS release packages require "
                "-DCMAKE_OSX_DEPLOYMENT_TARGET=${_deployment_target}; got "
                "'${CMAKE_OSX_DEPLOYMENT_TARGET}'"
            )
        endif()
    endif()

    foreach(_json_field IN ITEMS
            _rid
            LUA_CPP_PLATFORM_RELEASE_RUNNER
            _host_version
            _distribution_id
            _distribution_version
            CMAKE_SYSTEM_NAME
            CMAKE_SYSTEM_PROCESSOR
            CMAKE_GENERATOR
            CMAKE_CXX_COMPILER_ID
            CMAKE_CXX_COMPILER_VERSION
            CMAKE_VERSION
            _msvc_runtime
            _libc
            _libc_version
            _deployment_target)
        _lua_cpp_platform_json_escape(
            "${_json_field}_json"
            "${${_json_field}}"
        )
    endforeach()

    get_filename_component(
        _evidence_directory
        "${LUA_CPP_PLATFORM_EVIDENCE}"
        DIRECTORY
    )
    file(MAKE_DIRECTORY "${_evidence_directory}")
    file(WRITE "${LUA_CPP_PLATFORM_EVIDENCE}"
"{
  \"schema\": \"lua-cpp.platform-evidence/v1\",
  \"rid\": \"${_rid_json}\",
  \"runner\": \"${LUA_CPP_PLATFORM_RELEASE_RUNNER_json}\",
  \"host\": {
    \"system\": \"${CMAKE_HOST_SYSTEM_NAME}\",
    \"version\": \"${_host_version_json}\",
    \"distributionId\": \"${_distribution_id_json}\",
    \"distributionVersion\": \"${_distribution_version_json}\"
  },
  \"target\": {
    \"system\": \"${CMAKE_SYSTEM_NAME_json}\",
    \"processor\": \"${CMAKE_SYSTEM_PROCESSOR_json}\",
    \"pointerSize\": ${CMAKE_SIZEOF_VOID_P}
  },
  \"toolchain\": {
    \"generator\": \"${CMAKE_GENERATOR_json}\",
    \"compilerId\": \"${CMAKE_CXX_COMPILER_ID_json}\",
    \"compilerVersion\": \"${CMAKE_CXX_COMPILER_VERSION_json}\",
    \"cmakeVersion\": \"${CMAKE_VERSION_json}\"
  },
  \"runtime\": {
    \"msvcRuntime\": \"${_msvc_runtime_json}\",
    \"libc\": \"${_libc_json}\",
    \"libcVersion\": \"${_libc_version_json}\",
    \"deploymentTarget\": \"${_deployment_target_json}\"
  }
}
")
endfunction()
