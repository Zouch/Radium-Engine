# Adds convenience methods, see cmake/cleanCppExtensions.cmake
include(cleanCppExtensions)

# Creates options to turn on sanitizers, see cmake/sanitizers.cmake
include(sanitizers)

#------------------------------------------------------------------------------
# Compilation option
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_compile_options(/W3 )
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options( /bigobj )
  endif()
else()
  add_compile_options(-Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wunused -pedantic)
endif()


#------------------------------------------------------------------------------
# Libraries dependencies
find_package(OpenMP QUIET)

#-----------------------------------------------------------------------------------
# Prepare cmake package

include(CMakePackageConfigHelpers)
set(ConfigPackageLocation lib/cmake/Radium)

#------------------------------------------------------------------------------
# By using macro to add common dependencies you can avoid repetition when you have
# multiple binaries.
# Use with `configureTargetWithDefaultRaSettings(exampleApp)`
macro(configureTargetWithDefaultRaSettings TARGET)
  target_include_directories(${TARGET} PUBLIC
      $<BUILD_INTERFACE:${RADIUM_SRC_DIR}>
      $<INSTALL_INTERFACE:include>
      )
  set_target_properties(${TARGET} PROPERTIES VERSION ${RADIUM_VERSION})
endmacro()

#-----------------------------------------------------------------------------------
# Include sources

if(RADIUM_GENERATE_LIB_CORE)
    add_subdirectory(Core)
endif()
if(RADIUM_GENERATE_LIB_IO)
    add_subdirectory(IO)
endif()
if(RADIUM_GENERATE_LIB_ENGINE)
    add_subdirectory(Engine)
endif()
if(RADIUM_GENERATE_LIB_GUI)
    add_subdirectory(Gui)
endif()
if(RADIUM_GENERATE_LIB_PLUGINBASE)
    add_subdirectory(PluginBase)
endif()

# propagate global to parent
set(LocalDependencies ${LocalDependencies} PARENT_SCOPE)


#-----------------------------------------------------------------------------------
function(cat IN_FILE OUT_FILE)
    file(READ ${IN_FILE} CONTENTS)
    file(APPEND ${OUT_FILE} "${CONTENTS}")
endfunction()

# Prepare the RadiumConfigTemplate.in file:
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/RadiumConfigTemplate.in "")

# Fill the RadiumConfigTemplate.in

cat(../cmake/compilerVersion.cmake ${CMAKE_CURRENT_BINARY_DIR}/RadiumConfigTemplate.in)
cat(../cmake/Config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/RadiumConfigTemplate.in)
if( RADIUM_IO_VOLUMES )
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/RadiumConfigTemplate.in
        "if(IO_FOUND)
          set_target_properties( Radium::IO PROPERTIES RADIUM_IO_HAS_VOLUMES TRUE)
          endif()")
endif()
#-----------------------------------------------------------------------------------
# Generate cmake package
configure_radium_package(
    NAME Radium
    PACKAGE_CONFIG ${CMAKE_CURRENT_BINARY_DIR}/RadiumConfigTemplate.in
)

#install general scripts
install(
    FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/radium_setup_functions.cmake"
    DESTINATION
        ${ConfigPackageLocation}
)

if (MSVC OR MSVC_IDE OR MINGW)
    install(
        FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Windeployqt.cmake"
        DESTINATION
            ${ConfigPackageLocation}
    )
endif()

# # #-------------------------------------------------------------------------------
# # # exampleApp executable setup

# # #set(ICON_RESOURCE "")
# if(WIN32) #windows icon resource file, provided from /packaging directory
#     set(ICON_RESOURCE "${PROJECT_BINARY_DIR}/exampleApp.icon.rc")
# elseif (APPLE) #osx icon ressource file, provided from /packaging directory
#     set(ICON_RESOURCE "${PROJECT_SOURCE_DIR}/packaging/exampleApp.icns")
# endif()

# add_executable(exampleApp MACOSX_BUNDLE
#     ${ICON_RESOURCE}
#     ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
#     SomeClass.cpp
#     SomeClass.h
#     main.cpp)
# add_dependencies(exampleApp versionFileTouchForRebuild) # We want precise time of build in version
# target_include_directories(exampleApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# target_link_libraries (exampleApp PRIVATE Threads::Threads)

# if(APPLE)
#     set_target_properties(exampleApp PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "exampleApp")
#     set_target_properties(exampleApp PROPERTIES MACOSX_BUNDLE_BUNDLE_GUI_IDENTIFIER "com.exampleApp.exampleApp")
#     set_target_properties(exampleApp PROPERTIES MACOSX_BUNDLE_ICON_FILE exampleApp.icns)
#     set_target_properties(exampleApp PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${PROJECT_SOURCE_DIR}/packaging/MacOSXBundleInfo.plist.in)
#     set_source_files_properties(${PROJECT_SOURCE_DIR}/packaging/exampleApp.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
# endif()

# install(TARGETS exampleApp
#     BUNDLE DESTINATION "."
#     RUNTIME DESTINATION bin)

# Using macro from cmake/cleanCpppExtensions.cpp included on the top
# Provides exampleApp-run and exampleApp-dbg
#addRunAndDebugTargets(exampleApp)

# short convenience target from cmake/cleanCpppExtensions.cpp
#add_custom_target(run DEPENDS exampleApp-run)

# Manually specified convenience target for running the binary, we use macro
# add_custom_target(exampleApp-run
#     WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin
#     USES_TERMINAL
#     COMMAND ./exampleApp
#     DEPENDS Install_)

# And this would add -DBAR_ENABLE_FEATURE or equivalent to build command
# target_compile_definitions(exampleApp PRIVATE BAR_ENABLE_FEATURE)

# Add compile time definition based on option
# option(OPTION_NAME "description for end user" OFF)
# if(OPTION_NAME)
#     # you can do anything here, eg. setup new optional executable
#     target_compile_definitions(exampleApp PRIVATE DEFINE_NAME)
# endif()

# Add building new shared library
# add_library(exampleLibrary SHARED ${LIBRARY_SOURCES})
# target_link_libraries (exampleLibrary ${CMAKE_THREAD_LIBS_INIT})
# set_property(TARGET exampleLibrary PROPERTY CXX_STANDARD 14) # we want C++14

# How to include and link OpenGL and GLUT libraries
# find_package(OpenGL REQUIRED)
# find_package(GLUT REQUIRED)
# target_include_directories(exampleApp SYSTEM ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS} )
# target_link_libraries(exampleApp ${OPENGL_LIBRARIES} ${GLUT_LIBRARY} )

# How to use some other standard packages
# find_package(ZLIB REQUIRED)
# target_link_libraries (exampleApp PRIVATE ZLIB::ZLIB)
#
# find_package(OpenSSL REQUIRED)
# target_link_libraries (exampleApp PRIVATE OpenSSL::SSL OpenSSL::Crypto)
#
# find_package(CURL REQUIRED)
# target_link_libraries (exampleApp PRIVATE CURL::libcurl)

#------------------------------------------------------------------------------
# Unit tests

# add_subdirectory(unittest)

#-------------------------------------------------------------------------------
# Copy MINGW needed libraries for building on windows

if(MINGW)
    message(STATUS "MinGW detected")
    get_filename_component(GCC_PATH ${CMAKE_C_COMPILER} PATH)
    if(${GCC_PATH} MATCHES "mingw64/bin")
        set(libgcc "libgcc_s_seh-1.dll") #64bit
        message(STATUS "  64bit dlls is building")
    else()
        set(libgcc "libgcc_s_dw2-1.dll") #32bit
        message(STATUS "  32bit dlls is building")
    endif()

    install(FILES ${GCC_PATH}/${libgcc}
        ${GCC_PATH}/libstdc++-6.dll
        DESTINATION ./bin/
    )
endif(MINGW)


#------------------------------------------------------------------------------
# Other MISC targets

# Adds misc targets: format, cppcheck, tidy, see cmake/cleanCppExtensions.cmake
set( FORMAT_STYLE_FILENAME ${RADIUM_SRC_DIR}/scripts/clang-format )
addMiscTargets()


