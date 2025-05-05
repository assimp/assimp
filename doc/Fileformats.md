#### Supported file formats ####

### Importers

#### Working
- 3D
- [3DS](https://en.wikipedia.org/wiki/.3ds)
- AC
- [AC3D](https://en.wikipedia.org/wiki/AC3D)
- ACC
- AMJ
- ASE
- ASK
- ASSBIN (`Assimp Binary Importer`)
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
- [PMX](https://en.wikipedia.org/wiki/MikuMikuDance)
- PRJ
- Q3O
- Q3S
- RAW
- [SCN](https://en.wikipedia.org/wiki/TrueSpace) (Note: assimp supports TrueSpace `.scn` files, but this extension used by other unsupported 3D formats as well)
- SIB
- [SMD](https://developer.valvesoftware.com/wiki/SMD) (`StudioModel Data`)
- [STEP](https://en.wikipedia.org/wiki/ISO_10303-21)
- [STP](https://en.wikipedia.org/wiki/ISO_10303-21)
- [STL](https://en.wikipedia.org/wiki/STL_(file_format))
- TER
- UC
- [USD](https://en.wikipedia.org/wiki/Universal_Scene_Description) (`.usd`, `.usda`, `.usdc`, `.usdz`) ( experimental ) To build need to opt-in in `CMakeLists.txt`
- WRL ("classic" VRML, loaded via on-the-fly conversion to X3D `.xml`) To build need to opt-in in `CMakeLists.txt`
- VTA (SMD loader)
- [X](https://learn.microsoft.com/en-us/windows/win32/direct3d9/x-files--legacy-) (Legacy format introduced with DirectX 2.0)
- XGL
- ZGL

Additionally, some formats are supported by dependency on non-free code or external SDKs (not built by default):

- [C4D](https://en.wikipedia.org/wiki/Cinema_4D) (https://github.com/assimp/assimp/wiki/Cinema4D-&-Melange) Importing geometry + node hierarchy are currently supported.
  (Note that this proprietary format depends on prebuilt libraries and is only supported on windows and MacOS,
    i.e. not on Android/Linux/iOS)

#### Deprecated (support frozen)
- [BLEND](https://en.wikipedia.org/wiki/.blend_(file_format)) It is too time-consuming to maintain an undocumented format which contains so much more than we need.

#### Partially broken (working but with known problems)
- [M3D](https://bztsrc.gitlab.io/model3d/) Assimp support abandoned by author (to build need to opt-in in `CMakeLists.txt`)
- [X3D](https://en.wikipedia.org/wiki/X3D) Broke when migrating IRRXml to pugixml in 2020

#### Broken (not currently working)
- [3MF](https://en.wikipedia.org/wiki/3D_Manufacturing_Format) Broke in Feb 2024 (to build need to opt-in in `CMakeLists.txt`)
- [OGEX](https://en.wikipedia.org/wiki/Open_Game_Engine_Exchange) No clear working/broken demarcation (to build need to opt-in in `CMakeLists.txt`)

### Exporters

- [3DS](https://en.wikipedia.org/wiki/.3ds)
- [3MF](https://en.wikipedia.org/wiki/3D_Manufacturing_Format) ( experimental ) Note: importer broken
- ASSBIN (`Assimp Binary Importer`)
- ASSXML
- [DAE/Collada](https://en.wikipedia.org/wiki/COLLADA)
- [FBX](https://en.wikipedia.org/wiki/FBX) ( experimental )
- [glTF 1.0](https://en.wikipedia.org/wiki/GlTF#glTF_1.0) (partial)
- [glTF 2.0](https://en.wikipedia.org/wiki/GlTF#glTF_2.0) (partial)
- JSON (for WebGl, via https://github.com/acgessler/assimp2json)
- [M3D](https://bztsrc.gitlab.io/model3d/) Assimp support abandoned by author (to build need to opt-in in `CMakeLists.txt`)
- [OBJ](https://en.wikipedia.org/wiki/Wavefront_.obj_file)
- [OGEX](https://en.wikipedia.org/wiki/Open_Game_Engine_Exchange) ( experimental ) Note: importer broken
- [PBRTv4](https://github.com/mmp/pbrt-v4)
- [PLY](https://en.wikipedia.org/wiki/PLY_(file_format))
- [STEP](https://en.wikipedia.org/wiki/ISO_10303-21)
- [STL](https://en.wikipedia.org/wiki/STL_(file_format))
- [X](https://learn.microsoft.com/en-us/windows/win32/direct3d9/x-files--legacy-)
- [X3D](https://en.wikipedia.org/wiki/X3D)
