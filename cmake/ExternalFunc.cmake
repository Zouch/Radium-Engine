cmake_minimum_required(VERSION 3.12)

include(ExternalInclude)

macro(add_external_folder NAME FOLDER )
    # External resources/repositories are downloaded and built at configuration stage
    message(STATUS "[add_external_folder] process ${NAME} ${FOLDER}")
    list(APPEND CMAKE_MESSAGE_INDENT "[External] ")

    #Working directory relative to binary dir, to shorten folder name see issue 598
    set(EXT_WORKING_DIR ${CMAKE_BINARY_DIR}/ext/${NAME})

    message(STATUS "Create temporary directory")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E make_directory ${EXT_WORKING_DIR}/cmake
    )

    if("${ARGN}" STREQUAL "")
        message(STATUS "Configure cmake project")
    else ()
        message(STATUS "Configure cmake project with options ${ARGN}")
    endif()

    string(TOUPPER ${NAME} NAME_UPPER)
    set( UPDATE_EXTERNAL ON)
    if( RADIUM_SKIP_${NAME_UPPER}_EXTERNAL )
        set( UPDATE_EXTERNAL OFF)
    endif()

    # Check if install prefix has changed. If yes, force installing the externals again
    if( DEFINED CACHED_INSTALL_PREFIX )
        if( NOT "${CACHED_INSTALL_PREFIX}" STREQUAL "${CMAKE_INSTALL_PREFIX}" )
            message(STATUS "CMAKE_INSTALL_PREFIX has changed (from ${CACHED_INSTALL_PREFIX} to ${CMAKE_INSTALL_PREFIX}), reinstalling dependencies")
            set( UPDATE_EXTERNAL ON)
        endif()
    endif()

    # Check if the external ${NAME} is installed. If not, force installing the externals again
    if( NOT EXISTS "${CMAKE_INSTALL_PREFIX}/share/Radium-${NAME}.touch" )
        message(STATUS "CMAKE_INSTALL_PREFIX is empty (${CMAKE_INSTALL_PREFIX}), reinstalling dependencies")
        set( UPDATE_EXTERNAL ON)
    endif()

    if ( UPDATE_EXTERNAL )
        string(REPLACE ";" "" indent_string "${CMAKE_MESSAGE_INDENT}")
        execute_process(
            COMMAND ${CMAKE_COMMAND} ${FOLDER}
            ${RADIUM_EXTERNAL_CMAKE_OPTIONS}
            -DCMAKE_MESSAGE_INDENT=${indent_string}\;
            -Wno-dev
            ${ARGN}
            WORKING_DIRECTORY ${EXT_WORKING_DIR}
            RESULT_VARIABLE ret
        )
        if(NOT ret EQUAL "0")
            message(FATAL_ERROR "Cmake configure step failed. ")
        endif ()
        
        message(STATUS "Start build")

        set(RadiumExternalMakeTarget all)
        string(TOLOWER ${CMAKE_GENERATOR} generator_lower)
        if (MSVC AND NOT ${generator_lower} STREQUAL "ninja")
            set(RadiumExternalMakeTarget ALL_BUILD)
            message(STATUS "Enable compatibility mode for VS Generator")
        endif ()

        if (APPLE AND ${generator_lower} STREQUAL "xcode")
            set(RadiumExternalMakeTarget ALL_BUILD)
            message(STATUS "Enable compatibility mode for Xcode Generator")
        endif ()

        message(STATUS "EXEC")
        execute_process(
            COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE} --target ${RadiumExternalMakeTarget}
                WORKING_DIRECTORY ${EXT_WORKING_DIR}
                RESULT_VARIABLE ret
        )

        if(NOT ret EQUAL "0")
            message( FATAL_ERROR "Cmake build step failed. ")
        endif()
        message(STATUS "Build ended")

        #Configure package
        if(EXISTS "${FOLDER}/package.cmake")
            set(EXTERNAL_TARGET_PACKAGE_FILE "${EXT_WORKING_DIR}/cmake/package-${NAME}.cmake")
            message(STATUS "Configure package file ${EXTERNAL_TARGET_PACKAGE_FILE}")
            configure_file(${FOLDER}/package.cmake "${EXTERNAL_TARGET_PACKAGE_FILE}" COPYONLY)
            include("${EXTERNAL_TARGET_PACKAGE_FILE}")
            unset(EXTERNAL_TARGET_PACKAGE_FILE)
        endif()
        OPTION( RADIUM_SKIP_${NAME_UPPER}_EXTERNAL "Skip updating ${NAME}::external (disable for rebuild)" ON)

    else()
        message(STATUS "Skip ${NAME}::external update (use RADIUM_SKIP_${NAME_UPPER}_EXTERNAL to change behavior)")
    endif()

    #This dummy target add external/CMakeLists.txt to most IDEs
    set(external_sources ${FOLDER}/CMakeLists.txt)
    if(EXISTS "${FOLDER}/package.cmake")
        list(APPEND external_sources ${FOLDER}/package.cmake)
    endif()
    add_custom_target(External${NAME} ALL SOURCES ${external_sources})
    include("${EXT_WORKING_DIR}/cmake/package-${NAME}.cmake")

    # Create touch file
    file( TOUCH "${CMAKE_CURRENT_BINARY_DIR}/Radium-${NAME}.touch" )
    install(
      FILES
        "${CMAKE_CURRENT_BINARY_DIR}/Radium-${NAME}.touch"
      DESTINATION
        share/
    )
    list(REMOVE_AT CMAKE_MESSAGE_INDENT -1)
    message(STATUS "[add_external_folder] end process ${NAME} ${FOLDER}")

endmacro()
