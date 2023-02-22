#### Supported file formats ####

__Importers__:

- 3D
- [3DS](https://en.wikipedia.org/wiki/.3ds)
- [3MF](https://en.wikipedia.org/wiki/3D_Manufacturing_Format)
- AC
- [AC3D](https://en.wikipedia.org/wiki/AC3D)
- ACC
- AMJ
- ASE
- ASK
- B3D
- [BLEND](https://en.wikipedia.org/wiki/.blend_(file_format))
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
- [OGEX](https://en.wikipedia.org/wiki/Open_Game_Engine_Exchange)
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
- VTA
- X
- [X3D](https://en.wikipedia.org/wiki/X3D)
- XGL
- ZGL

Additionally, some formats are supported by dependency on non-free code or external SDKs (not built by default):

- [C4D](https://en.wikipedia.org/wiki/Cinema_4D) (https://github.com/assimp/assimp/wiki/Cinema4D-&-Melange) IMporting geometry + node hierarchy are currently supported

__Exporters__:

- DAE (Collada)
- STL
- OBJ
- PLY
- X
- 3DS
- JSON (for WebGl, via https://github.com/acgessler/assimp2json)
- ASSBIN
- STEP
- [PBRTv4](https://github.com/mmp/pbrt-v4)
- glTF 1.0 (partial)
- glTF 2.0 (partial)
- 3MF ( experimental )
- FBX ( experimental )
