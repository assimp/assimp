# Implementation status(v0.8.0)

* ✅ = Supported.
* ❕ = Partially supported or has limitation.
* 🚧 = Work in progress.
* empty cell = not yet supported.

NOTE: TinyUSDZ API is subject to change
NOTE: USDC(Binary) = Read only

## Generic Prim types

| type        | Ascii | USDC | Comment               |
| ----------- | ----- | ---- | --------------------- |
| Model       | ✅    | ✅   | def with no prim type |
| Scope       | ✅    | ✅   | no-op prim            |

## Geometry(usdGeom)

| type           | Ascii | USDC | Comment      |
| -----------    | ----- | ---- | -------      |
| Xform          | ✅    | ✅   |              |
| Mesh           | ✅    | ✅   |              |
| GeomSubset     | 🚧    | 🚧   |              |
| Points         | ✅    | ✅   |              |
| Cube           | ✅    | ✅   |              |
| Cylinder       | ✅    | ✅   |              |
| Sphere         | ✅    | ✅   |              |
| Capsule        | ✅    | ✅   |              |
| Cone           | ✅    | ✅   |              |
| BasisCurves    | ✅    | ✅   | for hair/fur |
| NurbsPatch     |       |      |              |
| NurbsCurves    | ✅    | ✅    |              |
| HermiteCurves  |       |      |              |
| PointInstancer | ✅    | ✅    |              |

## Camera(usdGeom)

| type        | Ascii | USDC | Comment |
| ----------- | ----- | ---- | ------- |
| Camera      | ✅    | ✅   |         |

## Lights(usdLux)

| type          | Ascii | USDC | Comment      |
| -----------   | ----- | ---- | -------      |
| DistantLight  | ✅    | ✅   |              |
| DiskLight     | ✅    | ✅   |              |
| RectLight     | ✅    | ✅   |              |
| SphereLight   | ✅    | ✅   |              |
| CylinderLight | ✅    | ✅   |              |
| DomeLight     | ✅    | ✅   |              |
| GeomtryLight  |       |      |              |
| PortalLight   |       |      |              |
| PluginLight   |       |      | Light shader |


* Light sets, light shaping, shadowAPI, LightFiler, etc are not supported yet.

## Material, shader(usdShade, usdImaging plugin)

| type              | Ascii | USDC | Comment |
| -----------       | ----- | ---- | ------- |
| UsdPreviewSurface | ✅    | ✅   |         |
| UsdUVTexture      | ❕    | ❕   | 1.      |
| UsdPrimvarReader  | ✅    | ✅   | 2.      |


1. UDIM texture is not supported.

## Skinning, BlendShapes(usdSkel)

| type        | Ascii | USDC | Comment      |
| ----------- | ----- | ---- | -------      |
| SkelRoot    | 🚧    | 🚧   | Parsing only |
| Skeleton    | 🚧    | 🚧   | Parsing only |
| SkelAnim    | 🚧    | 🚧   | Parsing only |
| BlendShape  | ✅    | ✅   | Supports inbetween blendshape attribute |

* Skinning evaulation/validation are work-in-progress.
* Skeleton utility functions(e.g. build joint hiearchy from list of `token[]`) are work-in-progress.

## Work in progress

* [x] Custom filesystem handler(Asset Resolution)
* [ ] Composition(VariantSet) syntax
  * [x] VariantSet Ascii parse
  * [x] VariantSet Ascii print
  * [x] VariantSet Crate parse
  * [ ] VariantSet(SpecTypeVariant) Crate write
* [ ] USDC serialization
* [ ] Skinning evaluation/validation
* [ ] Tydra(scene/render delegation)
* [ ] usdObj(wavefront .obj) support.
  * Please see [usdObj.md](usdObj.md)
* [ ] C-API for other languages
* [ ] Python binding and package.
* [ ] Composition Arcs
  * Parsing some Composition Arcs possible, needs Evaluation of it.
  * [x] subLayers
  * [x] references
  * [x] payloads(delayed load)
  * [x] variants/variantSets
  * [ ] specializers(priority is low)

## TODO

* [ ] Performance optimization
* [ ] UDIM texture
* [ ] MeshLight(GeometryLight)
* [ ] Collection API
  * e.g. Light Sets
* [ ] Delayed load of Layer/Stage
* [ ] Instancing
* [ ] Volume(usdVol)
  * [ ] MagicaVoxel vox for Volume?
  * [ ] VDBVolume support through TinyVDBIO? https://github.com/syoyo/tinyvdbio
* [ ] Audio(usdAudio)
* [ ] MaterialX support(usdMtlx)
* [ ] Physics(usdPhysics)
* [ ] and more...


EoL.
