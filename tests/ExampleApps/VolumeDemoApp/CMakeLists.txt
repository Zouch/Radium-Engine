cmake_minimum_required(VERSION 3.6)
if (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER "3.9")
    cmake_policy(SET CMP0071 NEW)
endif ()
if (APPLE)
    cmake_policy(SET CMP0042 NEW)
endif (APPLE)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

project(VolumeDemoApp VERSION 0.0.1)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

# Set default install location to installed-<Compiler_ID> folder in build dir
# we do not want to install to /usr by default
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/installed-${CMAKE_CXX_COMPILER_ID}-${CMAKE_BUILD_TYPE}" CACHE PATH
        "Install path prefix, prepended onto install directories." FORCE)
    message("Set install prefix to ${CMAKE_INSTALL_PREFIX}")
endif ()


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ///////////////////////////////
find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
set(Qt5_LIBRARIES Qt5::Core Qt5::Widgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT IN_RADIUM_BUILD_TREE)
    find_package(Radium REQUIRED Core Engine Gui PluginBase IO)
endif()

set(app_sources
    main.cpp
    )
set(app_headers
    )

set(app_uis
    )
qt5_wrap_ui(app_uis_moc ${app_uis})

set(app_resources

    )

add_executable(${PROJECT_NAME} MACOSX_BUNDLE
    ${app_sources}
    ${app_headers}
    ${app_uis}
    ${app_resources}
    )

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${RADIUM_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR} # Moc
    ${CMAKE_CURRENT_SOURCE_DIR}

    )


target_link_libraries(${PROJECT_NAME}
    PUBLIC
    Radium::Core Radium::Engine Radium::Gui Radium::IO
    ${Qt5_LIBRARIES}
    )

configure_radium_app(
    NAME ${PROJECT_NAME}
)
