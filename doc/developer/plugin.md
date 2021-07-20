\page develplugin How to write your own plugin
[TOC]

# Radium Plugin
Tutorial and documentation here: [https://github.com/STORM-IRIT/Radium-PluginExample](https://github.com/STORM-IRIT/Radium-PluginExample)

In addition to the Ra::Plugins::RadiumPluginInterface API,
Radium provides a set of tools simplifying the deployment of plugins, which take care of the portability of the binaries.

This manual describes how to configure, compile and use your plugin with any Radium-Plugin compatible application.

# Setting a CMakeLists.txt for a Radium plugin

To compile and use a plugin, the Radium libraries must be configured, compiled and installed beforehand.

Example CMakeLists.txt setup to compile a Radium plugin:

```cmake
 cmake_minimum_required(VERSION 3.6)
 #------------------------------------------------------------------------------
 # Policies and global parameters for CMake
 if (POLICY CMP0077)
     cmake_policy(SET CMP0077 NEW)
 endif ()
 set(CMAKE_DISABLE_SOURCE_CHANGES ON)
 set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
 #------------------------------------------------------------------------------
 # Project definition
 project(MyPlugin VERSION 1.0.0 LANGUAGES CXX)

 # Use installed Radium environment
 find_package(Radium REQUIRED Core Engine PluginBase)

 # Find and configure Qt environment (Radium requires Qt >= 5.1)
 find_package(Qt5 COMPONENTS Core REQUIRED)
 set(Qt5_LIBRARIES Qt5::Core)
 set(CMAKE_AUTOMOC ON)
 set(CMAKE_AUTORCC ON)

 # list all the sources and headers of the Plugin
 set(sources /*... */)
 set(headers /*... */)

 set(CMAKE_INCLUDE_CURRENT_DIR ON)

 # A Plugin is a library that uses these sources and headers.
 add_library( ${PROJECT_NAME} SHARED ${sources} ${headers} )

 target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
 target_compile_definitions(${PROJECT_NAME} PRIVATE "-D${PROJECT_NAME}_EXPORTS")

 target_link_libraries(${PROJECT_NAME} PUBLIC
                       Radium::Core Radium::Engine Radium::PluginBase
                       ${Qt5_LIBRARIES} )

 # Configure the plugin deployment using the Radium configuration tool
 configure_radium_plugin( NAME ${PROJECT_NAME} )
```

Such a CMakeLists.txt is as usual as others. The only command that is specific to Radium plugins is the last line
`configure_radium_plugin( NAME ${PROJECT_NAME} )` that aims at configuring the plugin installation along with
its associated resources.

The command `configure_radium_plugin`takes several parameters.
The full usage of this command is :

```cmake
configure_radium_plugin(
    NAME nameOfTheTargetOtInstall      # mandatory
    INSTALL_IN_RADIUM_BUNDLE           # optional
    RESOURCES ListOfResourcesDirectory # optional
    HELPER_LIBS ListOfHelperLibraries  # optional
     )
```

-   `NAME nameOfTheTargetOtInstall`. This parameter, mandatory, will configure how to install the target
    `nameOfTheTargetOtInstall`. This target must correspond to a configured target such the one obtained by
    `target_link_libraries(nameOfTheTargetOtInstall PUBLIC Radium::Core Radium::Engine Radium::PluginBase ${Qt5_LIBRARIES} )`

-   `RESOURCES ListOfResourcesDirectory`. This parameter, optional, will install several resources, needed by the plugin for its correct execution,
    so that the Radium resource locator system will be able to find them.
       Resources could be shader source files, images, data files, etc...

-   `HELPER_LIBS ListofHelperLibraries`. This parameter, optional, will allow fetching and install, alongside the plugin,
    libraries and their resources used by the plugin and not available in the Radium Bundle. This is useful when a
    plugin just defines the radium application interface toward services offered in an external, radium dependent, library.

-   `INSTALL_IN_RADIUM_BUNDLE`. This parameter, optional, will install the plugin and its associated resources directly
    into the Radium Bundle installation directory. This will allow to have Plugins relocatable at the same time as
    the Radium Bundle. If this parameter is given, it replaces the installed location configuration that uses
     the cmake standard  option `-DCMAKE_INSTALL_PREFIX=whereToInstallThings`

Note that, as it is the case for applications, Plugins does not need to be installed to be used by any applications.
Plugins could be loaded and used from their build-trees.

# Configuring the plugin

Configuring the plugin for compilation and install depends on several use cases that might evolve
during the plugin development.

There are three main uses cases we can identify :

1.  You are developing/testing your plugin. There is no need to install the plugin and no install option is needed when configuring using cmake.
    Any plugin compatible application might find and use it from its build tree (e.g `build-release/Plugins`).
    For this, you have to configure once your application by adding the build path of the plugin
    to the plugin search path of the application. Each time you will launch this application, it will search in all its
    registered search paths for compatible plugins.
    The advantage of not installing plugins while developing/testing is a fast startup of the application
    and efficient update of the Plugin resources as Resources directories are _linked_ (symbolic links on systems that
    support them) into the build tree.

2.  You have finished developing the plugin, and will only occasionally recompile it for maintenance. If you developed several plugins, for your own usage, and you want your
    plugin compatible applications to find and load them quickly,
     you might install your plugins in one unique location and tell the applications to load plugins from
    there. To do so, use the cmake configuration option `-DCMAKE_INSTALL_PREFIX=thePathToTheInstalledPlugins` and do a `make install`
    You can then configure your applications so that they will search plugins from the `thePathToTheInstalledPlugins` directory.
    Each time the application will start, all the plugins from this `thePathToTheInstalledPlugins` will be
    loaded, even those installed here after the configuration of the plugin location in the application.

3.  You want to distribute plugins binaries alongside the Radium bundle.
    In this case, you have to configure the plugin with the`INSTALL_IN_RADIUM_BUNDLE`option and do a `make install` to
    install the plugins in the Radium Bundle. Note that this requires write access to the Radium Bundle.


Once you have chosen the adequate use case, you can configure the plugin using cmake.

As we disable "in source" build in the CMakeLists.txt example above, we need to create a build directory:

```Shell
mkdir build-release && cd build-release
```

With Radium-Engine installed in the directory `pathToRadiumInstallation`,
configure and compile the project with :

```Shell
cmake ../ -DRadium_DIR=pathToRadiumInstallation/lib/cmake/Radium
cmake --build .
```

If you just want to use the plugin directly from its build-tree, the resulting dynamic library will be found in the
`build-release/Plugins/lib` directory, and the associated resources will be linked (or copied on systems that do not
support symbolic links) in the directory `build-release/Plugins/Resources`. In this case, note that the plugin from the
build-tree could generally not be moved as it is linked to used libraries using relative paths.

If you install the plugin, execute the command `cmake --install`
that will install the plugin according to your configuration choice as described above (default installation destination, user defined installation directory or Radium Bundle installation).
The dynamic library will then be copied into  `pathToInstallPlugins/lib` and the Resources will be copied into
`pathToInstallPlugins/Resources`.


Remember that resources associated with the plugin using the option `RESOURCES ListOfResourcesDirectory` of the
`configure_radium_plugin` command are _linked_ into the build-tree (on systems supporting symbolic links)
and _copied_ into the installed locations.

# Using the Plugin

Any application that inherits from `Ra::Gui::BaseApplication` could use any Plugin developed using the
Radium Engine.
As plugins are Qt objects that implement specific interface, it is also permitted to have any Qt
Application to use plugins. Meanwhile, the following documentation is related to the Radium BaseApplication
derived applications.

If the plugin is installed in the Radium Bundle directory, it will be automatically loaded by any plugin-aware
application.
Recall that, when developing a plugin, the installation in the Radium bundle directory is not always permitted nor efficient.

It could then be useful to use the Plugin from its own installation directory or directly from its build-tree.
To do that, the application could register the plugin directory location by calling the
`Ra::Gui::BaseApplication::addPluginDirectory( const std::string& pluginDir );` method that will record, in the
QSettings file associated with the application, the directory in which the Radium Engine will look for plugins.
Once this directory is registered, any plugin that will be found in this directory will be loaded at each application
startup if the build type of the plugin is compatible with the build type of the application (release, debug, ...).
If the plugin location that is registered correspond to the build tree location of the plugin, there is no need to
install it.

Note that the main Radium demonstration application (`main-app`) offer a menu entry
(`File/Plugins/Add plugin path`) that call the registration method once the user has selected a plugin location.
