\page basicsCompileRadium Compilation instructions
[TOC]

# Dependencies
All the dependencies are automatically fetched and compiled at build time.
 * [Core] Eigen, OpenMesh
 * [Engine] glm, globjects, glbindings
 * [IO] Assimp
 * [Gui] Qt Core, Qt Widgets and Qt OpenGL v5.5+ (5.14 recommended)
 * stb_image

**See @ref develbuildchain for technical details on dependencies management and how to provide locally installed dependencies.**

Minimal requirements
 * OpenGL 3+ / GLSL 330+
 * CMake 3.15.7+

# Supported compiler and platforms

The following platforms and tool chains have been tested and should work :

 * *Windows* : IDEs: Visual Studio 2019 (2017 is not supported due to embedded cmake version), QtCreator. Command Line: cmake+ninja+MSVC(2017 or 2019) .
 * *Mac OSX* : gcc 10 or higher, Apple clang
 * *Linux* : gcc 10  or higher, clang

## Continuous Integration:
 * *Linux (gcc10), Windows (MSVC 19.28.29335.0), and Mac OSX (AppleClang 12)* https://github.com/STORM-IRIT/Radium-Engine/actions

# Build instructions

## Getting dependencies
They are automatically fetched by cmake at compile time.
To force cmake re-fetching the dependencies, called
~~~bash
make update
~~~

## Folder structure
Radium-Engine relies on CMake buildchain on all supported platforms.
In most cases, building should be pretty straightforward, provided that cmake can locate the dependencies.
If cmake doesn't locate the Qt files (e.g. if you manually installed Qt as opposed to using your distribution's package),
see the troubleshooting section below.

Build output is generated in the `Radium-Engine/Bundle-*` directory (with `*` the name of the CXX compiler, followed by the build type), with the following structure, if externals are compiled along Radium
~~~
Bundle-*
 - bin/  include/  lib/  libdata/  LICENSE  README.md  Resources/  share/
~~~
Or if externals are compiled locally :
~~~
Bundle-*
 - bin/  include/  lib/  LICENSE  README.md  Resources/  share/
~~~

## Configure build

Radium offers the following build options:
~~~bash
// Enable coverage, gcc only. Experimental, need ENABLE_TESTING
RADIUM_ENABLE_COVERAGE:BOOL=OFF
--
// Enable testing. Tests are automatically built with target all
RADIUM_ENABLE_TESTING:BOOL=ON
--
// Include Radium::Core in CMake project
RADIUM_GENERATE_LIB_CORE:BOOL=ON
--
// Include Radium::Engine in CMake project
RADIUM_GENERATE_LIB_ENGINE:BOOL=ON
--
// Include Radium::Gui in CMake project
RADIUM_GENERATE_LIB_GUI:BOOL=ON
--
// Include Radium::IO in CMake project
RADIUM_GENERATE_LIB_IO:BOOL=ON
--
// Include Radium::PluginBase in CMake project
RADIUM_GENERATE_LIB_PLUGINBASE:BOOL=ON
--
// Check submodules during build (will be automatically disabled after run)
RADIUM_GIT_UPDATE_SUBMODULE:BOOL=ON
--
// Install documentation. If RadiumDoc is compiled, install documentation to bundle directory for install target
RADIUM_INSTALL_DOC:BOOL=ON
--
// Value of CMAKE_INSTALL_MESSAGE for dependencies. See documentations of CMAKE_INSTALL_MESSAGE for possible values
RADIUM_EXTERNAL_CMAKE_INSTALL_MESSAGE=NEVER
--
// Disable Radium Log messages
RADIUM_QUIET:BOOL=OFF
--
// Provide loaders based on Assimp library
RADIUM_IO_ASSIMP:BOOL=ON
--
// Provide depricated loaders (to be removed without notice)
RADIUM_IO_DEPRECATED:BOOL=ON
--
// Provide loaders based on TinyPly library
RADIUM_IO_TINYPLY:BOOL=ON
--
// [addExternalFolder] Skip updating Core::external (disable for rebuild)
RADIUM_SKIP_CORE_EXTERNAL:BOOL=ON
--
// [addExternalFolder] Skip updating Engine::external (disable for rebuild)
RADIUM_SKIP_ENGINE_EXTERNAL:BOOL=ON
--
// [addExternalFolder] Skip updating IO::external (disable for rebuild)
RADIUM_SKIP_IO_EXTERNAL:BOOL=ON
--
// Update version file each time the project is compiled (update compilation time in version.cpp)
RADIUM_UPDATE_VERSION:BOOL=ON
--
// Use double precision for Scalar
RADIUM_USE_DOUBLE:BOOL=OFF
~~~

All radium related cmake options (with their current values) can be printed with `cmake -LAH | grep -B1 RADIUM` (on linux like system)

\warning For computers with low RAM capacities (under 12G) we recommend to set the `CMAKE_BUILD_PARALLEL_LEVEL` environment variable to a reasonable value (i.e. 2) to prevent the computer from swapping.

##  Building on Linux/MacOS (command line instruction)

Out-of source builds are mandatory, we recommand to follow the usual sequence:

~~~bash
$ mkdir build
$ cd build
$ cmake ..
$ make
~~~

Finally, the `install` target will copy all the radium related library in the same place, usefull for App compilation

~~~bash
$ make install
~~~

Note that installation requires write access on the installation directory.

## Building on Microsoft Windows with Visual Studio

### Supported versions of MSVC
Radium requires MSVC 2019 or superior, as it relies on:
* C++11/C++14/C++17 features such as `constexpr`,
* cmake built-in support

Our Continuous Integration systems uses Microsoft Compiler 2017, in combination with cmake and ninja.
Using Visual Studio 2017 with cmake support is however not possible: VS is shipped with cmake: 3.12, while Radium requires cmake 3.13 at least. We recommand to use Visual Studio 2019 in that case.
Qt 5.15+ is distributed with binaries precompiled with MSVC 2019, but Qt binaries precompiled with MSVC2017 does not break the build.

### Dependencies

*Qt* distributes precompiled libraries for VS 2017 - 64 bits (tested with 5.10 and 5.12).
If using earlier versions of Qt (5.5)  or a different toolset you may have to compile Qt yourself.
You will probaby have to manually point cmake to the Qt folder.

Other dependencies (Eigen, Assimp, glbinding, globjects and glm) are fetched and compiled at configure-time if they are not present at this time on the host system (see @ref develbuildchain).


### Getting started with Visual Studio

Thanks to the integrated support of CMake in Visual Studio, you don't need a VS solution to build your project: open the Radium folder (via *File* > *Open* > *Folder ...* or `devenv.exe <foldername>`).
VS should run cmake, generate the target builds (Debug and Release by default).
Other build types can be added by editing `CMakeSettings.json`.

You may have Cmake errors occuring at the first run.
To fix them, you need to edit the VS-specific file `CMakeSettings.json`, via *CMake* > *Change CMake Settings* > path-to-CMakeLists (configuration-name) from the main menu.
For instance, it usually requires to set cmake build types manually, and to give path to Qt libraries.
To fix it, edit `CMakeSettings.json`, such that
~~~json
{
  "configurations": [
    {
      "name": "x64-Release",
      "generator": "Ninja",
      "configurationType": "Release",
      "inheritEnvironments": [ "msvc_x64_x64" ],
      "buildRoot": "C:/Users/XXX/Dev/builds/Radium/${name}",
      "installRoot": "C:/Users/XXX/Dev/Radium-install",
      "cmakeCommandArgs": "-DCMAKE_PREFIX_PATH=C:/Qt-5.10/5.10.0/msvc2017_64 -DRADIUM_UPDATE_VERSION=OFF",
      "buildCommandArgs": "",
      "ctestCommandArgs": ""
    },
    {
      "name": "x64-Debug",
      "generator": "Ninja",
      "configurationType": "Debug",
      "inheritEnvironments": [ "msvc_x64_x64" ],
      "buildRoot": "C:/Users/XXX/Dev/builds/Radium/${name}",
      "installRoot": "C:/Users/XXX/Dev/Radium-installdbg",
      "cmakeCommandArgs": "-DCMAKE_PREFIX_PATH=C:/Qt-5.10/5.10.0/msvc2017_64 -DRADIUM_UPDATE_VERSION=OFF",
      "buildCommandArgs": "",
      "ctestCommandArgs": ""
    }
  ]
}
~~~
*Note*: it is strongly encouraged to use `/` separators in your path, instead of `\\` as previously mentionned. See https://stackoverflow.com/questions/13737370/cmake-error-invalid-escape-sequence-u


*Note*: When compiling the dependencies you may hit the max path length fixed by Microsoft OS (260 characters). To fix this, you might need to change the path of your build dir to shorten it, or to change the limit on your system, see: https://docs.microsoft.com/en-us/windows/win32/fileio/naming-a-file#enable-long-paths-in-windows-10-version-1607-and-later

### Compilation

Right click on `CMakeList.txt > build > all`.
To install, you need to run any installation target, e.g. `Engine.dll (install)` or to select the menu `<Build>/<Install radiumproject>`

## Building with QtCreator

QtCreator is supported on Windows, MacOS and Linux.
No specific requirement here, just open Radium-Engine CMake project and enjoy !
