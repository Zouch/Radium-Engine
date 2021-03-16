cmake_minimum_required (VERSION 3.6)

project (radiumio-external VERSION 1.0.0)

include(ExternalProject)
include(ExternalInclude)

#------------------------------------------------------------------------------
# Eigen - Linear Algebra library

# force installing by default all the external projects
set_property(DIRECTORY PROPERTY EP_STEP_TARGETS install)

# Add fPIC for all dependencies
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

string(REPLACE ";" "" indent_string "${CMAKE_MESSAGE_INDENT}")
set(indent_string "${indent_string}--")

add_custom_target( IOExternals )

if (RADIUM_IO_ASSIMP)
    if (NOT DEFINED assimp_DIR)
        CheckExternalsPrerequisite()
        StatusMessage("[IOExternal]" "assimp" "remote git")
        # I found some problems when generating xcode project for Assimp. The debug version is expected even in Release mode.
        # As Release build does not generate the debug version, the script fail.
        # For now, only configure in Debug mode to work with xcode
        ExternalProject_Add(assimp
            GIT_REPOSITORY https://github.com/assimp/assimp.git
            GIT_TAG ec08092dbf559a9da352889317f07afa56c97bdb
            GIT_SHALLOW FALSE
            GIT_PROGRESS TRUE
            INSTALL_DIR "${CMAKE_INSTALL_PREFIX}"
            CMAKE_ARGS
            ${RADIUM_EXTERNAL_CMAKE_OPTIONS}
            -DASSIMP_BUILD_ASSIMP_TOOLS=False
            -DASSIMP_BUILD_SAMPLES=False
            -DASSIMP_BUILD_TESTS=False
            -DIGNORE_GIT_HASH=True
            -DASSIMP_NO_EXPORT=True
            "-DCMAKE_MESSAGE_INDENT=${indent_string}\;"
            )
        add_dependencies( IOExternals assimp )
    else()
        StatusMessage("" "assimp" "${assimp_DIR}")
    endif()
endif()


if(RADIUM_IO_TINYPLY)
    if (NOT DEFINED tinyply_DIR)
        CheckExternalsPrerequisite()
        StatusMessage("" "tinyply" "remote git")
        ExternalProject_Add( tinyply
            GIT_REPOSITORY https://github.com/ddiakopoulos/tinyply.git
            GIT_TAG tags/2.3.2
            GIT_SHALLOW TRUE
            GIT_PROGRESS TRUE
            INSTALL_DIR "${CMAKE_INSTALL_PREFIX}"
            CMAKE_ARGS
            ${RADIUM_EXTERNAL_CMAKE_OPTIONS}
            -DSHARED_LIB=TRUE
            "-DCMAKE_MESSAGE_INDENT=${indent_string}\;"                   
            )
        add_dependencies( IOExternals tinyply )
    else()
        StatusMessage("[IOExternal]" "tinyply" "${tinyply_DIR}")
    endif()
endif()
