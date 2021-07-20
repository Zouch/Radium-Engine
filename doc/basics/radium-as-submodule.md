\page basicsRadiumSubmodule Use Radium Libraries in your own project
[TOC]

## Quick instructions
Radium is now shiped with a cmake package. To get it:
 - Get latest release,
 - Build from source, and install to the directory of your choice.

Then, to use Radium in your own project, you need to set, at configure time `CMAKE_PREFIX_PATH=/path/to/install/or/release/dir/lib/cmake/Radium`
or to define `Radium_DIR=/path/to/install/or/release/dir/lib/cmake/Radium`.

Your `CMakeLists.txt` might then do the following:
~~~cmake
find_package(Radium REQUIRED)
target_link_libraries(${target} Radium::Core Radium::Engine Radium::IO Radium::Gui)
~~~
Include directories and link instructions are populated automatically by cmake.

In your source code:
~~~cpp
#include <Core/Math/DualQuaternion.hpp>
~~~

## Detailed instructions
See [CMake setup](@ref cmakeutilities).
