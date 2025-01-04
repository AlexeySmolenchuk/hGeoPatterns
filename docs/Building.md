[HOME](../Readme.md)

## Requirements

You need to have this software pre-installed:

* **Houdini** - tested with many versions from 19.0 to 20.5
* **RenderManProServer** - tested with 24.x, 25.x and 26.x
* **RenderManForHoudini** - corresponding version
* **PixarRenderMan-Examples** - not required but very handy to use with OSL Builder. Add manually to [*hGeoPatterns.json*](../hGeoPatterns.json#L5)
* **CMake**

Assuming you have installed RFH the way it's accessible from **hython**. This required for HDA building stage. You can quickly check by running hython and executing:

``` python
>>> import rfh, rman
>>> rman.Version
```
#### Windows
* **MSVC** - [version recommendations by SideFX](https://www.sidefx.com/docs/hdk/_h_d_k__intro__getting_started.html#HDK_Intro_Compiling_Intro_Windows)

You can install one of these fo free:
* **[Visual Studio Community 2022](https://visualstudio.microsoft.com/downloads/#visual-studio-community-2022)**
* **[Build Tools for Visual Studio 2022](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022)** - if you don't need IDE

#### Linux
* **GCC** - [version recommendations by SideFX](https://www.sidefx.com/docs/hdk/_h_d_k__intro__getting_started.html#HDK_Intro_GettingStarted_Compiling)


## Building
Update values for **HFS**, **RMANTREE**, **RFHTREE** variables in the [CMakeLists.txt](../CMakeLists.txt#L4-L15)

Then, assuming you opened terminal in the root of *hGeoPatterns*
``` sh
mkdir build
cd build
cmake ..
cmake --build . --config Release -j8
```

## Installation

#### Option A
Add root directory of hGeoPatterns to **HOUDINI_PACKAGE_DIR** environment variable

#### Option B
Update the first variable in [*hGeoPatterns.json*](../hGeoPatterns.json#L4) with absolute path to plugin root folder and put it in your *$HOUDINI_USER_PREF_DIR/packages*


## Other DCCs
If you want to build this plugin to use in other DCCs, you need Houdini installed on your machine. You need to minimize the number of dependencies. By default linking plugins to the whole Houdini package causes tons of dependencies, even unnecessary ones, which can cause library clashes. My solution would be to link to the HoudiniGEO library directly and manually provide all required FLAGS. On Linux, it looks like this:

```Cmake
# Add these definitions to CMakeLists.txt
include_directories(SYSTEM ${HFS}/toolkit/include )
link_directories( ${HFS}/dsolib ${HFS}/custom/houdini/dsolib )
link_libraries(HoudiniGEO HoudiniUT)
add_definitions(-DSIZEOF_VOID_P=8 -D_USE_MATH_DEFINES -DSESI_LITTLE_ENDIAN -DAMD64 -DLINUX -DUSE_PTHREADS)

# And comment this particular line
foreach (rixplugin_name ${rixplugin_names})
 add_library( ${rixplugin_name} SHARED src/${rixplugin_name}.cpp)
#-> target_link_libraries( ${rixplugin_name} Houdini )
 set_target_properties(${rixplugin_name} PROPERTIES
 RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/rixplugins
 LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/rixplugins
 PREFIX "")
endforeach ()
```
Build as described [here](#building)
Setup following environments before running Katana:

```sh
# Where libHoudiniGEO.so lives
export LD_LIBRARY_PATH=/opt/hfs20.5.445/dsolib
export HGEOPATTERNS_PATH=/path/to/your/hGeoPatterns
export RMAN_RIXPLUGINPATH=${HGEOPATTERNS_PATH}/rixplugins
export RMAN_SHADERPATH=${HGEOPATTERNS_PATH}/shaders
# This one is required for ocean shader
export HOUDINI_VEX_PATH=${HGEOPATTERNS_PATH}/vex/CVex:'&'
```
Now all shaders must work as expected and read all sorts of geometry formats Houdini can open.
Tested on Linux with Katana6.0 and a variety of Houdini[19.5-20.5] and RenderMan[25.0-26.3] combinations.

[HOME](../Readme.md)
