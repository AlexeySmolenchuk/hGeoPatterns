[HOME](../Readme.md)

## Requirements

You need to have this software pre-installed:

* **Houdini** - tested with 19.0 and 19.5
* **RenderManProServer** - tested with 24.x and 25.x
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

You can install one of those:
* **Visual Studio Community 2019** (free)
* **Build Tools for Visual Studio 2019** - if you don't need IDE

Can be downloaded from [here](https://visualstudio.microsoft.com/vs/older-downloads/#visual-studio-2019-and-other-products) or [here](https://my.visualstudio.com/Downloads?q=Visual%20Studio%202019).

#### Linux
* **GCC** - [version recommendations by SideFX](https://www.sidefx.com/docs/hdk/_h_d_k__intro__getting_started.html#HDK_Intro_GettingStarted_Compiling)


## Building
Update values for **HFS**, **RMANTREE**, **RFHTREE** variables in the [CMakeLists.txt](../CMakeLists.txt#L4-L15)

Then, assuming you opened terminal in the root of *hGeoPattern*
``` sh
mkdir build
cd build
cmake ..
cmake --build . --config Release -j8
```

## Installation

#### Option A
Add root directory of hGeoPattern to **HOUDINI_PACKAGE_DIR** environment variable

#### Option B
Update the first variable in [*hGeoPatterns.json*](../hGeoPatterns.json#L4) with absolute path to plugin root folder and put it in your *$HOUDINI_USER_PREF_DIR/packages*

[HOME](../Readme.md)
