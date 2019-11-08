# -------------------------- Usefull fonctions for application and plugins client  ------------------------------------
# CMAKE tips : To delay evaluation of some variable to build time, just escape their name (e.g \${NAME} instead of ${NAME}
# TODO : works well on MacOs : must be tested on other systems
# TODO, some part are only for MACOS but could be generalized to all systems (MACOSX_BUNDLE on MacOs, Bundle like directories on other systems
include(CMakeParseArguments)
# Install resources : link (copy under window) the resource dir DIRECTORY into the buildtree at the given BUILDLOCATION and install the
# required files into the <bundle>/Resources
# If called only with a directory, all the files in this directory and subdirs will be installed
# to be called with
# installResources ( TARGET theTarget`
#                    [BUILDLOCATION whereToLinkInTheBuildTree]
#                    DIRECTORY theBaseRessourceDirectory
#                    [FILES [file1 [file2 ... ]]]
#
function(installTargetResources)
    # "declare" and parse parameters
    cmake_parse_arguments(
            ARGS
            ""
            "TARGET;DIRECTORY;BUILDLOCATION"
            "FILES"
            ${ARGN}
    )
    # verify that the function was called with expected parameters
    if (NOT ARGS_TARGET)
        message(FATAL_ERROR " [installResources] You must provide a target that need these resources")
    endif ()
    if (NOT ARGS_DIRECTORY)
        message(FATAL_ERROR " [installResources] You must provide a resource directory")
    endif ()
    if (NOT ARGS_BUILDLOCATION)
        # linking resours in the current bin dir of the build tree
        set(ARGS_BUILDLOCATION ${CMAKE_CURRENT_BINARY_DIR})
    endif ()
    # compute resources dir for build tree and install tree
    get_filename_component(rsc_dir ${ARGS_DIRECTORY} NAME)
    set(buildtree_dir ${ARGS_BUILDLOCATION})
    # installing resources in the buildtree (link if available, copy if not)
    message(STATUS " [installResources] Linking resources directory ${ARGS_DIRECTORY} for target ${ARGS_TARGET} into ${buildtree_dir}/Resources/${rsc_dir}")
    file(MAKE_DIRECTORY "${buildtree_dir}")
    if (MSVC OR MSVC_IDE OR MINGW)
        add_custom_command(
                TARGET ${ARGS_TARGET}
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${ARGS_DIRECTORY} "${buildtree_dir}/${rsc_dir}"
                VERBATIM
        )
    else ()
        add_custom_command(
                TARGET ${ARGS_TARGET}
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E create_symlink ${ARGS_DIRECTORY} "${buildtree_dir}/${rsc_dir}"
                VERBATIM
        )
    endif ()

    # Install in the install tree
    # Identify the individual files (to preserve directory structure)
    message(STATUS " [installResources] configuring install for requested files of ${ARGS_DIRECTORY}")
    if (NOT ARGS_FILES)
        file(GLOB_RECURSE ARGS_FILES RELATIVE ${ARGS_DIRECTORY} ${ARGS_DIRECTORY}/*)
    endif ()
    foreach (file ${ARGS_FILES})
        get_filename_component(file_dir ${file} DIRECTORY)
        if (APPLE)
            install(
                    FILES ${ARGS_DIRECTORY}/${file}
                    DESTINATION ${ARGS_TARGET}.app/Contents/Resources/${rsc_dir}/${file_dir}
            )
        else ()
            install(
                    FILES ${ARGS_DIRECTORY}/${file}
                    DESTINATION Resources/${rsc_dir}/${file_dir}
            )
        endif ()
    endforeach ()
endfunction()
# --------------------------

# Configuration of the build and installation procedure for cmdline Radium application
# Allows to install application with dependent resources
# usage :
#   configure_xxx(
#         NAME theTargetName # <- this must be an executable
#         RESOURCES ResourceDir1 ResourceDir2 # <- acept a list of directories
# )
function(configure_cmdline_Radium_app)
    # "declare" and parse parameters
    cmake_parse_arguments(
            ARGS
            "USE_PLUGINS"
            "NAME"
            "RESOURCES" # list of directories containing the resources to install - optional
            ${ARGN}
    )
    if (NOT ARGS_NAME)
        message(FATAL_ERROR " [configure_radium_app] You must provide the main target of the application")
    endif ()
    # configure the application
    if (APPLE)
        get_target_property(IsMacBundle ${ARGS_NAME} MACOSX_BUNDLE)
        if (IsMacBundle)
            message(FATAL_ERROR " Error configuring ${ARGS_NAME} as a cmdline application. A bundle was asked for this target.")
        endif ()
    endif ()
    # Configure the executable installation
    install(
            TARGETS ${ARGS_NAME}
            RUNTIME DESTINATION bin
    )
    # install Radium Resources if there is a dependency on Engine or Guibase
    get_target_property(deps ${ARGS_NAME} INTERFACE_LINK_LIBRARIES)
    list(FIND deps "Radium::RadiumEngine" depEngine)
    list(FIND deps "Radium::RadiumGuiBase" depGuibase)
    if (depEngine GREATER_EQUAL "0" OR depGuibase GREATER_EQUAL "0")
        install(DIRECTORY ${RADIUM_RESOURCES_DIR} DESTINATION ${CMAKE_INSTALL_PREFIX})
    else ()
        message(STATUS " No Radium resource needed for ${ARGS_NAME}")
    endif ()
    # Configure the application own resources installation
    if (ARGS_RESOURCES)
        foreach (resLocation ${ARGS_RESOURCES})
            message(STATUS " Installing resources ${resLocation} for ${ARGS_NAME} ")
            installTargetResources(
                    TARGET ${ARGS_NAME}
                    DIRECTORY ${resLocation}
                    BUILDLOCATION ${CMAKE_CURRENT_BINARY_DIR}/Resources
            )
        endforeach ()
    endif ()
endfunction()


# NOTE that only MacosX is supported for now to handle bundled applications
# But the script could be generalized to all systems by integrfating the bundle structure
# (MACOSX_BUNDLE* and BundleName.app are explicitely used here)
function(configure_bundled_Radium_app)
    # "declare" and parse parameters
    cmake_parse_arguments(
            ARGS
            "USE_PLUGINS"
            "NAME"
            "RESOURCES" # list of directories containing the resources to install - optional
            ${ARGN}
    )
    if (NOT ARGS_NAME)
        message(FATAL_ERROR " [configure_radium_app] You must provide the main target of the application")
    endif ()
    # configure the application
    get_target_property(IsMacBundle ${ARGS_NAME} MACOSX_BUNDLE)
    if (NOT IsMacBundle)
        message(FATAL_ERROR " Error configuring ${ARGS_NAME} as a Bundled application. Only MacOsX is supported")
    endif ()

    set_target_properties(${ARGS_NAME} PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME ${ARGS_NAME})
    set_target_properties(${ARGS_NAME} PROPERTIES MACOSX_BUNDLE_BUNDLE_GUI_IDENTIFIER "com.radium.${ARGS_NAME}")
    set_target_properties(${ARGS_NAME} PROPERTIES MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
    set_target_properties(${ARGS_NAME} PROPERTIES MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

    # Configure the executable installation
    install(
            TARGETS ${ARGS_NAME}
            BUNDLE DESTINATION "."
    )
    if (ARGS_USE_PLUGINS)
        install(CODE "
        include(BundleUtilities)
        file(REMOVE_RECURSE ${CMAKE_INSTALL_PREFIX}/${ARGS_NAME}.app/Contents/Resources)
        file(COPY ${RADIUM_RESOURCES_DIR} DESTINATION ${CMAKE_INSTALL_PREFIX}/${ARGS_NAME}.app/Contents)
        if (EXISTS ${RADIUM_PLUGINS_DIR})
            file(COPY ${RADIUM_PLUGINS_DIR} DESTINATION ${CMAKE_INSTALL_PREFIX}/${ARGS_NAME}.app/Contents/)
        else()
            file(MAKE_DIRECTORY \"${CMAKE_INSTALL_PREFIX}/${ARGS_NAME}.app/Contents/Plugins/lib\")
        endif()
        file(GLOB RadiumAvailablePlugins
                RELATIVE ${RADIUM_PLUGINS_DIR}/lib/
                ${RADIUM_PLUGINS_DIR}/lib/*.dylib )
        set(InstalledPlugins)
        foreach (plugin \${RadiumAvailablePlugins})
            list( APPEND InstalledPlugins ${CMAKE_INSTALL_PREFIX}/${ARGS_NAME}.app/Contents/Plugins/lib/\${plugin} )
        endforeach ()
        fixup_bundle(${CMAKE_INSTALL_PREFIX}/${ARGS_NAME}.app \"\${InstalledPlugins}\" \"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}\")
        "
                )
    else ()
        install(CODE "
            message(STATUS \"Installing ${ARGS_NAME} without plugins\")
            include(BundleUtilities)
            file(REMOVE_RECURSE ${CMAKE_INSTALL_PREFIX}/${ARGS_NAME}.app/Contents/Resources)
            file(COPY ${RADIUM_RESOURCES_DIR} DESTINATION ${CMAKE_INSTALL_PREFIX}/${ARGS_NAME}.app/Contents)
            fixup_bundle(${CMAKE_INSTALL_PREFIX}/${ARGS_NAME}.app \"\" \"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}\")
            "
                )
    endif ()

    # Configure the resources installation
    if (ARGS_RESOURCES)
        foreach (resLocation ${ARGS_RESOURCES})
            message(STATUS " Installing resources ${resLocation} for ${ARGS_NAME} ")
            installTargetResources(
                    TARGET ${ARGS_NAME}
                    DIRECTORY ${resLocation}
                    BUILDLOCATION ${CMAKE_CURRENT_BINARY_DIR}/${ARGS_NAME}.app/Contents/Resources
            )
        endforeach ()
    endif ()
endfunction()

# Configuration of the build and installation procedure for Bundled Radium application
# Allows to install application with dependent resources
# usage :
#   configure_radium_app_install(
#         NAME theTargetName # <- this must be an executable
#         [USE_PLUGINS] # <- The application uses Radium Plugins : install available plugins into the bundle is it is one
#         [RESOURCES ResourceDir1 ResourceDir2] # <- acept a list of directories
# )
function(configure_radium_app_install)
    # "declare" and parse parameters
    cmake_parse_arguments(
            ARGS
            "USE_PLUGINS"
            "NAME"
            "RESOURCES" # list of directories containing the resources to install - optional
            ${ARGN}
    )
    if (NOT ARGS_NAME)
        message(FATAL_ERROR " [configure_radium_app] You must provide the main target of the application")
    endif ()
    # configure the application
    if (APPLE)
        get_target_property(IsMacBundle ${ARGS_NAME} MACOSX_BUNDLE)
        if (IsMacBundle)
            message(STATUS " Configuring application ${ARGS_NAME} as a MacOs Bundled application")
            configure_bundled_Radium_app(${ARGN})
        else ()
            message(STATUS " Configuring application ${ARGS_NAME} as a command line application")
            configure_cmdline_Radium_app(${ARGN})
        endif ()
    else ()
        message(STATUS " Configuring application ${ARGS_NAME} as a command line application")
        configure_cmdline_Radium_app(${ARGN})
    endif ()

endfunction()

# ---------------------------------------------------------------------------------------------
# Install plugin resources
# Not the same than application resources becaus of the destination directory (here, the Radium 'Bundle'
function(installPluginResources)
    # "declare" and parse parameters
    cmake_parse_arguments(
            ARGS
            ""
            "TARGET;DIRECTORY;BUILDLOCATION"
            "FILES"
            ${ARGN}
    )
    # verify that the function was called with expected parameters
    if (NOT ARGS_TARGET)
        message(FATAL_ERROR " [installPluginResources] You must provide a target that need these resources")
    endif ()
    if (NOT ARGS_DIRECTORY)
        message(FATAL_ERROR " [installPluginResources] You must provide a resource directory")
    endif ()
    if (NOT ARGS_BUILDLOCATION)
        # linking resours in the current bin dir of the build tree
        set(ARGS_BUILDLOCATION ${CMAKE_CURRENT_BINARY_DIR})
    endif ()
    # compute resources dir for build tree and install tree
    get_filename_component(rsc_dir ${ARGS_DIRECTORY} NAME)
    set(buildtree_dir ${ARGS_BUILDLOCATION})
    # installing resources in the buildtree (link if available, copy if not)
    message(STATUS " [installPluginResources] Linking resources directory ${ARGS_DIRECTORY} for target ${ARGS_TARGET} into ${buildtree_dir}/${rsc_dir}")
    file(MAKE_DIRECTORY "${buildtree_dir}")
    if (MSVC OR MSVC_IDE OR MINGW)
        add_custom_command(
                TARGET ${ARGS_TARGET}
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${ARGS_DIRECTORY} "${buildtree_dir}/${rsc_dir}"
                VERBATIM
        )
    else ()
        add_custom_command(
                TARGET ${ARGS_TARGET}
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E create_symlink ${ARGS_DIRECTORY} "${buildtree_dir}/${rsc_dir}"
                VERBATIM
        )
    endif ()

    # Install in the Radium install tree
    # Identify the individual files (to preserve directory structure)
    message(STATUS " [installResources] configuring install for requested files of ${ARGS_DIRECTORY}")
    if (NOT ARGS_FILES)
        file(GLOB_RECURSE ARGS_FILES RELATIVE ${ARGS_DIRECTORY} ${ARGS_DIRECTORY}/*)
    endif ()
    foreach (file ${ARGS_FILES})
        get_filename_component(file_dir ${file} DIRECTORY)
        install(
                FILES ${ARGS_DIRECTORY}/${file}
                DESTINATION Resources/${ARGS_TARGET}/${rsc_dir}/${file_dir}
        )
    endforeach ()
endfunction()
# --------------------------

# Configuration of the build and installation procedure for a Radium plugin
# Allows to install plugin with dependent resources and to import them in applications.
# Import by the application can be done at runtime from the Radium installation directory
# or, for bundled application, at intall time by adding the available plugins into the bundle
# usage :
#   configure_radium_plugin_install(
#         NAME theTargetName # <- this must be a plugin
#         RESOURCES ResourceDir1 ResourceDir2 # <- accept a list of directories
# )
function(configure_radium_plugin_install)
    # "declare" and parse parameters
    cmake_parse_arguments(
            ARGS
            "DO_NOT_INSTALL_IN_RADIUM_BUNDLE"
            "NAME"
            "RESOURCES" # list of directories containing the resources to install - optional
            ${ARGN}
    )
    if (NOT ARGS_NAME)
        message(FATAL_ERROR " [configure_radium_plugin_install] You must provide the main target of the plugin")
    endif ()
    set_target_properties(${ARGS_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Plugins/lib)
    # configure the plugin installation
    # a plugin always install in the Radium Installation Directory, except if asked explicitely by setting the
    # TODO This might cause problems.
    if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "${RADIUM_ROOT_DIR}/Plugins" CACHE PATH
                "Install path prefix, prepended onto install directories." FORCE)
        message("Set install prefix to ${CMAKE_INSTALL_PREFIX}")
    endif ()
    # CMAKE_INSTALL_DIR variable before calling
    install(
            TARGETS ${ARGS_NAME}
            LIBRARY DESTINATION lib
    )
    # Configure the resources installation
    if (ARGS_RESOURCES)
        foreach (resLocation ${ARGS_RESOURCES})
            message(STATUS " Installing plugin resources ${resLocation} for ${ARGS_NAME} ")
            installPluginResources(
                    TARGET ${ARGS_NAME}
                    DIRECTORY ${resLocation}
                    BUILDLOCATION ${CMAKE_CURRENT_BINARY_DIR}/Plugins/Resources/${ARGS_NAME}
            )
        endforeach ()
    endif ()
endfunction()