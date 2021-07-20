cmake_minimum_required(VERSION 3.6)

project(radiumengine-external VERSION 1.0.0)

include(ExternalProject)
include(ExternalInclude)

macro(add_dependency_if_compiled target dep)
    if(TARGET ${dep})
        add_dependencies(${target} ${dep})
    endif()
endmacro()

# ------------------------------------------------------------------------------
# Eigen - Linear Algebra library

# force installing by default all the external projects
set_property(DIRECTORY PROPERTY EP_STEP_TARGETS install)

# Add fPIC for all dependencies
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

string(REPLACE ";" "" indent_string "${CMAKE_MESSAGE_INDENT}")
set(indent_string "${indent_string}--")

add_custom_target(EngineExternals)

if(NOT DEFINED glm_DIR)
    check_externals_prerequisite()
    status_message("[EngineExternal]" "glm" "remote git")
    ExternalProject_Add(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 0.9.9.5
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
        INSTALL_DIR "${CMAKE_INSTALL_PREFIX}"
        CMAKE_ARGS ${RADIUM_EXTERNAL_CMAKE_OPTIONS} -DGLM_TEST_ENABLE=OFF -DBUILD_STATIC_LIBS=OFF
                   -DCMAKE_INSTALL_LIBDIR=lib "-DCMAKE_MESSAGE_INDENT=${indent_string}\;"
    )

    set(glm_DIR ${CMAKE_INSTALL_PREFIX}/lib/cmake/glm)
    add_dependencies(EngineExternals glm)
else()
    # because other might depends on it, but it's already done, define a dummy target
    add_custom_target(glm)
    status_message("" "glm" "${glm_DIR}")
endif()

if(NOT DEFINED glbinding_DIR)
    check_externals_prerequisite()
    status_message("[EngineExternal]" "glbinding" "remote git")
    ExternalProject_Add(
        glbinding
        GIT_REPOSITORY https://github.com/cginternals/glbinding.git
        GIT_TAG 663e19cf1ae6a5fa1acfb1bd952fc43f647ca79c
        GIT_SHALLOW FALSE
        GIT_PROGRESS TRUE
        PATCH_COMMAND git reset --hard && git apply -v --ignore-whitespace
                      "${CMAKE_CURRENT_LIST_DIR}/patches/glbinding.patch"
        INSTALL_DIR "${CMAKE_INSTALL_PREFIX}"
        CMAKE_ARGS ${RADIUM_EXTERNAL_CMAKE_OPTIONS} -DOPTION_BUILD_TESTS=OFF
                   -DOPTION_BUILD_DOCS=OFF -DOPTION_BUILD_TOOLS=OFF -DOPTION_BUILD_EXAMPLES=OFF
                   "-DCMAKE_MESSAGE_INDENT=${indent_string}\;"
    )

    set(glbinding_DIR ${CMAKE_INSTALL_PREFIX}/share/glbinding)
    add_dependencies(EngineExternals glbinding)
else()
    add_custom_target(glbinding)
    status_message("" "glbinding" "${glbinding_DIR}")
endif()

if(NOT DEFINED Eigen3_DIR)
    message(STATUS "globjects use bundled eigen3")
    set(Eigen3_DIR ${CMAKE_INSTALL_PREFIX}/share/eigen3/cmake/)
else()
    message(STATUS "globjects use eigen3 from ${Eigen3_DIR}")
endif()

if(NOT DEFINED globjects_DIR)
    add_custom_target(globjects-dep)
    add_dependency_if_compiled(globjects-dep Eigen3)
    add_dependency_if_compiled(globjects-dep glbinding)
    add_dependency_if_compiled(globjects-dep glm)

    check_externals_prerequisite()
    status_message("" "globjects" "remote git")
    ExternalProject_Add(
        globjects
        GIT_REPOSITORY https://github.com/dlyr/globjects.git
        GIT_TAG 11c559a07d9e310abb2f53725fd47cfaf538f8b1
        GIT_PROGRESS TRUE
        PATCH_COMMAND git reset --hard && git apply -v --ignore-whitespace
                      "${CMAKE_CURRENT_LIST_DIR}/patches/globjects.patch"
        INSTALL_DIR "${CMAKE_INSTALL_PREFIX}"
        DEPENDS glbinding glm globjects-dep
        CMAKE_ARGS ${RADIUM_EXTERNAL_CMAKE_OPTIONS}
                   -DOPTION_BUILD_DOCS=OFF
                   -DOPTION_BUILD_EXAMPLES=OFF
                   -DOPTION_USE_EIGEN=ON
                   -Dglbinding_DIR=${glbinding_DIR}
                   -Dglm_DIR=${glm_DIR}
                   -DEigen3_DIR=${Eigen3_DIR}
                   "-DCMAKE_MESSAGE_INDENT=${indent_string}\;"
    )

    set(globjects_DIR ${CMAKE_INSTALL_PREFIX}/share/globjects)
    add_dependencies(EngineExternals globjects)
else()
    status_message("" "globjects" "${globjects_DIR}")
endif()

if(NOT DEFINED stb_DIR)
    check_externals_prerequisite()
    status_message("" "stb" "remote git")
    ExternalProject_Add(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG 1034f5e5c4809ea0a7f4387e0cd37c5184de3cdd
        GIT_PROGRESS TRUE
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ${CMAKE_COMMAND} -E echo
                      "Installing stb_image.h to ${CMAKE_INSTALL_PREFIX}/include"
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}/include/stb/
        COMMAND ${CMAKE_COMMAND} -E copy_if_different <SOURCE_DIR>/stb_image.h
                ${CMAKE_INSTALL_PREFIX}/include/stb/
        COMMAND ${CMAKE_COMMAND} -E copy_if_different <SOURCE_DIR>/stb_image_write.h
                ${CMAKE_INSTALL_PREFIX}/include/stb/
        INSTALL_COMMAND ""
    )
    add_dependencies(EngineExternals stb)
else()
    status_message("" "stb" "${stb_DIR}")
    message(STATUS "Installing stb_image.h to ${CMAKE_INSTALL_PREFIX}/include")
    file(COPY ${stb_DIR}/stb_image.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include/stb/)
    file(COPY ${stb_DIR}/stb_image_write.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include/stb/)
endif()
