# Plugin sharing library
This example illustrates how to distribute a library alongside a plugin Radium, while making the
library available for any other plugin.

It has two components:
 - Upstream: Plugin shipped with library,
 - Downstream: User plugin using the library provided by Upstream.

## Configure and compile
The head `CMakefile.txt` compiles and install the cmake projects in Upstream and Downstream directories.
When compiling Radium, this process can be called using the target `ExamplePluginWithLib`:
```bash
make ExamplePluginWithLib
```
Note that calling this target will force the installation of Radium, as both Upstream and Downstream
plugins needs an installed Radium cmake package.

Alternatively, this process can be replicated by compiling the Upstream and Downstream projects separately,
by following the compilation instructions given in `Upstream/CMakefile.txt` and then `Downstream/CMakefile.txt`.

## Use
Load the Upstream and Downstream plugins into any plugin compatible Radium app, a confirmation
message should be printed to standard output.
