\page cmakeutilities CMake setup
[TOC]

As Radium relies on the [CMake](https://cmake.org/documentation/) build and configuration system, this
documentation focuses on how to configure your build system to use and extend Radium using Radium cmake utilities.

Complete functional examples using these cmake scripts are accessible in the `src/tests/ExampleApps` and
`src/tests/ExamplePluginWithLib` directories of the
[Radium source distribution](https://github.com/STORM-IRIT/Radium-Engine).

# Radium cmake utilities

Once installed, either from source or from pre-built binaries, Radium could be used and extended in any application,
library or plugin as a cmake package.

## Using Radium in your application
To integrate the Radium libraries into your build-chain configuration, you have to ask cmake to find the Radium package.
As for any cmake package, this could be done by adding the following line into your `CMakeLists.txt` file:
~~~{.cmake}
find_package(Radium REQUIRED Core Engine)
~~~
Remember to configure your project by giving the cmake command line option
`-DRadium_DIR=/path/to/installed/Radium/lib/cmake/Radium` so that the `find_package` command could find the Radium
package definition.
Once found, the Radium package defines a cmake target for each requested components (`Core` and `Engine` in the
example above) in the cmake namespace `Radium`.

When configuring your own target in your `CMakeLists.txt` file, you just need to link with those targets to get access
to all the public interface of Radium (include search path, libraries, ...), for instance:
~~~{.cmake}
target_link_libraries (myTarget PUBLIC Radium::Core Radium::Engine)
~~~

### Using Radium components

If your application does not need all the radium components, you can select which ones you want among the following :
- Core : search only for the availability of the target Radium::Core
- Engine : search only for the availability of the target Radium::Engine
- Gui : search for the Qt-based Gui toolkit
- PluginBase : search for the Qt-based plugin development interface
- IO : search only for the availability of the target Radium::IO

  On this target, you might also ask for support of several file loaders using the following properties defined on the
  target Radium::IO
    - RADIUM_IO_USE_ASSIMP : Identify if Radium::IO was compiled with assimp support
    - RADIUM_IO_USE_TINYPLY : Identify if Radium::IO was compiled with tinyply support.
      You might use these properties to define compilation macro in your code
      ~~~{.cmake}
      get_target_property(USE_ASSIMP Radium::IO RADIUM_IO_USE_ASSIMP)
      if (${USE_ASSIMP})
       target_compile_definitions(yourTarget PRIVATE ADD_ASSIMP_LOADER)
      endif()
      ~~~


The radium package also defines several cmake functions, described below, that you can use to ease the configuration of
your application, library or plugin, mainly to install them in a relocatable way while allowing their use from their
own build-tree.

The functions defined by the Radium package are the following:
1. Client application configuration
  - [`configure_radium_app`](#configure_radium_app).
2. Extending Radium through libraries
  - [`configure_radium_library`](#configure_radium_library).
  - [`install_target_resources`](#install_target_resources).
  - [`configure_radium_package`](#configure_radium_package).
  - [`radium_exported_resources`](#radium_exported_resources).
3. Extending Radium through plugins
  - [`configure_radium_plugin`](#configure_radium_plugin).

## Client application configuration
### configure_radium_app {#configure_radium_app}
~~~{.cmake}
configure_radium_app(
    NAME applicationName                        # The name of the executable target to configure as a relocatable application
    [USE_PLUGINS]                               # Set this if the application uses installed plugins.
    [RESOURCES ResourceDir1 ResourceDir2 ...]   # List of directories to install as application resources
)
~~~

This function configures the executable target `<NAME>` for installation so that the installation directory is
relocatable and distributable.

This function takes the following parameters:

*Parameter name*                            | *Parameter description*
--------------------------------------------|--------------------
`<NAME> applicationName`                    | The name of the *executable* target to configure and install
`<USE_PLUGINS>`                             | If this option is given, the plugins installed into the Radium bundle at
the installation time will be copied into the application bundle.
`<RESOURCES> ResourceDir1 ResourceDir2 ...` | Optional list of directories to be considered as application resources
and installed into the application bundle.

When installed into a directory `<prefix>`, the application bundle has the following structure on linux, windows or on
MacOsX (if the executable is not configured as a `MACOSX_BUNDLE`):

```
<prefix>/
├── bin/
|   └──  applicationName
├── Resources/
|   ├── ResourceDir1/
|   ├── ResourceDir2/
|   ...
```

For `MACOSX_BUNDLE` applications, a standard `.app` MacOsX application is generated (See
[https://developer.apple.com/.../BundleTypes/BundleTypes.html](https://developer.apple.com/library/archive/documentation/CoreFoundation/Conceptual/CFBundles/BundleTypes/BundleTypes.html#//apple_ref/doc/uid/10000123i-CH101-SW19)
for bundle structure documentation).

### Limitations
For now (Radium version of May 2021):
  - Applications on Linux can be relocated only on the build system (AppImages will come soon).

To get relocatable packages, download pre-built binaries here:
[https://github.com/STORM-IRIT/Radium-Releases/releases](https://github.com/STORM-IRIT/Radium-Releases/releases)

## Extending Radium through libraries
### configure_radium_library {#configure_radium_library}
~~~{.cmake}
configure_radium_library(
    TARGET targetName               # Name of the target to configure as a Radium Library
    FILES file1[file2[file3 ...]]   # list of headers to install
    [TARGET_DIR directoryName]      # Name of the directory where files are installed (default <prefix>/include/<TARGET>)
    [NAMESPACE NameSpace]           # Namespace of the imported target (default Radium)
    [PACKAGE_CONFIG file.cmake.in]  # name of the configure script model for the given target
    [PACKAGE_DIR packageDirName]    # Name of the directory where the cmake config file will be installed (default <prefix>/lib/cmake/Radium)
)
~~~
This cmake function configures the `<TARGET>` for installation and for further import in client project using
`find_package(<TARGET>)`.
In order to use the library either through an imported target from an installed binary or as a project target in another
component on the same build tree, this function defines an alias target with the same name as the imported one.
This function also defines the symbol `<TARGET>`_EXPORTS so that, it could be used to allow symbol import/export from
dynamic library.

This function takes the following parameters:

*Parameter name*   | *Parameter description*
-------------------|--------------------
`<TARGET>`         | The name of the target to configure
`<FILES>`          | Expected to be public headers, they will be installed in the include directory of the project installation configuration `${CMAKE_INSTALL_PREFIX}/include`.
`<TARGET_DIR>`     | Optional. `<FILES>` will be installed into the `${CMAKE_INSTALL_PREFIX}/include/<TARGET_DIR>` directory. If not, the files will be installed into `${CMAKE_INSTALL_PREFIX}/include/<TARGET>` directory.
`<NAMESPACE>`      | If given, the imported target will be `<NAMESPACE>::<TARGET>`. If not, the imported target will be `Radium::<TARGET>`
`<PACKAGE_CONFIG>` | If given, a configure script, to be used by `find_package`, will be generated. If not, only the exported targets will be generated.
`<PACKAGE_DIR>`    | If given, the cmake configuration script `<TARGET>Config.cmake` searched by `find_package(<TARGET>)` will be installed in the directory `${CMAKE_INSTALL_PREFIX}/<PACKAGE_DIR>`. If not, the configure script will be installed in the directory `<${CMAKE_INSTALL_PREFIX}/lib/cmake/Radium`.

Note that the parameters `<PACKAGE_CONFIG>` and `<PACKAGE_DIR>` could be omitted as the associated cmake package module
could be generated using the function [`configure_radium_package`](#configure_radium_package) documented below.
If these parameters are given here, they will be forwarded to this function.

In order to allow usage of installed library in all supported systems (Linux, macOS, Windows), one of the public header,
that might be named, e.g. `<TARGET>Macros.hpp` and included all the public or private headers, should contain the
following :

~~~{.cpp}
#pragma once
#include <Core/RaCore.hpp>
#if defined <TARGET>_EXPORTS
#    define <TARGET>_API DLL_EXPORT
#else
#    define <TARGET>_API DLL_IMPORT
#endif
~~~

Then, in the library source code, each exported symbol should be prefixed by `<TARGET>_API` e.g.
~~~{.cpp}
#pragma once
#include < <TARGET>/<TARGET>Macros.hpp >
class <TARGET>_API MyAwesomeMaterial : public Ra::Engine::Data::Material
{
    ...
};

<TARGET>_API bool initializeMyLibrary();
~~~

Assuming a library `MyLib` was configured as follows:
~~~{.cmake}
...
configure_radium_library(
    TARGET MyLib
    NAMESPACE MyLib
    PACKAGE_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    PACKAGE_DIR ${CMAKE_INSTALL_PREFIX}/lib/cmake
    FILES "${public_headers};${public_inlines}"
)
~~~

This library could be used either from its installed binaries :
~~~{.cmake}
find_package(MyLib)
...
target_link_libraries(
    MyProject
    PUBLIC
    Radium::Core
    Radium::Engine
    MyLib::MyLib
)
~~~
or from the same build tree :
~~~{.cmake}
add_subdirectory(MyLibSource)
...
target_link_libraries(
    MyProject
    PUBLIC
    Radium::Core
    Radium::Engine
    MyLib::MyLib
)
~~~

### install_target_resources {#install_target_resources}
~~~{.cmake}
install_target_resources(
    TARGET targetName                # Name of the target for which resources must be installed
    RESOURCES_DIR resourceDirectory  # location, in the source tree, of the resources
    [RESOURCES_INSTALL_DIR baseDir]  # Name of the base directory of the installed (or linked in the buildtree) resources
    [FILES [file1[file2...]]]        # Resources individual files to install
~~~

When a Radium based component needs to access several resources to implement its functionalities, this function
installs the required resources.

This function takes the following parameters:

*Parameter name*                     | *Parameter description*
-------------------------------------|--------------------
 `<TARGET> targetName`               | The name of the target to configure.
 `<RESOURCES_DIR> resourceDirectory` | The directory in the source tree that contains the resource to install.
 `<RESOURCES_INSTALL_DIR> baseDirectory`     | If given, the resources will be installed into the directory `<install_prefix>/Resources/<RESOURCES_INSTALL_DIR>/`. If not, the resources will be installed into the directory `<install_prefix>/Resources/` where `<install_prefix>` is the installation directory of the target or the current binary dir of the build tree.
  `<FILES> file1 file2 ...`           | If given, this allow to install only the given files from `<DIRECTORY>`. If not, all the files from `<DIRECTORY>` will be installed.

For a Radium-based software component, resources can be of several types:
  - Text files, that contain some glsl source code used for shaders.
  - Images used internally as textures or source of data.
  - Precomputed data, stored in whatever format.

To access resources, the component might rely on a Ra::Core::Resources::ResourcesLocator object. Such an object allows
accessing files located in some predefined directories:
  - Radium default resource directory. This directory is located at `<prefix>/Resources/Radium` where `<prefix>` is the
    Radium installation directory.
  - Current executable directory. This directory is where the currently running executable is located.

In order to allow client component to access its own resources, and to allow that either on the build-tree or once the
component is installed without having to recompile or modify anything, this function both links (on supported systems,
copy elsewhere) and installs the component resources at the given places.

As the process must be system and component independent it is the responsibility of the client package to configure
the proper paths for resources management.

According to the installation layout of the Radium lib, that matches the GNU standard, but also to the macOS bundle
architecture, Resources might be localized relatively to any binary object (library, executable, plugin, ...)
The Radium installation hierarchy, that embeds base libraries, their resources and some executable files is the
following:
```
<prefix>/
├── include/
|   ├── Core/
|   ├── Engine/
|   ...
├── lib/
|   ├── cmake/
|   |   └── Radium/
|   ├──  libCore.so
|   ...
├── bin/
|   ├──  sandbox
|   ...
├── Resources/
|   ├── Radium
|   |   ├── Shaders/
|   |   └── Config/
|   ...
```
In this hierarchy, Resources are located in the relative path `../Resources/` from any binary (executable or lib).
The source code structure of the Radium libraries allows to have the same property on the build tree.
When a Radium component needs to access some resources, it could either use the Ra::Core::Resources::ResourcesLocator::getRadiumResourcesDir() method or search for the resource relatively to a symbol defined in the component.

To do this, the source code (.cpp file) of the component could contain the following.
~~~{.cpp}
#include <MyComponent.hpp>

static int MyComponentMagick = 0x0F0F0F0F;
...
// get the resource path relatively to the current location of the binary
auto resourcesPath = Ra::Core::Resources::ResourcesLocator(
                    reinterpret_cast<void*>( &MyComponentMagick ),
                    "/Resources/MyComponentResources/",
                    "../" );
// Read files from the found resourcesPath that will prefix the Resource files or directory
~~~

When using the [`install_target_resources`](#install_target_resources) function to configure the component resources' installation, the following call should be made to allow access from both the build-tree, and the install-tree using the above snippet.

~~~{.cmake}
# Assuming MyComponentTarget was configured as a library, plugin or executable, this will configure the resource access in the build tree and the install tree
install_target_resources(
    TARGET MyComponentTarget                            # The name of the target
    RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/MyResource    # Where are the resources in the source tree
    RESOURCES_INSTALL_DIR MyComponentResources # the resources will be installed in <prefix>/Resources/MyComponentResources
)
~~~

### configure_radium_package {#configurePackage}
~~~{.cmake}
configure_radium_package(
    NAME packageName                # The name of the package to install
    PACKAGE_CONFIG configFile.in    # The package configuration file
    [PACKAGE_DIR packageDirName]    # Name of the directory where the cmake config file will be installed (default <prefix>/lib/cmake/Radium)
)
~~~
This cmake function configures the package `packageName` for installation and for further import in client project using `find_package(<TARGET>)`.

This function takes the following parameters:

*Parameter name*                 | *Parameter description*
---------------------------------|--------------------
`<NAME> packageName`             | The name of the package to configure and install
`<PACKAGE_CONFIG> configFile.in` | The configure script to be used by `find_package`.
`<PACKAGE_DIR> packageDirName`   | If given, the cmake configuration script `<TARGET>Config.cmake` searched by `find_package(<TARGET>)` will be installed in the directory `${CMAKE_INSTALL_PREFIX}/<PACKAGE_DIR>`. If not, the configure script will be installed in the directory `<${CMAKE_INSTALL_PREFIX}/lib/cmake/Radium`.

This function is called implicitly, when defining a single component package, when the parameters `<PACKAGE_CONFIG>` and `<PACKAGE_DIR>` are given to the library configuration function [`configure_radium_library`](#configure_radium_library).

This function also allows defining multi-component packages for selective import using the `find_package(packageName [COMPONENTS comp1 comp2 ...]` command when called explicitly with an appropriate `PACKAGE_CONFIG` parameter.

### radium_exported_resources {#radium_exported_resources}

~~~{.cmake}
radium_exported_resources(
    TARGET target            # The name of the exported target
    ACCESS_FROM_PACKAGE path # The path from the installe package module to the installed resources directory
    [PREFIX resourcesPrefix] # Name of the directory in wich the resources are installed
)
~~~
This function, intended to be used in a `Config.in`cmake script defines the resources access properties for the exported target `<target>`.
Based on these properties, client application and libraries might access to the resources of an imported target found by FindPackage(PackageName)

This function takes the following parameters:

*Parameter name*             | *Parameter description*
-----------------------------|--------------------
`<TARGET> target`            | The name of the package to configure and install
`<ACCESS_FROM_PACKAGE> path` | The path from the installed package module to the installed resources directory.
`<PREFIX> resourcesPrefix`   | Name of the directory in which the resources are installed.

As described above, the installation tree of any installed target, if configured with radium cmake utilities, looks like the following
```
prefix/
├── include/
|   ├── ...
├── lib/
|   ├── cmake/
|   |   └── targetConfig.cmake
|   ├──  libtarget.so (or.dylib, .dll)
|   ...
├── Resources/
|   ├── resourcesPrefix
```

where `target` is the name of the exported target, `targetConfig.cmake` the cmake package module installed when installing the target and `resourcesPrefix` the resource prefix defined when installing the resources for the target (can be empty).

In the above example, the `<ACCESS_FROM_PACKAGE> path` parameter of the function
[`radium_exported_resources`](#radium_exported_resources) should be set to `"../.."`
This parameter must contain the relative path that start from the package module installation directory,
here `<prefix>/lib/cmake`, to the resources' installation directory, here `<prefix>/Resources`.

## Extending Radium through plugins
### configure_radium_plugin {#configure_radium_plugin}

~~~{.cmake}
configure_radium_plugin(
    NAME pluginName                            # Name of the target (standard dynamic library) corresponding to the plugin
    [RESOURCES ResourceDir1 [ResourceDir2 ...] # Optional. List of resources directories (only directories are allowed as resources)
    [HELPER_LIBS lib1 [lib2 ...]]              # Optional. List of libraries (local target or imported targets) the plugin depends on
    [INSTALL_IN_RADIUM_BUNDLE]                 # Optional. If given, the plugin is installed into ${RADIUM_ROOT_DIR}/Plugins. If not, the installation is performed into ${CMAKE_INSTALL_PREFIX}
~~~
This cmake function configures the target `NAME pluginName` to be compiled, linked and optionally installed as a Radium Plugin.
The plugin and its optional resources might then be used in any plugin-compatible Radium application.
If the plugin is installed in the Radium distribution bundle, it will be embedded in all bundled application installed later.

This function takes the following parameters:

*Parameter name*        | *Parameter description*
------------------------|--------------------
`NAME pluginName`               | Name of the target to be linked and installed as a plugin
`[RESOURCES ResourceDir1 [ResourceDir2 ...]` | Optional. List of directories, in the source tree, containing the plugin own resources.
`[HELPER_LIBS helperlib1 [helperlib2 ...]]` | List of libraries the plugin depends on.
`[INSTALL_IN_RADIUM_BUNDLE]` | Optional. If given, the plugin is installed in the Radium general bundle (`${RADIUM_ROOT_DIR}/Plugins`) instead of being installed in the `${CMAKE_INSTALL_PREFIX}`

The `pluginName` target must be a dynamic library that exports a class inheriting from `QObject` and Ra::Plugins::RadiumPluginInterface.
The optional resources associated with the plugin are own resources that the plugin needs to be functional and will be installed as described below.
The optional helper libraries can be local targets, configured in the same build tree, or imported targets, resulting from a `find_package(...)`.
If a helper library need to access its own resources, it must be configured using [`configure_radium_library`](#configure_radium_library) if it is a local target or imported from an installed target configured the same way.

The directory hierarchy in which a plugin will be installed (or where the build target will be) is the following:
```
<prefix>/
├── lib/
|   ├──  libpluginName.so (or.dylib, .dll)
|   ├──  helperlib1
|   ├──  ...
├── Resources/
|   ├── pluginName
|   |     ├── ResourceDir1
|   |     ...
```
where `<prefix>` refer to the base plugin dir into the build tree or the installation directory.
 - In the build tree, `<prefix>` refers to the directory `${CMAKE_CURRENT_BINARY_DIR}/Plugins`. Resources directories are linked into `Resources` directory on system that supports links, copied otherwise. Helper libs are not copied into the hierarchy.
 - When installed into the Radium distribution bundle, `<prefix>` refers to `${RADIUM_ROOT_DIR}/Plugins`. Optional resources directories are copied into `Resources` directory.
 - When installed in the user specified install location, `<prefix>` refers to `${CMAKE_INSTALL_PREFIX}`. Optional resources directories are copied into `Resources` directory.

When a plugin is installed and depends on a helper library that embed its own resources, the helper resources are copied into the `<prefix>/Resources` directory so that the helper lib could find its resources as described in the section [`install_target_resources`](#install_target_resources).
In this hierarchy, a plugin will access to its resources in a way very similar than any other library. Only one level of hierarchy, whose name is the plugin name, will be added.


# How to write your CMakeLists.txt
When writing your cmake configuration script `CMakeLists.txt`, you might rely on the following guideline to configure the project `ProjectName`.
Note that these are only guidelines and that you can always write your cmake script from scratch, assuming you understand what you do.

## General cmake preamble
~~~{.cmake}
# -------------------------------------------------------
# Recommended preamble for cmake configuration
# -------------------------------------------------------
# Radium package requires cmake minimum version 3.13
cmake_minimum_required(3.13)
# It is recommended to disable in-source build
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
# if not explicitely set by the user, set the install prefix to a bundle directory in the build tree
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/installed-${CMAKE_CXX_COMPILER_ID}" CACHE PATH
        "Install path prefix, prepended onto install directories." FORCE)
    message("Set install prefix to ${CMAKE_INSTALL_PREFIX}")
    set(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT False)
endif ()
# -------------------------------------------------------
# Configure the project
# -------------------------------------------------------
project(ProjectName VERSION 1.0.0)

# The project requires Core and Engine component from Radium
find_package(Radium REQUIRED Core Engine)

...
~~~

Then, according to the type of the targets defined in your project (application, single library, set of libraries, plugin, mix of them), you might configure your project according to the following guidelines.

## Configuring client application
Configuring an installable and relocatable application is very easy with Radium cmake scripts.
First, an executable target that depends on Radium libraries (internal Radium libraries or client libraries) should be configured with a cmake script such as
~~~{.cmake}
...
# Configure an relocatable executable target (here a bundle on MacOsX)
add_executable(${PROJECT_NAME} MACOSX_BUNDLE
    ...
)
~~~

Then, the application is configured to be relocatable after installation with
~~~{.cmake}
# Configure the Radium application installation
configure_radium_app(
    NAME <applicationName>
    USE_PLUGINS
    RESOURCES <applicationResources>
)
~~~

Once compiled and installed, the directory `<${CMAKE_INSTALL_PREFIX}>`, set to `installed-<COMPILER_ID>-<CONFIGURATION>` into the root of the build tree if not specified at configure time, is a relocatable directory that contains the bundled application and associated resources.

Note that, on MacOsX, if the executable target is configured using the `MACOSX_BUNDLE` option, only the `bin` directory
is created into this bundle, and it contains the relocatable `.app` bundle.
If the option `MACOSX_BUNDLE` is not given, the executable will not be a bundled macOS `.app` but a command line
application and will be installed as any application on Linux.

The resources associated to an application are installed using the [`install_target_resources`](#install_target_resources).

## Configuring client and extension libraries
### Configuring the library target
An installable library could be used to add functionalities built over the Radium libraries and to make these functionalities available to developers.
Once installed, a library consists in:
 - a set of public headers
 - a dynamic library (`.so`, `.dylib` or `.dll`)
 - a cmake config file so that the library could be fetched by the cmake command `find_package`
 - [optionally] a set of associated resources

In order to fulfill Radium conventions, a library will be identified by a name into a namespace.

To configure an installable library based on Radium, it is recommended to have the following `CMakeLists.txt`

~~~{.cmake}
set(NAME_OF_LIBRARY NameOfTheLibrary)
# Build the lists of files constituting the target
set(sources
    # list of the source files (e.g. .cpp) for the library
)
set(public_headers
    # list of public headers (installed .hpp files) for the library
)
set(public_inlines
    # list of public inline code (installed .inl files) for the library
)
set(headers
    # list of private headers (not installed) for the library
)
set(inlines
    # list of private inline code (not installed) for the library
)
# this is optional
set(resources
    # list of files or directories that contains library specific resources (shaders, images, data, ...)
    Shaders
)

# configuring the target as a dynamic library
add_library(
    ${NAME_OF_LIBRARY} SHARED
    ${sources}
    ${headers}
    ${inlines}
    ${public_headers}
    ${public_inlines}
    ${resources}
)

# Setting specific target_* properties
...
target_link_libraries(
    ${NAME_OF_LIBRARY}
    PUBLIC
    # Give here the list of the dependency (e.g. Radium::Core ... ) to link with
)

# Configuring the library to be inserted into the Radium exosystem
message("Configure library ${NAME_OF_LIBRARY} for insertion into Radium exosystem")
configure_radium_library(
    TARGET ${NAME_OF_LIBRARY}
    NAMESPACE ${NAME_OF_LIBRARY} # The imported target will be ${NAME_OF_LIBRARY}::${NAME_OF_LIBRARY}
    PACKAGE_DIR ${CMAKE_INSTALL_PREFIX}/lib/cmake
    PACKAGE_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    FILES "${public_headers};${public_inlines}" # They will be installed in <prefix>/include/${NAME_OF_LIBRARY}/
)
~~~

The `Config.cmake.in` file used to configure the package could be written similarly to what described into the following section but with only one component.

### Configuring the cmake find_package module
When configuring a library, a cmake package configuration file should be written so that the cmake package configuration module is installed alongside the library.
Meanwhile, when several libraries must be used as components in a single package (e.g. the Radium internal libraries are all gathered into the single Radium package), a more general configuration module has to be defined.

For this, instead of defining a configuration package for each configured library, the parameter `PACKAGE_CONFIG` of the function [`configure_radium_library`](#configure_radium_library) should be omitted
and the function [`configure_radium_package`](#configurePackage) should be used.

Standard usage of this function requires to have some libraries configured like the following:
~~~{.cmake}
...
add_library(
    <firstLib> SHARED
    ...
}
configure_radium_library(
    TARGET <firstLib>
    FILES "${firstLib_public_headers}"
    TARGET_DIR <include_prefix>
    NAMESPACE <namespace>
    PACKAGE_DIR ${CMAKE_INSTALL_PREFIX}/lib/cmake
)
...

add_library(
    <secondLib> SHARED
    ...
}
configure_radium_library(
    TARGET <secondLib>
    FILES "${secondLib_public_headers}"
    TARGET_DIR <include_prefix>
    NAMESPACE <namespace>
    PACKAGE_DIR ${CMAKE_INSTALL_PREFIX}/lib/cmake
)
...
~~~

Once this is done, the package should be configured using
~~~{.cmake}
...
configure_radium_package(
    NAME <packageName>
    PACKAGE_DIR ${CMAKE_INSTALL_PREFIX}/lib/cmake
    PACKAGE_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/<packageName>Config.cmake.in
)
~~~

Where the configuration file `<packageName>Config.cmake.in` should be written as the example below.
(see [cmake documentation](https://cmake.org/cmake/help/v3.13/manual/cmake-packages.7.html) for deeper explanations on how to write a package configuration file).
~~~{.cmake}
# list the supported components
set(<packageName>_supported_components <firstLib> <secondLib>)

# mark the package as found
set(<packageName>_FOUND True)

# verify the list of requested component. If none is given, request for all components
if (NOT <packageName>_FIND_COMPONENTS)
    set(<packageName>_FIND_COMPONENTS ${<packageName>_supported_components})
endif()

# search for requested components
foreach(_comp ${<packageName>_FIND_COMPONENTS})
  list(FIND <packageName>_supported_components ${_comp} ${_comp}_FOUND)
  if (${${_comp}_FOUND} EQUAL -1)
    set(<packageName>_FOUND False)
    set(<packageName>_NOT_FOUND_MESSAGE "Unsupported <packageName> component: ${_comp}")
  else()
    set(${_comp}_FOUND True)
  endif()
endforeach()

# configure individual components
#------------------------------------------------------------------------------------------------------------
include(CMakeFindDependencyMacro)

# component <firstLib>
if (<firstLib>_FOUND)
    # manage component dependencies (e.g. some Radium components)
    if ( NOT Radium_FOUND)
        find_dependency(Radium COMPONENTS Core Engine REQUIRED)
    endif()
    # include the target definition generated by configure_radium_library (must be installed in the same directory than the package module)
    include("${CMAKE_CURRENT_LIST_DIR}/<firstLib>Targets.cmake" )
endif()

# component <secondLib>
if (<secondLib>_FOUND)
    # manage component dependencies (e.g. <firstLib> must be requested and <secondLib> depends also on Qt5)
    if (NOT <firstLib>_FOUND)
        set(<secondLib>_FOUND False)
        set(<packageName>_FOUND False)
        set(<packageName>_NOT_FOUND_MESSAGE "Component <secondLib> requires the component <firstLib>")
        # Note that you can also explicitely configure first lib instead of raising an error
    else()
        if (NOT Qt5_FOUND)
            find_dependency(Qt5 COMPONENTS Core Widgets REQUIRED)
        endif()
        include("${CMAKE_CURRENT_LIST_DIR}/<secondLib>Targets.cmake" )
    endif()
endif()
~~~

Based on the above example, once the package `<packageName>` is found, the targets `<namespace>::<firstLib>` and  `<namespace>::<secondLib>` will be imported in the current project and should be used as any imported Radium targets.

### Configuring library resources
When a library need to access resources, the [`install_target_resources`](#install_target_resources) must be called once the library target is configured as explain above.
This result, as an example, in the following cmake snippet if the library `${NAME_OF_LIBRARY}` need to access glsl source file from the resource directory `${CMAKE_CURRENT_SOURCE_DIR}/Shaders` in the source tree.
~~~{.cmake}
# Optional, if the library need resources
install_target_resources(
        TARGET ${NAME_OF_LIBRARY}
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Shaders
)
~~~

In this case, the library must also associate its resources with its exported target so that, when the library is imported in an application, plugin or other library, the resources will be made available.
For doing this, the corresponding package configuration file (e.g. `<packageName>Config.cmake.in`) should add properties to all exported targets describing their resources.
The following properties should be added to the corresponding targets:
  - `RADIUM_TARGET_RESOURCES_PREFIX` defines the directory prefix to access the library resources
  - `RADIUM_TARGET_INSTALLED_RESOURCES` specifies where are installed the resources on the system.

These properties, located in the installation tree, are the same as those defined when installing resources for a library and should be set by using the function [`radium_exported_resources`](#radium_exported_resources) in the
`<packageName>Config.cmake.in` such as
~~~{.cmake}
radium_exported_resources(
    TARGET <namespace>::<libtarget>
    ACCESS_FROM_PACKAGE <pathFromPackageToResourcesInInstallTree>
    PREFIX <resourcesPrefix>
)
~~~
where `<resourcesPrefix>` corresponds to the parameter `PREFIX` used when installing the resources for the target `<namespace>::<libtarget>`.

## Configuring an application plugin
See [How to write your own plugin](@ref develplugin).
