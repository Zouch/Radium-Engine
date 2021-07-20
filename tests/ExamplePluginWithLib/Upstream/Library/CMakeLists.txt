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
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
    message("Set default build type to ${CMAKE_BUILD_TYPE}")
endif()

project(ExampleLibraryUpstream VERSION 1.0.0)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/installed-${CMAKE_CXX_COMPILER_ID}"
        CACHE PATH "Install path prefix, prepended onto install directories." FORCE
    )
    message(STATUS "Set install prefix to ${CMAKE_INSTALL_PREFIX}")
    set(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT False)
endif()

# Radium and Qt stuff
message(STATUS " Example Library : Searching for package Radium")
find_package(Radium REQUIRED Core Engine)

set(markdowns README.md)
set(sources ExampleLibrary.cpp)
set(public_headers ExampleLibrary.hpp)
set(headers PrivateHeader.hpp)
set(resources LibraryResources/welcome.txt)
# Our library project uses these sources and headers.
add_library(
    ${PROJECT_NAME} SHARED ${sources} ${headers} ${public_headers} ${markdowns} ${resources}
)

target_link_libraries(${PROJECT_NAME} PUBLIC Radium::Core Radium::Engine)

# -----------------------------------------------------------------------------------
# Radium library configuration : manage package and installation
message(
    STATUS
        " Configure library ${PROJECT_NAME} for insertion into Radium exosystem at ${CMAKE_INSTALL_PREFIX}"
)
configure_radium_library(
    TARGET
    ${PROJECT_NAME}
    TARGET_DIR
    ${PROJECT_NAME}
    NAMESPACE
    ${PROJECT_NAME}
    PACKAGE_DIR
    ${CMAKE_INSTALL_PREFIX}/lib/cmake
    PACKAGE_CONFIG
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}-Config.cmake.in
    FILES
    "${public_headers}"
)

install_target_resources(
    TARGET ${PROJECT_NAME} DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/LibraryResources
)
