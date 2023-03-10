## Lets bring Houdini proceduralism to RenderMan!
Set of RIS patterns using native HDK functionality to make operations like sampling and reading arbitrary Houdini Geometry Data in RenderMan.

### RIS Patterns

#### SamplePoints
Sample closest points from file and store their sorted indexes and distances in ArrayData structure for further reading. Similar to pcopen/nearpoints vex functions.

#### ReadAttribute
Read specified point attribute from ArrayData structure indexes.
Empty filename means using the same file.

#### Closest
Shader similar to xyzdist vex function. Allows you to find the closest point on the surface/spline and store it in ClosestData structure for further reading.

#### Interpolator
Read interpolated attribute value from ClosestData structure. Similar to prim_attribute vex function.
Empty filename means using the same file.

### OSL Patterns
OSL shaders are examples demonstrating various usecases.
#### BuildCoords
#### SampleTexture
#### Ripples

### VOP structures
Custom structures defined in C++ and OSL headers also defined as houdini vop type in *vop/structs.json*, which allows you to work with this structures in **OSL Generic Shader Builder**.

### Note about derivatives in OSL and RIS
