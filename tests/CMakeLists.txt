# https://cmake.org/cmake/help/latest/manual/ctest.1.html
# https://cmake.org/Wiki/CMake/Testing_With_CTest
# https://cmake.org/cmake/help/latest/command/add_test.html
list(APPEND CMAKE_MESSAGE_INDENT "[Tests] ")

# Adds convenience methods, see cmake/CleanCppExtensions.cmake
include(CleanCppExtensions)

# Creates options to turn on Sanitizers, see cmake/Sanitizers.cmake
include(Sanitizers)

# Enable helpfull warnings and C++17 for all files
if(MSVC)
    add_compile_options(/std:c++17 /W3)
else()
    add_compile_options(-std=c++17 -Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wunused -pedantic)
endif()

if(MSVC OR MSVC_IDE OR MINGW)
    # must find the RadiumExternal directory. This will be set by RadiumConfig.cmake after install
    #
    # TODO : find a way to that more efficiently
    if(DEFINED cpplocate_DIR)
        find_dependency(cpplocate REQUIRED NO_DEFAULT_PATH)
    endif()
    if(DEFINED glbinding_DIR)
        find_dependency(glbinding REQUIRED NO_DEFAULT_PATH)
    endif()
    if(DEFINED globjects_DIR)
        find_dependency(globjects REQUIRED NO_DEFAULT_PATH)
    endif()
    if(RADIUM_IO_TINYPLY AND DEFINED tinyply_DIR)
        find_dependency(tinyply REQUIRED NO_DEFAULT_PATH)
    endif()
    set(RadiumExternalDlls_location "")
    if(cpplocate_DIR)
        add_imported_dir(FROM cpplocate::cpplocate TO RadiumExternalDlls_location)
    endif()
    if(glbinding_DIR)
        add_imported_dir(FROM glbinding::glbinding TO RadiumExternalDlls_location)
    endif()
    if(DEFINED globjects_DIR)
        add_imported_dir(FROM globjects::globjects TO RadiumExternalDlls_location)
    endif()
    if(RADIUM_IO_TINYPLY AND DEFINED tinyply_DIR)
        add_imported_dir(FROM tinyply TO RadiumExternalDlls_location)
    endif()
endif()
# ------------------------------------------------------------------------------
# CTest test suite use add_test() to add a new test to the test suit. When executed test fails
# (returns not 0), test failed. There are more options, check CMake documentation.
#
# the add_test() are done in subfolder's CMakeLists.txt

add_subdirectory(external)

# unittest use catch2 to define unittests on low level functions
add_subdirectory(unittest)

# integration run whole program with parameters, check if it will crash, produce correct results,
# etc.
add_subdirectory(integration)

# example apps are simple complete application, no test for now. TODO : we can imagine to capture
# screen output and compare to refs
add_subdirectory(ExampleApps)
add_subdirectory(ExamplePluginWithLib EXCLUDE_FROM_ALL)

# Convenience targets for fast testing, they depends on binaries (so the build is triggered, when
# sources were changed).
add_custom_target(
    check
    COMMAND echo [----] Running tests
    USES_TERMINAL
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -C Debug
    DEPENDS RadiumExampleApps unittests integration
)

add_custom_target(
    check_verbose
    COMMAND echo [----] Running tests
    USES_TERMINAL
    COMMAND ${CMAKE_CTEST_COMMAND} -V --output-on-failure -C Debug
    DEPENDS RadiumExampleApps unittests integration
)

list(REMOVE_AT CMAKE_MESSAGE_INDENT -1)
