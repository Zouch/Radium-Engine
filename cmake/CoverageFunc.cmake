# setup coverage
macro(setup_coverage)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "[Tests] Enable Coverage ¡Warning! slow down execution ¡Warning!")
        set(CMAKE_CXX_FLAGS
            "${CMAKE_CXX_FLAGS} -g -O0 -Wall -W -Wshadow -Wunused-variable -Wunused-parameter -Wunused-function -Wunused -Wno-system-headers -Wno-deprecated -Woverloaded-virtual -Wwrite-strings -fprofile-arcs -ftest-coverage"
        )
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -Wall -W -fprofile-arcs -ftest-coverage")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
        find_program(LCOV_BIN lcov)

        # adapted from https://github.com/RWTH-HPC/CMake-codecov/blob/master/cmake/FindGcov.cmake
        get_property(ENABLED_LANGUAGES GLOBAL PROPERTY ENABLED_LANGUAGES)
        foreach(LANG ${ENABLED_LANGUAGES})
            # Gcov evaluation is dependent on the used compiler. Check gcov support for each
            # compiler that is used. If gcov binary was already found for this compiler, do not try
            # to find it again.
            if(NOT GCOV_${CMAKE_${LANG}_COMPILER_ID}_BIN)
                get_filename_component(COMPILER_PATH "${CMAKE_${LANG}_COMPILER}" PATH)

                if("${CMAKE_${LANG}_COMPILER_ID}" STREQUAL "GNU")
                    # Some distributions like OSX (homebrew) ship gcov with the compiler version
                    # appended as gcov-x. To find this binary we'll build the suggested binary name
                    # with the compiler version.
                    string(REGEX MATCH "^[0-9]+" GCC_VERSION "${CMAKE_${LANG}_COMPILER_VERSION}")

                    find_program(GCOV_BIN NAMES gcov-${GCC_VERSION} gcov HINTS ${COMPILER_PATH})

                elseif("${CMAKE_${LANG}_COMPILER_ID}" MATCHES "^(Apple)?Clang$")
                    # Some distributions like Debian ship llvm-cov with the compiler version
                    # appended as llvm-cov-x.y. To find this binary we'll build the suggested binary
                    # name with the compiler version.
                    string(REGEX MATCH "^[0-9]+.[0-9]+" LLVM_VERSION
                                 "${CMAKE_${LANG}_COMPILER_VERSION}"
                    )

                    # llvm-cov prior version 3.5 seems to be not working with coverage evaluation
                    # tools, but these versions are compatible with the gcc gcov tool.
                    if(LLVM_VERSION VERSION_GREATER 3.4)
                        find_program(
                            LLVM_COV_BIN NAMES "llvm-cov-${LLVM_VERSION}" "llvm-cov"
                            HINTS ${COMPILER_PATH}
                        )
                        mark_as_advanced(LLVM_COV_BIN)

                        if(LLVM_COV_BIN)
                            find_program(
                                LLVM_COV_WRAPPER "llvm-cov-wrapper" PATHS ${CMAKE_MODULE_PATH}
                            )
                            if(LLVM_COV_WRAPPER)
                                set(GCOV_BIN "${LLVM_COV_WRAPPER}" CACHE FILEPATH "")

                                # set additional parameters
                                set(GCOV_${CMAKE_${LANG}_COMPILER_ID}_ENV
                                    "LLVM_COV_BIN=${LLVM_COV_BIN}"
                                    CACHE STRING "Environment variables for llvm-cov-wrapper."
                                )
                                mark_as_advanced(GCOV_${CMAKE_${LANG}_COMPILER_ID}_ENV)
                            endif()
                        endif()
                    endif()

                    if(NOT GCOV_BIN)
                        # Fall back to gcov binary if llvm-cov was not found or is incompatible.
                        # This is the default on OSX, but may crash on recent Linux versions.
                        find_program(GCOV_BIN gcov HINTS ${COMPILER_PATH})
                    endif()
                endif()

                if(GCOV_BIN)
                    set(GCOV_${CMAKE_${LANG}_COMPILER_ID}_BIN "${GCOV_BIN}"
                        CACHE STRING "${LANG} gcov binary."
                    )

                    if(NOT CMAKE_REQUIRED_QUIET)
                        message(
                            "-- Found gcov evaluation for ${LANG}"
                            "${CMAKE_${LANG}_COMPILER_ID}: ${GCOV_BIN}    ${GCOV_${CMAKE_${LANG}_COMPILER_ID}_BIN}"
                        )
                    endif()
                    set(GCOV_BIN_FOR_LCOV ${GCOV_BIN} CACHE STRING "save gcov bin for lcov")
                    unset(GCOV_BIN CACHE)
                endif()
            endif()
        endforeach()

        if(NOT LCOV_BIN AND NOT GCOV_BIN_FOR_LCOV)
            message(
                FATAL_ERROR
                    "lcov binary not found, while asking for coverage computation (RADIUM_ENABLE_COVERAGE=${RADIUM_ENABLE_COVERAGE}). Abort"
            )
        endif()
        set(RADIUM_ENABLE_TESTING "ON")
        set(ENABLE_COVERAGE "ON")
    else()
        message(
            FATAL_ERROR
                "[Tests] Coverage requested (RADIUM_ENABLE_COVERAGE), but not available, only supported in DEBUG with GCC (i.e. GNU/Debug). Current configuration is ${CMAKE_CXX_COMPILER_ID}/${CMAKE_BUILD_TYPE}"
        )
    endif()
endmacro()

# might be needed, was needed ...
# https://stackoverflow.com/questions/37434946/how-do-i-iterate-over-all-cmake-targets-programmatically
function(get_all_targets var)
    set(targets)
    get_all_targets_recursive(targets ${CMAKE_CURRENT_SOURCE_DIR})
    set(${var} ${targets} PARENT_SCOPE)
endfunction()

macro(get_all_targets_recursive targets dir)
    get_property(subdirectories DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
    foreach(subdir ${subdirectories})
        get_all_targets_recursive(${targets} ${subdir})
    endforeach()

    get_property(current_targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)

    foreach(target ${current_targets})
        get_target_property(type ${target} TYPE)
        if(${type} STREQUAL "EXECUTABLE" OR ${type} STREQUAL "SHARED_LIBRARY")
            list(APPEND ${targets} ${target})
        endif()
    endforeach()
endmacro()

macro(setup_coverage_targets ENABLE_COVERAGE LCOV_REMOVES)
    if(ENABLE_COVERAGE)
        add_custom_target(
            lcov_init
            COMMAND ${LCOV_BIN} --gcov-tool ${GCOV_BIN_FOR_LCOV} --initial --capture --directory
                    ${CMAKE_BINARY_DIR} --output-file ${CMAKE_BINARY_DIR}/init.info
            COMMAND
                ${LCOV_BIN} --gcov-tool ${GCOV_BIN_FOR_LCOV} --remove ${CMAKE_BINARY_DIR}/init.info
                ${LCOV_REMOVES} --output-file ${CMAKE_BINARY_DIR}/init.info
            BYPRODUCTS ${CMAKE_BINARY_DIR}/init.info
        )
        add_custom_target(
            lcov_zerocounter COMMAND ${LCOV_BIN} --gcov-tool ${GCOV_BIN_FOR_LCOV} --zerocounter
                                     --directory ${CMAKE_BINARY_DIR}
        )
        add_custom_target(
            lcov_capture
            COMMAND ${LCOV_BIN} --gcov-tool ${GCOV_BIN_FOR_LCOV} --capture --directory .
                    --output-file ${CMAKE_BINARY_DIR}/coverage.info
            COMMAND ${LCOV_BIN} --gcov-tool ${GCOV_BIN_FOR_LCOV} --remove coverage.info
                    ${LCOV_REMOVES} --output-file ${CMAKE_BINARY_DIR}/coverage.info
            COMMAND ${LCOV_BIN} --gcov-tool ${GCOV_BIN_FOR_LCOV} -a ${CMAKE_BINARY_DIR}/init.info -a
                    ${CMAKE_BINARY_DIR}/coverage.info -o ${CMAKE_BINARY_DIR}/total.info
            BYPRODUCTS ${CMAKE_BINARY_DIR}/total.info
        )
        add_custom_target(
            lcov_list COMMAND ${LCOV_BIN} --gcov-tool ${GCOV_BIN_FOR_LCOV} --list
                              ${CMAKE_BINARY_DIR}/total.info
        )
        add_custom_target(
            coverage_lcov
            COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target all --parallel
            COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target lcov_init --parallel
            COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target lcov_zerocounter
                    --parallel
            COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target check --parallel
            COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target lcov_capture --parallel
        )
        add_custom_target(
            lcov_remove COMMAND ${LCOV_BIN} --gcov-tool ${GCOV_BIN_FOR_LCOV} --remove coverage.info
                                ${LCOV_REMOVES} --output-file ${CMAKE_BINARY_DIR}/coverage.info
        )

        find_program(GENHTML_BIN genhtml)
        if(GENHTML_BIN)
            add_custom_target(
                coverage_report COMMAND ${GENHTML_BIN} -o ${CMAKE_BINARY_DIR}/lcov total.info
            )
        endif()
    endif()
endmacro()
