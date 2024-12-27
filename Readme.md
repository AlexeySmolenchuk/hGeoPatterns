# Let's bring Houdini proceduralism to RenderMan!
[![exampleimage](docs/img1.jpg) ![exampleimage](docs/img2.jpg) ![exampleimage](docs/img3.jpg)](https://alexeysmolenchuk.github.io/hGeoPatterns/)

### What is hGeoPatterns
**hGeoPatterns** is a collection of RenderMan plugins for sampling any Houdini compatible geometry. This project is a mixture of **RIS** and **HDK** functionality to make operations like sampling and reading arbitrary Houdini Geometry Data in RenderMan shading networks. Similar to familiar *xyzdist()*, *prim_attribute()*, *nearpoints()* VEX functions. OSL shaders, headers and VOP definitions are included to make shader authoring easy.

### Whats NEW
Added **oceanSampleLayer** shader which calls VEX code directly and allows to render native Houdini ocean spectrum.

![ocean](https://github.com/user-attachments/assets/cfe07d3b-d1d8-4e8d-9b26-48b891cc5002)

###  Installation
Use [prebuilt binaries](https://github.com/AlexeySmolenchuk/hGeoPatterns/releases) or compile for your system using [this instruction](docs/Building.md).

Add root directory of hGeoPatterns to **HOUDINI_PACKAGE_DIR** environment variable<br/>
OR<br/>
Update the first variable in [*hGeoPatterns.json*](hGeoPatterns.json#L4) with absolute path to plugin root folder and put it in your *$HOUDINI_USER_PREF_DIR/packages*

### Nodes Reference
Quick [nodes overview](docs/Reference.md)

### Examples
Examples from [demo page](https://alexeysmolenchuk.github.io/hGeoPatterns/) in .hipnc format can be found [here](https://github.com/AlexeySmolenchuk/hGeoPatterns/releases).

### Limitations
* XPU not supported

Thanks to **SideFX** for the extensive library and **Pixar RenderMan** for supporting the RixPattern interface.
