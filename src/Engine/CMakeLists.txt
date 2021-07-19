set(ra_engine_target Engine)
list(APPEND CMAKE_MESSAGE_INDENT "[${ra_engine_target}] ")

project( ${ra_engine_target}
         LANGUAGES CXX
         VERSION ${RADIUM_VERSION})

include( filelistEngine )
set(engine_shaders_full ${engine_shaders})

if(CMAKE_VERSION VERSION_LESS 3.12)
    string(REGEX REPLACE "([^;]+)" "${RADIUM_SHADERS_DIR}/\\1" engine_shaders_full "${engine_shaders}")
else()
    list(TRANSFORM engine_shaders_full PREPEND ${RADIUM_SHADERS_DIR}/)
endif()

add_library(
    ${ra_engine_target} SHARED
    ${engine_sources}
    ${engine_headers}
    ${engine_inlines}
)

add_custom_target(RadiumEngineShaders ALL SOURCES ${engine_shaders_full})


#LocalDependencies form parent scope
populate_local_dependencies(NAME "glm_DIR")
populate_local_dependencies(NAME "glbinding_DIR")
populate_local_dependencies(NAME "globjects_DIR")
populate_local_dependencies(NAME "stb_DIR")

add_external_folder(Engine ${RADIUM_EXTERNAL_DIR}/Engine ${LocalDependencies})

find_package(glbinding REQUIRED NO_DEFAULT_PATH )
find_package(globjects REQUIRED NO_DEFAULT_PATH )
find_package(Eigen3 REQUIRED  NO_DEFAULT_PATH )

#This one should be extracted directly from external project properties.
target_compile_definitions(${ra_engine_target}
    PUBLIC
    GLOBJECTS_USE_EIGEN
    )
target_compile_options(${ra_engine_target}
    PUBLIC
    ${RA_GLBINDINGS_COMPILE_OPTIONS}
    )

target_include_directories(${ra_engine_target} PUBLIC
    $<BUILD_INTERFACE:${GLBINDING_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${GLM_INCLUDE_DIR}>)
add_dependencies(${ra_engine_target} Core RadiumEngineShaders)
target_link_libraries(${ra_engine_target} PUBLIC Core glm glbinding::glbinding glbinding::glbinding-aux globjects::globjects)

target_compile_definitions(${ra_engine_target} PRIVATE "-DRA_ENGINE_EXPORTS")

message(STATUS "Configuring library ${ra_engine_target} with standard settings")
configure_radium_target(${ra_engine_target})
configure_radium_library(
    TARGET ${ra_engine_target}
    FILES "${engine_headers};${engine_inlines}"
)
message(STATUS "installing resources from ${RADIUM_SHADERS_DIR}")
install_target_resources(
    TARGET ${ra_engine_target}
    RESOURCES_DIR ${RADIUM_SHADERS_DIR}
    RESOURCES_INSTALL_DIR "Radium"
    FILES ${engine_shaders}
    )
message(STATUS "installing resources from ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Textures")
install_target_resources(
    TARGET ${ra_engine_target}
    RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Textures
    RESOURCES_INSTALL_DIR "Radium"
)

list(REMOVE_AT CMAKE_MESSAGE_INDENT -1)

