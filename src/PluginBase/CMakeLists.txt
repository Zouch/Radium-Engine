set(ra_pluginbase_target PluginBase)
list(APPEND CMAKE_MESSAGE_INDENT "[${ra_pluginbase_target}] ")

project(${ra_pluginbase_target} LANGUAGES CXX VERSION ${RADIUM_VERSION})

include(filelistPluginBase)

# Find packages
find_package(Qt5 COMPONENTS Core REQUIRED)

if(Qt5Core_VERSION VERSION_LESS 5.5)
    message(FATAL_ERROR "Qt5 or superior required (found ${Qt5Core_VERSION}).")
else()
    message(STATUS "QT ${Qt5Core_VERSION} found.")
endif()

# Qt5
set(Qt5_LIBRARIES Qt5::Core)
set(CMAKE_AUTOMOC ON)

add_library(
    ${ra_pluginbase_target} SHARED ${pluginbase_sources} ${pluginbase_headers}
                                   ${pluginbase_inlines} ${pluginbase_json}
)

add_dependencies(${ra_pluginbase_target} Core Engine)
target_link_libraries(${ra_pluginbase_target} PUBLIC Core Engine)
target_link_libraries(${ra_pluginbase_target} PRIVATE Qt5::Core)

target_compile_definitions(${ra_pluginbase_target} PRIVATE "-DRA_PLUGINBASE_EXPORTS")

message(STATUS "Configuring library ${ra_pluginbase_target} with standard settings")
configure_radium_target(${ra_pluginbase_target})
configure_radium_library(
    TARGET ${ra_pluginbase_target} FILES "${pluginbase_headers};${pluginbase_inlines}"
)

# install json configuration files
install(FILES ${pluginbase_json} DESTINATION ${ConfigPackageLocation})

list(REMOVE_AT CMAKE_MESSAGE_INDENT -1)
