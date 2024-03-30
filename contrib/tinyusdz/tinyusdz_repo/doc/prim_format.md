
# Mesh prim

* points : usually vec3f[]
* normals : usually vec3f[] varying
* primvars:st : usually vec2f[] varying
* primvars:st:indices : usually int[]  varying
* velocities : usually vec3f[]
* texcoord: usually `texCoord2f[]` (Texcoord bound to Mesh prim) 


### Mesh property

* doubleSided : bool
* extent : floa3[2]
* facevaryingLinearInterpolation : cornerPlus1
* faceVertexCounts : int[]
* faceVertexIndices : int[]

* orientation : ty: token. uniform. allowedTokens: ['rightHanded', 'leftHanded']
* visibility : ty: token. varying. allowedTokens: ['inherited', 'invisible']
* purpose : ty: token. uniform. allowedTokens: ['default', 'render', 'proxy', 'guide']
* subdivisionScheme : ty: token. uniform. allowedTokens: ['catmullClark', 'loop', 'bilinear', 'none']
* trianglesSubdivisionRule : ty: token. uniform. allowedTokens: ['catmullClark', 'smooth']
 
* xformOpOrder : ty: token. uniform.

* material:binding : Path
* proxyPrim : ty: relation. uniform

#### Optional property

* primvars:displayColor : ty: float3[], varying
  * Usually array len = 1(constant color over the mesh)
* primvars:displayOpacity : ty: float[], varying
  * Usually array len = 1(constant opacity over the mesh)

### SubD property

* cornerIndices : int[]
* cornerSharpnesses : float[]
* creaseIndices : int[]
* creaseLengths : int[]
* creaseSharpnesses : float[]
* holeIndices : int[]
* interpolateBoundary : token?

## Xform

* world bounding box : range3d
* local world xform : matrix4d
* purpose
* visibility
* xformOpOrder

## Shader

* inputs:id : UsdUVTexture
* inputs:fallback : fallback color(Usually vec4f)
* inputs:file : @filename@

### Texcoord reader

`UsdPrimvarReader_float2`

* inputs:varname(token) : Name of UV coordinate primvar in Mesh primitive.
* outputs:<name>(float2) : Output connection name. Usually `outputs:result`

## TODO

* [ ] non-vec3f `position`
* [ ] vertex color
* [ ] xformOpOrder
* [ ] Multi texcoord UVs



