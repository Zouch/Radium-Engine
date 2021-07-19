cmake_minimum_required(VERSION 3.13)
if (APPLE)
    cmake_policy(SET CMP0042 NEW)
endif (APPLE)

project(RadiumHeadless VERSION 1.0.0)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/installed-${CMAKE_CXX_COMPILER_ID}" CACHE PATH
        "Install path prefix, prepended onto install directories." FORCE)
    message("Set install prefix to ${CMAKE_INSTALL_PREFIX}")
    set(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT False)
endif ()

set(libName Headless)

#-----------------------------------------------------------------------------------
#       lib headless configuration
#-----------------------------------------------------------------------------------
message(STATUS "Configuring library ${PROJECT_NAME}::${libName}")

# Radium components : IO is not mandatory and should be removed once verified
if (NOT IN_RADIUM_BUILD_TREE)
    find_package(Radium REQUIRED Core Engine IO)
    set(DEFAULT_COMPILE_OPTIONS)
endif()

find_package(glfw3 3.3 REQUIRED)

set(CLI11_headers
    RadiumHeadless/CLI/App.hpp
    RadiumHeadless/CLI/CLI.hpp
    RadiumHeadless/CLI/Config.hpp
    RadiumHeadless/CLI/Error.hpp
    RadiumHeadless/CLI/Formatter.hpp
    RadiumHeadless/CLI/FormatterFwd.hpp
    RadiumHeadless/CLI/Macros.hpp
    RadiumHeadless/CLI/Option.hpp
    RadiumHeadless/CLI/Split.hpp
    RadiumHeadless/CLI/StringTools.hpp
    RadiumHeadless/CLI/Timer.hpp
    RadiumHeadless/CLI/TypeTools.hpp
    RadiumHeadless/CLI/Validators.hpp
    RadiumHeadless/CLI/Version.hpp
    )

set(${libName}_sources
    RadiumHeadless/CLIViewer.cpp
    RadiumHeadless/OpenGLContext/OpenGLContext.cpp
    )

set(${libName}_headers
    ${CLI11_headers}
    RadiumHeadless/CLIBaseApplication.hpp
    RadiumHeadless/CLIViewer.hpp
    RadiumHeadless/Headless.hpp
    RadiumHeadless/OpenGLContext/OpenGLContext.hpp
    )

add_library(
    ${libName} SHARED
    ${${libName}_sources}
    ${${libName}_headers}
)

target_include_directories(${libName} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/RadiumHeadless
    )

target_link_libraries(${libName} PUBLIC Radium::Core Radium::Engine Radium::IO PRIVATE glfw)

target_compile_definitions(${libName} PRIVATE ${libName}_EXPORTS  GLFW_INCLUDE_NONE)

target_compile_options(${libName}
    PRIVATE

    PUBLIC
    ${DEFAULT_COMPILE_OPTIONS}

    INTERFACE
)

configure_radium_library(
    TARGET ${libName}
    FILES "${${libName}_headers}"
    TARGET_DIR .
    NAMESPACE ${PROJECT_NAME}
    PACKAGE_DIR ${CMAKE_INSTALL_PREFIX}/lib/cmake
    PACKAGE_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
)
