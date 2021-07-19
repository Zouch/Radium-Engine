cmake_minimum_required(VERSION 3.13)

#------------------------------------------------------------------------------
# Policies and global parameters for CMake
if(POLICY CMP0077)
  cmake_policy(SET CMP0077 NEW)
endif()

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# Enable helpfull warnings and C++17 for all files
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Project setup, versioning stuff here, change when changing the version
# Note: keep the project name lower case only for easy linux packaging support
project (radiumproject VERSION 1.0.0)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompilerVersion.cmake)

set(VERSION_TYPE "beta" CACHE STRING "version type" FORCE)
site_name(VERSION_HOST) # read hostname to VERSION_HOST
set(VERSION_HOST "${VERSION_HOST}" CACHE STRING "host of build" FORCE)

set(RADIUM_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
set(RADIUM_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})

option(RADIUM_ENABLE_TESTING          "Enable testing. Tests are automatically built with target all, run with target check or test" ON)
option(RADIUM_ENABLE_COVERAGE         "Enable coverage, gcc only. Experimental, need ENABLE_TESTING" OFF)
option(RADIUM_USE_DOUBLE              "Use double precision for Scalar" OFF)
option(RADIUM_GENERATE_LIB_CORE       "Include Radium::Core in CMake project" ON)
option(RADIUM_GENERATE_LIB_IO         "Include Radium::IO in CMake project" ON)
option(RADIUM_GENERATE_LIB_ENGINE     "Include Radium::Engine in CMake project" ON)
option(RADIUM_GENERATE_LIB_GUI        "Include Radium::Gui in CMake project" ON)
option(RADIUM_GENERATE_LIB_PLUGINBASE "Include Radium::PluginBase in CMake project" ON)
option(RADIUM_UPDATE_VERSION          "Update version file each time the project is compiled (update compilation time in version.cpp)" ON)
option(RADIUM_INSTALL_DOC             "Install documentation. If RadiumDoc is compiled, install documentation to bundle directory for install target" ON)

set(RADIUM_EXTERNAL_CMAKE_INSTALL_MESSAGE NEVER)

set(RADIUM_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
set(RADIUM_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})

set(DISPLAY_WIDTH 80)

if(RADIUM_USE_DOUBLE)
    add_definitions(-DCORE_USE_DOUBLE)
endif()
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

# We can use include() and find_package() for our scripts in there
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(MessageFunc)
message_title(" ${PROJECT_NAME} Project configuration ")
message_info("Externals will be built with $ENV{CMAKE_BUILD_PARALLEL_LEVEL} core(s)")

#------------------------------------------------------------------------------
# General settings


if(NOT CMAKE_BUILD_TYPE)
  message_info("No CMAKE_BUILD_TYPE, using default CMAKE_BUILD_TYPE=Release")
  set(CMAKE_BUILD_TYPE "Release")
endif()

# Be nice to visual studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Be nice and export compile commands by default, this is handy for clang-tidy
# and for other tools.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Configure RPath, see cmake/RPath.cmake
include(RPath)

# Use gold linker to speed up linking time, see cmake/UseGoldLinker.cmake
include(UseGoldLinker)

# Helpful option enable build profiling to identify slowly compiling files
option(MEASURE_ALL "When enabled all commands will be passed through time command" OFF)
if(MEASURE_ALL)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "time")
endif()

# Append library and executable names with d in debug mode
set(CMAKE_DEBUG_POSTFIX d)

#-------------------------------------------------------------------------------
# Set default paths for Radium
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(RADIUM_BUNDLE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Bundle-${CMAKE_CXX_COMPILER_ID})
else ()
    set(RADIUM_BUNDLE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Bundle-${CMAKE_CXX_COMPILER_ID}-${CMAKE_BUILD_TYPE})
endif ()
set(RADIUM_EXTERNAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external")
set(RADIUM_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(RADIUM_SHADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Shaders")

#-------------------------------------------------------------------------------
# Set default install location to RADIUM_BUNDLE_DIRECTORY
# we do not want to install to /usr by default
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${RADIUM_BUNDLE_DIRECTORY}" CACHE PATH
        "Install path prefix, prepended onto install directories." FORCE)
    message_info("Set install prefix to ${CMAKE_INSTALL_PREFIX}")
    # prevent subsequent modification of CMAKE_INSTALL_PREFIX
    # based on CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT value
    unset(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
endif()

#------------------------------------------------------------------------------
# Custom Install target, used in run target in source/CMakeLists.txt
add_custom_target( Install_ COMMAND "${CMAKE_COMMAND}" --build . --target
    install WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

#------------------------------------------------------------------------------
# get changeset id
find_package(Git QUIET)

if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    option(RADIUM_GIT_UPDATE_SUBMODULE         "Check submodules during build (will be automatically disabled after run)" ON)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_CHANGESET)
    if ( GIT_CHANGESET )
      # remove new line sometime appearing in git changeset
      string(REGEX REPLACE "\n$" "" GIT_CHANGESET "${GIT_CHANGESET}")
    endif()
else()
  set(GIT_CHANGESET "")
endif()
message_info("Git Changeset: ${GIT_CHANGESET}")

message_end()

set(ENABLE_COVERAGE "")
mark_as_advanced(ENABLE_COVERAGE)
if(RADIUM_ENABLE_COVERAGE)
    include(CoverageFunc)
    #sets ENABLE_COVERAGE to ON if ok
    setup_coverage()
    
    include(CodeCoverage)
    append_coverage_compiler_flags()
endif(RADIUM_ENABLE_COVERAGE)

#------------------------------------------------------------------------------
# Dependency management utilities
include(ExternalFunc)
# Installation utilities
include(RadiumSetupFunctions)

# Documentation build
add_subdirectory(doc)

# This var gather local dependencies in subdirectories
set(LocalDependencies)

# Images, databases and other data which needs to be installed for project
#add_subdirectory(data)

# Source code
add_subdirectory(src)

# Add this target so that some IDE could propose to build all radium libs and examples at once
add_custom_target(RadiumLibs DEPENDS Core Engine Gui IO PluginBase)

# add a target allowing to force the installation of the library

set(RadiumForceInstallCommutators "")
if (MSVC)
        set(RadiumForceInstallCommutators "/p:Configuration=${CMAKE_BUILD_TYPE}")
endif(MSVC)

add_custom_target(ForceRadiumInstallation
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target install -- ${RadiumForceInstallCommutators}
    COMMENT "Forcing Radium installation")

# Testing
if (RADIUM_ENABLE_TESTING)
    enable_testing()
    include(CTest)
    add_subdirectory(tests)
    add_dependencies(RadiumLibs CoreExampleApp RadiumExampleApps)
endif ()

if(ENABLE_COVERAGE)
    set(LCOV_REMOVES "'/usr/*';'*external*';'*install*';'*/Catch2/*';'*_autogen/*';'*build*'")
    setup_coverage_targets(${ENABLE_COVERAGE} "${LCOV_REMOVES}")

    # Fastcov is not supported with gcov llvm: disabling for MacOS
    # Source: https://github.com/RPGillespie6/fastcov/issues/36
    if(UNIX AND NOT APPLE AND FASTCOV_PATH)
        setup_target_for_coverage_fastcov(NAME coverage_fastcov
            EXECUTABLE ctest -j $ENV{CMAKE_BUILD_PARALLEL_LEVEL}
            DEPENDENCIES RadiumLibs RadiumExampleApps unittests integration
            EXCLUDE "/usr/" "external" "install" "/Catch2/" "_autogen/" "build" "mocs")
    endif()
endif()

# Packaging stuff (deb, rpm, windows installer)
#add_subdirectory(packaging)

install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_PREFIX} COMPONENT meta)
install(FILES README.md DESTINATION ${CMAKE_INSTALL_PREFIX} COMPONENT meta)

#-------------------------------------------------------------------------------
# Wrap up of settings printed on build

message_title(" Final overview for ${PROJECT_NAME} ")
message_info("Version:               ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH} ${VERSION_TYPE} @ ${VERSION_HOST}")
message_info("Install prefix: ")
message_info("  ${CMAKE_INSTALL_PREFIX}")
message_info("Compiler: ${CMAKE_CXX_COMPILER} - ${CMAKE_CXX_COMPILER_ID} in version ${CMAKE_CXX_COMPILER_VERSION}.")
message_setting("CMAKE_BUILD_TYPE")
message_info("  possible options: Debug Release RelWithDebInfo MinSizeRel")
message_info("  set with ` cmake -DCMAKE_BUILD_TYPE=Debug .. `")
message_setting("RADIUM_ENABLE_TESTING")
message_setting("RADIUM_ENABLE_COVERAGE")
message_setting("RADIUM_USE_DOUBLE")
message_setting("RADIUM_GENERATE_LIB_CORE")
message_setting("RADIUM_GENERATE_LIB_ENGINE")
message_setting("RADIUM_GENERATE_LIB_GUI")
message_setting("RADIUM_GENERATE_LIB_IO")
message_setting("RADIUM_GENERATE_LIB_PLUGINBASE")
message_setting("RADIUM_GIT_UPDATE_SUBMODULE")
message_setting("RADIUM_IO_ASSIMP")
message_setting("RADIUM_IO_TINYPLY")
message_setting("RADIUM_IO_DEPRECATED")
message_setting("RADIUM_SKIP_CORE_EXTERNAL")
message_setting("RADIUM_SKIP_ENGINE_EXTERNAL")
message_setting("RADIUM_SKIP_IO_EXTERNAL")
message_setting("RADIUM_INSTALL_DOC")
message_setting("RADIUM_UPDATE_VERSION")
message_setting("RADIUM_QUIET")
message_setting("USE_GOLD_LINKER")
message_end()

set( CACHED_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}" CACHE INTERNAL "Previous value of CMAKE_INSTALL_PREFIX")
