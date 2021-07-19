cmake_minimum_required(VERSION 3.13)
if (APPLE)
    cmake_policy(SET CMP0042 NEW)
endif (APPLE)

project(HeadlessDemo VERSION 1.0.0)

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

add_subdirectory(libHeadless)

option(USE_RADIUM_ASSIMP            "Use assimp data loader from Radium" ON)
message(STATUS "Configuring demo application ${PROJECT_NAME}")

set(app_sources
    main.cpp
)

add_executable(${PROJECT_NAME}
    ${app_sources}
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    Radium::Core Radium::Engine Radium::IO
    RadiumHeadless::Headless
)

if (USE_RADIUM_ASSIMP)
    if (IN_RADIUM_BUILD_TREE)
        set(USE_ASSIMP ${RADIUM_IO_ASSIMP})
    else()
        get_target_property(USE_ASSIMP Radium::IO RADIUM_IO_USE_ASSIMP)
    endif()
    if (${USE_ASSIMP})
        message(STATUS "Enabling assimp loader from Radium::IO")
        target_compile_definitions(${PROJECT_NAME} PRIVATE ADD_ASSIMP_LOADER)
    else()
        message(STATUS "Assimp loader from Radium::IO not detected")
    endif()
endif()

target_compile_options(${PROJECT_NAME}
    PRIVATE

    PUBLIC
    ${DEFAULT_COMPILE_OPTIONS}

    INTERFACE
)

configure_radium_app(
    NAME ${PROJECT_NAME}
)
