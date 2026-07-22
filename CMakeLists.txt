cmake_minimum_required(VERSION 3.20)

if(POLICY CMP0184)
    cmake_policy(SET CMP0184 NEW)
endif()

project(lua_cpp VERSION 0.1.0 LANGUAGES C CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(LUA_CPP_BUILD_TOOLS "Build the interpreter and bytecode tools" ON)
option(LUA_CPP_BUILD_TESTS "Build the unit test runner" ON)
option(LUA_CPP_BUILD_BENCHMARKS "Build the runtime benchmark driver" OFF)
option(LUA_CPP_BUILD_FUZZERS "Build libFuzzer security-boundary targets" OFF)
option(LUA_CPP_BUILD_SHARED "Build the public C API shared library" ON)
option(LUA_CPP_ENABLE_COVERAGE "Enable Clang source-based coverage instrumentation" OFF)
set(LUA_CPP_SANITIZER "" CACHE STRING "Enable a runtime sanitizer: address, undefined, or thread")
set(LUA_CPP_LIBFUZZER_LIBRARY "" CACHE FILEPATH "Optional explicit libFuzzer runtime archive")
set_property(CACHE LUA_CPP_SANITIZER PROPERTY STRINGS "" address undefined thread)

if(LUA_CPP_SANITIZER)
    if(MSVC)
        message(FATAL_ERROR "LUA_CPP_SANITIZER is supported only by GCC and Clang builds")
    elseif(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "Unsupported compiler for LUA_CPP_SANITIZER: ${CMAKE_CXX_COMPILER_ID}")
    elseif(NOT LUA_CPP_SANITIZER MATCHES "^(address|undefined|thread)$")
        message(FATAL_ERROR "LUA_CPP_SANITIZER must be 'address', 'undefined', or 'thread'")
    endif()

    add_compile_options(-fsanitize=${LUA_CPP_SANITIZER} -fno-omit-frame-pointer)
    add_link_options(-fsanitize=${LUA_CPP_SANITIZER} -fno-omit-frame-pointer)
endif()

if(LUA_CPP_ENABLE_COVERAGE)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "LUA_CPP_ENABLE_COVERAGE requires Clang or AppleClang")
    endif()
    add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
    add_link_options(-fprofile-instr-generate -fcoverage-mapping)
endif()

if(LUA_CPP_BUILD_FUZZERS)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "LUA_CPP_BUILD_FUZZERS requires Clang or AppleClang")
    endif()
    if(LUA_CPP_SANITIZER STREQUAL "thread")
        message(FATAL_ERROR "libFuzzer's address/undefined instrumentation is incompatible with TSan")
    endif()
    add_compile_options(-fsanitize=fuzzer-no-link,address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

function(lua_configure_target_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /permissive- /utf-8 /FS)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
    endif()
endfunction()

set(LUA_CORE_SOURCES
    src/compiler/ast.cpp
    src/compiler/codegen/codegen.cpp
    src/compiler/codegen/codegen_binding.cpp
    src/compiler/codegen/codegen_stmt.cpp
    src/compiler/codegen/expression_emitter.cpp
    src/compiler/codegen/jump_patcher.cpp
    src/compiler/codegen/name_binder.cpp
    src/compiler/codegen/scope_manager.cpp
    src/compiler/codegen/statement_emitter.cpp
    src/compiler/lexer/lexer.cpp
    src/compiler/lexer/lexer_cursor.cpp
    src/compiler/opcode.cpp
    src/compiler/parser/parser.cpp
    src/compiler/parser/parser_expr.cpp
    src/compiler/parser/parser_func.cpp
    src/compiler/parser/parser_primary.cpp
    src/compiler/parser/parser_stmt.cpp
    src/compiler/parser/parser_table.cpp
    src/core/function.cpp
    src/core/gc_object.cpp
    src/core/gc_string.cpp
    src/core/metatable.cpp
    src/core/string_pool.cpp
    src/core/table.cpp
    src/core/upvalue.cpp
    src/core/userdata.cpp
    src/core/value.cpp
    src/core/thread.cpp
    src/api/lapi.cpp
    src/api/lauxlib.cpp
    src/gc/garbage_collector.cpp
    src/gc/gc_strategy.cpp
    src/gc/gc_mark.cpp
    src/gc/gc_sweep.cpp
    src/gc/gc_finalize.cpp
    src/gc/gc_weak.cpp
    src/io/dynamic_buffer.cpp
    src/io/file_loader.cpp
    src/io/input_stream.cpp
    src/lib/baselib.cpp
    src/lib/coroutinelib.cpp
    src/lib/debuglib.cpp
    src/lib/packagelib.cpp
    src/lib/iolib.cpp
    src/lib/lib_catalog.cpp
    src/lib/lib_manager.cpp
    src/lib/lib_registry.cpp
    src/lib/mathlib.cpp
    src/lib/oslib.cpp
    src/lib/stringlib.cpp
    src/lib/tablelib.cpp
    src/lib/testlib.cpp
    src/vm/state/global_state.cpp
    src/vm/state/lua_state.cpp
    src/vm/state/stack.cpp
    src/vm/vm_arith.cpp
    src/vm/vm_call.cpp
    src/vm/vm_dispatch_strategy.cpp
    src/vm/vm_handlers.cpp
    src/vm/vm_handlers/vm_handlers_arith.cpp
    src/vm/vm_handlers/vm_handlers_branch.cpp
    src/vm/vm_handlers/vm_handlers_call.cpp
    src/vm/vm_handlers/vm_handlers_closure.cpp
    src/vm/vm_handlers/vm_handlers_data.cpp
    src/vm/vm_handlers/vm_handlers_global_upvalue.cpp
    src/vm/vm_handlers/vm_handlers_loop.cpp
    src/vm/vm_handlers/vm_handlers_table.cpp
    src/vm/vm_handlers/vm_handlers_unary.cpp
    src/vm/vm_entry.cpp
    src/vm/vm_frame.cpp
    src/vm/vm_loop.cpp
    src/vm/vm_ops.cpp
    src/vm/vm_table.cpp
    src/vm/vm_trace.cpp
    src/vm/vm.cpp
    src/debug/value_serializer.cpp
    src/debug/json_trace_sink.cpp
    src/compiler/codegen/function_compiler.cpp
    src/runtime/native_module_registry.cpp
    src/runtime/bytecode_verifier.cpp
)

set(LUA_REPL_SOURCES
    src/repl.cpp
    src/repl/repl_comp.cpp
    src/repl/repl_ctx.cpp
    src/repl/repl_exe.cpp
    src/repl/repl_hist.cpp
    src/repl/repl_meta.cpp
    src/repl/repl_prompt.cpp
    src/repl/repl_sig.cpp
)

set(LUA_TEST_SOURCES
    src/app/app_options.cpp
    src/bytecode/bytecode_printer.cpp
    ${LUA_REPL_SOURCES}
    tests/unit/framework/test_framework.cpp
    tests/unit/framework/test_runner.cpp
    tests/unit/app/test_app_options.cpp
    tests/unit/app/test_repl_commands.cpp
    tests/unit/api/test_lua_c_api.cpp
    tests/unit/bytecode/test_bytecode_printer.cpp
    tests/unit/compiler/test_ast_visitor.cpp
    tests/unit/compiler/test_indexed_access.cpp
    tests/unit/compiler/test_lexer_lookahead.cpp
    tests/unit/compiler/test_lexer_number.cpp
    tests/unit/compiler/test_method_call.cpp
    tests/unit/compiler/test_parser_boundaries.cpp
    tests/unit/compiler/test_parser_error_recovery.cpp
    tests/unit/compiler/test_parser_memory_pool.cpp
    tests/unit/compiler/test_parser_recursion.cpp
    tests/unit/compiler/test_storevar.cpp
    tests/unit/compiler/test_lvalue_pipeline.cpp
    tests/unit/compiler/test_value_pipeline.cpp
    tests/unit/compiler/test_syntax_sugar.cpp
    tests/unit/compiler/test_binary_unary_expr.cpp
    tests/unit/compiler/test_codegen_characterization.cpp
    tests/unit/compiler/test_codegen_conditions.cpp
    tests/unit/compiler/test_codegen_multret.cpp
    tests/unit/compiler/test_call_pipeline.cpp
    tests/unit/compiler/test_symbol_binding.cpp
    tests/unit/compiler/test_codegen_result_types.cpp
    tests/unit/compiler/test_codegen_state.cpp
    tests/unit/compiler/test_jump_patcher.cpp
    tests/unit/compiler/test_scope_manager.cpp
    tests/unit/compiler/test_expression_emitter.cpp
    tests/unit/compiler/test_statement_emitter.cpp
    tests/unit/compiler/test_bytecode_builder.cpp
    tests/unit/compiler/test_function_codegen.cpp
    tests/unit/compiler/test_lua_functions.cpp
    tests/unit/core/test_function.cpp
    tests/unit/core/test_gc_string.cpp
    tests/unit/core/test_table.cpp
    tests/unit/core/test_value.cpp
    tests/unit/gc/test_gc.cpp
    tests/unit/io/test_dynamic_buffer.cpp
    tests/unit/io/test_file_loader.cpp
    tests/unit/io/test_input_stream_file.cpp
    tests/unit/io/test_input_stream_stream.cpp
    tests/unit/io/test_input_stream_string.cpp
    tests/unit/metamethod/test_metamethod_arith.cpp
    tests/unit/metamethod/test_metamethod_complete.cpp
    tests/unit/stdlib/test_baselib.cpp
    tests/unit/stdlib/test_coroutinelib.cpp
    tests/unit/stdlib/test_debuglib.cpp
    tests/unit/stdlib/test_lib_catalog.cpp
    tests/unit/stdlib/test_mathlib.cpp
    tests/unit/stdlib/test_packagelib.cpp
    tests/unit/stdlib/test_oslib.cpp
    tests/unit/stdlib/test_stringlib.cpp
    tests/unit/stdlib/test_tablelib.cpp
    tests/unit/vm/test_function_call.cpp
    tests/unit/vm/test_lua_state_init.cpp
    tests/unit/vm/test_runtime_services.cpp
    tests/unit/vm/test_vm_trace_debug.cpp
    tests/unit/vm/test_vm_dispatch.cpp
    tests/unit/vm/test_vm_internal_boundaries.cpp
    tests/unit/vm/test_vm_core.cpp
    tests/unit/official/test_official_suite.cpp
)

add_library(lua_core STATIC ${LUA_CORE_SOURCES})
add_library(LuaCpp::Lua ALIAS lua_core)
target_include_directories(lua_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
if(LUA_CPP_SANITIZER)
    # Static-library consumers must link the same sanitizer runtime that
    # instrumented lua_core. Keep this requirement on the exported target.
    target_link_options(lua_core INTERFACE -fsanitize=${LUA_CPP_SANITIZER})
endif()
if(LUA_CPP_ENABLE_COVERAGE)
    target_link_options(lua_core INTERFACE -fprofile-instr-generate -fcoverage-mapping)
endif()
set_target_properties(lua_core PROPERTIES EXPORT_NAME Lua)
if(CMAKE_DL_LIBS)
    target_link_libraries(lua_core PUBLIC ${CMAKE_DL_LIBS})
endif()

lua_configure_target_warnings(lua_core)

if(LUA_CPP_BUILD_SHARED)
    # Keep the public C ABI available as a production target, independent of
    # the test tree. The export manifests are also checked by the API contract.
    add_library(lua_public_api_shared SHARED ${LUA_CORE_SOURCES})
    add_library(LuaCpp::Shared ALIAS lua_public_api_shared)
    target_include_directories(lua_public_api_shared PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
    if(LUA_CPP_SANITIZER)
        target_link_options(lua_public_api_shared INTERFACE -fsanitize=${LUA_CPP_SANITIZER})
    endif()
    if(LUA_CPP_ENABLE_COVERAGE)
        target_link_options(lua_public_api_shared INTERFACE -fprofile-instr-generate -fcoverage-mapping)
    endif()
    if(CMAKE_DL_LIBS)
        target_link_libraries(lua_public_api_shared PRIVATE ${CMAKE_DL_LIBS})
    endif()
    if(WIN32)
        target_sources(lua_public_api_shared PRIVATE tests/compatibility/lua_public_api_exports.def)
    elseif(APPLE)
        # Mach-O prefixes C symbols with an underscore. Derive its export list
        # from the Windows manifest so the API contract has one symbol source.
        file(STRINGS tests/compatibility/lua_public_api_exports.def lua_public_api_export_lines)
        set(lua_public_api_apple_exports "")
        foreach(export_line IN LISTS lua_public_api_export_lines)
            string(STRIP "${export_line}" export_line)
            if(export_line STREQUAL "" OR export_line STREQUAL "EXPORTS")
                continue()
            endif()
            string(REGEX MATCH "^[A-Za-z_][A-Za-z0-9_]*" export_symbol "${export_line}")
            if(export_symbol)
                string(APPEND lua_public_api_apple_exports "_${export_symbol}\n")
            endif()
        endforeach()
        set(lua_public_api_apple_export_file "${CMAKE_CURRENT_BINARY_DIR}/lua_public_api_exports.apple")
        file(WRITE "${lua_public_api_apple_export_file}" "${lua_public_api_apple_exports}")
        target_link_options(lua_public_api_shared PRIVATE
            "SHELL:-Wl,-exported_symbols_list,${lua_public_api_apple_export_file}"
        )
    elseif(UNIX AND NOT APPLE)
        target_link_options(lua_public_api_shared PRIVATE
            "SHELL:-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/tests/compatibility/lua_public_api_exports.map"
        )
    endif()
    set_target_properties(lua_public_api_shared PROPERTIES
        EXPORT_NAME Shared
        OUTPUT_NAME lua_public_api
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
    lua_configure_target_warnings(lua_public_api_shared)
endif()

set(lua_cpp_install_targets lua_core)
if(TARGET lua_public_api_shared)
    list(APPEND lua_cpp_install_targets lua_public_api_shared)
endif()

install(TARGETS ${lua_cpp_install_targets}
    EXPORT LuaCppTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES
    src/lua.h
    src/lauxlib.h
    src/lualib.h
    src/lua_cpp_version.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/lua_cpp)

configure_package_config_file(
    cmake/LuaCppConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/LuaCppConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LuaCpp
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/LuaCppConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMinorVersion
)
install(EXPORT LuaCppTargets
    FILE LuaCppTargets.cmake
    NAMESPACE LuaCpp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LuaCpp
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/LuaCppConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/LuaCppConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LuaCpp
)

if(LUA_CPP_BUILD_FUZZERS)
    foreach(fuzzer_name IN ITEMS undump bytecode_verifier parser stdlib_numeric_arguments)
        add_executable(fuzz_${fuzzer_name} tests/fuzz/fuzz_${fuzzer_name}.cpp)
        target_link_libraries(fuzz_${fuzzer_name} PRIVATE lua_core)
        if(LUA_CPP_LIBFUZZER_LIBRARY)
            if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
                target_link_libraries(fuzz_${fuzzer_name} PRIVATE
                    "-Wl,--start-group"
                    "${LUA_CPP_LIBFUZZER_LIBRARY}"
                    stdc++
                    "-Wl,--end-group"
                )
            else()
                target_link_libraries(fuzz_${fuzzer_name} PRIVATE "${LUA_CPP_LIBFUZZER_LIBRARY}")
            endif()
            target_link_options(fuzz_${fuzzer_name} PRIVATE -fsanitize=address,undefined)
        else()
            target_link_options(fuzz_${fuzzer_name} PRIVATE -fsanitize=fuzzer,address,undefined)
        endif()
        target_include_directories(fuzz_${fuzzer_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
        lua_configure_target_warnings(fuzz_${fuzzer_name})
    endforeach()
endif()

if(LUA_CPP_BUILD_BENCHMARKS)
    add_executable(lua_runtime_bench benchmarks/runtime_bench.cpp)
    target_link_libraries(lua_runtime_bench PRIVATE lua_core)
    target_include_directories(lua_runtime_bench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    lua_configure_target_warnings(lua_runtime_bench)
endif()

function(lua_configure_runtime_stack target_name)
    if(MSVC)
        # Debug builds keep larger VM dispatch frames; Lua callbacks from C
        # library functions can recurse through the VM before Lua's own stack
        # guard is reached.
        target_link_options(${target_name} PRIVATE "/STACK:16777216")
    endif()
endfunction()

if(LUA_CPP_BUILD_TOOLS)
    add_executable(lua_app
        src/app/app_options.cpp
        src/bytecode/bytecode_printer.cpp
        src/main.cpp
        ${LUA_REPL_SOURCES}
    )
    target_link_libraries(lua_app PRIVATE lua_core)
    target_include_directories(lua_app PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    add_executable(lua_bytecode
        src/bytecode/bytecode_main.cpp
        src/bytecode/bytecode_printer.cpp
    )

    add_executable(lua_embedding_example examples/embedding.cpp)
    target_link_libraries(lua_embedding_example PRIVATE lua_core)
    target_include_directories(lua_embedding_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(lua_bytecode PRIVATE lua_core)
    target_include_directories(lua_bytecode PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    lua_configure_target_warnings(lua_app)
    lua_configure_target_warnings(lua_bytecode)
    lua_configure_target_warnings(lua_embedding_example)
    lua_configure_runtime_stack(lua_app)
    lua_configure_runtime_stack(lua_bytecode)
    lua_configure_runtime_stack(lua_embedding_example)
endif()

include(CTest)

if(BUILD_TESTING AND LUA_CPP_BUILD_BENCHMARKS)
    add_test(
        NAME runtime_benchmark_contract
        COMMAND $<TARGET_FILE:lua_runtime_bench>
            --profile ci
            --json ${CMAKE_CURRENT_BINARY_DIR}/runtime-bench-ci.json
    )
    set_tests_properties(runtime_benchmark_contract PROPERTIES
        LABELS benchmark
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        TIMEOUT 900
    )
endif()

if(BUILD_TESTING AND LUA_CPP_BUILD_TESTS)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    add_executable(lua_test ${LUA_TEST_SOURCES})
    target_link_libraries(lua_test PRIVATE lua_core)
    target_include_directories(lua_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/framework
        ${CMAKE_CURRENT_SOURCE_DIR}/lua_test/include
    )

    lua_configure_target_warnings(lua_test)
    lua_configure_runtime_stack(lua_test)
    if(UNIX AND NOT MSVC)
        target_link_options(lua_test PRIVATE -rdynamic)
    endif()

    add_executable(lua_public_api_consumer
        tests/compatibility/public_api_c_compile.c
        tests/compatibility/public_api_cpp_consumer.cpp
    )
    target_link_libraries(lua_public_api_consumer PRIVATE lua_core)
    target_include_directories(lua_public_api_consumer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    if(MSVC)
        # This consumer intentionally proves the optimized /EHsc ABI even
        # when the surrounding test tree is configured as Debug.
        set_property(TARGET lua_public_api_consumer PROPERTY MSVC_RUNTIME_CHECKS "")
        target_compile_options(lua_public_api_consumer PRIVATE
            $<$<COMPILE_LANGUAGE:C>:/W4>
            $<$<COMPILE_LANGUAGE:CXX>:/W4>
            $<$<COMPILE_LANGUAGE:CXX>:/permissive->
            $<$<COMPILE_LANGUAGE:CXX>:/utf-8>
            $<$<COMPILE_LANGUAGE:CXX>:/O2>
            $<$<COMPILE_LANGUAGE:CXX>:/EHsc>
        )
    else()
        target_compile_options(lua_public_api_consumer PRIVATE
            $<$<COMPILE_LANGUAGE:C>:-Wall>
            $<$<COMPILE_LANGUAGE:C>:-Wextra>
            $<$<COMPILE_LANGUAGE:C>:-Wpedantic>
            $<$<COMPILE_LANGUAGE:CXX>:-Wall>
            $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
            $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
            $<$<COMPILE_LANGUAGE:CXX>:-Wconversion>
            $<$<COMPILE_LANGUAGE:CXX>:-O2>
        )
    endif()
    lua_configure_runtime_stack(lua_public_api_consumer)

    if(TARGET lua_public_api_shared)
        add_executable(lua_public_api_shared_consumer
            tests/compatibility/public_api_c_compile.c
            tests/compatibility/public_api_cpp_consumer.cpp
        )
        target_link_libraries(lua_public_api_shared_consumer PRIVATE lua_public_api_shared)
        target_include_directories(lua_public_api_shared_consumer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
        if(MSVC)
            set_property(TARGET lua_public_api_shared_consumer PROPERTY MSVC_RUNTIME_CHECKS "")
            target_compile_options(lua_public_api_shared_consumer PRIVATE
                $<$<COMPILE_LANGUAGE:C>:/W4>
                $<$<COMPILE_LANGUAGE:CXX>:/W4>
                $<$<COMPILE_LANGUAGE:CXX>:/permissive->
                $<$<COMPILE_LANGUAGE:CXX>:/utf-8>
                $<$<COMPILE_LANGUAGE:CXX>:/O2>
                $<$<COMPILE_LANGUAGE:CXX>:/EHsc>
            )
        else()
            target_compile_options(lua_public_api_shared_consumer PRIVATE
                $<$<COMPILE_LANGUAGE:C>:-Wall>
                $<$<COMPILE_LANGUAGE:C>:-Wextra>
                $<$<COMPILE_LANGUAGE:C>:-Wpedantic>
                $<$<COMPILE_LANGUAGE:CXX>:-Wall>
                $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
                $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
                $<$<COMPILE_LANGUAGE:CXX>:-Wconversion>
                $<$<COMPILE_LANGUAGE:CXX>:-O2>
            )
        endif()
        lua_configure_runtime_stack(lua_public_api_shared_consumer)
    endif()

    add_executable(lua51_c_api_differential_probe tests/compatibility/lua51_c_api_differential_probe.c)
    target_link_libraries(lua51_c_api_differential_probe PRIVATE lua_core)
    target_include_directories(lua51_c_api_differential_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    set_target_properties(lua51_c_api_differential_probe PROPERTIES LINKER_LANGUAGE CXX)
    if(MSVC)
        target_compile_options(lua51_c_api_differential_probe PRIVATE /W4)
    else()
        target_compile_options(lua51_c_api_differential_probe PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    lua_configure_runtime_stack(lua51_c_api_differential_probe)

    add_executable(lua_public_module_host tests/compatibility/public_native_module_host.cpp)
    target_link_libraries(lua_public_module_host PRIVATE lua_core)
    target_include_directories(lua_public_module_host PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    set_target_properties(lua_public_module_host PROPERTIES ENABLE_EXPORTS ON)
    # Let the lifecycle host exercise a loader search-name alias before the
    # fixture's absolute path; the registry must collapse both OS handles.
    if(APPLE)
        set_property(TARGET lua_public_module_host PROPERTY BUILD_RPATH "@executable_path")
    elseif(UNIX)
        set_property(TARGET lua_public_module_host PROPERTY BUILD_RPATH "$ORIGIN")
    endif()
    if(MSVC)
        target_sources(lua_public_module_host PRIVATE tests/compatibility/lua_public_api_exports.def)
    endif()
    lua_configure_target_warnings(lua_public_module_host)
    lua_configure_runtime_stack(lua_public_module_host)

    add_library(lua_public_native_module_host MODULE tests/compatibility/public_native_module.c)
    target_link_libraries(lua_public_native_module_host PRIVATE lua_public_module_host)
    target_include_directories(lua_public_native_module_host PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    set_target_properties(lua_public_native_module_host PROPERTIES PREFIX "" OUTPUT_NAME "publicfixture_host")
    lua_configure_target_warnings(lua_public_native_module_host)

    add_test(
        NAME lua51_public_api_contract
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_lua51_public_api_contract.py
    )
    set_tests_properties(lua51_public_api_contract PROPERTIES
        LABELS api-contract
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    add_test(NAME lua_public_api_consumer COMMAND $<TARGET_FILE:lua_public_api_consumer>)
    set_tests_properties(lua_public_api_consumer PROPERTIES
        LABELS api-contract
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    if(TARGET lua_public_api_shared_consumer)
        add_test(NAME lua_public_api_shared_consumer COMMAND $<TARGET_FILE:lua_public_api_shared_consumer>)
        set_tests_properties(lua_public_api_shared_consumer PROPERTIES
            LABELS "api-contract;shared-library"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()

    add_test(
        NAME cmake_package_consumer
        COMMAND ${CMAKE_COMMAND}
            -DPROJECT_BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}
            -DPROJECT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
            -DTEST_CONFIGURATION=$<CONFIG>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/packaging/verify_package.cmake
    )
    set_tests_properties(cmake_package_consumer PROPERTIES
        LABELS "api-contract;package"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        TIMEOUT 180
    )

    add_test(NAME lua51_c_api_differential_probe COMMAND $<TARGET_FILE:lua51_c_api_differential_probe>)
    set_tests_properties(lua51_c_api_differential_probe PROPERTIES
        LABELS "api-contract;c-api-differential"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    add_test(
        NAME lua_public_native_module_host
        COMMAND $<TARGET_FILE:lua_public_module_host> $<TARGET_FILE:lua_public_native_module_host>
    )
    set_tests_properties(lua_public_native_module_host PROPERTIES
        LABELS "api-contract;native-module"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    if(LUA_CPP_BUILD_TOOLS)
        set_target_properties(lua_app PROPERTIES ENABLE_EXPORTS ON)
        if(MSVC)
            target_sources(lua_app PRIVATE tests/compatibility/lua_public_api_exports.def)
        elseif(APPLE)
            target_link_options(lua_app PRIVATE -Wl,-u,_lua_createtable)
        else()
            target_link_options(lua_app PRIVATE -Wl,--undefined=lua_createtable)
        endif()

        add_library(lua_public_native_module_app MODULE tests/compatibility/public_native_module.c)
        target_link_libraries(lua_public_native_module_app PRIVATE lua_app)
        target_include_directories(lua_public_native_module_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
        set_target_properties(lua_public_native_module_app PROPERTIES PREFIX "" OUTPUT_NAME "publicfixture_app")
        lua_configure_target_warnings(lua_public_native_module_app)

        add_test(
            NAME lua_public_native_module_app
            COMMAND $<TARGET_FILE:lua_app>
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/compatibility/public_native_module_app.lua
                $<TARGET_FILE:lua_public_native_module_app>
        )
        set_tests_properties(lua_public_native_module_app PROPERTIES
            LABELS "api-contract;native-module"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )

        set_target_properties(lua_embedding_example PROPERTIES ENABLE_EXPORTS ON)
        if(MSVC)
            target_sources(lua_embedding_example PRIVATE tests/compatibility/lua_public_api_exports.def)
        endif()

        add_library(lua_public_native_module_embedding MODULE tests/compatibility/public_native_module.c)
        target_link_libraries(lua_public_native_module_embedding PRIVATE lua_embedding_example)
        target_include_directories(lua_public_native_module_embedding PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
        set_target_properties(lua_public_native_module_embedding PROPERTIES
            PREFIX ""
            OUTPUT_NAME "publicfixture_embedding"
        )
        lua_configure_target_warnings(lua_public_native_module_embedding)

        add_test(
            NAME lua_public_native_module_embedding
            COMMAND $<TARGET_FILE:lua_embedding_example> $<TARGET_FILE:lua_public_native_module_embedding>
        )
        set_tests_properties(lua_public_native_module_embedding PROPERTIES
            LABELS "api-contract;native-module"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()

    add_test(
        NAME lua_test
        COMMAND $<TARGET_FILE:lua_test> --exclude-filter "Lua 5.1 Official"
    )
    set_tests_properties(lua_test PROPERTIES
        LABELS unit
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    add_test(
        NAME official_smoke
        COMMAND $<TARGET_FILE:lua_test> --filter "Lua 5.1 Official Smoke"
    )
    set_tests_properties(official_smoke PROPERTIES
        LABELS official-smoke
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        TIMEOUT 180
    )

    add_test(
        NAME official_testc
        COMMAND $<TARGET_FILE:lua_test> --filter "Lua 5.1 Official TestC"
    )
    set_tests_properties(official_testc PROPERTIES
        LABELS official-testc
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        TIMEOUT 180
    )

    if(LUA_CPP_BUILD_TOOLS)
        set_tests_properties(official_smoke official_testc PROPERTIES
            ENVIRONMENT "LUA_TEST_APP_PATH=$<TARGET_FILE:lua_app>"
        )
    endif()

    if(LUA_CPP_BUILD_TOOLS)
        add_test(NAME example_hello COMMAND $<TARGET_FILE:lua_app> ${CMAKE_CURRENT_SOURCE_DIR}/examples/hello.lua)
        add_test(NAME example_control_flow COMMAND $<TARGET_FILE:lua_app> ${CMAKE_CURRENT_SOURCE_DIR}/examples/control_flow.lua)
        add_test(NAME example_tables_and_methods COMMAND $<TARGET_FILE:lua_app> ${CMAKE_CURRENT_SOURCE_DIR}/examples/tables_and_methods.lua)
        add_test(NAME example_metamethods COMMAND $<TARGET_FILE:lua_app> ${CMAKE_CURRENT_SOURCE_DIR}/examples/metamethods.lua)
        add_test(NAME example_embedding COMMAND $<TARGET_FILE:lua_embedding_example>)
        set_tests_properties(
            example_hello
            example_control_flow
            example_tables_and_methods
            example_metamethods
            example_embedding
            PROPERTIES
                LABELS examples
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()
endif()
