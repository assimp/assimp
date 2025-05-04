#### Supported file formats ####

__Importers__:

## Working
- 3D
- [3DS](https://en.wikipedia.org/wiki/.3ds)
- AC
- [AC3D](https://en.wikipedia.org/wiki/AC3D)
- ACC
- AMJ
- ASE
- ASK
- B3D
- [BVH](https://en.wikipedia.org/wiki/Biovision_Hierarchy)
- CSM
- COB
- [DAE/Collada](https://en.wikipedia.org/wiki/COLLADA)
- [DXF](https://en.wikipedia.org/wiki/AutoCAD_DXF)
- ENFF
- [FBX](https://en.wikipedia.org/wiki/FBX)
- [glTF 1.0](https://en.wikipedia.org/wiki/GlTF#glTF_1.0) + GLB
- [glTF 2.0](https://en.wikipedia.org/wiki/GlTF#glTF_2.0):
  At the moment for glTF2.0 the following extensions are supported:
  + KHR_lights_punctual ( 5.0 )
  + KHR_materials_pbrSpecularGlossiness ( 5.0 )
  + KHR_materials_unlit ( 5.0 )
  + KHR_texture_transform ( 5.1 under test )
- HMB
- IFC-STEP
- IQM
- IRR / IRRMESH
- [LWO](https://en.wikipedia.org/wiki/LightWave_3D)
- LWS
- LXO
- [M3D](https://bztsrc.gitlab.io/model3d)
- MD2
- MD3
- MD5
- MDC
- MDL
- MESH / MESH.XML
- MOT
- MS3D
- NDO
- NFF
- [OBJ](https://en.wikipedia.org/wiki/Wavefront_.obj_file)
- [OFF](https://en.wikipedia.org/wiki/OFF_(file_format))
- [PLY](https://en.wikipedia.org/wiki/PLY_(file_format))
- PMX
- PRJ
- Q3O
- Q3S
- RAW
- SCN
- SIB
- SMD
- [STP](https://en.wikipedia.org/wiki/ISO_10303-21)
- [STL](https://en.wikipedia.org/wiki/STL_(file_format))
- TER
- UC
- [USD](https://en.wikipedia.org/wiki/Universal_Scene_Description)
- VTA
- X
- XGL
- ZGL

Additionally, some formats are supported by dependency on non-free code or external SDKs (not built by default):

- [C4D](https://en.wikipedia.org/wiki/Cinema_4D) (https://github.com/assimp/assimp/wiki/Cinema4D-&-Melange) IMporting geometry + node hierarchy are currently supported

## Deprecated (support frozen)
- [BLEND](https://en.wikipedia.org/wiki/.blend_(file_format)) It is too time-consuming to maintain an undocumented format which contains so much more than we need.

## Partially broken (working but with known problems)
- [X3D](https://en.wikipedia.org/wiki/X3D) Broke when migrating IRRXml to pugixml in 2020

## Broken (not currently working)
- [3MF](https://en.wikipedia.org/wiki/3D_Manufacturing_Format) Broke in Feb 2024
- [OGEX](https://en.wikipedia.org/wiki/Open_Game_Engine_Exchange) No clear working/broken demarcation

__Exporters__:

- 3DS
- 3MF ( experimental )
- ASSBIN
- DAE (Collada)
- FBX ( experimental )
- glTF 1.0 (partial)
- glTF 2.0 (partial)
- JSON (for WebGl, via https://github.com/acgessler/assimp2json)
- OBJ
- OGEX ( experimental )
- [PBRTv4](https://github.com/mmp/pbrt-v4)
- PLY
- STEP
- STL
- X
