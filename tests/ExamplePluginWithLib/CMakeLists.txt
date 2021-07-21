cmake_minimum_required(VERSION 3.6)
# ------------------------------------------------------------------------------
# Policies and global parameters for CMake
if(POLICY CMP0077)
    # allow to define options cache variable before the option is declared
    # https://cmake.org/cmake/help/latest/policy/CMP0077.html
    cmake_policy(SET CMP0077 NEW)
endif()
if(APPLE)
    # MACOSX_RPATH is enabled by default. https://cmake.org/cmake/help/latest/policy/CMP0042.html
    cmake_policy(SET CMP0042 NEW)
endif(APPLE)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
    message("Set default build type to ${CMAKE_BUILD_TYPE}")
endif()

project(ExamplePluginWithLib VERSION 1.0.0)

# Prevent the plugins to be installed in the Radium bundle. Install them locally.
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(PLUGIN_DEMO_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Plugins-${CMAKE_CXX_COMPILER_ID})
else()
    set(PLUGIN_DEMO_DIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR}/Plugins-${CMAKE_CXX_COMPILER_ID}-${CMAKE_BUILD_TYPE}
    )
endif()
message(STATUS "Installing plugin demo into ${PLUGIN_DEMO_DIRECTORY}")

set(Radium_DIR ${CMAKE_INSTALL_PREFIX}/lib/cmake/Radium)
set(CMAKE_INSTALL_PREFIX ${PLUGIN_DEMO_DIRECTORY})
set(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT False)
add_custom_target(${PROJECT_NAME})

include(ExternalProject)

macro(addLocalExternalFolder NAME FOLDER)

    ExternalProject_Add(
        ${NAME}
        PREFIX "${CMAKE_CURRENT_BINARY_DIR}/${FOLDER}"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND
            ${CMAKE_COMMAND} "${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER}"
            -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
            -DCMAKE_PREFIX_PATH=${PLUGIN_DEMO_DIRECTORY}/lib/cmake -DRadium_DIR=${Radium_DIR}
            -DQt5_DIR=${Qt5_DIR} -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        DOWNLOAD_COMMAND ""
        UPDATE_COMMAND ""
        EXCLUDE_FROM_ALL STEP_TARGETS
        install
    )
    add_dependencies(${NAME} ForceRadiumInstallation)
    add_dependencies(ExamplePluginWithLib ${NAME})

endmacro()
addlocalexternalfolder(PluginUpstreamTarget Upstream)
addlocalexternalfolder(PluginDownstreamTarget Downstream)
add_dependencies(PluginDownstreamTarget PluginUpstreamTarget)

add_custom_target(${PROJECT_NAME}-Doc SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
