### [HOME](../Readme.md) / [Reference](Reference.md) / closest

C++ plugin

This shader is very similar to the **xyzdist** VEX function. It allows you to find the closest point on the surface / spline and store it in the **[ClosestData](../osl/include/hGeoStructsOSL.h)**  structure for further query of interpolated across primitive attribute value with **[interpolator](interpolator.md)**.

![network_example](network_example.png)

Allows you to read data from any Houdini known geometry. This could be PrimPoly, PolySoups, Curves as well as Packed primitives, AlembicRefs and UsdRefs from **.bgeo**, **.bgeo.sc** or any other format which Houdini can digest e.g. **.abc** or **.usd**.
