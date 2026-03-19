### [HOME](../Readme.md) / [Reference](Reference.md) / samplePoints

C++ plugin

Allows you to find closest points from the geometry file and store their sorted indexes and distances in the **[ArrayData](../osl/include/hGeoStructsOSL.h)** structure for further reading. Similar to **pcopen**/**nearpoints** VEX functions.

You can search for up to 16 points.

![network_example](network_example_pc.png)

Allows you to read data from any Houdini known geometry. It can be unconnected points or points from PrimPoly, PolySoups, Curves as well as Packed primitives, AlembicRefs and UsdRefs from `.pc`, `.bgeo`, `.bgeo.sc` or any other format which Houdini can digest e.g. ``.abc`` or ``.usd``.
