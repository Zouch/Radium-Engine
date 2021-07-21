cmake_minimum_required(VERSION 3.6)
# ------------------------------------------------------------------------------
# Policies and global parameters for CMake
if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

project(ExampleLibraryUpstreamPlugin VERSION 1.0.0)

if(APPLE)
    cmake_policy(SET CMP0042 NEW)
endif(APPLE)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/installed-${CMAKE_CXX_COMPILER_ID}"
        CACHE PATH "Install path prefix, prepended onto install directories." FORCE
    )
    message("Set install prefix to ${CMAKE_INSTALL_PREFIX}")
    set(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT False)
endif()

# Radium and Qt stuff
message(STATUS "Searching for package Radium")
find_package(Radium REQUIRED Core Engine PluginBase)

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
set(Qt5_LIBRARIES Qt5::Core Qt5::Widgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
include_directories(${CMAKE_CURRENT_BINARY_DIR} # Moc
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(sources PluginA.cpp)

set(headers PluginA.hpp)

# Our plugin project uses these sources and headers.
add_library(${PROJECT_NAME} SHARED ${sources} ${headers})

target_compile_definitions(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_EXPORTS)

target_link_libraries(
    ${PROJECT_NAME} Radium::Core Radium::Engine Radium::PluginBase ${Qt5_LIBRARIES}
    ExampleLibraryUpstream
)

# -----------------------------------------------------------------------------------
# Radium plugin configuration : manage package and installation
configure_radium_plugin(NAME ${PROJECT_NAME} HELPER_LIBS ExampleLibraryUpstream)
