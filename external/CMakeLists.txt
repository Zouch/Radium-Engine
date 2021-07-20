cmake_minimum_required(VERSION 3.13)
set(RADIUM_DEPENDENCIES_PROJECT_NAME radium_externals)
project(${RADIUM_DEPENDENCIES_PROJECT_NAME})

# prevent this cmake script to be used through a add_subdirectory of another project than
# ${RADIUM_DEPENDENCIES_PROJECT_NAME} ...
if(NOT CMAKE_PROJECT_NAME STREQUAL ${RADIUM_DEPENDENCIES_PROJECT_NAME})
    message(
        FATAL_ERROR
            "This file must be used as a root project and not
  included as subdir in any other project (included from
  ${CMAKE_PROJECT_NAME} !)"
    )
else()
    set(CMAKE_DISABLE_SOURCE_CHANGES ON)
    set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    project(${RADIUM_DEPENDENCIES_PROJECT_NAME} VERSION 1.0.0)

    # We can use include() and find_package() for our scripts in there
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)

    # Check compiler version
    include(CompilerVersion)

    # Changing the default value for CMAKE_BUILD_PARALLEL_LEVEL
    if(NOT DEFINED ENV{CMAKE_BUILD_PARALLEL_LEVEL})
        include(ProcessorCount)
        ProcessorCount(N)
        if(NOT N EQUAL 0)
            set(CTEST_BUILD_FLAGS -j${N})
            set(ctest_test_args ${ctest_test_args} PARALLEL_LEVEL ${N})
            set(ENV{CMAKE_BUILD_PARALLEL_LEVEL} ${N})
        endif()
    endif()

    message(STATUS "")
    message(STATUS "    == ${PROJECT_NAME} Project configuration ==")
    message(STATUS "")
    message(STATUS "Externals will be built with $ENV{CMAKE_BUILD_PARALLEL_LEVEL} core(s)")

    # -----------------------------------------------------------------------------
    # Set default install location to dist folder in build dir we do not want to install to /usr by
    # default
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release)
        message(STATUS "Set default build type to ${CMAKE_BUILD_TYPE}")
    endif()

    # -----------------------------------------------------------------------------

    # Set default install location to RADIUM_EXTERNAL_INSTALL
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(RADIUM_EXTERNAL_INSTALL ${CMAKE_CURRENT_BINARY_DIR}/Bundle-${CMAKE_CXX_COMPILER_ID})
    else()
        set(RADIUM_EXTERNAL_INSTALL
            ${CMAKE_CURRENT_BINARY_DIR}/Bundle-${CMAKE_CXX_COMPILER_ID}-${CMAKE_BUILD_TYPE}
        )
    endif()

    # -----------------------------------------------------------------------------
    # we do not want to install to /usr by default
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "${RADIUM_EXTERNAL_INSTALL}"
            CACHE PATH "Install path prefix, prepended onto install directories." FORCE
        )
    endif()

    # Ensure that external dependencies built as standalone are not installed in a subdir of Radium
    # source tree
    string(LENGTH ${CMAKE_SOURCE_DIR} SourceDirLength)
    string(LENGTH "external/" ExternalDirLength)
    math(EXPR RadiumPrefixLength " ${SourceDirLength} - ${ExternalDirLength}")
    string(SUBSTRING ${CMAKE_SOURCE_DIR} 0 ${RadiumPrefixLength} RadiumPREFIX)
    string(FIND ${CMAKE_INSTALL_PREFIX} ${RadiumPREFIX} IsPrefix)
    if(${IsPrefix} GREATER -1)
        message(
            FATAL_ERROR "Externals can't be installed into a subdirectory of Radium source tree."
        )
    endif()

    message(STATUS "Set install prefix to ${CMAKE_INSTALL_PREFIX}")

    # add core_dependencies
    add_subdirectory(Core)
    # add Engine dependencies
    add_subdirectory(Engine)
    # set dependencies between external targets
    add_dependencies(EngineExternals CoreExternals)
    # add IO dependencies, all activated by default by this cmake script
    option(RADIUM_IO_ASSIMP "Provide loaders based on Assimp library" ON)
    option(RADIUM_IO_TINYPLY "Provide loaders based on TinyPly library" ON)
    add_subdirectory(IO)
endif()
