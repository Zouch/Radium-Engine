#------------------------------------------------------------------------------
# Unit tests via Catch framework
#
# For testing on the function/class level.

# -----------------------------------------------------------------------------

add_executable(unittests
    unittest.cpp
    unittestUtils.hpp
    Core/algebra.cpp
    Core/animation.cpp
    Core/camera.cpp
    Core/color.cpp
    Core/containers.cpp
    Core/distance.cpp
    Core/indexmap.cpp
    Core/indexview.cpp
    Core/mapiterators.cpp
    Core/observer.cpp
    Core/obb.cpp
    Core/polyline.cpp
    Core/raycast.cpp
    Core/resources.cpp
    Core/string.cpp
    Core/topomesh.cpp
    Engine/signalmanager.cpp
    Gui/keymapping.cpp
    IO/volumeloader.cpp
    )

target_compile_definitions(unittests PRIVATE UNIT_TESTS) # add -DUNIT_TESTS define
target_link_libraries(unittests PRIVATE Catch2::Catch2 Core Gui)

add_dependencies(unittests Catch2 Core)
target_include_directories(unittests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

include(../external/Catch2/src/Catch2_download/contrib/Catch.cmake)

catch_discover_tests(unittests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Filesystem COMPONENTS Final Experimental REQUIRED)
find_package(Qt5 COMPONENTS Core Widgets OpenGL Xml REQUIRED)

if(Qt5Core_VERSION VERSION_LESS 5.5)
    message(FATAL_ERROR "Qt5 or superior required (found ${Qt5Core_VERSION}).")
else ()
    message(STATUS "QT ${Qt5Core_VERSION} found." )
endif()

# Qt5
set(Qt5_LIBRARIES Qt5::Core Qt5::Widgets Qt5::OpenGL Qt5::Xml)
target_link_libraries (unittests PRIVATE ${Qt5_LIBRARIES} PRIVATE std::filesystem)
target_compile_definitions(unittests PRIVATE -DCXX_FILESYSTEM_HAVE_FS -DCXX_FILESYSTEM_IS_EXPERIMENTAL=$<BOOL:${CXX_FILESYSTEM_IS_EXPERIMENTAL}> -DCXX_FILESYSTEM_NAMESPACE=${CXX_FILESYSTEM_NAMESPACE})


# convenience target for running only the unit tests
add_custom_target(run_unittests
    #this way we can use fake data from /tests/unittest/ dir (if we have any):
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND $<TARGET_FILE:unittests>
    DEPENDS unittests)

# Verbose printing of results
add_custom_target(run_unittests_verbose
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND $<TARGET_FILE:unittests> -s -a -r=compact
    DEPENDS unittests)
