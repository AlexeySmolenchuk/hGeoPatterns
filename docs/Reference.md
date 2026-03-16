### [HOME](../Readme.md) / Reference

#### C++/OSL Shaders
**[buildCoords](buildCoords.md)**
**[closest](closest.md)**
**[displacementLayer](displacementLayer.md)**
**[filterArrayData](filterArrayData.md)**
**[generateCoords](generateCoords.md)**
**[interpolator](interpolator.md)**
**[oceanSampleLayers](oceanSampleLayers.md)**
**[pointCloudFilter](pointCloudFilter.md)**
**[readAttribute](readAttribute.md)**
**[samplePoints](samplePoints.md)**
**[sampleTextures](sampleTextures.md)**
**[sampleVolume](sampleVolume.md)**
**[utilityNodes](utilityNodes.md)**

#### VOP structures
Custom structures are defined in C++ and OSL headers. They are also defined as a Houdini Vop type in [`vop/structs.json`](../vop/hGeoStructs.json), which allows you to work with those structures in **OSL Generic Shader Builder**.
There also structure for RenderMan Manifold inputs.

#### Note about derivatives in OSL and RIS
The derivative calculation is required for Bump Mapping and Mip-Map optimization. RIS and OSL have very different mechanisms of derivative calculation in RenderMan. Be careful when using C++/RIS results as texture coordinates With OSL textures. Check if it works with **Dx()**, and **Dy()** functions or manually control texture filtering with the **width** parameter.
