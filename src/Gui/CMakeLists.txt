set(ra_gui_target Gui)
list(APPEND CMAKE_MESSAGE_INDENT "[${ra_gui_target}] ")

project( ${ra_gui_target}
         LANGUAGES CXX
         VERSION ${RADIUM_VERSION})

include( filelistGui )

# Find packages
find_package(Qt5 COMPONENTS Core Widgets OpenGL Xml REQUIRED)
find_package(OpenGL REQUIRED)

if(Qt5Core_VERSION VERSION_LESS 5.5)
    message(FATAL_ERROR "Qt5 or superior required (found ${Qt5Core_VERSION}).")
else ()
    message(STATUS "QT ${Qt5Core_VERSION} found." )
endif()

# Qt5
set(Qt5_LIBRARIES Qt5::Core Qt5::Widgets Qt5::OpenGL Qt5::Xml)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR} # Moc
    )
qt5_wrap_ui(gui_uis ${gui_uis})

add_library(
    ${ra_gui_target} SHARED
    ${gui_sources}
    ${gui_headers}
    ${gui_inlines}
    ${gui_uis}
    ${gui_resources}
)

add_dependencies(${ra_gui_target} Core Engine PluginBase IO)
target_link_libraries(${ra_gui_target} PUBLIC Core Engine PluginBase IO)
target_link_libraries (${ra_gui_target} PRIVATE ${Qt5_LIBRARIES} OpenGL::GL)

if( CMAKE_BUILD_TYPE MATCHES Debug )
    target_compile_definitions(${ra_gui_target} PUBLIC GUI_IS_COMPILED_WITH_DEBUG_INFO)
endif()

# Ask RadiumIO for supported loaders
get_target_property(USE_ASSIMP IO IO_ASSIMP)
if (${USE_ASSIMP})
    target_compile_definitions(${ra_gui_target} PRIVATE "-DIO_USE_ASSIMP")
endif ()
get_target_property(USE_TINYPLY IO IO_TINYPLY)
if (${USE_TINYPLY})
    target_compile_definitions(${ra_gui_target} PRIVATE "-DIO_USE_TINYPLY")
endif ()
get_target_property(HANDLE_VOLUMES IO IO_VOLUMES)
if (${HANDLE_VOLUMES})
    target_compile_definitions(${ra_gui_target} PRIVATE "-DIO_HANDLE_VOLUMES")
endif ()

target_compile_definitions(${ra_gui_target} PRIVATE "-DRA_GUI_EXPORTS")

message(STATUS "Configuring library ${ra_gui_target} with standard settings")
configureTargetWithDefaultRaSettings(${ra_gui_target})
configure_radium_library(
    TARGET ${ra_gui_target}
    FILES "${gui_headers};${gui_inlines}"
)
message(STATUS "installing resources from Configs")
install_target_resources(
    TARGET ${ra_gui_target} 
    RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Configs
    RESOURCES_INSTALL_DIR "Radium"
)
list(REMOVE_AT CMAKE_MESSAGE_INDENT -1)


