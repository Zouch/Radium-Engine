#----------------------------  Test for required compiler version -------------------------------
set(RADIUM_MIN_GCC_VERSION "8.0")
set(RADIUM_MIN_MSVC_VERSION "9.28")
set(RADIUM_MIN_AppleClang_VERSION "12.0")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${RADIUM_MIN_GCC_VERSION})
        message(FATAL_ERROR "Insufficient gcc version. Found ${CMAKE_CXX_COMPILER_VERSION},"
                            "requires >= ${RADIUM_MIN_GCC_VERSION}"
        )
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${RADIUM_MIN_MSVC_VERSION})
        message(FATAL_ERROR "Insufficient msvc version. Found ${CMAKE_CXX_COMPILER_VERSION},"
                            "requires >= ${RADIUM_MIN_MSVC_VERSION}"
        )
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${RADIUM_MIN_AppleClang_VERSION})
        message(FATAL_ERROR "Insufficient AppleClang version. Found ${CMAKE_CXX_COMPILER_VERSION},"
                            "requires >= ${RADIUM_MIN_AppleClang_VERSION}"
        )
    endif()
elseif(...)
    message(
        FATAL_ERROR
            "You are using an unsupported compiler"
            "(${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION})!"
            "Compilation has only been tested with GCC>=${RADIUM_MIN_GCC_VERSION},"
            "AppleClang>=${RADIUM_MIN_AppleClang_VERSION} and MSVC>=${RADIUM_MIN_MSVC_VERSION}"
    )
endif()
