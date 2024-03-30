# TinyUSDZ documentation

For full documentation visit [mkdocs.org](https://www.mkdocs.org).

## Mesh prim

* points : usually vec3f[]
* normals : usually vec3f[] varying
* primvars:st : usually vec2f[] varying
* primvars:st:indices : usually int[]  varying
* velocities : usually vec3f[]
* texcoord: usually `texCoord2f[]` (Texcoord bound to Mesh prim) 


#### Mesh property

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

##### Optional property

* primvars:displayColor : ty: float3[], varying
  * Usually array len = 1(constant color over the mesh)
* primvars:displayOpacity : ty: float[], varying
  * Usually array len = 1(constant opacity over the mesh)

#### SubD property

* cornerIndices : int[]
* cornerSharpnesses : float[]
* creaseIndices : int[]
* creaseLengths : int[]
* creaseSharpnesses : float[]
* holeIndices : int[]
* interpolateBoundary : token?

### Xform

* world bounding box : range3d
* local world xform : matrix4d
* purpose
* visibility
* xformOpOrder

### Shader

* inputs:id : UsdUVTexture
* inputs:fallback : fallback color(Usually vec4f)
* inputs:file : @filename@

#### Texcoord reader

`UsdPrimvarReader_float2`

* inputs:varname(token) : Name of UV coordinate primvar in Mesh primitive.
* outputs:<name>(float2) : Output connection name. Usually `outputs:result`

### TODO

* [ ] non-vec3f `position`
* [ ] vertex color
* [ ] xformOpOrder
* [ ] Multi texcoord UVs


## PreviewSurface

https://graphics.pixar.com/usd/docs/UsdPreviewSurface-Proposal.html 

### Inputs

| Name               | Type         | Default value, possible value |
| :--                | :--          | :--                           |
| diffuseColor       | color3f      | (0.18, 0.18, 0.18)            |
| emissiveColor      | color3f      | (0.0, 0.0, 0.0)               |
| roughness          | float        | 0.5                           | 
| clearcoat          | float        | 0.0                           | 
| clearcoatRoughness | float        | 0.01                          | 
| opacity            | float        | 1.0                           | 
| opacityThreshold   | float        | 0.0                           | 
| ior                | float        | 1.5                           | 
| normal             | normal3f     | (0.0, 0.0, 1.0)               |
| displacement       | float        | 0.0                           |
| occlusion          | float        | 1.0                           |
| useSpeclarWorkflow | int          | 1(specular) or 0(metallic)    |

Specular workflow

| Name               | Type         | Default value   |
| :--                | :--          | :--             |
| specularColor      | color3f      | (0.0, 0.0, 0.0) |

Metallic workflow

| Name               | Type         | Default value   |
| :--                | :--          | :--             |
| metallic           | float        | 0.0             |


### Outputs

* surface - token
* displacement - token

### UsdUVTexture

#### Inputs

| Name               | Type         | Default value, possible value |
| :--                | :--          | :--                           |
| file               | asset        | string                        |
| st                 | float2       | (0.0, 0.0)                    |
| wrapS              | token        | black, clamp, repeat, mirror  |
| wrapT              | token        | black, clamp, repeat, mirror  |
| fallback           | float4       | (0.0, 0.0, 0.0, 1.0)          |
| scale              | float4       | (1.0, 1.0, 1.0, 1.0)          |
| bias               | float4       | (0.0, 0.0, 0.0, 0.0)          |

#### Outputs

* r, g, b, a


### UsdTransform2d

#### Inputs

* in - float2 - (0.0, 0.0)
* rotation - float - (0.0)
  * counter-clockwise rotation in degrees around the origin
* scale - float2 - (1.0, 1.0)
* translation - float2 (0.0, 0.0)

#### Outpuits

* result - float2


### TODO

* [ ] Primvar Reader(arbitrary vertex attributes)
* [ ] displacement
* [ ] Texture transform


## Notes on USDC crate file format

### Bootstrap(header)

* magic header: "PXR-USDC" : 8 bytes
* version number: uint8 x 8(0 = major, 1 = minor, 2 = patch, rest unused) :8 bytes
* int64_t tocOffset
* int64_t _reserved[8]

=> total 88 bytes

### Version

From pxr/usd/usd/crateFile.cpp

```
// Version history:
// 0.9.0: Added support for the timecode and timecode[] value types.
// 0.8.0: Added support for SdfPayloadListOp values and SdfPayload values with
//        layer offsets.
// 0.7.0: Array sizes written as 64 bit ints.
```

* 0.8.0(USD v20.11)
  
### Compression

version 0.4.0 or later uses LZ4 for compression and interger compression(pxr original? compression. its backend still uses LZ4) for index arrays.

### TOC(table of contents)

List of Sections.

### Sections

There are 6 known sections.

#### TOKENS

List of strings(tokens). tokens are unique.

#### STRINGS

List of StringIndices(index to `tokens`)

#### FIELDS

Data field info. This is a bit tricky data format.
See the source code for details.

Field is consist of

* index to token
* 8 byte info. 2 byte for type an so on, 6 byte for primitive value or offset to the value(e.g. for array data)

#### FIELDSETS

List of indices

#### PATHS

List of path indices.

Need to reconstruct
                          
#### SPECS

List of specs

###### Spec

* path index
* fieldsets index 
* SdfSpecType
