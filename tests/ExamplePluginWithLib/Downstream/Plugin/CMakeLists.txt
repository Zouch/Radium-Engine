# This Plugin uses an external helper library (ExampleLibraryUpstream).
# This library can be searched and used in its build tree or from an installation one.
if(NOT TARGET ExampleLibraryUpstream)
    message(STATUS "Searching for package ExampleLibraryUpstream")
    find_package(ExampleLibraryUpstream REQUIRED)
endif()
# once found (or used from the same buildtree than this project), the helper library has name
# ExampleLibraryUpstream::ExampleLibraryUpstream

# Populate the plugin by its sources
set(sources PluginB.cpp)
set(headers PluginB.hpp)
# Our library project uses these sources and headers.
add_library(${PROJECT_NAME} SHARED ${sources} ${headers})

target_compile_definitions(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_EXPORTS)

target_link_libraries(
    ${PROJECT_NAME} Radium::Core Radium::Engine Radium::PluginBase ${Qt5_LIBRARIES}
    ExampleLibraryUpstream::ExampleLibraryUpstream
)

# -----------------------------------------------------------------------------------
# Radium plugin configuration : manage package and installation
message(STATUS "Configuring the plugin ${PROJECT_NAME}")
configure_radium_plugin(
    NAME ${PROJECT_NAME} HELPER_LIBS ExampleLibraryUpstream::ExampleLibraryUpstream
)
