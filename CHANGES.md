----------------------------------------------------------------------
CHANGELOG
----------------------------------------------------------------------
# 6.0.2
## What's Changed
* Fix export fbx: Wrong Materials in LayerElementMaterial if a node contains multi meshes  by @Riv1s-sSsA01 in https://github.com/assimp/assimp/pull/6103
* Fix compile error when ASSIMP_DOUBLE_PRESICION enable by @Riv1s-sSsA01 in https://github.com/assimp/assimp/pull/6091
* Updated Inner Cone formula for Spot Lights in GLTF by @crazyjackel in https://github.com/assimp/assimp/pull/6078
* Update/update pugi xml by @kimkulling in https://github.com/assimp/assimp/pull/6229
* Fixes CVE-2025-2751: Out-of-bounds Read in Assimp::CSMImporter::InternReadFile (closes #6012) by @VinzSpring in https://github.com/assimp/assimp/pull/6224
* Fixes CVE-2025-2757: Heap-based Buffer Overflow in AI_MD5_PARSE_STRING_IN_QUOTATION (closes #6019) by @VinzSpring in https://github.com/assimp/assimp/pull/6223
* Fixes CVE-2025-2750: out of bounds write by assigning to wrong array element count tracking (closes #6011) by @VinzSpring in https://github.com/assimp/assimp/pull/6225
* fix-CVE-2025-3158: closes #6023 Fixes CVE-2025-3158: Heap-based Buffer Overflow in Assimp::LWO::AnimResolver::UpdateAnimRangeSetup by @VinzSpring in https://github.com/assimp/assimp/pull/6222
* Update SECURITY.md by @kimkulling in https://github.com/assimp/assimp/pull/6230
* Fix the function aiGetMaterialColor when the flag ASSIMP_DOUBLE_PRECISION is enabled by @Riv1s-sSsA01 in https://github.com/assimp/assimp/pull/6090
* Prepare 6.0.2 by @kimkulling in https://github.com/assimp/assimp/pull/6231

**Full Changelog**: https://github.com/assimp/assimp/compare/v6.0.1...v6.0.2

# 6.0.1
## What's Changed
* Build
  * Fix building on Haiku by @Begasus in https://github.com/assimp/assimp/pull/5255
* Postprocessing
  * Reduce memory consumption in JoinVerticesProcess::ProcessMesh() signi… by @ockeymm in https://github.com/assimp/assimp/pull/5252
* Fix: Add check for invalid input argument by @kimkulling in https://github.com/assimp/assimp/pull/5258
* Replace an assert by a error log. by @kimkulling in https://github.com/assimp/assimp/pull/5260
* Extension of skinning data export to GLB/GLTF format by @fvbj in https://github.com/assimp/assimp/pull/5243
* Fix output floating-point values to fbx by @Riv1s-sSsA01 in https://github.com/assimp/assimp/pull/5265
* Update ImproveCacheLocality.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5268
* Update Readme.md by @kimkulling in https://github.com/assimp/assimp/pull/5277
* Deep arsdk bone double free by @kimkulling in https://github.com/assimp/assimp/pull/5291
* Fix Spelling error by @JulianKnodt in https://github.com/assimp/assimp/pull/5295
* use size in order to be compatible with float and double by @sloriot in https://github.com/assimp/assimp/pull/5270
* Fix: Add missing transformation for normalized normals. by @kimkulling in https://github.com/assimp/assimp/pull/5301
* Fix: Implicit Conversion Error by @Ipomoea in https://github.com/assimp/assimp/pull/5271
* Fix add checks for indices by @kimkulling in https://github.com/assimp/assimp/pull/5306
* Update FBXBinaryTokenizer.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5311
* link to external minizip with full path by @aumuell in https://github.com/assimp/assimp/pull/5278
* utf8 header not found by @TarcioV in https://github.com/assimp/assimp/pull/5279
* Rm unnecessary deg->radian conversion in FBX exporter by @JulianKnodt in https://github.com/assimp/assimp/pull/5281
* Fix empty mesh handling by @kimkulling in https://github.com/assimp/assimp/pull/5318
* Refactoring: Some cleanups by @kimkulling in https://github.com/assimp/assimp/pull/5319
* Fix invalid read of `uint` from `uvwsrc` by @JulianKnodt in https://github.com/assimp/assimp/pull/5282
* Remove double delete by @kimkulling in https://github.com/assimp/assimp/pull/5325
* fix mesh-name error. by @copycd in https://github.com/assimp/assimp/pull/5294
* COLLADA fixes for textures in C4D input by @wmatyjewicz in https://github.com/assimp/assimp/pull/5293
* Use the correct allocator for deleting objects in case of duplicate a… by @FlorianBorn71 in https://github.com/assimp/assimp/pull/5305
* Fix container overflow in MMD parser by @aavenel in https://github.com/assimp/assimp/pull/5309
* Fix: PLY heap buffer overflow by @aavenel in https://github.com/assimp/assimp/pull/5310
* Update Readme.md by @cuppajoeman in https://github.com/assimp/assimp/pull/5312
* Update Readme.md by @cuppajoeman in https://github.com/assimp/assimp/pull/5313
* Fix: Check if index for mesh access is out of range by @kimkulling in https://github.com/assimp/assimp/pull/5338
* Update FBXConverter.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5349
* FBX: Use correct time scaling by @kimkulling in https://github.com/assimp/assimp/pull/5355
* Drop explicit inclusion of contrib/ headers by @umlaeute in https://github.com/assimp/assimp/pull/5316
* Update Build.md by @cuppajoeman in https://github.com/assimp/assimp/pull/5314
* Fix buffer overflow in FBX::Util::DecodeBase64() by @ttxine in https://github.com/assimp/assimp/pull/5322
* Readme.md:  correct 2 errors in section headers by @stephengold in https://github.com/assimp/assimp/pull/5351
* Fix double free in Video::~Video() by @ttxine in https://github.com/assimp/assimp/pull/5323
* FBXMeshGeometry:  solve issue #5116 using patch provided by darktjm by @stephengold in https://github.com/assimp/assimp/pull/5333
* Fix target names not being imported on some gLTF2 models by @Futuremappermydud in https://github.com/assimp/assimp/pull/5356
* correct grammar/typographic errors in comments (8 files) by @stephengold in https://github.com/assimp/assimp/pull/5343
* KHR_materials_specular fixes by @rudybear in https://github.com/assimp/assimp/pull/5347
* Disable Hunter by @kimkulling in https://github.com/assimp/assimp/pull/5388
* fixed several issues by @MarkaRagnos0815 in https://github.com/assimp/assimp/pull/5359
* Fix leak by @kimkulling in https://github.com/assimp/assimp/pull/5391
* Check validity of archive without parsing by @kimkulling in https://github.com/assimp/assimp/pull/5393
* Fix integer overflow by @kimkulling in https://github.com/assimp/assimp/pull/5394
* Add a test before generating the txture folder by @kimkulling in https://github.com/assimp/assimp/pull/5400
* Build: Disable building zlib for non-windows by @kimkulling in https://github.com/assimp/assimp/pull/5401
* null check. by @copycd in https://github.com/assimp/assimp/pull/5402
* Bump actions/upload-artifact from 3 to 4 by @dependabot in https://github.com/assimp/assimp/pull/5384
* fix: KHR_materials_pbrSpecularGlossiness/diffuseFactor convert to pbr… by @guguTang in https://github.com/assimp/assimp/pull/5410
* fix building errors for MinGW by @0xf0ad in https://github.com/assimp/assimp/pull/5376
* dynamic_cast error. by @copycd in https://github.com/assimp/assimp/pull/5406
* Add missing IRR textures by @tellypresence in https://github.com/assimp/assimp/pull/5374
* Update Dockerfile by @kimkulling in https://github.com/assimp/assimp/pull/5412
* Fix handling of X3D IndexedLineSet nodes by @andre-schulz in https://github.com/assimp/assimp/pull/5362
* Improve acc file loading by @IOBYTE in https://github.com/assimp/assimp/pull/5360
* Readme.md:  present hyperlinks in a more uniform style by @stephengold in https://github.com/assimp/assimp/pull/5364
* FBX Blendshape `FullWeight: Vec<Float>` -> `FullWeight: Vec<Double>` by @JulianKnodt in https://github.com/assimp/assimp/pull/5441
* Fix for issues #5422, #3411, and #5443 -- DXF insert scaling fix and colour fix by @seanth in https://github.com/assimp/assimp/pull/5426
* Update StbCommon.h to stay up-to-date with stb_image.h. by @tigert1998 in https://github.com/assimp/assimp/pull/5436
* Introduce aiBuffer by @kimkulling in https://github.com/assimp/assimp/pull/5444
* Add bounds checks to the parsing utilities. by @kimkulling in https://github.com/assimp/assimp/pull/5421
* Fix crash in viewer by @kimkulling in https://github.com/assimp/assimp/pull/5446
* Static code analysis fixes by @kimkulling in https://github.com/assimp/assimp/pull/5447
* Kimkulling/fix bahavior of remove redundat mats issue 5438 by @kimkulling in https://github.com/assimp/assimp/pull/5451
* Fix X importer breakage introduced in commit f844c33  by @tellypresence in https://github.com/assimp/assimp/pull/5372
* Fileformats.md:  clarify that import of .blend files is deprecated by @stephengold in https://github.com/assimp/assimp/pull/5350
* feat:1.add 3mf vertex color read 2.fix 3mf read texture bug by @GalenXiao in https://github.com/assimp/assimp/pull/5361
* More GLTF loading hardening by @FlorianBorn71 in https://github.com/assimp/assimp/pull/5415
* Bump actions/cache from 3 to 4 by @dependabot in https://github.com/assimp/assimp/pull/5431
* Update CMakeLists.txt by @mosfet80 in https://github.com/assimp/assimp/pull/5379
* `Blendshape`->`Geometry` in FBX Export by @JulianKnodt in https://github.com/assimp/assimp/pull/5419
* Fix identity matrix check by @fvbj in https://github.com/assimp/assimp/pull/5445
* Fix PyAssimp under Python >= 3.12 and macOS library search support by @Th3T3chn0G1t in https://github.com/assimp/assimp/pull/5397
* Add ISC LICENSE file by @severin-lemaignan in https://github.com/assimp/assimp/pull/5465
* ColladaParser: check values length by @etam in https://github.com/assimp/assimp/pull/5462
* Include defs in not cpp-section by @kimkulling in https://github.com/assimp/assimp/pull/5466
* Add correct double zero check by @kimkulling in https://github.com/assimp/assimp/pull/5471
* Add zlib-header to ZipArchiveIOSystem.h by @kimkulling in https://github.com/assimp/assimp/pull/5473
* Add 2024 to copyright infos by @kimkulling in https://github.com/assimp/assimp/pull/5475
* Append a new setting "AI_CONFIG_EXPORT_FBX_TRANSPARENCY_FACTOR_REFER_TO_OPACITY" by @Riv1s-sSsA01 in https://github.com/assimp/assimp/pull/5450
* Eliminate non-ascii comments in clipper by @adfwer233 in https://github.com/assimp/assimp/pull/5480
* Fix compilation for MSVC14. by @LukasBanana in https://github.com/assimp/assimp/pull/5490
* Add correction of fbx model rotation by @kimkulling in https://github.com/assimp/assimp/pull/5494
* Delete tools/make directory by @mosfet80 in https://github.com/assimp/assimp/pull/5491
* Delete packaging/windows-mkzip directory by @mosfet80 in https://github.com/assimp/assimp/pull/5492
* Fix #5420 duplicate degrees to radians conversion in fbx importer by @Biohazard90 in https://github.com/assimp/assimp/pull/5427
* Respect merge identical vertices in ObjExporter by @JulianKnodt in https://github.com/assimp/assimp/pull/5521
* Fix utDefaultIOStream test under MinGW by @thenanisore in https://github.com/assimp/assimp/pull/5525
* Fix typos by @RoboSchmied in https://github.com/assimp/assimp/pull/5518
* Add initial macOS support to C4D importer by @AlexTMjugador in https://github.com/assimp/assimp/pull/5516
* Update hunter into CMakeLists.txt by @mosfet80 in https://github.com/assimp/assimp/pull/5505
* Fix: add missing import for `AI_CONFIG_CHECK_IDENTITY_MATRIX_EPSILON_DEFAULT` by @tomheaton in https://github.com/assimp/assimp/pull/5507
* updated json by @mosfet80 in https://github.com/assimp/assimp/pull/5501
* Cleanup: Fix review findings by @kimkulling in https://github.com/assimp/assimp/pull/5528
* Update CMakeLists.txt by @kimkulling in https://github.com/assimp/assimp/pull/5531
* CMake: Allow linking draco statically if ASSIMP_BUILD_DRACO_STATIC is set. by @alexrp in https://github.com/assimp/assimp/pull/5535
* updated minizip to last version by @mosfet80 in https://github.com/assimp/assimp/pull/5498
* updated STBIMAGElib by @mosfet80 in https://github.com/assimp/assimp/pull/5500
* fix issue #5461 (segfault after removing redundant materials) by @stephengold in https://github.com/assimp/assimp/pull/5467
* Update ComputeUVMappingProcess.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5541
* add some ASSIMP_INSTALL checks by @ZeunO8 in https://github.com/assimp/assimp/pull/5545
* Fix SplitByBoneCount typo that prevented node updates by @Succ3s in https://github.com/assimp/assimp/pull/5550
* Q3DLoader: Fix possible material string overflow by @kimkulling in https://github.com/assimp/assimp/pull/5556
* Reverts the changes introduced by commit ad766cb in February 2022. by @johannesugb in https://github.com/assimp/assimp/pull/5542
* fix a collada import bug by @xiaoxiaopifu in https://github.com/assimp/assimp/pull/5561
* mention IQM loader in Fileformats.md by @Garux in https://github.com/assimp/assimp/pull/5560
* Kimkulling/fix pyassimp compatibility by @kimkulling in https://github.com/assimp/assimp/pull/5563
* fix ASE loader crash when *MATERIAL_COUNT or *NUMSUBMTLS is not specified or is 0 by @Garux in https://github.com/assimp/assimp/pull/5559
* Add checks for invalid buffer and size by @kimkulling in https://github.com/assimp/assimp/pull/5570
* Make sure for releases revision will be zero by @kimkulling in https://github.com/assimp/assimp/pull/5571
* glTF2Importer: Support .vrm extension by @uyjulian in https://github.com/assimp/assimp/pull/5569
* Prepare v5.4.1 by @kimkulling in https://github.com/assimp/assimp/pull/5573
* Remove deprecated c++11 warnings by @kimkulling in https://github.com/assimp/assimp/pull/5576
* fix ci by disabling tests by @kimkulling in https://github.com/assimp/assimp/pull/5583
* Fix integer overflow by @kimkulling in https://github.com/assimp/assimp/pull/5581
* Assimp viewer fixes by @JLouis-B in https://github.com/assimp/assimp/pull/5582
* Optimize readability by @kimkulling in https://github.com/assimp/assimp/pull/5578
* Temporary fix for #5557 GCC 13+ build issue -Warray-bounds by @dbs4261 in https://github.com/assimp/assimp/pull/5577
* Fix a bug that could cause assertion failure. by @vengine in https://github.com/assimp/assimp/pull/5575
* Fix possible nullptr dereferencing. by @kimkulling in https://github.com/assimp/assimp/pull/5595
* Update ObjFileParser.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5598
* Fix for #5592 Disabled maybe-uninitialized error for AssetLib/Obj/ObjFileParser.cpp by @dbs4261 in https://github.com/assimp/assimp/pull/5593
* updated zip by @mosfet80 in https://github.com/assimp/assimp/pull/5499
* Postprocessing: Fix endless loop by @kimkulling in https://github.com/assimp/assimp/pull/5605
* Build: Fix compilation for VS-2022 debug mode - warning by @kimkulling in https://github.com/assimp/assimp/pull/5606
* Converted a size_t to mz_uint that was being treated as an error by @BradlyLanducci in https://github.com/assimp/assimp/pull/5600
* Add trim to xml string parsing by @kimkulling in https://github.com/assimp/assimp/pull/5611
* Replace duplicated trim by @kimkulling in https://github.com/assimp/assimp/pull/5613
* Move aiScene constructor by @kimkulling in https://github.com/assimp/assimp/pull/5614
* Move revision.h and revision.h.in to include folder by @kimkulling in https://github.com/assimp/assimp/pull/5615
* Update MDLMaterialLoader.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5620
* Create inno_setup by @kimkulling in https://github.com/assimp/assimp/pull/5621
* clean HunterGate.cmake by @mosfet80 in https://github.com/assimp/assimp/pull/5619
* Draft: Update init of aiString by @kimkulling in https://github.com/assimp/assimp/pull/5623
* Fix init aistring issue 5622 inpython module by @kimkulling in https://github.com/assimp/assimp/pull/5625
* update dotnet example by @mosfet80 in https://github.com/assimp/assimp/pull/5618
* Make stepfile schema validation more robust. by @kimkulling in https://github.com/assimp/assimp/pull/5627
* fix PLY binary export color from float to uchar by @micott in https://github.com/assimp/assimp/pull/5608
* Some FBXs do not have "Materials" information, which can cause parsing errors by @ycn2022 in https://github.com/assimp/assimp/pull/5624
* Fix collada uv channels - temporary was stored and then updated. by @StepanHrbek in https://github.com/assimp/assimp/pull/5630
* remove ASE parsing break by @Garux in https://github.com/assimp/assimp/pull/5634
* FBX-Exporter: Fix nullptr dereferencing by @kimkulling in https://github.com/assimp/assimp/pull/5638
* Fix FBX exporting incorrect bone order by @JulianKnodt in https://github.com/assimp/assimp/pull/5435
* fixes potential memory leak on malformed obj file by @TinyTinni in https://github.com/assimp/assimp/pull/5645
* Update zip.c by @ThatOSDev in https://github.com/assimp/assimp/pull/5639
* Fixes some uninit bool loads by @TinyTinni in https://github.com/assimp/assimp/pull/5644
* Fix names of enum values in docstring of aiProcess_FindDegenerates by @mapret in https://github.com/assimp/assimp/pull/5640
* Fix: StackAllocator Undefined Reference fix by @thearchivalone in https://github.com/assimp/assimp/pull/5650
* Plx: Fix out of bound access by @kimkulling in https://github.com/assimp/assimp/pull/5651
* Docker: Fix security finding by @kimkulling in https://github.com/assimp/assimp/pull/5655
* Fix potential heapbuffer overflow in md5 parsing by @TinyTinni in https://github.com/assimp/assimp/pull/5652
* Replace raw pointers by std::string by @kimkulling in https://github.com/assimp/assimp/pull/5656
* Fix compile warning by @kimkulling in https://github.com/assimp/assimp/pull/5657
* Allow empty slots in mTextureCoords by @StepanHrbek in https://github.com/assimp/assimp/pull/5636
* Update Readme.md by @kimkulling in https://github.com/assimp/assimp/pull/5663
* Update Readme.md by @kimkulling in https://github.com/assimp/assimp/pull/5665
* [USD] Integrate "tinyusdz" project by @tellypresence in https://github.com/assimp/assimp/pull/5628
* Kimkulling/fix double precision tests by @kimkulling in https://github.com/assimp/assimp/pull/5660
* Update Python structs with missing fields that were causing core dumps by @vjf in https://github.com/assimp/assimp/pull/5673
* Introduce interpolation mode to vectro and quaternion keys by @kimkulling in https://github.com/assimp/assimp/pull/5674
* Fix a fuzz test heap buffer overflow in mdl material loader by @sgayda2 in https://github.com/assimp/assimp/pull/5658
* Mosfet80 updatedpoli2tri by @kimkulling in https://github.com/assimp/assimp/pull/5682
* CalcTangents: zero vector is invalid for tangent/bitangent by @JensEhrhardt-eOPUS in https://github.com/assimp/assimp/pull/5432
* A fuzzed stride could cause the max count to become negative and henc… by @FlorianBorn71 in https://github.com/assimp/assimp/pull/5414
* Return false instead of crash by @kimkulling in https://github.com/assimp/assimp/pull/5685
* Make coord transfor for hs1 files optional by @kimkulling in https://github.com/assimp/assimp/pull/5687
* Update DefaultIOSystem.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5697
* FBX exporter - handle multiple vertex color channels by @Kimbatt in https://github.com/assimp/assimp/pull/5695
* Fixing static builds on Windows by @natevm in https://github.com/assimp/assimp/pull/5713
* Added AND condition in poly2tri dll_symbol.h to only define macros fo… by @mkuritsu in https://github.com/assimp/assimp/pull/5693
* Fix MSVC PDBs and permit them to be disabled if required by @RichardTea in https://github.com/assimp/assimp/pull/5710
* Use DRACO_GLTF_BITSTREAM by @RichardTea in https://github.com/assimp/assimp/pull/5709
* include Exceptional.h in 3DSExporter.cpp by @Fiskmans in https://github.com/assimp/assimp/pull/5707
* Remove recursive include by @Fiskmans in https://github.com/assimp/assimp/pull/5705
* Fix: Possible out-of-bound read in findDegenerate by @TinyTinni in https://github.com/assimp/assimp/pull/5679
* Revert variable name by @tellypresence in https://github.com/assimp/assimp/pull/5715
* Add compile option /source-charset:utf-8 for MSVC by @kenichiice in https://github.com/assimp/assimp/pull/5716
* Fix leak in loader by @kimkulling in https://github.com/assimp/assimp/pull/5718
* Expose aiGetEmbeddedTexture to C-API by @sacereda in https://github.com/assimp/assimp/pull/5382
* Sparky kitty studios master by @kimkulling in https://github.com/assimp/assimp/pull/5727
* Added more Maya materials by @Sanchikuuus in https://github.com/assimp/assimp/pull/5101
* Fix to check both types of slashes in GetShortFilename by @imdongye in https://github.com/assimp/assimp/pull/5728
* Bump actions/download-artifact from 1 to 4 by @dependabot in https://github.com/assimp/assimp/pull/5732
* Bump actions/upload-artifact from 1 to 4 by @dependabot in https://github.com/assimp/assimp/pull/5731
* Bump softprops/action-gh-release from 1 to 2 by @dependabot in https://github.com/assimp/assimp/pull/5730
* Fix copying private data when source pointer is NULL by @vjf in https://github.com/assimp/assimp/pull/5733
* Fix potential memory leak in SceneCombiner for LWS/IRR/MD3 loader by @TinyTinni in https://github.com/assimp/assimp/pull/5721
* Fix to correctly determine 'multi-configuration' on Windows by @kenichiice in https://github.com/assimp/assimp/pull/5720
* Fix casting typo in D3MFExporter::writeBaseMaterials by @ochafik in https://github.com/assimp/assimp/pull/5681
* FBX: add metadata of ainode as properties by @fuhaixi in https://github.com/assimp/assimp/pull/5675
* feat: add option for creating XCFramework and configure minimum iOS target by @AKosmachyov in https://github.com/assimp/assimp/pull/5648
* Update PyAssimp structs with Skeleton & SkeletonBone members by @vjf in https://github.com/assimp/assimp/pull/5734
* The total length is incorrect when exporting gltf2 by @Fav in https://github.com/assimp/assimp/pull/5647
* `build`: Add ccache support by @ochafik in https://github.com/assimp/assimp/pull/5686
* Update ccpp.yml by @kimkulling in https://github.com/assimp/assimp/pull/5740
* Ply-Importer: Fix vulnerability by @kimkulling in https://github.com/assimp/assimp/pull/5739
* prepare v5.4.3 by @kimkulling in https://github.com/assimp/assimp/pull/5741
* Zero-length mChildren arrays should be nullptr by @RichardTea in https://github.com/assimp/assimp/pull/5749
* Allow usage of pugixml from a superproject by @diiigle in https://github.com/assimp/assimp/pull/5752
* Prevents PLY from parsing duplicate defined elements by @TinyTinni in https://github.com/assimp/assimp/pull/5743
* Add option to ignore FBX custom axes by @RichardTea in https://github.com/assimp/assimp/pull/5754
* Kimkulling/mark blender versions as not supported by @kimkulling in https://github.com/assimp/assimp/pull/5370
* Fix leak by @kimkulling in https://github.com/assimp/assimp/pull/5762
* Fix invalid access by @cla7aye15I4nd in https://github.com/assimp/assimp/pull/5765
* Fix buffer overflow in MD3Loader by @cla7aye15I4nd in https://github.com/assimp/assimp/pull/5763
* Fix stack overflow by @cla7aye15I4nd in https://github.com/assimp/assimp/pull/5764
* FBX Import - Restored Absolute Transform Calculation by @lxw404 in https://github.com/assimp/assimp/pull/5751
* Fix naming in aiMaterial comment by @PatrickDahlin in https://github.com/assimp/assimp/pull/5780
* Update dll_symbol.h by @kimkulling in https://github.com/assimp/assimp/pull/5781
* Fix for build with ASSIMP_BUILD_NO_VALIDATEDS_PROCESS by @Pichas in https://github.com/assimp/assimp/pull/5774
* Update CMakeLists.txt by @kimkulling in https://github.com/assimp/assimp/pull/5782
* FBX Blendshapes: Do not require normals by @JulianKnodt in https://github.com/assimp/assimp/pull/5776
* Update Build.md by @kimkulling in https://github.com/assimp/assimp/pull/5796
* Update Readme.md by @kimkulling in https://github.com/assimp/assimp/pull/5797
* SplitLargeMeshes: Fix crash by @kimkulling in https://github.com/assimp/assimp/pull/5799
* Installer: fix images for installer by @kimkulling in https://github.com/assimp/assimp/pull/5800
* Bugfix/installer add missing images by @kimkulling in https://github.com/assimp/assimp/pull/5803
* Fix bug introduced in commit 168ae22 of 27 Oct 2019 by @tellypresence in https://github.com/assimp/assimp/pull/5813
* Fix issue 5767: Can't load USD from memory by @Pichas in https://github.com/assimp/assimp/pull/5818
* Fix FBX animation bug (issue 3390) by @tellypresence in https://github.com/assimp/assimp/pull/5815
* [Fix issue 5823] Hotfix for broken lightwave normals by @tellypresence in https://github.com/assimp/assimp/pull/5824
* Fixed bug in DefaultLogger::set by @chefrolle695 in https://github.com/assimp/assimp/pull/5826
* Fix a bug in the assbin loader that reads uninitialized memory by @qingyouzhao in https://github.com/assimp/assimp/pull/5801
* Fix issue 2889 (molecule_ascii.cob load failure): change integers to floating point values in color triplets by @tellypresence in https://github.com/assimp/assimp/pull/5819
* Add unit tests for X3D models which were broken at 5 Oct 2020 commit 3b9d4cf by @tellypresence in https://github.com/assimp/assimp/pull/5828
* Update inno_setup-actions by @mosfet80 in https://github.com/assimp/assimp/pull/5833
* Simplify re-enabling M3D build support by @tellypresence in https://github.com/assimp/assimp/pull/5835
* Update hunter by @mosfet80 in https://github.com/assimp/assimp/pull/5831
* Store current exception when caught in ASSIMP_CATCH_GLOBAL_EXCEPTIONS by @mischmit in https://github.com/assimp/assimp/pull/5810
* Fix issue 5816 (cone.nff load failure): repair faulty line in 3D model file by @tellypresence in https://github.com/assimp/assimp/pull/5817
* Readme: Add project activity view item by @kimkulling in https://github.com/assimp/assimp/pull/5854
* Cleanup Unit Tests Output by @AMZN-Gene in https://github.com/assimp/assimp/pull/5852
* USD Skinned Mesh by @AMZN-Gene in https://github.com/assimp/assimp/pull/5812
* Update tinyusdz by @tellypresence in https://github.com/assimp/assimp/pull/5849
* +Add vertex duplication during face normal generation by @diiigle in https://github.com/assimp/assimp/pull/5805
* Fix use of uninitialized value. by @feuerste in https://github.com/assimp/assimp/pull/5867
* Update CMakeLists.txt to fix gcc/clang++ issue by @jwbla in https://github.com/assimp/assimp/pull/5863
* Add reference screenshots for complex bundled test 3D model files by @tellypresence in https://github.com/assimp/assimp/pull/5822
* Obj: Fix Sonarcube findings by @kimkulling in https://github.com/assimp/assimp/pull/5873
* Try to resolve image paths by replacing backslashes or forward slashes in EmbedTexturesProcess by @david-campos in https://github.com/assimp/assimp/pull/5844
* Material: Fix the build for c compiler by @kimkulling in https://github.com/assimp/assimp/pull/5879
* Material: Fix sonarcube finding by @kimkulling in https://github.com/assimp/assimp/pull/5880
* Remove strcpy. by @kimkulling in https://github.com/assimp/assimp/pull/5802
* Fix potential uninitialized variable in clipper by @miselin in https://github.com/assimp/assimp/pull/5881
* Check that mMaterials not null before access by @JulianKnodt in https://github.com/assimp/assimp/pull/5874
* Cleanup: Delete code/.editorconfig by @kimkulling in https://github.com/assimp/assimp/pull/5889
* Readme.md: Add sonarcube badge by @kimkulling in https://github.com/assimp/assimp/pull/5893
* Obj: fix nullptr access. by @kimkulling in https://github.com/assimp/assimp/pull/5894
* Update cpp-pm / hunter by @mosfet80 in https://github.com/assimp/assimp/pull/5885
* Add CI to automatically build and attach binaries to releases by @Saalvage in https://github.com/assimp/assimp/pull/5892
* Simplify JoinVerticesProcess by @JulianKnodt in https://github.com/assimp/assimp/pull/5895
* USD Keyframe Animations by @AMZN-Gene in https://github.com/assimp/assimp/pull/5856
* Fix compiler error when double precision is selected, by @hankarun in https://github.com/assimp/assimp/pull/5902
* Synchronize `DefaultLogger` by @Saalvage in https://github.com/assimp/assimp/pull/5898
* Do not create GLTF Mesh if no faces by @JulianKnodt in https://github.com/assimp/assimp/pull/5878
* FBX Blendshape: export float & same # verts by @JulianKnodt in https://github.com/assimp/assimp/pull/5775
* bugfix: Fixed the issue that draco compressed gltf files cannot be lo… by @HandsomeXi in https://github.com/assimp/assimp/pull/5883
* pbrt: Validate mesh in WriteMesh before AttributeBegin call by @lijenicol in https://github.com/assimp/assimp/pull/5884
* Introducing assimp Guru on Gurubase.io by @kursataktas in https://github.com/assimp/assimp/pull/5887
* Fix: Fix build for mingw10 by @kimkulling in https://github.com/assimp/assimp/pull/5916
* Fix use after free in the CallbackToLogRedirector by @tyler92 in https://github.com/assimp/assimp/pull/5918
* USD Mesh Node Fix by @AMZN-Gene in https://github.com/assimp/assimp/pull/5915
* Fixed warnings by @sacereda in https://github.com/assimp/assimp/pull/5903
* Replace C# port with maintained fork by @Saalvage in https://github.com/assimp/assimp/pull/5922
* Fix heap-buffer-overflow in OpenDDLParser by @tyler92 in https://github.com/assimp/assimp/pull/5919
* Fix parsing of comments at the end of lines for tokens with variable number of elements. (#5890) by @scschaefer in https://github.com/assimp/assimp/pull/5891
* Fix buffer overflow in MD5Parser::SkipSpacesAndLineEnd by @tyler92 in https://github.com/assimp/assimp/pull/5921
* Fix: Fix name collision by @kimkulling in https://github.com/assimp/assimp/pull/5937
* Bug/evaluate matrix4x4 access by @kimkulling in https://github.com/assimp/assimp/pull/5936
* glTF importers: Avoid strncpy truncating away the ' \0' character by @david-campos in https://github.com/assimp/assimp/pull/5931
* Export tangents in GLTF by @JulianKnodt in https://github.com/assimp/assimp/pull/5900
* Disable logs for fuzzer by default by @tyler92 in https://github.com/assimp/assimp/pull/5938
* Fix docs for aiImportFileExWithProperties to not talk about the importer keeping the Scene alive by @david-campos in https://github.com/assimp/assimp/pull/5925
* Fix stack overflow in LWS loader by @tyler92 in https://github.com/assimp/assimp/pull/5941
* Introduce VRML format (.wrl and .x3dv) 3D model support by @tellypresence in https://github.com/assimp/assimp/pull/5857
* Verify negative values in Quake1 MDL header by @tyler92 in https://github.com/assimp/assimp/pull/5940
* Fix heap buffer overflow in HMP loader by @tyler92 in https://github.com/assimp/assimp/pull/5939
* pragma warning bug fix when using g++ on windows by @stekap000 in https://github.com/assimp/assimp/pull/5943
* AssbinImporter::ReadInternFile now closes stream before throwing by @david-campos in https://github.com/assimp/assimp/pull/5927
* Updated Material.cpp to Add Missing Texture Types to String by @crazyjackel in https://github.com/assimp/assimp/pull/5945
* Docker: Optimize usage by @kimkulling in https://github.com/assimp/assimp/pull/5948
* Bugfix/cosmetic code cleanup by @kimkulling in https://github.com/assimp/assimp/pull/5947
* Add arm64-simulator support to iOS build script by @DwayneCoussement in https://github.com/assimp/assimp/pull/5920
* Add aiProcess_ValidateDataStructure flag to the fuzzer by @tyler92 in https://github.com/assimp/assimp/pull/5951
* Update OpenDDLParser.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5953
* [AMF] Fix texture mapping by @tellypresence in https://github.com/assimp/assimp/pull/5949
* [FBX] Allow export multi materials per node by @JulianKnodt in https://github.com/assimp/assimp/pull/5888
* Assimp master head fixes for failure to compile by @enginmanap in https://github.com/assimp/assimp/pull/5899
* Prefix MTL textures with the MTL directory path by @david-campos in https://github.com/assimp/assimp/pull/5928
* Add customExtension support to the scene by @BurntRanch in https://github.com/assimp/assimp/pull/5954
* Avoid exporting all primitives, which are not triangles. by @kimkulling in https://github.com/assimp/assimp/pull/5964
* Added GLTF Extension KHR_materials_anisotropy by @luho383 in https://github.com/assimp/assimp/pull/5950
* Add POST_BUILD option to ADD_CUSTOM_COMMAND by @MikeChemi in https://github.com/assimp/assimp/pull/5962
* Fix heap buffer overflow in PLY parser by @tyler92 in https://github.com/assimp/assimp/pull/5956
* Optimise building tinyusd library by @Pichas in https://github.com/assimp/assimp/pull/5959
* Add gltf metallic-roughness texture type by @tellypresence in https://github.com/assimp/assimp/pull/5968
* fix: reduce gltf2 export time by @1323236654 in https://github.com/assimp/assimp/pull/5972
* Flag Documentation Fix by @snave333 in https://github.com/assimp/assimp/pull/5978
* Doc: Make hint clearer by @kimkulling in https://github.com/assimp/assimp/pull/5988
* Clean STEPFileReader.cpp by @mosfet80 in https://github.com/assimp/assimp/pull/5973
* Update Readme.md: Add new viewer by @kimkulling in https://github.com/assimp/assimp/pull/5991
* Doc: Separate viewer by @kimkulling in https://github.com/assimp/assimp/pull/5995
* Use correct data type for animation key by @kimkulling in https://github.com/assimp/assimp/pull/5998
* Use ear-cutting library for triangulation by @Saalvage in https://github.com/assimp/assimp/pull/5977
* Fixing PyAssimp misalignment errors with certain structures by @fishguy6564 in https://github.com/assimp/assimp/pull/6001
* Bugfix/fix mingw issue 5975 by @kimkulling in https://github.com/assimp/assimp/pull/6005
* IFC: Remove redundand check by @kimkulling in https://github.com/assimp/assimp/pull/6006
* Obj: remove smooth-normals postprocessing by @kimkulling in https://github.com/assimp/assimp/pull/6031
* Refactorings: glTF cleanups by @kimkulling in https://github.com/assimp/assimp/pull/6028
* Fix memory leak in OpenGEXImporter by @UnionTech-Software in https://github.com/assimp/assimp/pull/6036
* Use std::copy to copy array and remove user destructor to make sure is_trivially_copyable in order to avoid -Wno-error=nontrivial-memcall by @cielavenir in https://github.com/assimp/assimp/pull/6029
* Fix: Let OpenGEX accept color3 types by @kimkulling in https://github.com/assimp/assimp/pull/6040
* ASE: Fix possible out of bound access. by @kimkulling in https://github.com/assimp/assimp/pull/6045
* MDL: Limit max texture sizes by @kimkulling in https://github.com/assimp/assimp/pull/6046
* MDL: Fix overflow check by @kimkulling in https://github.com/assimp/assimp/pull/6047
* Fix: Avoid override in line parsing by @kimkulling in https://github.com/assimp/assimp/pull/6048
* Bugfix: Fix possible nullptr dereferencing by @kimkulling in https://github.com/assimp/assimp/pull/6049
* Potential fix for code scanning alert no. 63: Potential use after free by @kimkulling in https://github.com/assimp/assimp/pull/6050
* ASE: Use correct vertex container by @kimkulling in https://github.com/assimp/assimp/pull/6051
* CMS: Fix possible overflow access by @kimkulling in https://github.com/assimp/assimp/pull/6052
* [OpenGEX] disable partial implementation of light import (causes model load failure) by @tellypresence in https://github.com/assimp/assimp/pull/6044
* Update tinyusdz git hash (fix USD animation) by @tellypresence in https://github.com/assimp/assimp/pull/6034
* [draft] Check the hunter build by @kimkulling in https://github.com/assimp/assimp/pull/6061
* NDO: Fix possible overflow access by @yuntongzhang in https://github.com/assimp/assimp/pull/6055
* Fix Cinema4D Import by @krishty in https://github.com/assimp/assimp/pull/6062
* Remove Redundant `virtual` by @krishty in https://github.com/assimp/assimp/pull/6064
* feat: created the aiGetStringC_Str() function. by @leliel-git in https://github.com/assimp/assimp/pull/6059
* Fix Whitespace by @krishty in https://github.com/assimp/assimp/pull/6063
* Harmonize Importer #includes by @krishty in https://github.com/assimp/assimp/pull/6065
* More `constexpr` by @krishty in https://github.com/assimp/assimp/pull/6066
* Renamed and inlined hasSkeletons() to HasSkeletons() for API consistency by @Alexelnet in https://github.com/assimp/assimp/pull/6072
* Fix set by @kimkulling in https://github.com/assimp/assimp/pull/6073
* Bugfix/ensure collada parsing works issue 1488 by @kimkulling in https://github.com/assimp/assimp/pull/6087
* Not to export empty "LayerElementNormal" or "LayerElementColor" nodes to fbx by @Riv1s-sSsA01 in https://github.com/assimp/assimp/pull/6092
* Use unique pointer to fix possible leak by @kimkulling in https://github.com/assimp/assimp/pull/6104
* Refactoring of PR #6092 by @Riv1s-sSsA01 in https://github.com/assimp/assimp/pull/6101
* fix: Fix build on armv6/armv7 by @yurivict in https://github.com/assimp/assimp/pull/6123
* Bugfix: Handling no of texture coordinates correctly by @kimkulling in https://github.com/assimp/assimp/pull/6124
* fix: possible Heap-based Buffer Overflow in ConvertToUTF8 function by @TinyTinni in https://github.com/assimp/assimp/pull/6122
* Refactor by @kimkulling in https://github.com/assimp/assimp/pull/6127
* support for cmake findpackage module mode by @kiroeko in https://github.com/assimp/assimp/pull/6121
* Replace exception by error in log by @kimkulling in https://github.com/assimp/assimp/pull/6133
* Fix a out of bound buffer access in ParsingUtils GetNextLine by @qingyouzhao in https://github.com/assimp/assimp/pull/6134
* Fix a bug where string erases throws out of range by @qingyouzhao in https://github.com/assimp/assimp/pull/6135
* Fix: Support uint16 indices in OpenGEX as well by @kimkulling in https://github.com/assimp/assimp/pull/6137
* Fix crashes by @kimkulling in https://github.com/assimp/assimp/pull/6138
* + Only the recognition of KTX2 compressed images as texture objects within the glb model is currently handled. by @copycd in https://github.com/assimp/assimp/pull/6139
* add missing constants by @daef in https://github.com/assimp/assimp/pull/6116
* Fix warning abut inexistent warning by @limdor in https://github.com/assimp/assimp/pull/6153
* Fix: Fix leak when sortbyp failes with exception by @kimkulling in https://github.com/assimp/assimp/pull/6166
* Update contrib/zip to fix data loss warning by @limdor in https://github.com/assimp/assimp/pull/6152
* Fix out-of-bounds dereferencing by @Marti2203 in https://github.com/assimp/assimp/pull/6150
* [#5983] Fix bugs introduced in fbx export by @JulianKnodt in https://github.com/assimp/assimp/pull/6000
* Doc: add C++ / c minimum by @kimkulling in https://github.com/assimp/assimp/pull/6187
* Unreal refactorings by @kimkulling in https://github.com/assimp/assimp/pull/6182
* update draco lib by @mosfet80 in https://github.com/assimp/assimp/pull/6094
* fix: missing OS separator in outfile by @Latios96 in https://github.com/assimp/assimp/pull/6098
* Add Missing Strings to aiTextureTypeToString by @crasong in https://github.com/assimp/assimp/pull/6188
* Fix issue compiling when assimp added as subdirectory by @plemanski in https://github.com/assimp/assimp/pull/6186
* Add clamping logic for to_ktime by @Marti2203 in https://github.com/assimp/assimp/pull/6149
* Add explicit "fallthrough" to switch by @tellypresence in https://github.com/assimp/assimp/pull/6143
* Fix HUNTER_ERROR_PAGE by @deccer in https://github.com/assimp/assimp/pull/6200
* Fix a bug in importing binary PLY file (#1) by @yurik42 in https://github.com/assimp/assimp/pull/6060
* Fix export fbx PolygonVertexIndex by @Riv1s-sSsA01 in https://github.com/assimp/assimp/pull/6102
* fix: closes #6069 CVE-2025-3196 by @VinzSpring in https://github.com/assimp/assimp/pull/6154
* Fix: Add "preservePivots" condition when importing FBX animation by @Nor-s in https://github.com/assimp/assimp/pull/6115
* Version: Adapt version by @kimkulling in https://github.com/assimp/assimp/pull/6212

## New Contributors
* @Begasus made their first contribution in https://github.com/assimp/assimp/pull/5255
* @ockeymm made their first contribution in https://github.com/assimp/assimp/pull/5252
* @fvbj made their first contribution in https://github.com/assimp/assimp/pull/5243
* @JulianKnodt made their first contribution in https://github.com/assimp/assimp/pull/5295
* @sloriot made their first contribution in https://github.com/assimp/assimp/pull/5270
* @Ipomoea made their first contribution in https://github.com/assimp/assimp/pull/5271
* @aumuell made their first contribution in https://github.com/assimp/assimp/pull/5278
* @TarcioV made their first contribution in https://github.com/assimp/assimp/pull/5279
* @copycd made their first contribution in https://github.com/assimp/assimp/pull/5294
* @cuppajoeman made their first contribution in https://github.com/assimp/assimp/pull/5312
* @ttxine made their first contribution in https://github.com/assimp/assimp/pull/5322
* @Futuremappermydud made their first contribution in https://github.com/assimp/assimp/pull/5356
* @MarkaRagnos0815 made their first contribution in https://github.com/assimp/assimp/pull/5359
* @0xf0ad made their first contribution in https://github.com/assimp/assimp/pull/5376
* @seanth made their first contribution in https://github.com/assimp/assimp/pull/5426
* @tigert1998 made their first contribution in https://github.com/assimp/assimp/pull/5436
* @GalenXiao made their first contribution in https://github.com/assimp/assimp/pull/5361
* @Th3T3chn0G1t made their first contribution in https://github.com/assimp/assimp/pull/5397
* @etam made their first contribution in https://github.com/assimp/assimp/pull/5462
* @adfwer233 made their first contribution in https://github.com/assimp/assimp/pull/5480
* @LukasBanana made their first contribution in https://github.com/assimp/assimp/pull/5490
* @thenanisore made their first contribution in https://github.com/assimp/assimp/pull/5525
* @RoboSchmied made their first contribution in https://github.com/assimp/assimp/pull/5518
* @AlexTMjugador made their first contribution in https://github.com/assimp/assimp/pull/5516
* @tomheaton made their first contribution in https://github.com/assimp/assimp/pull/5507
* @alexrp made their first contribution in https://github.com/assimp/assimp/pull/5535
* @ZeunO8 made their first contribution in https://github.com/assimp/assimp/pull/5545
* @Succ3s made their first contribution in https://github.com/assimp/assimp/pull/5550
* @johannesugb made their first contribution in https://github.com/assimp/assimp/pull/5542
* @xiaoxiaopifu made their first contribution in https://github.com/assimp/assimp/pull/5561
* @uyjulian made their first contribution in https://github.com/assimp/assimp/pull/5569
* @dbs4261 made their first contribution in https://github.com/assimp/assimp/pull/5577
* @vengine made their first contribution in https://github.com/assimp/assimp/pull/5575
* @BradlyLanducci made their first contribution in https://github.com/assimp/assimp/pull/5600
* @micott made their first contribution in https://github.com/assimp/assimp/pull/5608
* @ycn2022 made their first contribution in https://github.com/assimp/assimp/pull/5624
* @ThatOSDev made their first contribution in https://github.com/assimp/assimp/pull/5639
* @mapret made their first contribution in https://github.com/assimp/assimp/pull/5640
* @thearchivalone made their first contribution in https://github.com/assimp/assimp/pull/5650
* @sgayda2 made their first contribution in https://github.com/assimp/assimp/pull/5658
* @JensEhrhardt-eOPUS made their first contribution in https://github.com/assimp/assimp/pull/5432
* @Kimbatt made their first contribution in https://github.com/assimp/assimp/pull/5695
* @natevm made their first contribution in https://github.com/assimp/assimp/pull/5713
* @mkuritsu made their first contribution in https://github.com/assimp/assimp/pull/5693
* @Sanchikuuus made their first contribution in https://github.com/assimp/assimp/pull/5101
* @imdongye made their first contribution in https://github.com/assimp/assimp/pull/5728
* @ochafik made their first contribution in https://github.com/assimp/assimp/pull/5681
* @fuhaixi made their first contribution in https://github.com/assimp/assimp/pull/5675
* @AKosmachyov made their first contribution in https://github.com/assimp/assimp/pull/5648
* @Fav made their first contribution in https://github.com/assimp/assimp/pull/5647
* @cla7aye15I4nd made their first contribution in https://github.com/assimp/assimp/pull/5765
* @lxw404 made their first contribution in https://github.com/assimp/assimp/pull/5751
* @PatrickDahlin made their first contribution in https://github.com/assimp/assimp/pull/5780
* @Pichas made their first contribution in https://github.com/assimp/assimp/pull/5774
* @chefrolle695 made their first contribution in https://github.com/assimp/assimp/pull/5826
* @qingyouzhao made their first contribution in https://github.com/assimp/assimp/pull/5801
* @mischmit made their first contribution in https://github.com/assimp/assimp/pull/5810
* @AMZN-Gene made their first contribution in https://github.com/assimp/assimp/pull/5852
* @jwbla made their first contribution in https://github.com/assimp/assimp/pull/5863
* @david-campos made their first contribution in https://github.com/assimp/assimp/pull/5844
* @miselin made their first contribution in https://github.com/assimp/assimp/pull/5881
* @hankarun made their first contribution in https://github.com/assimp/assimp/pull/5902
* @HandsomeXi made their first contribution in https://github.com/assimp/assimp/pull/5883
* @lijenicol made their first contribution in https://github.com/assimp/assimp/pull/5884
* @kursataktas made their first contribution in https://github.com/assimp/assimp/pull/5887
* @tyler92 made their first contribution in https://github.com/assimp/assimp/pull/5918
* @scschaefer made their first contribution in https://github.com/assimp/assimp/pull/5891
* @stekap000 made their first contribution in https://github.com/assimp/assimp/pull/5943
* @crazyjackel made their first contribution in https://github.com/assimp/assimp/pull/5945
* @DwayneCoussement made their first contribution in https://github.com/assimp/assimp/pull/5920
* @BurntRanch made their first contribution in https://github.com/assimp/assimp/pull/5954
* @MikeChemi made their first contribution in https://github.com/assimp/assimp/pull/5962
* @1323236654 made their first contribution in https://github.com/assimp/assimp/pull/5972
* @snave333 made their first contribution in https://github.com/assimp/assimp/pull/5978
* @fishguy6564 made their first contribution in https://github.com/assimp/assimp/pull/6001
* @UnionTech-Software made their first contribution in https://github.com/assimp/assimp/pull/6036
* @cielavenir made their first contribution in https://github.com/assimp/assimp/pull/6029
* @yuntongzhang made their first contribution in https://github.com/assimp/assimp/pull/6055
* @leliel-git made their first contribution in https://github.com/assimp/assimp/pull/6059
* @Alexelnet made their first contribution in https://github.com/assimp/assimp/pull/6072
* @yurivict made their first contribution in https://github.com/assimp/assimp/pull/6123
* @kiroeko made their first contribution in https://github.com/assimp/assimp/pull/6121
* @limdor made their first contribution in https://github.com/assimp/assimp/pull/6153
* @Marti2203 made their first contribution in https://github.com/assimp/assimp/pull/6150
* @Latios96 made their first contribution in https://github.com/assimp/assimp/pull/6098
* @crasong made their first contribution in https://github.com/assimp/assimp/pull/6188
* @plemanski made their first contribution in https://github.com/assimp/assimp/pull/6186
* @deccer made their first contribution in https://github.com/assimp/assimp/pull/6200
* @yurik42 made their first contribution in https://github.com/assimp/assimp/pull/6060
* @VinzSpring made their first contribution in https://github.com/assimp/assimp/pull/6154

**Full Changelog**: https://github.com/assimp/assimp/compare/v5.3.1...v6.0.1

# 6.0.0
## What's Changed
* Fix building on Haiku by @Begasus in https://github.com/assimp/assimp/pull/5255
* Reduce memory consumption in JoinVerticesProcess::ProcessMesh() signi… by @ockeymm in https://github.com/assimp/assimp/pull/5252
* Fix: Add check for invalid input argument by @kimkulling in https://github.com/assimp/assimp/pull/5258
* Replace an assert by a error log. by @kimkulling in https://github.com/assimp/assimp/pull/5260
* Extension of skinning data export to GLB/GLTF format by @fvbj in https://github.com/assimp/assimp/pull/5243
* Fix output floating-point values to fbx by @Riv1s-sSsA01 in https://github.com/assimp/assimp/pull/5265
* Update ImproveCacheLocality.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5268
* Update Readme.md by @kimkulling in https://github.com/assimp/assimp/pull/5277
* Deep arsdk bone double free by @kimkulling in https://github.com/assimp/assimp/pull/5291
* Fix Spelling error by @JulianKnodt in https://github.com/assimp/assimp/pull/5295
* use size in order to be compatible with float and double by @sloriot in https://github.com/assimp/assimp/pull/5270
* Fix: Add missing transformation for normalized normals. by @kimkulling in https://github.com/assimp/assimp/pull/5301
* Fix: Implicit Conversion Error by @Ipomoea in https://github.com/assimp/assimp/pull/5271
* Fix add checks for indices by @kimkulling in https://github.com/assimp/assimp/pull/5306
* Update FBXBinaryTokenizer.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5311
* link to external minizip with full path by @aumuell in https://github.com/assimp/assimp/pull/5278
* utf8 header not found by @TarcioV in https://github.com/assimp/assimp/pull/5279
* Rm unnecessary deg->radian conversion in FBX exporter by @JulianKnodt in https://github.com/assimp/assimp/pull/5281
* Fix empty mesh handling by @kimkulling in https://github.com/assimp/assimp/pull/5318
* Refactoring: Some cleanups by @kimkulling in https://github.com/assimp/assimp/pull/5319
* Fix invalid read of `uint` from `uvwsrc` by @JulianKnodt in https://github.com/assimp/assimp/pull/5282
* Remove double delete by @kimkulling in https://github.com/assimp/assimp/pull/5325
* fix mesh-name error. by @copycd in https://github.com/assimp/assimp/pull/5294
* COLLADA fixes for textures in C4D input by @wmatyjewicz in https://github.com/assimp/assimp/pull/5293
* Use the correct allocator for deleting objects in case of duplicate a… by @FlorianBorn71 in https://github.com/assimp/assimp/pull/5305
* Fix container overflow in MMD parser by @aavenel in https://github.com/assimp/assimp/pull/5309
* Fix: PLY heap buffer overflow by @aavenel in https://github.com/assimp/assimp/pull/5310
* Update Readme.md by @cuppajoeman in https://github.com/assimp/assimp/pull/5312
* Update Readme.md by @cuppajoeman in https://github.com/assimp/assimp/pull/5313
* Fix: Check if index for mesh access is out of range by @kimkulling in https://github.com/assimp/assimp/pull/5338
* Update FBXConverter.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5349
* FBX: Use correct time scaling by @kimkulling in https://github.com/assimp/assimp/pull/5355
* Drop explicit inclusion of contrib/ headers by @umlaeute in https://github.com/assimp/assimp/pull/5316
* Update Build.md by @cuppajoeman in https://github.com/assimp/assimp/pull/5314
* Fix buffer overflow in FBX::Util::DecodeBase64() by @ttxine in https://github.com/assimp/assimp/pull/5322
* Readme.md:  correct 2 errors in section headers by @stephengold in https://github.com/assimp/assimp/pull/5351
* Fix double free in Video::~Video() by @ttxine in https://github.com/assimp/assimp/pull/5323
* FBXMeshGeometry:  solve issue #5116 using patch provided by darktjm by @stephengold in https://github.com/assimp/assimp/pull/5333
* Fix target names not being imported on some gLTF2 models by @Futuremappermydud in https://github.com/assimp/assimp/pull/5356
* correct grammar/typographic errors in comments (8 files) by @stephengold in https://github.com/assimp/assimp/pull/5343
* KHR_materials_specular fixes by @rudybear in https://github.com/assimp/assimp/pull/5347
* Disable Hunter by @kimkulling in https://github.com/assimp/assimp/pull/5388
* fixed several issues by @MarkaRagnos0815 in https://github.com/assimp/assimp/pull/5359
* Fix leak by @kimkulling in https://github.com/assimp/assimp/pull/5391
* Check validity of archive without parsing by @kimkulling in https://github.com/assimp/assimp/pull/5393
* Fix integer overflow by @kimkulling in https://github.com/assimp/assimp/pull/5394
* Add a test before generating the txture folder by @kimkulling in https://github.com/assimp/assimp/pull/5400
* Build: Disable building zlib for non-windows by @kimkulling in https://github.com/assimp/assimp/pull/5401
* null check. by @copycd in https://github.com/assimp/assimp/pull/5402
* Bump actions/upload-artifact from 3 to 4 by @dependabot in https://github.com/assimp/assimp/pull/5384
* fix: KHR_materials_pbrSpecularGlossiness/diffuseFactor convert to pbr… by @guguTang in https://github.com/assimp/assimp/pull/5410
* fix building errors for MinGW by @0xf0ad in https://github.com/assimp/assimp/pull/5376
* dynamic_cast error. by @copycd in https://github.com/assimp/assimp/pull/5406
* Add missing IRR textures by @tellypresence in https://github.com/assimp/assimp/pull/5374
* Update Dockerfile by @kimkulling in https://github.com/assimp/assimp/pull/5412
* Fix handling of X3D IndexedLineSet nodes by @andre-schulz in https://github.com/assimp/assimp/pull/5362
* Improve acc file loading by @IOBYTE in https://github.com/assimp/assimp/pull/5360
* Readme.md:  present hyperlinks in a more uniform style by @stephengold in https://github.com/assimp/assimp/pull/5364
* FBX Blendshape `FullWeight: Vec<Float>` -> `FullWeight: Vec<Double>` by @JulianKnodt in https://github.com/assimp/assimp/pull/5441
* Fix for issues #5422, #3411, and #5443 -- DXF insert scaling fix and colour fix by @seanth in https://github.com/assimp/assimp/pull/5426
* Update StbCommon.h to stay up-to-date with stb_image.h. by @tigert1998 in https://github.com/assimp/assimp/pull/5436
* Introduce aiBuffer by @kimkulling in https://github.com/assimp/assimp/pull/5444
* Add bounds checks to the parsing utilities. by @kimkulling in https://github.com/assimp/assimp/pull/5421
* Fix crash in viewer by @kimkulling in https://github.com/assimp/assimp/pull/5446
* Static code analysis fixes by @kimkulling in https://github.com/assimp/assimp/pull/5447
* Kimkulling/fix bahavior of remove redundat mats issue 5438 by @kimkulling in https://github.com/assimp/assimp/pull/5451
* Fix X importer breakage introduced in commit f844c33  by @tellypresence in https://github.com/assimp/assimp/pull/5372
* Fileformats.md:  clarify that import of .blend files is deprecated by @stephengold in https://github.com/assimp/assimp/pull/5350
* feat:1.add 3mf vertex color read 2.fix 3mf read texture bug by @GalenXiao in https://github.com/assimp/assimp/pull/5361
* More GLTF loading hardening by @FlorianBorn71 in https://github.com/assimp/assimp/pull/5415
* Bump actions/cache from 3 to 4 by @dependabot in https://github.com/assimp/assimp/pull/5431
* Update CMakeLists.txt by @mosfet80 in https://github.com/assimp/assimp/pull/5379
* `Blendshape`->`Geometry` in FBX Export by @JulianKnodt in https://github.com/assimp/assimp/pull/5419
* Fix identity matrix check by @fvbj in https://github.com/assimp/assimp/pull/5445
* Fix PyAssimp under Python >= 3.12 and macOS library search support by @Th3T3chn0G1t in https://github.com/assimp/assimp/pull/5397
* Add ISC LICENSE file by @severin-lemaignan in https://github.com/assimp/assimp/pull/5465
* ColladaParser: check values length by @etam in https://github.com/assimp/assimp/pull/5462
* Include defs in not cpp-section by @kimkulling in https://github.com/assimp/assimp/pull/5466
* Add correct double zero check by @kimkulling in https://github.com/assimp/assimp/pull/5471
* Add zlib-header to ZipArchiveIOSystem.h by @kimkulling in https://github.com/assimp/assimp/pull/5473
* Add 2024 to copyright infos by @kimkulling in https://github.com/assimp/assimp/pull/5475
* Append a new setting "AI_CONFIG_EXPORT_FBX_TRANSPARENCY_FACTOR_REFER_TO_OPACITY" by @Riv1s-sSsA01 in https://github.com/assimp/assimp/pull/5450
* Eliminate non-ascii comments in clipper by @adfwer233 in https://github.com/assimp/assimp/pull/5480
* Fix compilation for MSVC14. by @LukasBanana in https://github.com/assimp/assimp/pull/5490
* Add correction of fbx model rotation by @kimkulling in https://github.com/assimp/assimp/pull/5494
* Delete tools/make directory by @mosfet80 in https://github.com/assimp/assimp/pull/5491
* Delete packaging/windows-mkzip directory by @mosfet80 in https://github.com/assimp/assimp/pull/5492
* Fix #5420 duplicate degrees to radians conversion in fbx importer by @Biohazard90 in https://github.com/assimp/assimp/pull/5427
* Respect merge identical vertices in ObjExporter by @JulianKnodt in https://github.com/assimp/assimp/pull/5521
* Fix utDefaultIOStream test under MinGW by @thenanisore in https://github.com/assimp/assimp/pull/5525
* Fix typos by @RoboSchmied in https://github.com/assimp/assimp/pull/5518
* Add initial macOS support to C4D importer by @AlexTMjugador in https://github.com/assimp/assimp/pull/5516
* Update hunter into CMakeLists.txt by @mosfet80 in https://github.com/assimp/assimp/pull/5505
* Fix: add missing import for `AI_CONFIG_CHECK_IDENTITY_MATRIX_EPSILON_DEFAULT` by @tomheaton in https://github.com/assimp/assimp/pull/5507
* updated json by @mosfet80 in https://github.com/assimp/assimp/pull/5501
* Cleanup: Fix review findings by @kimkulling in https://github.com/assimp/assimp/pull/5528
* Update CMakeLists.txt by @kimkulling in https://github.com/assimp/assimp/pull/5531
* CMake: Allow linking draco statically if ASSIMP_BUILD_DRACO_STATIC is set. by @alexrp in https://github.com/assimp/assimp/pull/5535
* updated minizip to last version by @mosfet80 in https://github.com/assimp/assimp/pull/5498
* updated STBIMAGElib by @mosfet80 in https://github.com/assimp/assimp/pull/5500
* fix issue #5461 (segfault after removing redundant materials) by @stephengold in https://github.com/assimp/assimp/pull/5467
* Update ComputeUVMappingProcess.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5541
* add some ASSIMP_INSTALL checks by @ZeunO8 in https://github.com/assimp/assimp/pull/5545
* Fix SplitByBoneCount typo that prevented node updates by @Succ3s in https://github.com/assimp/assimp/pull/5550
* Q3DLoader: Fix possible material string overflow by @kimkulling in https://github.com/assimp/assimp/pull/5556
* Reverts the changes introduced by commit ad766cb in February 2022. by @johannesugb in https://github.com/assimp/assimp/pull/5542
* fix a collada import bug by @xiaoxiaopifu in https://github.com/assimp/assimp/pull/5561
* mention IQM loader in Fileformats.md by @Garux in https://github.com/assimp/assimp/pull/5560
* Kimkulling/fix pyassimp compatibility by @kimkulling in https://github.com/assimp/assimp/pull/5563
* fix ASE loader crash when *MATERIAL_COUNT or *NUMSUBMTLS is not specified or is 0 by @Garux in https://github.com/assimp/assimp/pull/5559
* Add checks for invalid buffer and size by @kimkulling in https://github.com/assimp/assimp/pull/5570
* Make sure for releases revision will be zero by @kimkulling in https://github.com/assimp/assimp/pull/5571
* glTF2Importer: Support .vrm extension by @uyjulian in https://github.com/assimp/assimp/pull/5569
* Prepare v5.4.1 by @kimkulling in https://github.com/assimp/assimp/pull/5573
* Remove deprecated c++11 warnings by @kimkulling in https://github.com/assimp/assimp/pull/5576
* fix ci by disabling tests by @kimkulling in https://github.com/assimp/assimp/pull/5583
* Fix integer overflow by @kimkulling in https://github.com/assimp/assimp/pull/5581
* Assimp viewer fixes by @JLouis-B in https://github.com/assimp/assimp/pull/5582
* Optimize readability by @kimkulling in https://github.com/assimp/assimp/pull/5578
* Temporary fix for #5557 GCC 13+ build issue -Warray-bounds by @dbs4261 in https://github.com/assimp/assimp/pull/5577
* Fix a bug that could cause assertion failure. by @vengine in https://github.com/assimp/assimp/pull/5575
* Fix possible nullptr dereferencing. by @kimkulling in https://github.com/assimp/assimp/pull/5595
* Update ObjFileParser.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5598
* Fix for #5592 Disabled maybe-uninitialized error for AssetLib/Obj/ObjFileParser.cpp by @dbs4261 in https://github.com/assimp/assimp/pull/5593
* updated zip by @mosfet80 in https://github.com/assimp/assimp/pull/5499
* Postprocessing: Fix endless loop by @kimkulling in https://github.com/assimp/assimp/pull/5605
* Build: Fix compilation for VS-2022 debug mode - warning by @kimkulling in https://github.com/assimp/assimp/pull/5606
* Converted a size_t to mz_uint that was being treated as an error by @BradlyLanducci in https://github.com/assimp/assimp/pull/5600
* Add trim to xml string parsing by @kimkulling in https://github.com/assimp/assimp/pull/5611
* Replace duplicated trim by @kimkulling in https://github.com/assimp/assimp/pull/5613
* Move aiScene constructor by @kimkulling in https://github.com/assimp/assimp/pull/5614
* Move revision.h and revision.h.in to include folder by @kimkulling in https://github.com/assimp/assimp/pull/5615
* Update MDLMaterialLoader.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5620
* Create inno_setup by @kimkulling in https://github.com/assimp/assimp/pull/5621
* clean HunterGate.cmake by @mosfet80 in https://github.com/assimp/assimp/pull/5619
* Draft: Update init of aiString by @kimkulling in https://github.com/assimp/assimp/pull/5623
* Fix init aistring issue 5622 inpython module by @kimkulling in https://github.com/assimp/assimp/pull/5625
* update dotnet example by @mosfet80 in https://github.com/assimp/assimp/pull/5618
* Make stepfile schema validation more robust. by @kimkulling in https://github.com/assimp/assimp/pull/5627
* fix PLY binary export color from float to uchar by @micott in https://github.com/assimp/assimp/pull/5608
* Some FBXs do not have "Materials" information, which can cause parsing errors by @ycn2022 in https://github.com/assimp/assimp/pull/5624
* Fix collada uv channels - temporary was stored and then updated. by @StepanHrbek in https://github.com/assimp/assimp/pull/5630
* remove ASE parsing break by @Garux in https://github.com/assimp/assimp/pull/5634
* FBX-Exporter: Fix nullptr dereferencing by @kimkulling in https://github.com/assimp/assimp/pull/5638
* Fix FBX exporting incorrect bone order by @JulianKnodt in https://github.com/assimp/assimp/pull/5435
* fixes potential memory leak on malformed obj file by @TinyTinni in https://github.com/assimp/assimp/pull/5645
* Update zip.c by @ThatOSDev in https://github.com/assimp/assimp/pull/5639
* Fixes some uninit bool loads by @TinyTinni in https://github.com/assimp/assimp/pull/5644
* Fix names of enum values in docstring of aiProcess_FindDegenerates by @mapret in https://github.com/assimp/assimp/pull/5640
* Fix: StackAllocator Undefined Reference fix by @thearchivalone in https://github.com/assimp/assimp/pull/5650
* Plx: Fix out of bound access by @kimkulling in https://github.com/assimp/assimp/pull/5651
* Docker: Fix security finding by @kimkulling in https://github.com/assimp/assimp/pull/5655
* Fix potential heapbuffer overflow in md5 parsing by @TinyTinni in https://github.com/assimp/assimp/pull/5652
* Replace raw pointers by std::string by @kimkulling in https://github.com/assimp/assimp/pull/5656
* Fix compile warning by @kimkulling in https://github.com/assimp/assimp/pull/5657
* Allow empty slots in mTextureCoords by @StepanHrbek in https://github.com/assimp/assimp/pull/5636
* Update Readme.md by @kimkulling in https://github.com/assimp/assimp/pull/5663
* Update Readme.md by @kimkulling in https://github.com/assimp/assimp/pull/5665
* [USD] Integrate "tinyusdz" project by @tellypresence in https://github.com/assimp/assimp/pull/5628
* Kimkulling/fix double precision tests by @kimkulling in https://github.com/assimp/assimp/pull/5660
* Update Python structs with missing fields that were causing core dumps by @vjf in https://github.com/assimp/assimp/pull/5673
* Introduce interpolation mode to vectro and quaternion keys by @kimkulling in https://github.com/assimp/assimp/pull/5674
* Fix a fuzz test heap buffer overflow in mdl material loader by @sgayda2 in https://github.com/assimp/assimp/pull/5658
* Mosfet80 updatedpoli2tri by @kimkulling in https://github.com/assimp/assimp/pull/5682
* CalcTangents: zero vector is invalid for tangent/bitangent by @JensEhrhardt-eOPUS in https://github.com/assimp/assimp/pull/5432
* A fuzzed stride could cause the max count to become negative and henc… by @FlorianBorn71 in https://github.com/assimp/assimp/pull/5414
* Return false instead of crash by @kimkulling in https://github.com/assimp/assimp/pull/5685
* Make coord transfor for hs1 files optional by @kimkulling in https://github.com/assimp/assimp/pull/5687
* Update DefaultIOSystem.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5697
* FBX exporter - handle multiple vertex color channels by @Kimbatt in https://github.com/assimp/assimp/pull/5695
* Fixing static builds on Windows by @natevm in https://github.com/assimp/assimp/pull/5713
* Added AND condition in poly2tri dll_symbol.h to only define macros fo… by @mkuritsu in https://github.com/assimp/assimp/pull/5693
* Fix MSVC PDBs and permit them to be disabled if required by @RichardTea in https://github.com/assimp/assimp/pull/5710
* Use DRACO_GLTF_BITSTREAM by @RichardTea in https://github.com/assimp/assimp/pull/5709
* include Exceptional.h in 3DSExporter.cpp by @Fiskmans in https://github.com/assimp/assimp/pull/5707
* Remove recursive include by @Fiskmans in https://github.com/assimp/assimp/pull/5705
* Fix: Possible out-of-bound read in findDegenerate by @TinyTinni in https://github.com/assimp/assimp/pull/5679
* Revert variable name by @tellypresence in https://github.com/assimp/assimp/pull/5715
* Add compile option /source-charset:utf-8 for MSVC by @kenichiice in https://github.com/assimp/assimp/pull/5716
* Fix leak in loader by @kimkulling in https://github.com/assimp/assimp/pull/5718
* Expose aiGetEmbeddedTexture to C-API by @sacereda in https://github.com/assimp/assimp/pull/5382
* Sparky kitty studios master by @kimkulling in https://github.com/assimp/assimp/pull/5727
* Added more Maya materials by @Sanchikuuus in https://github.com/assimp/assimp/pull/5101
* Fix to check both types of slashes in GetShortFilename by @imdongye in https://github.com/assimp/assimp/pull/5728
* Bump actions/download-artifact from 1 to 4 by @dependabot in https://github.com/assimp/assimp/pull/5732
* Bump actions/upload-artifact from 1 to 4 by @dependabot in https://github.com/assimp/assimp/pull/5731
* Bump softprops/action-gh-release from 1 to 2 by @dependabot in https://github.com/assimp/assimp/pull/5730
* Fix copying private data when source pointer is NULL by @vjf in https://github.com/assimp/assimp/pull/5733
* Fix potential memory leak in SceneCombiner for LWS/IRR/MD3 loader by @TinyTinni in https://github.com/assimp/assimp/pull/5721
* Fix to correctly determine 'multi-configuration' on Windows by @kenichiice in https://github.com/assimp/assimp/pull/5720
* Fix casting typo in D3MFExporter::writeBaseMaterials by @ochafik in https://github.com/assimp/assimp/pull/5681
* FBX: add metadata of ainode as properties by @fuhaixi in https://github.com/assimp/assimp/pull/5675
* feat: add option for creating XCFramework and configure minimum iOS target by @AKosmachyov in https://github.com/assimp/assimp/pull/5648
* Update PyAssimp structs with Skeleton & SkeletonBone members by @vjf in https://github.com/assimp/assimp/pull/5734
* The total length is incorrect when exporting gltf2 by @Fav in https://github.com/assimp/assimp/pull/5647
* `build`: Add ccache support by @ochafik in https://github.com/assimp/assimp/pull/5686
* Update ccpp.yml by @kimkulling in https://github.com/assimp/assimp/pull/5740
* Ply-Importer: Fix vulnerability by @kimkulling in https://github.com/assimp/assimp/pull/5739
* prepare v5.4.3 by @kimkulling in https://github.com/assimp/assimp/pull/5741
* Zero-length mChildren arrays should be nullptr by @RichardTea in https://github.com/assimp/assimp/pull/5749
* Allow usage of pugixml from a superproject by @diiigle in https://github.com/assimp/assimp/pull/5752
* Prevents PLY from parsing duplicate defined elements by @TinyTinni in https://github.com/assimp/assimp/pull/5743
* Add option to ignore FBX custom axes by @RichardTea in https://github.com/assimp/assimp/pull/5754
* Kimkulling/mark blender versions as not supported by @kimkulling in https://github.com/assimp/assimp/pull/5370
* Fix leak by @kimkulling in https://github.com/assimp/assimp/pull/5762
* Fix invalid access by @cla7aye15I4nd in https://github.com/assimp/assimp/pull/5765
* Fix buffer overflow in MD3Loader by @cla7aye15I4nd in https://github.com/assimp/assimp/pull/5763
* Fix stack overflow by @cla7aye15I4nd in https://github.com/assimp/assimp/pull/5764
* FBX Import - Restored Absolute Transform Calculation by @lxw404 in https://github.com/assimp/assimp/pull/5751
* Fix naming in aiMaterial comment by @PatrickDahlin in https://github.com/assimp/assimp/pull/5780
* Update dll_symbol.h by @kimkulling in https://github.com/assimp/assimp/pull/5781
* Fix for build with ASSIMP_BUILD_NO_VALIDATEDS_PROCESS by @Pichas in https://github.com/assimp/assimp/pull/5774
* Update CMakeLists.txt by @kimkulling in https://github.com/assimp/assimp/pull/5782
* FBX Blendshapes: Do not require normals by @JulianKnodt in https://github.com/assimp/assimp/pull/5776
* Update Build.md by @kimkulling in https://github.com/assimp/assimp/pull/5796
* Update Readme.md by @kimkulling in https://github.com/assimp/assimp/pull/5797
* SplitLargeMeshes: Fix crash by @kimkulling in https://github.com/assimp/assimp/pull/5799
* Installer: fix images for installer by @kimkulling in https://github.com/assimp/assimp/pull/5800
* Bugfix/installer add missing images by @kimkulling in https://github.com/assimp/assimp/pull/5803
* Fix bug introduced in commit 168ae22 of 27 Oct 2019 by @tellypresence in https://github.com/assimp/assimp/pull/5813
* Fix issue 5767: Can't load USD from memory by @Pichas in https://github.com/assimp/assimp/pull/5818
* Fix FBX animation bug (issue 3390) by @tellypresence in https://github.com/assimp/assimp/pull/5815
* [Fix issue 5823] Hotfix for broken lightwave normals by @tellypresence in https://github.com/assimp/assimp/pull/5824
* Fixed bug in DefaultLogger::set by @chefrolle695 in https://github.com/assimp/assimp/pull/5826
* Fix a bug in the assbin loader that reads uninitialized memory by @qingyouzhao in https://github.com/assimp/assimp/pull/5801
* Fix issue 2889 (molecule_ascii.cob load failure): change integers to floating point values in color triplets by @tellypresence in https://github.com/assimp/assimp/pull/5819
* Add unit tests for X3D models which were broken at 5 Oct 2020 commit 3b9d4cf by @tellypresence in https://github.com/assimp/assimp/pull/5828
* Update inno_setup-actions by @mosfet80 in https://github.com/assimp/assimp/pull/5833
* Simplify re-enabling M3D build support by @tellypresence in https://github.com/assimp/assimp/pull/5835
* Update hunter by @mosfet80 in https://github.com/assimp/assimp/pull/5831
* Store current exception when caught in ASSIMP_CATCH_GLOBAL_EXCEPTIONS by @mischmit in https://github.com/assimp/assimp/pull/5810
* Fix issue 5816 (cone.nff load failure): repair faulty line in 3D model file by @tellypresence in https://github.com/assimp/assimp/pull/5817
* Readme: Add project activity view item by @kimkulling in https://github.com/assimp/assimp/pull/5854
* Cleanup Unit Tests Output by @AMZN-Gene in https://github.com/assimp/assimp/pull/5852
* USD Skinned Mesh by @AMZN-Gene in https://github.com/assimp/assimp/pull/5812
* Update tinyusdz by @tellypresence in https://github.com/assimp/assimp/pull/5849
* +Add vertex duplication during face normal generation by @diiigle in https://github.com/assimp/assimp/pull/5805
* Fix use of uninitialized value. by @feuerste in https://github.com/assimp/assimp/pull/5867
* Update CMakeLists.txt to fix gcc/clang++ issue by @jwbla in https://github.com/assimp/assimp/pull/5863
* Add reference screenshots for complex bundled test 3D model files by @tellypresence in https://github.com/assimp/assimp/pull/5822
* Obj: Fix Sonarcube findings by @kimkulling in https://github.com/assimp/assimp/pull/5873
* Try to resolve image paths by replacing backslashes or forward slashes in EmbedTexturesProcess by @david-campos in https://github.com/assimp/assimp/pull/5844
* Material: Fix the build for c compiler by @kimkulling in https://github.com/assimp/assimp/pull/5879
* Material: Fix sonarcube finding by @kimkulling in https://github.com/assimp/assimp/pull/5880
* Remove strcpy. by @kimkulling in https://github.com/assimp/assimp/pull/5802
* Fix potential uninitialized variable in clipper by @miselin in https://github.com/assimp/assimp/pull/5881
* Check that mMaterials not null before access by @JulianKnodt in https://github.com/assimp/assimp/pull/5874
* Cleanup: Delete code/.editorconfig by @kimkulling in https://github.com/assimp/assimp/pull/5889
* Readme.md: Add sonarcube badge by @kimkulling in https://github.com/assimp/assimp/pull/5893
* Obj: fix nullptr access. by @kimkulling in https://github.com/assimp/assimp/pull/5894
* Update cpp-pm / hunter by @mosfet80 in https://github.com/assimp/assimp/pull/5885
* Add CI to automatically build and attach binaries to releases by @Saalvage in https://github.com/assimp/assimp/pull/5892
* Simplify JoinVerticesProcess by @JulianKnodt in https://github.com/assimp/assimp/pull/5895
* USD Keyframe Animations by @AMZN-Gene in https://github.com/assimp/assimp/pull/5856
* Fix compiler error when double precision is selected, by @hankarun in https://github.com/assimp/assimp/pull/5902
* Synchronize `DefaultLogger` by @Saalvage in https://github.com/assimp/assimp/pull/5898
* Do not create GLTF Mesh if no faces by @JulianKnodt in https://github.com/assimp/assimp/pull/5878
* FBX Blendshape: export float & same # verts by @JulianKnodt in https://github.com/assimp/assimp/pull/5775
* bugfix: Fixed the issue that draco compressed gltf files cannot be lo… by @HandsomeXi in https://github.com/assimp/assimp/pull/5883
* pbrt: Validate mesh in WriteMesh before AttributeBegin call by @lijenicol in https://github.com/assimp/assimp/pull/5884
* Introducing assimp Guru on Gurubase.io by @kursataktas in https://github.com/assimp/assimp/pull/5887
* Fix: Fix build for mingw10 by @kimkulling in https://github.com/assimp/assimp/pull/5916
* Fix use after free in the CallbackToLogRedirector by @tyler92 in https://github.com/assimp/assimp/pull/5918
* USD Mesh Node Fix by @AMZN-Gene in https://github.com/assimp/assimp/pull/5915
* Fixed warnings by @sacereda in https://github.com/assimp/assimp/pull/5903
* Replace C# port with maintained fork by @Saalvage in https://github.com/assimp/assimp/pull/5922
* Fix heap-buffer-overflow in OpenDDLParser by @tyler92 in https://github.com/assimp/assimp/pull/5919
* Fix parsing of comments at the end of lines for tokens with variable number of elements. (#5890) by @scschaefer in https://github.com/assimp/assimp/pull/5891
* Fix buffer overflow in MD5Parser::SkipSpacesAndLineEnd by @tyler92 in https://github.com/assimp/assimp/pull/5921
* Fix: Fix name collision by @kimkulling in https://github.com/assimp/assimp/pull/5937
* Bug/evaluate matrix4x4 access by @kimkulling in https://github.com/assimp/assimp/pull/5936
* glTF importers: Avoid strncpy truncating away the ' \0' character by @david-campos in https://github.com/assimp/assimp/pull/5931
* Export tangents in GLTF by @JulianKnodt in https://github.com/assimp/assimp/pull/5900
* Disable logs for fuzzer by default by @tyler92 in https://github.com/assimp/assimp/pull/5938
* Fix docs for aiImportFileExWithProperties to not talk about the importer keeping the Scene alive by @david-campos in https://github.com/assimp/assimp/pull/5925
* Fix stack overflow in LWS loader by @tyler92 in https://github.com/assimp/assimp/pull/5941
* Introduce VRML format (.wrl and .x3dv) 3D model support by @tellypresence in https://github.com/assimp/assimp/pull/5857
* Verify negative values in Quake1 MDL header by @tyler92 in https://github.com/assimp/assimp/pull/5940
* Fix heap buffer overflow in HMP loader by @tyler92 in https://github.com/assimp/assimp/pull/5939
* pragma warning bug fix when using g++ on windows by @stekap000 in https://github.com/assimp/assimp/pull/5943
* AssbinImporter::ReadInternFile now closes stream before throwing by @david-campos in https://github.com/assimp/assimp/pull/5927
* Updated Material.cpp to Add Missing Texture Types to String by @crazyjackel in https://github.com/assimp/assimp/pull/5945
* Docker: Optimize usage by @kimkulling in https://github.com/assimp/assimp/pull/5948
* Bugfix/cosmetic code cleanup by @kimkulling in https://github.com/assimp/assimp/pull/5947
* Add arm64-simulator support to iOS build script by @DwayneCoussement in https://github.com/assimp/assimp/pull/5920
* Add aiProcess_ValidateDataStructure flag to the fuzzer by @tyler92 in https://github.com/assimp/assimp/pull/5951
* Update OpenDDLParser.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5953
* [AMF] Fix texture mapping by @tellypresence in https://github.com/assimp/assimp/pull/5949
* [FBX] Allow export multi materials per node by @JulianKnodt in https://github.com/assimp/assimp/pull/5888
* Assimp master head fixes for failure to compile by @enginmanap in https://github.com/assimp/assimp/pull/5899
* Prefix MTL textures with the MTL directory path by @david-campos in https://github.com/assimp/assimp/pull/5928
* Add customExtension support to the scene by @BurntRanch in https://github.com/assimp/assimp/pull/5954
* Avoid exporting all primitives, which are not triangles. by @kimkulling in https://github.com/assimp/assimp/pull/5964
* Added GLTF Extension KHR_materials_anisotropy by @luho383 in https://github.com/assimp/assimp/pull/5950
* Add POST_BUILD option to ADD_CUSTOM_COMMAND by @MikeChemi in https://github.com/assimp/assimp/pull/5962
* Fix heap buffer overflow in PLY parser by @tyler92 in https://github.com/assimp/assimp/pull/5956
* Optimise building tinyusd library by @Pichas in https://github.com/assimp/assimp/pull/5959
* Add gltf metallic-roughness texture type by @tellypresence in https://github.com/assimp/assimp/pull/5968
* fix: reduce gltf2 export time by @1323236654 in https://github.com/assimp/assimp/pull/5972
* Flag Documentation Fix by @snave333 in https://github.com/assimp/assimp/pull/5978
* Doc: Make hint clearer by @kimkulling in https://github.com/assimp/assimp/pull/5988
* Clean STEPFileReader.cpp by @mosfet80 in https://github.com/assimp/assimp/pull/5973
* Update Readme.md: Add new viewer by @kimkulling in https://github.com/assimp/assimp/pull/5991
* Doc: Separate viewer by @kimkulling in https://github.com/assimp/assimp/pull/5995
* Use correct data type for animation key by @kimkulling in https://github.com/assimp/assimp/pull/5998
* Use ear-cutting library for triangulation by @Saalvage in https://github.com/assimp/assimp/pull/5977
* Fixing PyAssimp misalignment errors with certain structures by @fishguy6564 in https://github.com/assimp/assimp/pull/6001
* Bugfix/fix mingw issue 5975 by @kimkulling in https://github.com/assimp/assimp/pull/6005
* IFC: Remove redundand check by @kimkulling in https://github.com/assimp/assimp/pull/6006
* Obj: remove smooth-normals postprocessing by @kimkulling in https://github.com/assimp/assimp/pull/6031
* Refactorings: glTF cleanups by @kimkulling in https://github.com/assimp/assimp/pull/6028
* Fix memory leak in OpenGEXImporter by @UnionTech-Software in https://github.com/assimp/assimp/pull/6036
* Use std::copy to copy array and remove user destructor to make sure is_trivially_copyable in order to avoid -Wno-error=nontrivial-memcall by @cielavenir in https://github.com/assimp/assimp/pull/6029
* Fix: Let OpenGEX accept color3 types by @kimkulling in https://github.com/assimp/assimp/pull/6040
* ASE: Fix possible out of bound access. by @kimkulling in https://github.com/assimp/assimp/pull/6045
* MDL: Limit max texture sizes by @kimkulling in https://github.com/assimp/assimp/pull/6046
* MDL: Fix overflow check by @kimkulling in https://github.com/assimp/assimp/pull/6047
* Fix: Avoid override in line parsing by @kimkulling in https://github.com/assimp/assimp/pull/6048
* Bugfix: Fix possible nullptr dereferencing by @kimkulling in https://github.com/assimp/assimp/pull/6049
* Potential fix for code scanning alert no. 63: Potential use after free by @kimkulling in https://github.com/assimp/assimp/pull/6050
* ASE: Use correct vertex container by @kimkulling in https://github.com/assimp/assimp/pull/6051
* CMS: Fix possible overflow access by @kimkulling in https://github.com/assimp/assimp/pull/6052
* [OpenGEX] disable partial implementation of light import (causes model load failure) by @tellypresence in https://github.com/assimp/assimp/pull/6044
* Update tinyusdz git hash (fix USD animation) by @tellypresence in https://github.com/assimp/assimp/pull/6034
* [draft] Check the hunter build by @kimkulling in https://github.com/assimp/assimp/pull/6061
* NDO: Fix possible overflow access by @yuntongzhang in https://github.com/assimp/assimp/pull/6055
* Fix Cinema4D Import by @krishty in https://github.com/assimp/assimp/pull/6062
* Remove Redundant `virtual` by @krishty in https://github.com/assimp/assimp/pull/6064
* feat: created the aiGetStringC_Str() function. by @leliel-git in https://github.com/assimp/assimp/pull/6059
* Fix Whitespace by @krishty in https://github.com/assimp/assimp/pull/6063
* Harmonize Importer #includes by @krishty in https://github.com/assimp/assimp/pull/6065
* More `constexpr` by @krishty in https://github.com/assimp/assimp/pull/6066
* Renamed and inlined hasSkeletons() to HasSkeletons() for API consistency by @Alexelnet in https://github.com/assimp/assimp/pull/6072
* Fix set by @kimkulling in https://github.com/assimp/assimp/pull/6073
* Bugfix/ensure collada parsing works issue 1488 by @kimkulling in https://github.com/assimp/assimp/pull/6087
* Not to export empty "LayerElementNormal" or "LayerElementColor" nodes to fbx by @Riv1s-sSsA01 in https://github.com/assimp/assimp/pull/6092
* Use unique pointer to fix possible leak by @kimkulling in https://github.com/assimp/assimp/pull/6104
* Refactoring of PR #6092 by @Riv1s-sSsA01 in https://github.com/assimp/assimp/pull/6101
* fix: Fix build on armv6/armv7 by @yurivict in https://github.com/assimp/assimp/pull/6123
* Bugfix: Handling no of texture coordinates correctly by @kimkulling in https://github.com/assimp/assimp/pull/6124
* fix: possible Heap-based Buffer Overflow in ConvertToUTF8 function by @TinyTinni in https://github.com/assimp/assimp/pull/6122
* Refactor by @kimkulling in https://github.com/assimp/assimp/pull/6127
* support for cmake findpackage module mode by @kiroeko in https://github.com/assimp/assimp/pull/6121
* Replace exception by error in log by @kimkulling in https://github.com/assimp/assimp/pull/6133
* Fix a out of bound buffer access in ParsingUtils GetNextLine by @qingyouzhao in https://github.com/assimp/assimp/pull/6134
* Fix a bug where string erases throws out of range by @qingyouzhao in https://github.com/assimp/assimp/pull/6135
* Fix: Support uint16 indices in OpenGEX as well by @kimkulling in https://github.com/assimp/assimp/pull/6137
* Fix crashes by @kimkulling in https://github.com/assimp/assimp/pull/6138
* + Only the recognition of KTX2 compressed images as texture objects within the glb model is currently handled. by @copycd in https://github.com/assimp/assimp/pull/6139
* add missing constants by @daef in https://github.com/assimp/assimp/pull/6116
* Fix warning abut inexistent warning by @limdor in https://github.com/assimp/assimp/pull/6153
* Fix: Fix leak when sortbyp failes with exception by @kimkulling in https://github.com/assimp/assimp/pull/6166
* Update contrib/zip to fix data loss warning by @limdor in https://github.com/assimp/assimp/pull/6152
* Fix out-of-bounds dereferencing by @Marti2203 in https://github.com/assimp/assimp/pull/6150
* [#5983] Fix bugs introduced in fbx export by @JulianKnodt in https://github.com/assimp/assimp/pull/6000
* Doc: add C++ / c minimum by @kimkulling in https://github.com/assimp/assimp/pull/6187
* Unreal refactorings by @kimkulling in https://github.com/assimp/assimp/pull/6182
* update draco lib by @mosfet80 in https://github.com/assimp/assimp/pull/6094
* fix: missing OS separator in outfile by @Latios96 in https://github.com/assimp/assimp/pull/6098
* Add Missing Strings to aiTextureTypeToString by @crasong in https://github.com/assimp/assimp/pull/6188
* Fix issue compiling when assimp added as subdirectory by @plemanski in https://github.com/assimp/assimp/pull/6186
* Add clamping logic for to_ktime by @Marti2203 in https://github.com/assimp/assimp/pull/6149
* Add explicit "fallthrough" to switch by @tellypresence in https://github.com/assimp/assimp/pull/6143
* Fix HUNTER_ERROR_PAGE by @deccer in https://github.com/assimp/assimp/pull/6200
* Fix a bug in importing binary PLY file (#1) by @yurik42 in https://github.com/assimp/assimp/pull/6060
* Fix export fbx PolygonVertexIndex by @Riv1s-sSsA01 in https://github.com/assimp/assimp/pull/6102
* fix: closes #6069 CVE-2025-3196 by @VinzSpring in https://github.com/assimp/assimp/pull/6154
* Fix: Add "preservePivots" condition when importing FBX animation by @Nor-s in https://github.com/assimp/assimp/pull/6115

## New Contributors
* @Begasus made their first contribution in https://github.com/assimp/assimp/pull/5255
* @ockeymm made their first contribution in https://github.com/assimp/assimp/pull/5252
* @fvbj made their first contribution in https://github.com/assimp/assimp/pull/5243
* @JulianKnodt made their first contribution in https://github.com/assimp/assimp/pull/5295
* @sloriot made their first contribution in https://github.com/assimp/assimp/pull/5270
* @Ipomoea made their first contribution in https://github.com/assimp/assimp/pull/5271
* @aumuell made their first contribution in https://github.com/assimp/assimp/pull/5278
* @TarcioV made their first contribution in https://github.com/assimp/assimp/pull/5279
* @copycd made their first contribution in https://github.com/assimp/assimp/pull/5294
* @cuppajoeman made their first contribution in https://github.com/assimp/assimp/pull/5312
* @ttxine made their first contribution in https://github.com/assimp/assimp/pull/5322
* @Futuremappermydud made their first contribution in https://github.com/assimp/assimp/pull/5356
* @MarkaRagnos0815 made their first contribution in https://github.com/assimp/assimp/pull/5359
* @0xf0ad made their first contribution in https://github.com/assimp/assimp/pull/5376
* @seanth made their first contribution in https://github.com/assimp/assimp/pull/5426
* @tigert1998 made their first contribution in https://github.com/assimp/assimp/pull/5436
* @GalenXiao made their first contribution in https://github.com/assimp/assimp/pull/5361
* @Th3T3chn0G1t made their first contribution in https://github.com/assimp/assimp/pull/5397
* @etam made their first contribution in https://github.com/assimp/assimp/pull/5462
* @adfwer233 made their first contribution in https://github.com/assimp/assimp/pull/5480
* @LukasBanana made their first contribution in https://github.com/assimp/assimp/pull/5490
* @thenanisore made their first contribution in https://github.com/assimp/assimp/pull/5525
* @RoboSchmied made their first contribution in https://github.com/assimp/assimp/pull/5518
* @AlexTMjugador made their first contribution in https://github.com/assimp/assimp/pull/5516
* @tomheaton made their first contribution in https://github.com/assimp/assimp/pull/5507
* @alexrp made their first contribution in https://github.com/assimp/assimp/pull/5535
* @ZeunO8 made their first contribution in https://github.com/assimp/assimp/pull/5545
* @Succ3s made their first contribution in https://github.com/assimp/assimp/pull/5550
* @johannesugb made their first contribution in https://github.com/assimp/assimp/pull/5542
* @xiaoxiaopifu made their first contribution in https://github.com/assimp/assimp/pull/5561
* @uyjulian made their first contribution in https://github.com/assimp/assimp/pull/5569
* @dbs4261 made their first contribution in https://github.com/assimp/assimp/pull/5577
* @vengine made their first contribution in https://github.com/assimp/assimp/pull/5575
* @BradlyLanducci made their first contribution in https://github.com/assimp/assimp/pull/5600
* @micott made their first contribution in https://github.com/assimp/assimp/pull/5608
* @ycn2022 made their first contribution in https://github.com/assimp/assimp/pull/5624
* @ThatOSDev made their first contribution in https://github.com/assimp/assimp/pull/5639
* @mapret made their first contribution in https://github.com/assimp/assimp/pull/5640
* @thearchivalone made their first contribution in https://github.com/assimp/assimp/pull/5650
* @sgayda2 made their first contribution in https://github.com/assimp/assimp/pull/5658
* @JensEhrhardt-eOPUS made their first contribution in https://github.com/assimp/assimp/pull/5432
* @Kimbatt made their first contribution in https://github.com/assimp/assimp/pull/5695
* @natevm made their first contribution in https://github.com/assimp/assimp/pull/5713
* @mkuritsu made their first contribution in https://github.com/assimp/assimp/pull/5693
* @Sanchikuuus made their first contribution in https://github.com/assimp/assimp/pull/5101
* @imdongye made their first contribution in https://github.com/assimp/assimp/pull/5728
* @ochafik made their first contribution in https://github.com/assimp/assimp/pull/5681
* @fuhaixi made their first contribution in https://github.com/assimp/assimp/pull/5675
* @AKosmachyov made their first contribution in https://github.com/assimp/assimp/pull/5648
* @Fav made their first contribution in https://github.com/assimp/assimp/pull/5647
* @cla7aye15I4nd made their first contribution in https://github.com/assimp/assimp/pull/5765
* @lxw404 made their first contribution in https://github.com/assimp/assimp/pull/5751
* @PatrickDahlin made their first contribution in https://github.com/assimp/assimp/pull/5780
* @Pichas made their first contribution in https://github.com/assimp/assimp/pull/5774
* @chefrolle695 made their first contribution in https://github.com/assimp/assimp/pull/5826
* @qingyouzhao made their first contribution in https://github.com/assimp/assimp/pull/5801
* @mischmit made their first contribution in https://github.com/assimp/assimp/pull/5810
* @AMZN-Gene made their first contribution in https://github.com/assimp/assimp/pull/5852
* @jwbla made their first contribution in https://github.com/assimp/assimp/pull/5863
* @david-campos made their first contribution in https://github.com/assimp/assimp/pull/5844
* @miselin made their first contribution in https://github.com/assimp/assimp/pull/5881
* @hankarun made their first contribution in https://github.com/assimp/assimp/pull/5902
* @HandsomeXi made their first contribution in https://github.com/assimp/assimp/pull/5883
* @lijenicol made their first contribution in https://github.com/assimp/assimp/pull/5884
* @kursataktas made their first contribution in https://github.com/assimp/assimp/pull/5887
* @tyler92 made their first contribution in https://github.com/assimp/assimp/pull/5918
* @scschaefer made their first contribution in https://github.com/assimp/assimp/pull/5891
* @stekap000 made their first contribution in https://github.com/assimp/assimp/pull/5943
* @crazyjackel made their first contribution in https://github.com/assimp/assimp/pull/5945
* @DwayneCoussement made their first contribution in https://github.com/assimp/assimp/pull/5920
* @BurntRanch made their first contribution in https://github.com/assimp/assimp/pull/5954
* @MikeChemi made their first contribution in https://github.com/assimp/assimp/pull/5962
* @1323236654 made their first contribution in https://github.com/assimp/assimp/pull/5972
* @snave333 made their first contribution in https://github.com/assimp/assimp/pull/5978
* @fishguy6564 made their first contribution in https://github.com/assimp/assimp/pull/6001
* @UnionTech-Software made their first contribution in https://github.com/assimp/assimp/pull/6036
* @cielavenir made their first contribution in https://github.com/assimp/assimp/pull/6029
* @yuntongzhang made their first contribution in https://github.com/assimp/assimp/pull/6055
* @leliel-git made their first contribution in https://github.com/assimp/assimp/pull/6059
* @Alexelnet made their first contribution in https://github.com/assimp/assimp/pull/6072
* @yurivict made their first contribution in https://github.com/assimp/assimp/pull/6123
* @kiroeko made their first contribution in https://github.com/assimp/assimp/pull/6121
* @limdor made their first contribution in https://github.com/assimp/assimp/pull/6153
* @Marti2203 made their first contribution in https://github.com/assimp/assimp/pull/6150
* @Latios96 made their first contribution in https://github.com/assimp/assimp/pull/6098
* @crasong made their first contribution in https://github.com/assimp/assimp/pull/6188
* @plemanski made their first contribution in https://github.com/assimp/assimp/pull/6186
* @deccer made their first contribution in https://github.com/assimp/assimp/pull/6200
* @yurik42 made their first contribution in https://github.com/assimp/assimp/pull/6060
* @VinzSpring made their first contribution in https://github.com/assimp/assimp/pull/6154

**Full Changelog**: https://github.com/assimp/assimp/compare/v5.3.1...v6.0.0

# 5.4.3
## What's Changed
* Fix building on Haiku by @Begasus in https://github.com/assimp/assimp/pull/5255
* Reduce memory consumption in JoinVerticesProcess::ProcessMesh() signi… by @ockeymm in https://github.com/assimp/assimp/pull/5252
* Fix: Add check for invalid input argument by @kimkulling in https://github.com/assimp/assimp/pull/5258
* Replace an assert by an error log. by @kimkulling in https://github.com/assimp/assimp/pull/5260
* Extension of skinning data export to GLB/GLTF format by @fvbj in https://github.com/assimp/assimp/pull/5243
* Fix output floating-point values to fbx by @Riv1s-sSsA01 in https://github.com/assimp/assimp/pull/5265
* Update ImproveCacheLocality.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5268
* Update Readme.md by @kimkulling in https://github.com/assimp/assimp/pull/5277
* Deep arsdk bone double free by @kimkulling in https://github.com/assimp/assimp/pull/5291
* Fix Spelling error by @JulianKnodt in https://github.com/assimp/assimp/pull/5295
* Use size in order to be compatible with float and double by @sloriot in https://github.com/assimp/assimp/pull/5270
* Fix: Add missing transformation for normalized normals. by @kimkulling in https://github.com/assimp/assimp/pull/5301
* Fix: Implicit Conversion Error by @Ipomoea in https://github.com/assimp/assimp/pull/5271
* Fix add checks for indices by @kimkulling in https://github.com/assimp/assimp/pull/5306
* Update FBXBinaryTokenizer.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5311
* link to external minizip with full path by @aumuell in https://github.com/assimp/assimp/pull/5278
* utf8 header not found by @TarcioV in https://github.com/assimp/assimp/pull/5279
* Rm unnecessary deg->radian conversion in FBX exporter by @JulianKnodt in https://github.com/assimp/assimp/pull/5281
* Fix empty mesh handling by @kimkulling in https://github.com/assimp/assimp/pull/5318
* Refactoring: Some cleanups by @kimkulling in https://github.com/assimp/assimp/pull/5319
* Fix invalid read of `uint` from `uvwsrc` by @JulianKnodt in https://github.com/assimp/assimp/pull/5282
* Remove double delete by @kimkulling in https://github.com/assimp/assimp/pull/5325
* Fix mesh-name error. by @copycd in https://github.com/assimp/assimp/pull/5294
* COLLADA fixes for textures in C4D input by @wmatyjewicz in https://github.com/assimp/assimp/pull/5293
* Use the correct allocator for deleting objects in case of duplicate a… by @FlorianBorn71 in https://github.com/assimp/assimp/pull/5305
* Fix container overflow in MMD parser by @aavenel in https://github.com/assimp/assimp/pull/5309
* Fix: PLY heap buffer overflow by @aavenel in https://github.com/assimp/assimp/pull/5310
* Update Readme.md by @cuppajoeman in https://github.com/assimp/assimp/pull/5312
* Update Readme.md by @cuppajoeman in https://github.com/assimp/assimp/pull/5313
* Fix: Check if index for mesh access is out of range by @kimkulling in https://github.com/assimp/assimp/pull/5338
* Update FBXConverter.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5349
* FBX: Use correct time scaling by @kimkulling in https://github.com/assimp/assimp/pull/5355
* Drop explicit inclusion of contrib/ headers by @umlaeute in https://github.com/assimp/assimp/pull/5316
* Update Build.md by @cuppajoeman in https://github.com/assimp/assimp/pull/5314
* Fix buffer overflow in FBX::Util::DecodeBase64() by @ttxine in https://github.com/assimp/assimp/pull/5322
* Readme.md:  correct 2 errors in section headers by @stephengold in https://github.com/assimp/assimp/pull/5351
* Fix double free in Video::~Video() by @ttxine in https://github.com/assimp/assimp/pull/5323
* FBXMeshGeometry:  solve issue #5116 using patch provided by darktjm by @stephengold in https://github.com/assimp/assimp/pull/5333
* Fix target names not being imported on some gLTF2 models by @Futuremappermydud in https://github.com/assimp/assimp/pull/5356
* correct grammar/typographic errors in comments (8 files) by @stephengold in https://github.com/assimp/assimp/pull/5343
* KHR_materials_specular fixes by @rudybear in https://github.com/assimp/assimp/pull/5347
* Disable Hunter by @kimkulling in https://github.com/assimp/assimp/pull/5388
* fixed several issues by @MarkaRagnos0815 in https://github.com/assimp/assimp/pull/5359
* Fix leak by @kimkulling in https://github.com/assimp/assimp/pull/5391
* Check validity of archive without parsing by @kimkulling in https://github.com/assimp/assimp/pull/5393
* Fix integer overflow by @kimkulling in https://github.com/assimp/assimp/pull/5394
* Add a test before generating the txture folder by @kimkulling in https://github.com/assimp/assimp/pull/5400
* Build: Disable building zlib for non-windows by @kimkulling in https://github.com/assimp/assimp/pull/5401
* null check. by @copycd in https://github.com/assimp/assimp/pull/5402
* Bump actions/upload-artifact from 3 to 4 by @dependabot in https://github.com/assimp/assimp/pull/5384
* fix: KHR_materials_pbrSpecularGlossiness/diffuseFactor convert to pbr… by @guguTang in https://github.com/assimp/assimp/pull/5410
* fix building errors for MinGW by @0xf0ad in https://github.com/assimp/assimp/pull/5376
* dynamic_cast error. by @copycd in https://github.com/assimp/assimp/pull/5406
* Add missing IRR textures by @tellypresence in https://github.com/assimp/assimp/pull/5374
* Update Dockerfile by @kimkulling in https://github.com/assimp/assimp/pull/5412
* Fix handling of X3D IndexedLineSet nodes by @andre-schulz in https://github.com/assimp/assimp/pull/5362
* Improve acc file loading by @IOBYTE in https://github.com/assimp/assimp/pull/5360
* Readme.md:  present hyperlinks in a more uniform style by @stephengold in https://github.com/assimp/assimp/pull/5364
* FBX Blendshape `FullWeight: Vec<Float>` -> `FullWeight: Vec<Double>` by @JulianKnodt in https://github.com/assimp/assimp/pull/5441
* Fix for issues #5422, #3411, and #5443 -- DXF insert scaling fix and colour fix by @seanth in https://github.com/assimp/assimp/pull/5426
* Update StbCommon.h to stay up-to-date with stb_image.h. by @tigert1998 in https://github.com/assimp/assimp/pull/5436
* Introduce aiBuffer by @kimkulling in https://github.com/assimp/assimp/pull/5444
* Add bounds checks to the parsing utilities. by @kimkulling in https://github.com/assimp/assimp/pull/5421
* Fix crash in viewer by @kimkulling in https://github.com/assimp/assimp/pull/5446
* Static code analysis fixes by @kimkulling in https://github.com/assimp/assimp/pull/5447
* Kimkulling/fix behavior of remove redundant mats issue 5438 by @kimkulling in https://github.com/assimp/assimp/pull/5451
* Fix X importer breakage introduced in commit f844c33  by @tellypresence in https://github.com/assimp/assimp/pull/5372
* Fileformats.md:  clarify that import of .blend files is deprecated by @stephengold in https://github.com/assimp/assimp/pull/5350
* feat:1.add 3mf vertex color read 2.fix 3mf read texture bug by @GalenXiao in https://github.com/assimp/assimp/pull/5361
* More GLTF loading hardening by @FlorianBorn71 in https://github.com/assimp/assimp/pull/5415
* Bump actions/cache from 3 to 4 by @dependabot in https://github.com/assimp/assimp/pull/5431
* Update CMakeLists.txt by @mosfet80 in https://github.com/assimp/assimp/pull/5379
* `Blendshape`->`Geometry` in FBX Export by @JulianKnodt in https://github.com/assimp/assimp/pull/5419
* Fix identity matrix check by @fvbj in https://github.com/assimp/assimp/pull/5445
* Fix PyAssimp under Python >= 3.12 and macOS library search support by @Th3T3chn0G1t in https://github.com/assimp/assimp/pull/5397
* Add ISC LICENSE file by @severin-lemaignan in https://github.com/assimp/assimp/pull/5465
* ColladaParser: check values length by @etam in https://github.com/assimp/assimp/pull/5462
* Include defs in not cpp-section by @kimkulling in https://github.com/assimp/assimp/pull/5466
* Add correct double zero check by @kimkulling in https://github.com/assimp/assimp/pull/5471
* Add zlib-header to ZipArchiveIOSystem.h by @kimkulling in https://github.com/assimp/assimp/pull/5473
* Add 2024 to copyright infos by @kimkulling in https://github.com/assimp/assimp/pull/5475
* Append a new setting "AI_CONFIG_EXPORT_FBX_TRANSPARENCY_FACTOR_REFER_TO_OPACITY" by @Riv1s-sSsA01 in https://github.com/assimp/assimp/pull/5450
* Eliminate non-ascii comments in clipper by @adfwer233 in https://github.com/assimp/assimp/pull/5480
* Fix compilation for MSVC14. by @LukasBanana in https://github.com/assimp/assimp/pull/5490
* Add correction of fbx model rotation by @kimkulling in https://github.com/assimp/assimp/pull/5494
* Delete tools/make directory by @mosfet80 in https://github.com/assimp/assimp/pull/5491
* Delete packaging/windows-mkzip directory by @mosfet80 in https://github.com/assimp/assimp/pull/5492
* Fix #5420 duplicate degrees to radians conversion in fbx importer by @Biohazard90 in https://github.com/assimp/assimp/pull/5427
* Respect merge identical vertices in ObjExporter by @JulianKnodt in https://github.com/assimp/assimp/pull/5521
* Fix utDefaultIOStream test under MinGW by @thenanisore in https://github.com/assimp/assimp/pull/5525
* Fix typos by @RoboSchmied in https://github.com/assimp/assimp/pull/5518
* Add initial macOS support to C4D importer by @AlexTMjugador in https://github.com/assimp/assimp/pull/5516
* Update hunter into CMakeLists.txt by @mosfet80 in https://github.com/assimp/assimp/pull/5505
* Fix: add a missing import for `AI_CONFIG_CHECK_IDENTITY_MATRIX_EPSILON_DEFAULT` by @tomheaton in https://github.com/assimp/assimp/pull/5507
* updated json by @mosfet80 in https://github.com/assimp/assimp/pull/5501
* Cleanup: Fix review findings by @kimkulling in https://github.com/assimp/assimp/pull/5528
* Update CMakeLists.txt by @kimkulling in https://github.com/assimp/assimp/pull/5531
* CMake: Allow linking draco statically if ASSIMP_BUILD_DRACO_STATIC is set. by @alexrp in https://github.com/assimp/assimp/pull/5535
* updated minizip to last version by @mosfet80 in https://github.com/assimp/assimp/pull/5498
* updated STBIMAGElib by @mosfet80 in https://github.com/assimp/assimp/pull/5500
* fix issue #5461 (segfault after removing redundant materials) by @stephengold in https://github.com/assimp/assimp/pull/5467
* Update ComputeUVMappingProcess.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5541
* add some ASSIMP_INSTALL checks by @ZeunO8 in https://github.com/assimp/assimp/pull/5545
* Fix SplitByBoneCount typo that prevented node updates by @Succ3s in https://github.com/assimp/assimp/pull/5550
* Q3DLoader: Fix possible material string overflow by @kimkulling in https://github.com/assimp/assimp/pull/5556
* Reverts the changes introduced by commit ad766cb in February 2022. by @johannesugb in https://github.com/assimp/assimp/pull/5542
* fix a collada import bug by @xiaoxiaopifu in https://github.com/assimp/assimp/pull/5561
* mention IQM loader in Fileformats.md by @Garux in https://github.com/assimp/assimp/pull/5560
* Kimkulling/fix pyassimp compatibility by @kimkulling in https://github.com/assimp/assimp/pull/5563
* fix ASE loader crash when *MATERIAL_COUNT or *NUMSUBMTLS is not specified or is 0 by @Garux in https://github.com/assimp/assimp/pull/5559
* Add checks for invalid buffer and size by @kimkulling in https://github.com/assimp/assimp/pull/5570
* Make sure for release revision will be zero by @kimkulling in https://github.com/assimp/assimp/pull/5571
* glTF2Importer: Support .vrm extension by @uyjulian in https://github.com/assimp/assimp/pull/5569
* Prepare v5.4.1 by @kimkulling in https://github.com/assimp/assimp/pull/5573
* Remove deprecated c++11 warnings by @kimkulling in https://github.com/assimp/assimp/pull/5576
* fix ci by disabling tests by @kimkulling in https://github.com/assimp/assimp/pull/5583
* Fix integer overflow by @kimkulling in https://github.com/assimp/assimp/pull/5581
* Assimp viewer fixes by @JLouis-B in https://github.com/assimp/assimp/pull/5582
* Optimize readability by @kimkulling in https://github.com/assimp/assimp/pull/5578
* Temporary fix for #5557 GCC 13+ build issue -Warray-bounds by @dbs4261 in https://github.com/assimp/assimp/pull/5577
* Fix a bug that could cause an assertion failure. by @vengine in https://github.com/assimp/assimp/pull/5575
* Fix possible nullptr dereferencing. by @kimkulling in https://github.com/assimp/assimp/pull/5595
* Update ObjFileParser.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5598
* Fix for #5592 Disabled maybe-uninitialized error for AssetLib/Obj/ObjFileParser.cpp by @dbs4261 in https://github.com/assimp/assimp/pull/5593
* updated zip by @mosfet80 in https://github.com/assimp/assimp/pull/5499
* Postprocessing: Fix endless loop by @kimkulling in https://github.com/assimp/assimp/pull/5605
* Build: Fix compilation for VS-2022 debug mode - warning by @kimkulling in https://github.com/assimp/assimp/pull/5606
* Converted a size_t to mz_uint that was being treated as an error by @BradlyLanducci in https://github.com/assimp/assimp/pull/5600
* Add trim to xml string parsing by @kimkulling in https://github.com/assimp/assimp/pull/5611
* Replace duplicated trim by @kimkulling in https://github.com/assimp/assimp/pull/5613
* Move aiScene constructor by @kimkulling in https://github.com/assimp/assimp/pull/5614
* Move revision.h and revision.h.in to include folder by @kimkulling in https://github.com/assimp/assimp/pull/5615
* Update MDLMaterialLoader.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5620
* Create inno_setup by @kimkulling in https://github.com/assimp/assimp/pull/5621
* clean HunterGate.cmake by @mosfet80 in https://github.com/assimp/assimp/pull/5619
* Draft: Update init of aiString by @kimkulling in https://github.com/assimp/assimp/pull/5623
* Fix init aiString issue 5622 in python module by @kimkulling in https://github.com/assimp/assimp/pull/5625
* update dotnet example by @mosfet80 in https://github.com/assimp/assimp/pull/5618
* Make step file schema validation more robust. by @kimkulling in https://github.com/assimp/assimp/pull/5627
* fix PLY binary export color from float to uchar by @michaelsctts in https://github.com/assimp/assimp/pull/5608
* Some FBXs do not have "Materials" information, which can cause parsing errors by @ycn2022 in https://github.com/assimp/assimp/pull/5624
* Fix collada uv channels - temporary was stored and then updated. by @StepanHrbek in https://github.com/assimp/assimp/pull/5630
* remove ASE parsing break by @Garux in https://github.com/assimp/assimp/pull/5634
* FBX-Exporter: Fix nullptr dereferencing by @kimkulling in https://github.com/assimp/assimp/pull/5638
* Fix FBX exporting incorrect bone order by @JulianKnodt in https://github.com/assimp/assimp/pull/5435
* fixes potential memory leak on malformed obj file by @TinyTinni in https://github.com/assimp/assimp/pull/5645
* Update zip.c by @ThatOSDev in https://github.com/assimp/assimp/pull/5639
* Fixes some uninit bool loads by @TinyTinni in https://github.com/assimp/assimp/pull/5644
* Fix names of enum values in docstring of aiProcess_FindDegenerates by @mapret in https://github.com/assimp/assimp/pull/5640
* Fix: StackAllocator Undefined Reference fix by @bedwardly-down in https://github.com/assimp/assimp/pull/5650
* Plx: Fix out-of-bound access by @kimkulling in https://github.com/assimp/assimp/pull/5651
* Docker: Fix security finding by @kimkulling in https://github.com/assimp/assimp/pull/5655
* Fix potential heapbuffer overflow in md5 parsing by @TinyTinni in https://github.com/assimp/assimp/pull/5652
* Replace raw pointers by std::string by @kimkulling in https://github.com/assimp/assimp/pull/5656
* Fix compile warning by @kimkulling in https://github.com/assimp/assimp/pull/5657
* Allow empty slots in mTextureCoords by @StepanHrbek in https://github.com/assimp/assimp/pull/5636
* Update Readme.md by @kimkulling in https://github.com/assimp/assimp/pull/5663
* Update Readme.md by @kimkulling in https://github.com/assimp/assimp/pull/5665
* [USD] Integrate "tinyusdz" project by @tellypresence in https://github.com/assimp/assimp/pull/5628
* Kimkulling/fix double precision tests by @kimkulling in https://github.com/assimp/assimp/pull/5660
* Update Python structs with missing fields that were causing core dumps by @vjf in https://github.com/assimp/assimp/pull/5673
* Introduce interpolation mode to vector and quaternion keys by @kimkulling in https://github.com/assimp/assimp/pull/5674
* Fix a fuzz test heap buffer overflow in mdl material loader by @sgayda2 in https://github.com/assimp/assimp/pull/5658
* Mosfet80 updatedpoli2tri by @kimkulling in https://github.com/assimp/assimp/pull/5682
* CalcTangents: zero vector is invalid for tangent/bitangent by @JensEhrhardt-eOPUS in https://github.com/assimp/assimp/pull/5432
* A fuzzed stride could cause the max count to become negative and henc… by @FlorianBorn71 in https://github.com/assimp/assimp/pull/5414
* Return false instead of a crash by @kimkulling in https://github.com/assimp/assimp/pull/5685
* Make coord transfor for hs1 files optional by @kimkulling in https://github.com/assimp/assimp/pull/5687
* Update DefaultIOSystem.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5697
* FBX exporter - handle multiple vertex color channels by @Kimbatt in https://github.com/assimp/assimp/pull/5695
* Fixing static builds on Windows by @natevm in https://github.com/assimp/assimp/pull/5713
* Added AND condition in poly2tri dll_symbol.h to only define macros fo… by @mkuritsu in https://github.com/assimp/assimp/pull/5693
* Fix MSVC PDBs and permit them to be disabled if required by @RichardTea in https://github.com/assimp/assimp/pull/5710
* Use DRACO_GLTF_BITSTREAM by @RichardTea in https://github.com/assimp/assimp/pull/5709
* include Exceptional.h in 3DSExporter.cpp by @Fiskmans in https://github.com/assimp/assimp/pull/5707
* Remove recursive include by @Fiskmans in https://github.com/assimp/assimp/pull/5705
* Fix: Possible out-of-bound read in findDegenerate by @TinyTinni in https://github.com/assimp/assimp/pull/5679
* Revert variable name by @tellypresence in https://github.com/assimp/assimp/pull/5715
* Add compile option /source-charset:utf-8 for MSVC by @kenichiice in https://github.com/assimp/assimp/pull/5716
* Fix leak in loader by @kimkulling in https://github.com/assimp/assimp/pull/5718
* Expose aiGetEmbeddedTexture to C-API by @sacereda in https://github.com/assimp/assimp/pull/5382
* Sparky kitty studios master by @kimkulling in https://github.com/assimp/assimp/pull/5727
* Added more Maya materials by @Sanchikuuus in https://github.com/assimp/assimp/pull/5101
* Fix to check both types of slashes in GetShortFilename by @imdongye in https://github.com/assimp/assimp/pull/5728
* Bump actions/download-artifact from 1 to 4 by @dependabot in https://github.com/assimp/assimp/pull/5732
* Bump actions/upload-artifact from 1 to 4 by @dependabot in https://github.com/assimp/assimp/pull/5731
* Bump softprops/action-gh-release from 1 to 2 by @dependabot in https://github.com/assimp/assimp/pull/5730
* Fix copying private data when the source pointer is NULL by @vjf in https://github.com/assimp/assimp/pull/5733
* Fix potential memory leak in SceneCombiner for LWS/IRR/MD3 loader by @TinyTinni in https://github.com/assimp/assimp/pull/5721
* Fix to correctly determine 'multi-configuration' on Windows by @kenichiice in https://github.com/assimp/assimp/pull/5720
* Fix casting typo in D3MFExporter::writeBaseMaterials by @ochafik in https://github.com/assimp/assimp/pull/5681
* FBX: add metadata of aiNode as properties by @fuhaixi in https://github.com/assimp/assimp/pull/5675
* feat: add an option for creating XCFramework and configure minimum iOS target by @AKosmachyov in https://github.com/assimp/assimp/pull/5648
* Update PyAssimp structs with Skeleton & SkeletonBone members by @vjf in https://github.com/assimp/assimp/pull/5734
* The total length is incorrect when exporting gltf2 by @Fav in https://github.com/assimp/assimp/pull/5647
* `build`: Add ccache support by @ochafik in https://github.com/assimp/assimp/pull/5686
* Update ccpp.yml by @kimkulling in https://github.com/assimp/assimp/pull/5740
* Ply-Importer: Fix vulnerability by @kimkulling in https://github.com/assimp/assimp/pull/5739
* prepare v5.4.3 by @kimkulling in https://github.com/assimp/assimp/pull/5741

## New Contributors
* @Begasus made their first contribution in https://github.com/assimp/assimp/pull/5255
* @ockeymm made their first contribution in https://github.com/assimp/assimp/pull/5252
* @fvbj made their first contribution in https://github.com/assimp/assimp/pull/5243
* @JulianKnodt made their first contribution in https://github.com/assimp/assimp/pull/5295
* @sloriot made their first contribution in https://github.com/assimp/assimp/pull/5270
* @Ipomoea made their first contribution in https://github.com/assimp/assimp/pull/5271
* @aumuell made their first contribution in https://github.com/assimp/assimp/pull/5278
* @TarcioV made their first contribution in https://github.com/assimp/assimp/pull/5279
* @copycd made their first contribution in https://github.com/assimp/assimp/pull/5294
* @cuppajoeman made their first contribution in https://github.com/assimp/assimp/pull/5312
* @ttxine made their first contribution in https://github.com/assimp/assimp/pull/5322
* @Futuremappermydud made their first contribution in https://github.com/assimp/assimp/pull/5356
* @MarkaRagnos0815 made their first contribution in https://github.com/assimp/assimp/pull/5359
* @0xf0ad made their first contribution in https://github.com/assimp/assimp/pull/5376
* @seanth made their first contribution in https://github.com/assimp/assimp/pull/5426
* @tigert1998 made their first contribution in https://github.com/assimp/assimp/pull/5436
* @GalenXiao made their first contribution in https://github.com/assimp/assimp/pull/5361
* @Th3T3chn0G1t made their first contribution in https://github.com/assimp/assimp/pull/5397
* @etam made their first contribution in https://github.com/assimp/assimp/pull/5462
* @adfwer233 made their first contribution in https://github.com/assimp/assimp/pull/5480
* @LukasBanana made their first contribution in https://github.com/assimp/assimp/pull/5490
* @thenanisore made their first contribution in https://github.com/assimp/assimp/pull/5525
* @RoboSchmied made their first contribution in https://github.com/assimp/assimp/pull/5518
* @AlexTMjugador made their first contribution in https://github.com/assimp/assimp/pull/5516
* @tomheaton made their first contribution in https://github.com/assimp/assimp/pull/5507
* @alexrp made their first contribution in https://github.com/assimp/assimp/pull/5535
* @ZeunO8 made their first contribution in https://github.com/assimp/assimp/pull/5545
* @Succ3s made their first contribution in https://github.com/assimp/assimp/pull/5550
* @johannesugb made their first contribution in https://github.com/assimp/assimp/pull/5542
* @xiaoxiaopifu made their first contribution in https://github.com/assimp/assimp/pull/5561
* @uyjulian made their first contribution in https://github.com/assimp/assimp/pull/5569
* @dbs4261 made their first contribution in https://github.com/assimp/assimp/pull/5577
* @vengine made their first contribution in https://github.com/assimp/assimp/pull/5575
* @BradlyLanducci made their first contribution in https://github.com/assimp/assimp/pull/5600
* @michaelsctts made their first contribution in https://github.com/assimp/assimp/pull/5608
* @ycn2022 made their first contribution in https://github.com/assimp/assimp/pull/5624
* @ThatOSDev made their first contribution in https://github.com/assimp/assimp/pull/5639
* @mapret made their first contribution in https://github.com/assimp/assimp/pull/5640
* @bedwardly-down made their first contribution in https://github.com/assimp/assimp/pull/5650
* @sgayda2 made their first contribution in https://github.com/assimp/assimp/pull/5658
* @JensEhrhardt-eOPUS made their first contribution in https://github.com/assimp/assimp/pull/5432
* @Kimbatt made their first contribution in https://github.com/assimp/assimp/pull/5695
* @natevm made their first contribution in https://github.com/assimp/assimp/pull/5713
* @mkuritsu made their first contribution in https://github.com/assimp/assimp/pull/5693
* @Sanchikuuus made their first contribution in https://github.com/assimp/assimp/pull/5101
* @imdongye made their first contribution in https://github.com/assimp/assimp/pull/5728
* @ochafik made their first contribution in https://github.com/assimp/assimp/pull/5681
* @fuhaixi made their first contribution in https://github.com/assimp/assimp/pull/5675
* @AKosmachyov made their first contribution in https://github.com/assimp/assimp/pull/5648
* @Fav made their first contribution in https://github.com/assimp/assimp/pull/5647

**Full Changelog**: https://github.com/assimp/assimp/compare/v5.3.1...v5.4.3

# 5.4.2
## What's Changed
* Fix building on Haiku by @Begasus in https://github.com/assimp/assimp/pull/5255
* Reduce memory consumption in JoinVerticesProcess::ProcessMesh() signi… by @ockeymm in https://github.com/assimp/assimp/pull/5252
* Fix: Add check for invalid input argument by @kimkulling in https://github.com/assimp/assimp/pull/5258
* Replace an assert by a error log. by @kimkulling in https://github.com/assimp/assimp/pull/5260
* Extension of skinning data export to GLB/GLTF format by @fvbj in https://github.com/assimp/assimp/pull/5243
* Fix output floating-point values to fbx by @Riv1s-sSsA01 in https://github.com/assimp/assimp/pull/5265
* Update ImproveCacheLocality.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5268
* Update Readme.md by @kimkulling in https://github.com/assimp/assimp/pull/5277
* Deep arsdk bone double free by @kimkulling in https://github.com/assimp/assimp/pull/5291
* Fix Spelling error by @JulianKnodt in https://github.com/assimp/assimp/pull/5295
* use size in order to be compatible with float and double by @sloriot in https://github.com/assimp/assimp/pull/5270
* Fix: Add missing transformation for normalized normals. by @kimkulling in https://github.com/assimp/assimp/pull/5301
* Fix: Implicit Conversion Error by @Ipomoea in https://github.com/assimp/assimp/pull/5271
* Fix add checks for indices by @kimkulling in https://github.com/assimp/assimp/pull/5306
* Update FBXBinaryTokenizer.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5311
* link to external minizip with full path by @aumuell in https://github.com/assimp/assimp/pull/5278
* utf8 header not found by @TarcioV in https://github.com/assimp/assimp/pull/5279
* Rm unnecessary deg->radian conversion in FBX exporter by @JulianKnodt in https://github.com/assimp/assimp/pull/5281
* Fix empty mesh handling by @kimkulling in https://github.com/assimp/assimp/pull/5318
* Refactoring: Some cleanups by @kimkulling in https://github.com/assimp/assimp/pull/5319
* Fix invalid read of `uint` from `uvwsrc` by @JulianKnodt in https://github.com/assimp/assimp/pull/5282
* Remove double delete by @kimkulling in https://github.com/assimp/assimp/pull/5325
* fix mesh-name error. by @copycd in https://github.com/assimp/assimp/pull/5294
* COLLADA fixes for textures in C4D input by @wmatyjewicz in https://github.com/assimp/assimp/pull/5293
* Use the correct allocator for deleting objects in case of duplicate a… by @FlorianBorn71 in https://github.com/assimp/assimp/pull/5305
* Fix container overflow in MMD parser by @aavenel in https://github.com/assimp/assimp/pull/5309
* Fix: PLY heap buffer overflow by @aavenel in https://github.com/assimp/assimp/pull/5310
* Update Readme.md by @cuppajoeman in https://github.com/assimp/assimp/pull/5312
* Update Readme.md by @cuppajoeman in https://github.com/assimp/assimp/pull/5313
* Fix: Check if index for mesh access is out of range by @kimkulling in https://github.com/assimp/assimp/pull/5338
* Update FBXConverter.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5349
* FBX: Use correct time scaling by @kimkulling in https://github.com/assimp/assimp/pull/5355
* Drop explicit inclusion of contrib/ headers by @umlaeute in https://github.com/assimp/assimp/pull/5316
* Update Build.md by @cuppajoeman in https://github.com/assimp/assimp/pull/5314
* Fix buffer overflow in FBX::Util::DecodeBase64() by @ttxine in https://github.com/assimp/assimp/pull/5322
* Readme.md:  correct 2 errors in section headers by @stephengold in https://github.com/assimp/assimp/pull/5351
* Fix double free in Video::~Video() by @ttxine in https://github.com/assimp/assimp/pull/5323
* FBXMeshGeometry:  solve issue #5116 using patch provided by darktjm by @stephengold in https://github.com/assimp/assimp/pull/5333
* Fix target names not being imported on some gLTF2 models by @Futuremappermydud in https://github.com/assimp/assimp/pull/5356
* correct grammar/typographic errors in comments (8 files) by @stephengold in https://github.com/assimp/assimp/pull/5343
* KHR_materials_specular fixes by @rudybear in https://github.com/assimp/assimp/pull/5347
* Disable Hunter by @kimkulling in https://github.com/assimp/assimp/pull/5388
* fixed several issues by @MarkaRagnos0815 in https://github.com/assimp/assimp/pull/5359
* Fix leak by @kimkulling in https://github.com/assimp/assimp/pull/5391
* Check validity of archive without parsing by @kimkulling in https://github.com/assimp/assimp/pull/5393
* Fix integer overflow by @kimkulling in https://github.com/assimp/assimp/pull/5394
* Add a test before generating the txture folder by @kimkulling in https://github.com/assimp/assimp/pull/5400
* Build: Disable building zlib for non-windows by @kimkulling in https://github.com/assimp/assimp/pull/5401
* null check. by @copycd in https://github.com/assimp/assimp/pull/5402
* Bump actions/upload-artifact from 3 to 4 by @dependabot in https://github.com/assimp/assimp/pull/5384
* fix: KHR_materials_pbrSpecularGlossiness/diffuseFactor convert to pbr… by @guguTang in https://github.com/assimp/assimp/pull/5410
* fix building errors for MinGW by @0xf0ad in https://github.com/assimp/assimp/pull/5376
* dynamic_cast error. by @copycd in https://github.com/assimp/assimp/pull/5406
* Add missing IRR textures by @tellypresence in https://github.com/assimp/assimp/pull/5374
* Update Dockerfile by @kimkulling in https://github.com/assimp/assimp/pull/5412
* Fix handling of X3D IndexedLineSet nodes by @andre-schulz in https://github.com/assimp/assimp/pull/5362
* Improve acc file loading by @IOBYTE in https://github.com/assimp/assimp/pull/5360
* Readme.md:  present hyperlinks in a more uniform style by @stephengold in https://github.com/assimp/assimp/pull/5364
* FBX Blendshape `FullWeight: Vec<Float>` -> `FullWeight: Vec<Double>` by @JulianKnodt in https://github.com/assimp/assimp/pull/5441
* Fix for issues #5422, #3411, and #5443 -- DXF insert scaling fix and colour fix by @seanth in https://github.com/assimp/assimp/pull/5426
* Update StbCommon.h to stay up-to-date with stb_image.h. by @tigert1998 in https://github.com/assimp/assimp/pull/5436
* Introduce aiBuffer by @kimkulling in https://github.com/assimp/assimp/pull/5444
* Add bounds checks to the parsing utilities. by @kimkulling in https://github.com/assimp/assimp/pull/5421
* Fix crash in viewer by @kimkulling in https://github.com/assimp/assimp/pull/5446
* Static code analysis fixes by @kimkulling in https://github.com/assimp/assimp/pull/5447
* Kimkulling/fix bahavior of remove redundat mats issue 5438 by @kimkulling in https://github.com/assimp/assimp/pull/5451
* Fix X importer breakage introduced in commit f844c33  by @tellypresence in https://github.com/assimp/assimp/pull/5372
* Fileformats.md:  clarify that import of .blend files is deprecated by @stephengold in https://github.com/assimp/assimp/pull/5350
* feat:1.add 3mf vertex color read 2.fix 3mf read texture bug by @GalenXiao in https://github.com/assimp/assimp/pull/5361
* More GLTF loading hardening by @FlorianBorn71 in https://github.com/assimp/assimp/pull/5415
* Bump actions/cache from 3 to 4 by @dependabot in https://github.com/assimp/assimp/pull/5431
* Update CMakeLists.txt by @mosfet80 in https://github.com/assimp/assimp/pull/5379
* `Blendshape`->`Geometry` in FBX Export by @JulianKnodt in https://github.com/assimp/assimp/pull/5419
* Fix identity matrix check by @fvbj in https://github.com/assimp/assimp/pull/5445
* Fix PyAssimp under Python >= 3.12 and macOS library search support by @Th3T3chn0G1t in https://github.com/assimp/assimp/pull/5397
* Add ISC LICENSE file by @severin-lemaignan in https://github.com/assimp/assimp/pull/5465
* ColladaParser: check values length by @etam in https://github.com/assimp/assimp/pull/5462
* Include defs in not cpp-section by @kimkulling in https://github.com/assimp/assimp/pull/5466
* Add correct double zero check by @kimkulling in https://github.com/assimp/assimp/pull/5471
* Add zlib-header to ZipArchiveIOSystem.h by @kimkulling in https://github.com/assimp/assimp/pull/5473
* Add 2024 to copyright infos by @kimkulling in https://github.com/assimp/assimp/pull/5475
* Append a new setting "AI_CONFIG_EXPORT_FBX_TRANSPARENCY_FACTOR_REFER_TO_OPACITY" by @Riv1s-sSsA01 in https://github.com/assimp/assimp/pull/5450
* Eliminate non-ascii comments in clipper by @adfwer233 in https://github.com/assimp/assimp/pull/5480
* Fix compilation for MSVC14. by @LukasBanana in https://github.com/assimp/assimp/pull/5490
* Add correction of fbx model rotation by @kimkulling in https://github.com/assimp/assimp/pull/5494
* Delete tools/make directory by @mosfet80 in https://github.com/assimp/assimp/pull/5491
* Delete packaging/windows-mkzip directory by @mosfet80 in https://github.com/assimp/assimp/pull/5492
* Fix #5420 duplicate degrees to radians conversion in fbx importer by @Biohazard90 in https://github.com/assimp/assimp/pull/5427
* Respect merge identical vertices in ObjExporter by @JulianKnodt in https://github.com/assimp/assimp/pull/5521
* Fix utDefaultIOStream test under MinGW by @thenanisore in https://github.com/assimp/assimp/pull/5525
* Fix typos by @RoboSchmied in https://github.com/assimp/assimp/pull/5518
* Add initial macOS support to C4D importer by @AlexTMjugador in https://github.com/assimp/assimp/pull/5516
* Update hunter into CMakeLists.txt by @mosfet80 in https://github.com/assimp/assimp/pull/5505
* Fix: add missing import for `AI_CONFIG_CHECK_IDENTITY_MATRIX_EPSILON_DEFAULT` by @tomheaton in https://github.com/assimp/assimp/pull/5507
* updated json by @mosfet80 in https://github.com/assimp/assimp/pull/5501
* Cleanup: Fix review findings by @kimkulling in https://github.com/assimp/assimp/pull/5528
* Update CMakeLists.txt by @kimkulling in https://github.com/assimp/assimp/pull/5531
* CMake: Allow linking draco statically if ASSIMP_BUILD_DRACO_STATIC is set. by @alexrp in https://github.com/assimp/assimp/pull/5535
* updated minizip to last version by @mosfet80 in https://github.com/assimp/assimp/pull/5498
* updated STBIMAGElib by @mosfet80 in https://github.com/assimp/assimp/pull/5500
* fix issue #5461 (segfault after removing redundant materials) by @stephengold in https://github.com/assimp/assimp/pull/5467
* Update ComputeUVMappingProcess.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5541
* add some ASSIMP_INSTALL checks by @ZeunO8 in https://github.com/assimp/assimp/pull/5545
* Fix SplitByBoneCount typo that prevented node updates by @Succ3s in https://github.com/assimp/assimp/pull/5550
* Q3DLoader: Fix possible material string overflow by @kimkulling in https://github.com/assimp/assimp/pull/5556
* Reverts the changes introduced by commit ad766cb in February 2022. by @johannesugb in https://github.com/assimp/assimp/pull/5542
* fix a collada import bug by @xiaoxiaopifu in https://github.com/assimp/assimp/pull/5561
* mention IQM loader in Fileformats.md by @Garux in https://github.com/assimp/assimp/pull/5560
* Kimkulling/fix pyassimp compatibility by @kimkulling in https://github.com/assimp/assimp/pull/5563
* fix ASE loader crash when *MATERIAL_COUNT or *NUMSUBMTLS is not specified or is 0 by @Garux in https://github.com/assimp/assimp/pull/5559
* Add checks for invalid buffer and size by @kimkulling in https://github.com/assimp/assimp/pull/5570
* Make sure for releases revision will be zero by @kimkulling in https://github.com/assimp/assimp/pull/5571
* glTF2Importer: Support .vrm extension by @uyjulian in https://github.com/assimp/assimp/pull/5569
* Prepare v5.4.1 by @kimkulling in https://github.com/assimp/assimp/pull/5573
* Remove deprecated c++11 warnings by @kimkulling in https://github.com/assimp/assimp/pull/5576
* fix ci by disabling tests by @kimkulling in https://github.com/assimp/assimp/pull/5583
* Fix integer overflow by @kimkulling in https://github.com/assimp/assimp/pull/5581
* Assimp viewer fixes by @JLouis-B in https://github.com/assimp/assimp/pull/5582
* Optimize readability by @kimkulling in https://github.com/assimp/assimp/pull/5578
* Temporary fix for #5557 GCC 13+ build issue -Warray-bounds by @dbs4261 in https://github.com/assimp/assimp/pull/5577
* Fix a bug that could cause assertion failure. by @vengine in https://github.com/assimp/assimp/pull/5575
* Fix possible nullptr dereferencing. by @kimkulling in https://github.com/assimp/assimp/pull/5595
* Update ObjFileParser.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5598
* Fix for #5592 Disabled maybe-uninitialized error for AssetLib/Obj/ObjFileParser.cpp by @dbs4261 in https://github.com/assimp/assimp/pull/5593
* updated zip by @mosfet80 in https://github.com/assimp/assimp/pull/5499
* Postprocessing: Fix endless loop by @kimkulling in https://github.com/assimp/assimp/pull/5605
* Build: Fix compilation for VS-2022 debug mode - warning by @kimkulling in https://github.com/assimp/assimp/pull/5606
* Converted a size_t to mz_uint that was being treated as an error by @BradlyLanducci in https://github.com/assimp/assimp/pull/5600
* Add trim to xml string parsing by @kimkulling in https://github.com/assimp/assimp/pull/5611
* Replace duplicated trim by @kimkulling in https://github.com/assimp/assimp/pull/5613
* Move aiScene constructor by @kimkulling in https://github.com/assimp/assimp/pull/5614
* Move revision.h and revision.h.in to include folder by @kimkulling in https://github.com/assimp/assimp/pull/5615
* Update MDLMaterialLoader.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5620
* Create inno_setup by @kimkulling in https://github.com/assimp/assimp/pull/5621
* clean HunterGate.cmake by @mosfet80 in https://github.com/assimp/assimp/pull/5619
* Draft: Update init of aiString by @kimkulling in https://github.com/assimp/assimp/pull/5623
* Fix init aistring issue 5622 inpython module by @kimkulling in https://github.com/assimp/assimp/pull/5625
* update dotnet example by @mosfet80 in https://github.com/assimp/assimp/pull/5618
* Make stepfile schema validation more robust. by @kimkulling in https://github.com/assimp/assimp/pull/5627
* fix PLY binary export color from float to uchar by @michaelsctts in https://github.com/assimp/assimp/pull/5608
* Some FBXs do not have "Materials" information, which can cause parsing errors by @ycn2022 in https://github.com/assimp/assimp/pull/5624
* Fix collada uv channels - temporary was stored and then updated. by @StepanHrbek in https://github.com/assimp/assimp/pull/5630
* remove ASE parsing break by @Garux in https://github.com/assimp/assimp/pull/5634
* FBX-Exporter: Fix nullptr dereferencing by @kimkulling in https://github.com/assimp/assimp/pull/5638
* Fix FBX exporting incorrect bone order by @JulianKnodt in https://github.com/assimp/assimp/pull/5435
* fixes potential memory leak on malformed obj file by @TinyTinni in https://github.com/assimp/assimp/pull/5645
* Update zip.c by @ThatOSDev in https://github.com/assimp/assimp/pull/5639
* Fixes some uninit bool loads by @TinyTinni in https://github.com/assimp/assimp/pull/5644
* Fix names of enum values in docstring of aiProcess_FindDegenerates by @mapret in https://github.com/assimp/assimp/pull/5640
* Fix: StackAllocator Undefined Reference fix by @bedwardly-down in https://github.com/assimp/assimp/pull/5650
* Plx: Fix out of bound access by @kimkulling in https://github.com/assimp/assimp/pull/5651

## New Contributors
* @Begasus made their first contribution in https://github.com/assimp/assimp/pull/5255
* @ockeymm made their first contribution in https://github.com/assimp/assimp/pull/5252
* @fvbj made their first contribution in https://github.com/assimp/assimp/pull/5243
* @JulianKnodt made their first contribution in https://github.com/assimp/assimp/pull/5295
* @sloriot made their first contribution in https://github.com/assimp/assimp/pull/5270
* @Ipomoea made their first contribution in https://github.com/assimp/assimp/pull/5271
* @aumuell made their first contribution in https://github.com/assimp/assimp/pull/5278
* @TarcioV made their first contribution in https://github.com/assimp/assimp/pull/5279
* @copycd made their first contribution in https://github.com/assimp/assimp/pull/5294
* @cuppajoeman made their first contribution in https://github.com/assimp/assimp/pull/5312
* @ttxine made their first contribution in https://github.com/assimp/assimp/pull/5322
* @Futuremappermydud made their first contribution in https://github.com/assimp/assimp/pull/5356
* @MarkaRagnos0815 made their first contribution in https://github.com/assimp/assimp/pull/5359
* @0xf0ad made their first contribution in https://github.com/assimp/assimp/pull/5376
* @seanth made their first contribution in https://github.com/assimp/assimp/pull/5426
* @tigert1998 made their first contribution in https://github.com/assimp/assimp/pull/5436
* @GalenXiao made their first contribution in https://github.com/assimp/assimp/pull/5361
* @Th3T3chn0G1t made their first contribution in https://github.com/assimp/assimp/pull/5397
* @etam made their first contribution in https://github.com/assimp/assimp/pull/5462
* @adfwer233 made their first contribution in https://github.com/assimp/assimp/pull/5480
* @LukasBanana made their first contribution in https://github.com/assimp/assimp/pull/5490
* @thenanisore made their first contribution in https://github.com/assimp/assimp/pull/5525
* @RoboSchmied made their first contribution in https://github.com/assimp/assimp/pull/5518
* @AlexTMjugador made their first contribution in https://github.com/assimp/assimp/pull/5516
* @tomheaton made their first contribution in https://github.com/assimp/assimp/pull/5507
* @alexrp made their first contribution in https://github.com/assimp/assimp/pull/5535
* @ZeunO8 made their first contribution in https://github.com/assimp/assimp/pull/5545
* @Succ3s made their first contribution in https://github.com/assimp/assimp/pull/5550
* @johannesugb made their first contribution in https://github.com/assimp/assimp/pull/5542
* @xiaoxiaopifu made their first contribution in https://github.com/assimp/assimp/pull/5561
* @uyjulian made their first contribution in https://github.com/assimp/assimp/pull/5569
* @dbs4261 made their first contribution in https://github.com/assimp/assimp/pull/5577
* @vengine made their first contribution in https://github.com/assimp/assimp/pull/5575
* @BradlyLanducci made their first contribution in https://github.com/assimp/assimp/pull/5600
* @michaelsctts made their first contribution in https://github.com/assimp/assimp/pull/5608
* @ycn2022 made their first contribution in https://github.com/assimp/assimp/pull/5624
* @ThatOSDev made their first contribution in https://github.com/assimp/assimp/pull/5639
* @mapret made their first contribution in https://github.com/assimp/assimp/pull/5640
* @bedwardly-down made their first contribution in https://github.com/assimp/assimp/pull/5650

**Full Changelog**: https://github.com/assimp/assimp/compare/v5.3.1...v5.4.2

# 5.4.1
## What's Changed
* CMake: Allow linking draco statically if ASSIMP_BUILD_DRACO_STATIC is set. by @alexrp in https://github.com/assimp/assimp/pull/5535
* Deps: updated minizip to last version by @mosfet80 in https://github.com/assimp/assimp/pull/5498
* Deps: updated STBIMAGElib by @mosfet80 in https://github.com/assimp/assimp/pull/5500
* Fix issue #5461 (segfault after removing redundant materials) by @stephengold in https://github.com/assimp/assimp/pull/5467
* Update ComputeUVMappingProcess.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5541
* Add some ASSIMP_INSTALL checks by @ZeunO8 in https://github.com/assimp/assimp/pull/5545
* Fix SplitByBoneCount typo that prevented node updates by @Succ3s in https://github.com/assimp/assimp/pull/5550
* Q3DLoader: Fix possible material string overflow by @kimkulling in https://github.com/assimp/assimp/pull/5556
* Reverts the changes introduced by commit ad766cb in February 2022. by @johannesugb in https://github.com/assimp/assimp/pull/5542
* Fix a collada import bug by @xiaoxiaopifu in https://github.com/assimp/assimp/pull/5561
* Mention IQM loader in Fileformats.md by @Garux in https://github.com/assimp/assimp/pull/5560
* Kimkulling/fix pyassimp compatibility by @kimkulling in https://github.com/assimp/assimp/pull/5563
* Fix ASE loader crash when *MATERIAL_COUNT or *NUMSUBMTLS is not specified or is 0 by @Garux in https://github.com/assimp/assimp/pull/5559
* Add checks for invalid buffer and size by @kimkulling in https://github.com/assimp/assimp/pull/5570
* Make sure for releases revision will be zero by @kimkulling in https://github.com/assimp/assimp/pull/5571
* glTF2Importer: Support .vrm extension by @uyjulian in https://github.com/assimp/assimp/pull/5569
* Prepare v5.4.1 by @kimkulling in https://github.com/assimp/assimp/pull/5573

## New Contributors
* @alexrp made their first contribution in https://github.com/assimp/assimp/pull/5535
* @ZeunO8 made their first contribution in https://github.com/assimp/assimp/pull/5545
* @Succ3s made their first contribution in https://github.com/assimp/assimp/pull/5550
* @johannesugb made their first contribution in https://github.com/assimp/assimp/pull/5542
* @xiaoxiaopifu made their first contribution in https://github.com/assimp/assimp/pull/5561
* @uyjulian made their first contribution in https://github.com/assimp/assimp/pull/5569

**Full Changelog**: https://github.com/assimp/assimp/compare/v5.4.0...v5.4.1

# 5.4.0
## What's Changed
* Fix building on Haiku by @Begasus in https://github.com/assimp/assimp/pull/5255
* Reduce memory consumption in JoinVerticesProcess::ProcessMesh() signi… by @ockeymm in https://github.com/assimp/assimp/pull/5252
* Fix: Add check for invalid input argument by @kimkulling in https://github.com/assimp/assimp/pull/5258
* Replace an assert by an error log. by @kimkulling in https://github.com/assimp/assimp/pull/5260
* Extension of skinning data export to GLB/GLTF format by @fvbj in https://github.com/assimp/assimp/pull/5243
* Fix output floating-point values to fbx by @Riv1s-sSsA01 in https://github.com/assimp/assimp/pull/5265
* Update ImproveCacheLocality.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5268
* Update Readme.md by @kimkulling in https://github.com/assimp/assimp/pull/5277
* Deep arsdk bone double free by @kimkulling in https://github.com/assimp/assimp/pull/5291
* Fix Spelling error by @JulianKnodt in https://github.com/assimp/assimp/pull/5295
* use size to be compatible with float and double by @sloriot in https://github.com/assimp/assimp/pull/5270
* Fix: Add missing transformation for normalized normals. by @kimkulling in https://github.com/assimp/assimp/pull/5301
* Fix: Implicit Conversion Error by @Ipomoea in https://github.com/assimp/assimp/pull/5271
* Fix add checks for indices by @kimkulling in https://github.com/assimp/assimp/pull/5306
* Update FBXBinaryTokenizer.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5311
* link to external minizip with full path by @aumuell in https://github.com/assimp/assimp/pull/5278
* utf8 header not found by @TarcioV in https://github.com/assimp/assimp/pull/5279
* Rm unnecessary deg->radian conversion in FBX exporter by @JulianKnodt in https://github.com/assimp/assimp/pull/5281
* Fix empty mesh handling by @kimkulling in https://github.com/assimp/assimp/pull/5318
* Refactoring: Some cleanups by @kimkulling in https://github.com/assimp/assimp/pull/5319
* Fix invalid read of `uint` from `uvwsrc` by @JulianKnodt in https://github.com/assimp/assimp/pull/5282
* Remove double delete by @kimkulling in https://github.com/assimp/assimp/pull/5325
* fix the mesh-name error. by @copycd in https://github.com/assimp/assimp/pull/5294
* COLLADA fixes for textures in C4D input by @wmatyjewicz in https://github.com/assimp/assimp/pull/5293
* Use the correct allocator for deleting objects in case of duplicate a… by @FlorianBorn71 in https://github.com/assimp/assimp/pull/5305
* Fix container overflow in MMD parser by @aavenel in https://github.com/assimp/assimp/pull/5309
* Fix: PLY heap buffer overflow by @aavenel in https://github.com/assimp/assimp/pull/5310
* Update Readme.md by @cuppajoeman in https://github.com/assimp/assimp/pull/5312
* Update Readme.md by @cuppajoeman in https://github.com/assimp/assimp/pull/5313
* Fix: Check if index for mesh access is out of range by @kimkulling in https://github.com/assimp/assimp/pull/5338
* Update FBXConverter.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5349
* FBX: Use correct time scaling by @kimkulling in https://github.com/assimp/assimp/pull/5355
* Drop explicit inclusion of contrib/ headers by @umlaeute in https://github.com/assimp/assimp/pull/5316
* Update Build.md by @cuppajoeman in https://github.com/assimp/assimp/pull/5314
* Fix buffer overflow in FBX::Util::DecodeBase64() by @ttxine in https://github.com/assimp/assimp/pull/5322
* Readme.md:  correct 2 errors in section headers by @stephengold in https://github.com/assimp/assimp/pull/5351
* Fix double free in Video::~Video() by @ttxine in https://github.com/assimp/assimp/pull/5323
* FBXMeshGeometry:  solve issue #5116 using patch provided by darktjm by @stephengold in https://github.com/assimp/assimp/pull/5333
* Fix target names not being imported on some gLTF2 models by @Futuremappermydud in https://github.com/assimp/assimp/pull/5356
* correct grammar/typographic errors in comments (8 files) by @stephengold in https://github.com/assimp/assimp/pull/5343
* KHR_materials_specular fixes by @rudybear in https://github.com/assimp/assimp/pull/5347
* Disable Hunter by @kimkulling in https://github.com/assimp/assimp/pull/5388
* fixed several issues by @MarkaRagnos0815 in https://github.com/assimp/assimp/pull/5359
* Fix leak by @kimkulling in https://github.com/assimp/assimp/pull/5391
* Check the validity of the archive without parsing by @kimkulling in https://github.com/assimp/assimp/pull/5393
* Fix integer overflow by @kimkulling in https://github.com/assimp/assimp/pull/5394
* Add a test before generating the texture folder by @kimkulling in https://github.com/assimp/assimp/pull/5400
* Build: Disable building zlib for non-windows by @kimkulling in https://github.com/assimp/assimp/pull/5401
* null check. by @copycd in https://github.com/assimp/assimp/pull/5402
* Bump actions/upload-artifact from 3 to 4 by @dependabot in https://github.com/assimp/assimp/pull/5384
* fix: KHR_materials_pbrSpecularGlossiness/diffuseFactor convert to pbr… by @guguTang in https://github.com/assimp/assimp/pull/5410
* fix building errors for MinGW by @0xf0ad in https://github.com/assimp/assimp/pull/5376
* dynamic_cast error. by @copycd in https://github.com/assimp/assimp/pull/5406
* Add missing IRR textures by @tellypresence in https://github.com/assimp/assimp/pull/5374
* Update Dockerfile by @kimkulling in https://github.com/assimp/assimp/pull/5412
* Fix handling of X3D IndexedLineSet nodes by @andre-schulz in https://github.com/assimp/assimp/pull/5362
* Improve acc file loading by @IOBYTE in https://github.com/assimp/assimp/pull/5360
* Readme.md:  present hyperlinks in a more uniform style by @stephengold in https://github.com/assimp/assimp/pull/5364
* FBX Blendshape `FullWeight: Vec<Float>` -> `FullWeight: Vec<Double>` by @JulianKnodt in https://github.com/assimp/assimp/pull/5441
* Fix for issues #5422, #3411, and #5443 -- DXF insert scaling fix and colour fix by @seanth in https://github.com/assimp/assimp/pull/5426
* Update StbCommon.h to stay up-to-date with stb_image.h. by @tigert1998 in https://github.com/assimp/assimp/pull/5436
* Introduce aiBuffer by @kimkulling in https://github.com/assimp/assimp/pull/5444
* Add bounds checks to the parsing utilities. by @kimkulling in https://github.com/assimp/assimp/pull/5421
* Fix crash in viewer by @kimkulling in https://github.com/assimp/assimp/pull/5446
* Static code analysis fixes by @kimkulling in https://github.com/assimp/assimp/pull/5447
* Kimkulling/fix behavior of remove redundant mats issue 5438 by @kimkulling in https://github.com/assimp/assimp/pull/5451
* Fix X importer breakage introduced in commit f844c33  by @tellypresence in https://github.com/assimp/assimp/pull/5372
* Fileformats.md:  clarify that import of .blend files is deprecated by @stephengold in https://github.com/assimp/assimp/pull/5350
* feat:1.add 3mf vertex color read 2.fix 3mf read texture bug by @GalenXiao in https://github.com/assimp/assimp/pull/5361
* More GLTF loading hardening by @FlorianBorn71 in https://github.com/assimp/assimp/pull/5415
* Bump actions/cache from 3 to 4 by @dependabot in https://github.com/assimp/assimp/pull/5431
* Update CMakeLists.txt by @mosfet80 in https://github.com/assimp/assimp/pull/5379
* `Blendshape`->`Geometry` in FBX Export by @JulianKnodt in https://github.com/assimp/assimp/pull/5419
* Fix identity matrix check by @fvbj in https://github.com/assimp/assimp/pull/5445
* Fix PyAssimp under Python >= 3.12 and macOS library search support by @Th3T3chn0G1t in https://github.com/assimp/assimp/pull/5397
* Add ISC LICENSE file by @severin-lemaignan in https://github.com/assimp/assimp/pull/5465
* ColladaParser: check values length by @etam in https://github.com/assimp/assimp/pull/5462
* Include defs in not cpp-section by @kimkulling in https://github.com/assimp/assimp/pull/5466
* Add correct double zero check by @kimkulling in https://github.com/assimp/assimp/pull/5471
* Add zlib-header to ZipArchiveIOSystem.h by @kimkulling in https://github.com/assimp/assimp/pull/5473
* Add 2024 to copyright infos by @kimkulling in https://github.com/assimp/assimp/pull/5475
* Append a new setting "AI_CONFIG_EXPORT_FBX_TRANSPARENCY_FACTOR_REFER_TO_OPACITY" by @Riv1s-sSsA01 in https://github.com/assimp/assimp/pull/5450
* Eliminate non-ascii comments in clipper by @adfwer233 in https://github.com/assimp/assimp/pull/5480
* Fix compilation for MSVC14. by @LukasBanana in https://github.com/assimp/assimp/pull/5490
* Add correction of fbx model rotation by @kimkulling in https://github.com/assimp/assimp/pull/5494
* Delete tools/make directory by @mosfet80 in https://github.com/assimp/assimp/pull/5491
* Delete packaging/windows-mkzip directory by @mosfet80 in https://github.com/assimp/assimp/pull/5492
* Fix #5420 duplicate degrees to radians conversion in fbx importer by @Biohazard90 in https://github.com/assimp/assimp/pull/5427
* Respect merge identical vertices in ObjExporter by @JulianKnodt in https://github.com/assimp/assimp/pull/5521
* Fix utDefaultIOStream test under MinGW by @thenanisore in https://github.com/assimp/assimp/pull/5525
* Fix typos by @RoboSchmied in https://github.com/assimp/assimp/pull/5518
* Add initial macOS support to C4D importer by @AlexTMjugador in https://github.com/assimp/assimp/pull/5516
* Update hunter into CMakeLists.txt by @mosfet80 in https://github.com/assimp/assimp/pull/5505
* Fix: add a missing import for `AI_CONFIG_CHECK_IDENTITY_MATRIX_EPSILON_DEFAULT` by @tomheaton in https://github.com/assimp/assimp/pull/5507
* updated json by @mosfet80 in https://github.com/assimp/assimp/pull/5501
* Cleanup: Fix review findings by @kimkulling in https://github.com/assimp/assimp/pull/5528
* Update CMakeLists.txt by @kimkulling in https://github.com/assimp/assimp/pull/5531

## New Contributors
* @Begasus made their first contribution in https://github.com/assimp/assimp/pull/5255
* @ockeymm made their first contribution in https://github.com/assimp/assimp/pull/5252
* @fvbj made their first contribution in https://github.com/assimp/assimp/pull/5243
* @JulianKnodt made their first contribution in https://github.com/assimp/assimp/pull/5295
* @sloriot made their first contribution in https://github.com/assimp/assimp/pull/5270
* @Ipomoea made their first contribution in https://github.com/assimp/assimp/pull/5271
* @aumuell made their first contribution in https://github.com/assimp/assimp/pull/5278
* @TarcioV made their first contribution in https://github.com/assimp/assimp/pull/5279
* @copycd made their first contribution in https://github.com/assimp/assimp/pull/5294
* @cuppajoeman made their first contribution in https://github.com/assimp/assimp/pull/5312
* @ttxine made their first contribution in https://github.com/assimp/assimp/pull/5322
* @Futuremappermydud made their first contribution in https://github.com/assimp/assimp/pull/5356
* @MarkaRagnos0815 made their first contribution in https://github.com/assimp/assimp/pull/5359
* @0xf0ad made their first contribution in https://github.com/assimp/assimp/pull/5376
* @seanth made their first contribution in https://github.com/assimp/assimp/pull/5426
* @tigert1998 made their first contribution in https://github.com/assimp/assimp/pull/5436
* @GalenXiao made their first contribution in https://github.com/assimp/assimp/pull/5361
* @Th3T3chn0G1t made their first contribution in https://github.com/assimp/assimp/pull/5397
* @etam made their first contribution in https://github.com/assimp/assimp/pull/5462
* @adfwer233 made their first contribution in https://github.com/assimp/assimp/pull/5480
* @LukasBanana made their first contribution in https://github.com/assimp/assimp/pull/5490
* @thenanisore made their first contribution in https://github.com/assimp/assimp/pull/5525
* @RoboSchmied made their first contribution in https://github.com/assimp/assimp/pull/5518
* @AlexTMjugador made their first contribution in https://github.com/assimp/assimp/pull/5516
* @tomheaton made their first contribution in https://github.com/assimp/assimp/pull/5507

**Full Changelog**: https://github.com/assimp/assimp/compare/v5.3.1...v5.4.0

# 5.3.0
## What's Changed
* fix variable name  by @mosfet80 in https://github.com/assimp/assimp/pull/5249
* Bugfix: Remove unused header from types by @kimkulling in https://github.com/assimp/assimp/pull/5250
* contrib/zip/src/zip.h:  correct 2 spelling errors in comments by @stephengold in https://github.com/assimp/assimp/pull/5248
* Updated cpp-pm/hunter into CMakeLists.txt by @mosfet80 in https://github.com/assimp/assimp/pull/5236

## New Contributors
* @stephengold made their first contribution in https://github.com/assimp/assimp/pull/5248

**Full Changelog**: https://github.com/assimp/assimp/compare/v5.3.0...v5.3.1

## What's Changed
* Perfect forward val to utMaybe.cpp by @Skylion007 in https://github.com/assimp/assimp/pull/4717
* Fix config documentation for STL pointcloud export by @kimkulling in https://github.com/assimp/assimp/pull/4721
* Missing path by @ramzeng in https://github.com/assimp/assimp/pull/4723
* Fix: Use ASCII treeview in assimp-cmd. by @kimkulling in https://github.com/assimp/assimp/pull/4728
* Add check for wall switch from cmake by @kimkulling in https://github.com/assimp/assimp/pull/4731
* Adjust cmake version by @waebbl in https://github.com/assimp/assimp/pull/4730
* IMP: Reorganize doc by @kimkulling in https://github.com/assimp/assimp/pull/4732
* FIX: Fix head overflow in MD5-parser. by @kimkulling in https://github.com/assimp/assimp/pull/4736
* Introduce newer versions for MSVC Version by @kimkulling in https://github.com/assimp/assimp/pull/4739
* Prefix symbols from stb_image.h by @p12tic in https://github.com/assimp/assimp/pull/4737
* GitHub Workflows security hardening by @sashashura in https://github.com/assimp/assimp/pull/4734
* FIX: C++ std::tuple constexpr initial list on old compiler by @feishengfei in https://github.com/assimp/assimp/pull/4742
* Make FBX parser resilient to missing data streams by @FlorianBorn71 in https://github.com/assimp/assimp/pull/4738
* fix incorrect documentation of defaults by @cwoac in https://github.com/assimp/assimp/pull/4745
* Fixed issue with clang complaining about sprintf and vsprintf being depreciated by @slinky55 in https://github.com/assimp/assimp/pull/4744
* Fix build error: ‘temp’ may be used uninitialized in this function by @rhabacker in https://github.com/assimp/assimp/pull/4754
* FIX: Use correct epsilon by @kimkulling in https://github.com/assimp/assimp/pull/4756
* Use correct flags for openddl for static libs by @kimkulling in https://github.com/assimp/assimp/pull/4757
* Fix: Add missing handling for double export in json by @kimkulling in https://github.com/assimp/assimp/pull/4761
* * fix bug reading ply file in case of presence of "end_header\n<BINARY_DATA>..." with <BINARY_DATA> starting by "\n" by @emvivre in https://github.com/assimp/assimp/pull/4755
* Fixed error with trailing zero symbols as placeholder character by @Let0s in https://github.com/assimp/assimp/pull/4759
* Fix: Avoid nullptr dereferencing + refactorings. by @kimkulling in https://github.com/assimp/assimp/pull/4776
* chore: add missing std moves and perfect forwards by @Skylion007 in https://github.com/assimp/assimp/pull/4785
* Update dependabot.yml by @kimkulling in https://github.com/assimp/assimp/pull/4794
* Add missing header for Ubuntu 16 and Mac by @kimkulling in https://github.com/assimp/assimp/pull/4800
* Don't hide out-of-memory during FBX import by @jakrams in https://github.com/assimp/assimp/pull/4801
* Added support for KHR_materials_emissive_strength by @Beilinson in https://github.com/assimp/assimp/pull/4787
* Add overflow check for invalid data. by @kimkulling in https://github.com/assimp/assimp/pull/4809
* Add CIFuzz GitHub action by @DavidKorczynski in https://github.com/assimp/assimp/pull/4807
* Fixed some grammar and spelling mistakes by @CMDR-JohnAlex in https://github.com/assimp/assimp/pull/4805
* Introduce --parallel instead of .j by @kimkulling in https://github.com/assimp/assimp/pull/4813
* Modernize smartptrs and use C++11 literals by @Skylion007 in https://github.com/assimp/assimp/pull/4792
* [BlenderDNA.h] Declare explicit specializations by @tkoeppe in https://github.com/assimp/assimp/pull/4816
* FIX: Fix possible division by zero by @kimkulling in https://github.com/assimp/assimp/pull/4820
* Avoid undefined-shift in Assimp::ASE::Parser::ParseLV4MeshFace. by @kimkulling in https://github.com/assimp/assimp/pull/4829
* Ensure the face pointer is not nullptr by @kimkulling in https://github.com/assimp/assimp/pull/4832
* fix warnings-as-errors for msvc 2019 x64 by @Gargaj in https://github.com/assimp/assimp/pull/4825
* Fixes Heap-buffer-overflow READ 4 in Assimp::ScenePreprocessor::ProcssMesh by @sashashura in https://github.com/assimp/assimp/pull/4836
* Fixes Heap-buffer-overflow READ 1 in Assimp::MD5::MD5Parser::ParseHeader by @sashashura in https://github.com/assimp/assimp/pull/4837
* Fixes Heap-buffer-overflow READ 1 in Assimp::ObjFileParser::getFace by @sashashura in https://github.com/assimp/assimp/pull/4838
* Fixed bug when exporting binary FBX by @umesh-huawei in https://github.com/assimp/assimp/pull/4824
* illegal token on right-side-of ::Windows by @rohit-kumar-j in https://github.com/assimp/assimp/pull/4846
* Update unzip.c by @kimkulling in https://github.com/assimp/assimp/pull/4848
* Refactoring: Move assert handler header to include by @kimkulling in https://github.com/assimp/assimp/pull/4850
* sprintf to snprintf with known MAXLEN for buffer. by @sfjohnston in https://github.com/assimp/assimp/pull/4852
* {cmake} Remove dead code by @asmaloney in https://github.com/assimp/assimp/pull/4858
* Fix: Fix signed unsigned mismatch by @kimkulling in https://github.com/assimp/assimp/pull/4859
* Fix: Fix possible division by zero by @kimkulling in https://github.com/assimp/assimp/pull/4861
* Update the getting help section by @kimkulling in https://github.com/assimp/assimp/pull/4863
* Fix several spelling mistakes by @asmaloney in https://github.com/assimp/assimp/pull/4855
* Change mMethod type to enum aiMorphingMethod by @tellypresence in https://github.com/assimp/assimp/pull/4873
* Remove deprecated comment by @kimkulling in https://github.com/assimp/assimp/pull/4876
* Generalize JoinVerticesProcess for multiple UV and color channels by @drbct in https://github.com/assimp/assimp/pull/4872
* Fix #4262 Build With M3D Import Only by @krishty in https://github.com/assimp/assimp/pull/4879
* Fix #4877 by @MMory in https://github.com/assimp/assimp/pull/4878
* Update LimitBoneWeightsProcess.cpp by @kimkulling in https://github.com/assimp/assimp/pull/4895
* Remove /Zi compiler flag for MSVC, release config by @kimkulling in https://github.com/assimp/assimp/pull/4896
* Ensure initializer exists by @kimkulling in https://github.com/assimp/assimp/pull/4897
* Trim Trailing Whitespace by @krishty in https://github.com/assimp/assimp/pull/4882
* Remove Useless “virtual” by @krishty in https://github.com/assimp/assimp/pull/4884
* Replace Variables With Literals by @krishty in https://github.com/assimp/assimp/pull/4885
* Fix: fix incorrect math for calculating the horizontal FOV of a perspective camera in gltf2 import #4435 by @shimaowo in https://github.com/assimp/assimp/pull/4886
* Remove Stray Semicolon by @krishty in https://github.com/assimp/assimp/pull/4887
* Tidy Up Constructors and Destructors by @krishty in https://github.com/assimp/assimp/pull/4888
* Fix MSVC Warnings With “emplace_back()” by @krishty in https://github.com/assimp/assimp/pull/4889
* Correctly consider aiProcess_FlipWindingOrder and aiProcess_MakeLeftHanded when generating normals by @lsnoel in https://github.com/assimp/assimp/pull/4892
* Update morph mesh documentation now that gltf known to work by @tellypresence in https://github.com/assimp/assimp/pull/4904
* Fix Build Without ArmaturePopulate Post Process Step by @krishty in https://github.com/assimp/assimp/pull/4880
* Fix: Remove deprecated dependency. by @kimkulling in https://github.com/assimp/assimp/pull/4910
* Optimized usedVertexIndices in JoinVerticesProcess by using bitmask instead of unordered_set by @AdamCichocki in https://github.com/assimp/assimp/pull/4901
* Fix issue #4866 by continuing to reset read loop after hitting a comment by @PencilAmazing in https://github.com/assimp/assimp/pull/4899
* Don't build zlib if ASSIMP_BUILD_ZLIB=OFF by @shaunrd0 in https://github.com/assimp/assimp/pull/4898
* Unit test warning fixes by @turol in https://github.com/assimp/assimp/pull/4932
* Fix Terragen loader by @turol in https://github.com/assimp/assimp/pull/4934
* Fixes PLY reader when the header ends with \r\n by @TinyTinni in https://github.com/assimp/assimp/pull/4936
* ACLoader: add support for reading more than one texture per object by @IOBYTE in https://github.com/assimp/assimp/pull/4935
* Delete .coveralls.yml by @kimkulling in https://github.com/assimp/assimp/pull/4941
* Fix: Fix memleak when exiting method by exception by @kimkulling in https://github.com/assimp/assimp/pull/4943
* The member 'Flush()' needs to be marked as an override for msvc clang compiling by @jiannanya in https://github.com/assimp/assimp/pull/4945
* Add skeleton doc by @kimkulling in https://github.com/assimp/assimp/pull/4946
* Fix PyAssimp README typo by @shammellee in https://github.com/assimp/assimp/pull/4960
* Add missing pod types. by @kimkulling in https://github.com/assimp/assimp/pull/4967
* Fix implicit conversion errors on macOS by @aaronmjacobs in https://github.com/assimp/assimp/pull/4965
* Update mesh.h by @kimkulling in https://github.com/assimp/assimp/pull/4962
* Move string definitions into the conditional block to fix unused variable warnings by @turol in https://github.com/assimp/assimp/pull/4969
* Fix: Fix typo in the doc by @kimkulling in https://github.com/assimp/assimp/pull/4974
* Fix index out of bounds by @mjunix in https://github.com/assimp/assimp/pull/4970
* Fix index out of bounds by @mjunix in https://github.com/assimp/assimp/pull/4971
* fix regression in join vertices post process. by @ockeymm123 in https://github.com/assimp/assimp/pull/4940
* Fix a leak in FBXDocument when duplicate object IDs are found by @avaneyev in https://github.com/assimp/assimp/pull/4963
* LWO fixes by @turol in https://github.com/assimp/assimp/pull/4977
* Fix build error when building SimpleTexturedDirectx11 with VS2022. by @Jackie9527 in https://github.com/assimp/assimp/pull/4989
* Fix: Use C++17 compliant utf8 encoding. by @kimkulling in https://github.com/assimp/assimp/pull/4986
* remove debug message from MemoryIOStream by @urshanselmann in https://github.com/assimp/assimp/pull/4994
* After Kim's addition to metadata types, use it in the FBX converter by @FlorianBorn71 in https://github.com/assimp/assimp/pull/4999
* Optimize subdivision process by @turol in https://github.com/assimp/assimp/pull/5000
* upgrade draco to 1.5.6 by @Jackie9527 in https://github.com/assimp/assimp/pull/4978
* C-API: Code cleanup by @kimkulling in https://github.com/assimp/assimp/pull/5004
* upgrade stb_image to v2.28. by @Jackie9527 in https://github.com/assimp/assimp/pull/4991
* bugfix the three vertices are collinear by @Jackie9527 in https://github.com/assimp/assimp/pull/4979
* Kimkulling/refactoring geoutils by @kimkulling in https://github.com/assimp/assimp/pull/5009
* Remove alarm badge by @kimkulling in https://github.com/assimp/assimp/pull/5011
* Fix Half-Life 1 MDL importer bone hierarchy. by @malortie in https://github.com/assimp/assimp/pull/5007
* Make Blender MVert no field optional by @turol in https://github.com/assimp/assimp/pull/5006
* bugfix removes duplicated data. by @Jackie9527 in https://github.com/assimp/assimp/pull/4980
* Validate node hierarchy parents by @turol in https://github.com/assimp/assimp/pull/5001
* Add more ASE model unit tests by @turol in https://github.com/assimp/assimp/pull/5023
* bugfix fails to check if the point in the triangle. by @Jackie9527 in https://github.com/assimp/assimp/pull/4981
* Revert 3D model corrupted by a8a1ca9 by @tellypresence in https://github.com/assimp/assimp/pull/5019
* Two bug fixes in Python port. by @MakerOfWyverns in https://github.com/assimp/assimp/pull/4985
* Add more AMF unit tests by @turol in https://github.com/assimp/assimp/pull/5026
* Create CODE_OF_CONDUCT.md by @kimkulling in https://github.com/assimp/assimp/pull/5030
* Add UTF-8 versions of UTF-16LE IRR/IRRMesh files by @tellypresence in https://github.com/assimp/assimp/pull/5017
* Test more files by @turol in https://github.com/assimp/assimp/pull/5031
* Update Readme.md by @kimkulling in https://github.com/assimp/assimp/pull/5034
* Update Readme.md by @kimkulling in https://github.com/assimp/assimp/pull/5035
* Update Readme.md by @kimkulling in https://github.com/assimp/assimp/pull/5032
* Fix link issue in UWP builds without functional replacement by @liedtkeInTUM in https://github.com/assimp/assimp/pull/5037
* Add build options to fix issues with clang 15. by @Jackie9527 in https://github.com/assimp/assimp/pull/4993
* Remove unused -Wno-shadow-field-in-constructor. by @Jackie9527 in https://github.com/assimp/assimp/pull/5052
* Fix Issue #4486 using the fix described by @jianliang79 by @aniongithub in https://github.com/assimp/assimp/pull/5053
* Fix warning related to nested-anon-types. by @Jackie9527 in https://github.com/assimp/assimp/pull/5040
* GLTF Importer: Build a list of the actual vertices so it works well with shared attribute lists by @FlorianBorn71 in https://github.com/assimp/assimp/pull/5003
* FBX import: Fix camera rotation by @inhosens in https://github.com/assimp/assimp/pull/5025
* std::getenv is not supported using uwp by @liedtkeInTUM in https://github.com/assimp/assimp/pull/5039
* Fix warning related to format-non-iso. by @Jackie9527 in https://github.com/assimp/assimp/pull/5041
* Fix warning related to unreachable-code. by @Jackie9527 in https://github.com/assimp/assimp/pull/5042
* Fix pre transform vertices with cameras by @sutajo in https://github.com/assimp/assimp/pull/5056
* glTF2: Fix incorrect camera position by @sutajo in https://github.com/assimp/assimp/pull/5055
* ConvertToLHProcess now inverts viewing direction by @sutajo in https://github.com/assimp/assimp/pull/5057
* Delete License.txt by @kimkulling in https://github.com/assimp/assimp/pull/5064
* Fix possible dereferencing of invalid pointers. by @kimkulling in https://github.com/assimp/assimp/pull/5066
* Fix: Fix leak in Scope class, FBX by @kimkulling in https://github.com/assimp/assimp/pull/5067
* Replace relative paths with local assets/textures by @tellypresence in https://github.com/assimp/assimp/pull/5043
* Fix: Avoid integer overflow in inversion operation by @kimkulling in https://github.com/assimp/assimp/pull/5068
* Fix warning related to unreachable-code-return. by @Jackie9527 in https://github.com/assimp/assimp/pull/5045
* Fix warning related to unreachable-code-break. by @Jackie9527 in https://github.com/assimp/assimp/pull/5046
* Fix warning related to missing-noreturn. by @Jackie9527 in https://github.com/assimp/assimp/pull/5047
* Remove unused -Wno-deprecated-copy-with-user-provided-dtor. by @Jackie9527 in https://github.com/assimp/assimp/pull/5048
* Fix warning related to inconsistent-missing-destructor-override. by @Jackie9527 in https://github.com/assimp/assimp/pull/5049
* Remove -Wno-deprecated-copy-with-dtor. by @Jackie9527 in https://github.com/assimp/assimp/pull/5051
* Build Zlib if missing for other platforms by @danoli3 in https://github.com/assimp/assimp/pull/5063
* Fix warning related to missing-variable-declarations. by @Jackie9527 in https://github.com/assimp/assimp/pull/5070
* Add missing cast by @sutajo in https://github.com/assimp/assimp/pull/5073
* Fix build warnings. by @Jackie9527 in https://github.com/assimp/assimp/pull/5075
* fix unreachable code by @showintime in https://github.com/assimp/assimp/pull/5078
* Kimkulling/refactoring geoutils by @kimkulling in https://github.com/assimp/assimp/pull/5086
* Fix: Copy aiMetadata in SceneCombiner by @luho383 in https://github.com/assimp/assimp/pull/5092
* Update: Enable export for fuzzer tests by @kimkulling in https://github.com/assimp/assimp/pull/5095
* Florian born71 small allocation fix in fbx loader by @kimkulling in https://github.com/assimp/assimp/pull/5096
* Small allocation fix in fbx loader by @FlorianBorn71 in https://github.com/assimp/assimp/pull/4494
* Update ASELoader.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5099
* Updated DirectX Loader to assign appropriate material index in ConvertMaterials Function by @naota29 in https://github.com/assimp/assimp/pull/4806
* Removed KHR_materials_pbrSpecularGlossiness, added KHR_materials_specular by @Beilinson in https://github.com/assimp/assimp/pull/4786
* Fix librt link by @mwestphal in https://github.com/assimp/assimp/pull/5087
* Fix pbrt exporter coordinate system and FoV by @skogler in https://github.com/assimp/assimp/pull/5082
* Fix warning related to unused-function. by @Jackie9527 in https://github.com/assimp/assimp/pull/5083
* C tech development corp gltf2 metadata export by @kimkulling in https://github.com/assimp/assimp/pull/5109
* Update_ Use the latest ubuntu image and switch to ninja by @kimkulling in https://github.com/assimp/assimp/pull/5114
* Fix Heap-buffer-overflow WRITE in Assimp::ObjFileImporter::createVertexArray by @sashashura in https://github.com/assimp/assimp/pull/5112
* Update pugiXML library by @mosfet80 in https://github.com/assimp/assimp/pull/5125
* Fix UNKNOWN READ in aiTexture::~aiTexture by @sashashura in https://github.com/assimp/assimp/pull/5129
* Fixed missing config property lookup for removal of empty bones. by @Biohazard90 in https://github.com/assimp/assimp/pull/5133
* Update cpp-pm / hunter by @mosfet80 in https://github.com/assimp/assimp/pull/5103
* Fix Heap-buffer-overflow READ in Assimp::MD5::MD5MeshParser::MD5MeshParser by @sashashura in https://github.com/assimp/assimp/pull/5110
* Skinning weights in gltf were broken by PR#5003 (vertex remapping) by @FlorianBorn71 in https://github.com/assimp/assimp/pull/5090
* Fix memory leak by @sashashura in https://github.com/assimp/assimp/pull/5134
* Fix UNKNOWN WRITE in Assimp::SortByPTypeProcess::Execute by @sashashura in https://github.com/assimp/assimp/pull/5138
* Delete old unused patch by @mosfet80 in https://github.com/assimp/assimp/pull/5130
* Add handling for negative indices. by @kimkulling in https://github.com/assimp/assimp/pull/5146
* Fix Heap-buffer-overflow READ in Assimp::ObjFileParser::getFace by @sashashura in https://github.com/assimp/assimp/pull/5111
* Fix UNKNOWN READ crash in UpdateMeshReferences by @sashashura in https://github.com/assimp/assimp/pull/5113
* Fix Heap-buffer-overflow READ in Assimp::FileSystemFilter::Cleanup by @sashashura in https://github.com/assimp/assimp/pull/5117
* Fix Stack-buffer-overflow READ in aiMaterial::AddBinaryProperty by @sashashura in https://github.com/assimp/assimp/pull/5120
* Fix unknown write in Assimp::ObjFileMtlImporter::getFloatValue by @sashashura in https://github.com/assimp/assimp/pull/5119
* Fix Memcpy-param-overlap in unzReadCurrentFile by @sashashura in https://github.com/assimp/assimp/pull/5121
* Fix Heap-buffer-overflow READ in Assimp::MD5::MD5Parser::ParseSection by @sashashura in https://github.com/assimp/assimp/pull/5122
* Fix UNKNOWN WRITE in std::__1::list<Assimp::LWO::Envelope, std::__1::allocator<Assimp::LWO::Envelope> by @sashashura in https://github.com/assimp/assimp/pull/5126
* Fix Bad-cast to Assimp::D3DS::Material from invalid vptr in Assimp::ASE::Parser::ParseLV2MaterialBlock by @sashashura in https://github.com/assimp/assimp/pull/5127
* Fix Objimported supports OBJ partially specified vertex colors by @emaame in https://github.com/assimp/assimp/pull/5140
* Fix UNKNOWN READ in Assimp::MDLImporter::ParseSkinLump_3DGS_MDL7 by @sashashura in https://github.com/assimp/assimp/pull/5128
* update utf8 from 2.3.4 to 3.2.3 by @mosfet80 in https://github.com/assimp/assimp/pull/5148
* [pyassimp] bumped pyassimp version to 5.2.5 by @severin-lemaignan in https://github.com/assimp/assimp/pull/5151
* switch to zlib 1.2.13 by @mosfet80 in https://github.com/assimp/assimp/pull/5147
* show the correct pugixml version by @mosfet80 in https://github.com/assimp/assimp/pull/5149
* Fix: disabled dangling-reference warning in gcc13 by @thegeeko in https://github.com/assimp/assimp/pull/5163
* [pyassimp] Fix passing of file extension string. by @feuerste in https://github.com/assimp/assimp/pull/5164
* Unify way to check readable blender files. by @feuerste in https://github.com/assimp/assimp/pull/5153
* [pyassimp] Replace static list of file extensions with the actually supported ones. by @feuerste in https://github.com/assimp/assimp/pull/5162
* Create licence.md by @kimkulling in https://github.com/assimp/assimp/pull/5167
* Fix IRR and IRRMESH importers by @PencilAmazing in https://github.com/assimp/assimp/pull/5166
* Fix UNKNOWN READ in Assimp::SMDImporter::ParseNodeInfo by @sashashura in https://github.com/assimp/assimp/pull/5168
* Improve binary check for gltf and gltf2. by @feuerste in https://github.com/assimp/assimp/pull/5154
* Add missing header. by @feuerste in https://github.com/assimp/assimp/pull/5172
* Extend token search flag from alpha to graph. by @feuerste in https://github.com/assimp/assimp/pull/5155
* Unify extension check for importers. by @feuerste in https://github.com/assimp/assimp/pull/5157
* Update run-cmake into sanitizer.yml by @mosfet80 in https://github.com/assimp/assimp/pull/5159
* Fix detection of `KHR_materials_specular` on glTF2 export. by @feuerste in https://github.com/assimp/assimp/pull/5176
* Handle gcs cloud storage file extensions with versioning. by @feuerste in https://github.com/assimp/assimp/pull/5156
* Remove /WX from CMakeLists for MSVC by @SirLynix in https://github.com/assimp/assimp/pull/5183
* Fix malformed irr files by @tellypresence in https://github.com/assimp/assimp/pull/5182
* Remove deprecated swig files. by @kimkulling in https://github.com/assimp/assimp/pull/5188
* Add missing rapidjson headers to `glTF2Asset.inl`. by @feuerste in https://github.com/assimp/assimp/pull/5186
* Bug fix and improvement to FBX camera field-of-view during import. by @sfjohnston in https://github.com/assimp/assimp/pull/5175
* Be more precise regarding index buffer by @paroj in https://github.com/assimp/assimp/pull/5200
* Fix UNKNOWN READ in Assimp::MDLImporter::InternReadFile_Quake1 by @sashashura in https://github.com/assimp/assimp/pull/5191
* Fix violation of the strict aliasing rule in `BaseImporter::CheckMagicToken`. by @feuerste in https://github.com/assimp/assimp/pull/5174
* Fix UNKNOWN READ in std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<ch by @sashashura in https://github.com/assimp/assimp/pull/5199
* Fix warning-as-error by @Gargaj in https://github.com/assimp/assimp/pull/5194
* Fix Invalid-free in Assimp::FBX::Scope::Scope by @sashashura in https://github.com/assimp/assimp/pull/5207
* Use correct PDB paths by @AnyOldName3 in https://github.com/assimp/assimp/pull/5213
* Ensure that the strength attribute is the same when importing or exporting gltf2 by @guguTang in https://github.com/assimp/assimp/pull/5212
* bump openddl-parser to v0.5.1 by @sashashura in https://github.com/assimp/assimp/pull/5209
* fix incorrect default for material::get with aiColor3D by @malytomas in https://github.com/assimp/assimp/pull/5161
* Collada: added import property to disable unit size scaling by @martinweber in https://github.com/assimp/assimp/pull/5193
* IRR - Fix UTF-16 file parsing (and possibly more?) by @PencilAmazing in https://github.com/assimp/assimp/pull/5192
* Add DIFFUSE_ROUGHNESS_TEXTURE for gltf2 exporter by @vulcanozz in https://github.com/assimp/assimp/pull/5170
* Semi-desert master by @kimkulling in https://github.com/assimp/assimp/pull/5216
* Update Triangulate Process by @aaronmack in https://github.com/assimp/assimp/pull/5205
* DXF: Support negative index in VERTEX by @kenichiice in https://github.com/assimp/assimp/pull/5118
* Update sanitizer.yml by @mosfet80 in https://github.com/assimp/assimp/pull/5221
* Mosfet80 clipper update by @kimkulling in https://github.com/assimp/assimp/pull/5220
* Fix: Fix compilation for clang 14.0.3 by @kimkulling in https://github.com/assimp/assimp/pull/5223
* Fix: Remove useless parameter to specify c++ lib by @kimkulling in https://github.com/assimp/assimp/pull/5225
* Bump actions/checkout from 3 to 4 by @dependabot in https://github.com/assimp/assimp/pull/5224
* Fix draco build path by @edunad in https://github.com/assimp/assimp/pull/5222
* Update GenVertexNormalsProcess.cpp by @kimkulling in https://github.com/assimp/assimp/pull/5228
* ai_epsilon bug fixed in C by @je3f0o in https://github.com/assimp/assimp/pull/5231
* Update googletest to 1.14.0 by @kimkulling in https://github.com/assimp/assimp/pull/5235
* Doc: Add wiki link by @kimkulling in https://github.com/assimp/assimp/pull/5241
* Kimkulling/prepare version 5.3.0 rc1 by @kimkulling in https://github.com/assimp/assimp/pull/4911

## New Contributors
* @ramzeng made their first contribution in https://github.com/assimp/assimp/pull/4723
* @p12tic made their first contribution in https://github.com/assimp/assimp/pull/4737
* @feishengfei made their first contribution in https://github.com/assimp/assimp/pull/4742
* @FlorianBorn71 made their first contribution in https://github.com/assimp/assimp/pull/4738
* @cwoac made their first contribution in https://github.com/assimp/assimp/pull/4745
* @slinky55 made their first contribution in https://github.com/assimp/assimp/pull/4744
* @Let0s made their first contribution in https://github.com/assimp/assimp/pull/4759
* @Beilinson made their first contribution in https://github.com/assimp/assimp/pull/4787
* @DavidKorczynski made their first contribution in https://github.com/assimp/assimp/pull/4807
* @CMDR-JohnAlex made their first contribution in https://github.com/assimp/assimp/pull/4805
* @tkoeppe made their first contribution in https://github.com/assimp/assimp/pull/4816
* @umesh-huawei made their first contribution in https://github.com/assimp/assimp/pull/4824
* @rohit-kumar-j made their first contribution in https://github.com/assimp/assimp/pull/4846
* @sfjohnston made their first contribution in https://github.com/assimp/assimp/pull/4852
* @drbct made their first contribution in https://github.com/assimp/assimp/pull/4872
* @MMory made their first contribution in https://github.com/assimp/assimp/pull/4878
* @shimaowo made their first contribution in https://github.com/assimp/assimp/pull/4886
* @lsnoel made their first contribution in https://github.com/assimp/assimp/pull/4892
* @AdamCichocki made their first contribution in https://github.com/assimp/assimp/pull/4901
* @shaunrd0 made their first contribution in https://github.com/assimp/assimp/pull/4898
* @jiannanya made their first contribution in https://github.com/assimp/assimp/pull/4945
* @shammellee made their first contribution in https://github.com/assimp/assimp/pull/4960
* @aaronmjacobs made their first contribution in https://github.com/assimp/assimp/pull/4965
* @mjunix made their first contribution in https://github.com/assimp/assimp/pull/4970
* @ockeymm123 made their first contribution in https://github.com/assimp/assimp/pull/4940
* @avaneyev made their first contribution in https://github.com/assimp/assimp/pull/4963
* @Jackie9527 made their first contribution in https://github.com/assimp/assimp/pull/4989
* @MakerOfWyverns made their first contribution in https://github.com/assimp/assimp/pull/4985
* @aniongithub made their first contribution in https://github.com/assimp/assimp/pull/5053
* @sutajo made their first contribution in https://github.com/assimp/assimp/pull/5056
* @showintime made their first contribution in https://github.com/assimp/assimp/pull/5078
* @naota29 made their first contribution in https://github.com/assimp/assimp/pull/4806
* @mwestphal made their first contribution in https://github.com/assimp/assimp/pull/5087
* @skogler made their first contribution in https://github.com/assimp/assimp/pull/5082
* @mosfet80 made their first contribution in https://github.com/assimp/assimp/pull/5125
* @Biohazard90 made their first contribution in https://github.com/assimp/assimp/pull/5133
* @emaame made their first contribution in https://github.com/assimp/assimp/pull/5140
* @thegeeko made their first contribution in https://github.com/assimp/assimp/pull/5163
* @feuerste made their first contribution in https://github.com/assimp/assimp/pull/5164
* @AnyOldName3 made their first contribution in https://github.com/assimp/assimp/pull/5213
* @guguTang made their first contribution in https://github.com/assimp/assimp/pull/5212
* @martinweber made their first contribution in https://github.com/assimp/assimp/pull/5193
* @vulcanozz made their first contribution in https://github.com/assimp/assimp/pull/5170
* @aaronmack made their first contribution in https://github.com/assimp/assimp/pull/5205
* @kenichiice made their first contribution in https://github.com/assimp/assimp/pull/5118
* @edunad made their first contribution in https://github.com/assimp/assimp/pull/5222
* @je3f0o made their first contribution in https://github.com/assimp/assimp/pull/5231

**Full Changelog**: https://github.com/assimp/assimp/compare/v5.2.5...v5.3.0

# 5.2.5
## What's Changed
* Add unittest to reproduce undefined behavior by @kimkulling in https://github.com/assimp/assimp/pull/4532
* Cleanups by @kimkulling in https://github.com/assimp/assimp/pull/4537
* Link static linkage for std_image. by @kimkulling in https://github.com/assimp/assimp/pull/4478
* fix msvc warnings-as-errors by @Gargaj in https://github.com/assimp/assimp/pull/4549
* Remove dll-export tags from static library builds. by @Underdisc in https://github.com/assimp/assimp/pull/4555
* Fix Import a specific FBX model will freeze the app #4395 by @Nor-s in https://github.com/assimp/assimp/pull/4554
* Create SECURITY.md by @kimkulling in https://github.com/assimp/assimp/pull/4565
* Pragma warnings cause build fail with MinGW by @ethaninfinity in https://github.com/assimp/assimp/pull/4564
* Fixed FBXConverter build error when using double precision by @matthewclendening in https://github.com/assimp/assimp/pull/4546
* Fix  possible nullptr exception by @kimkulling in https://github.com/assimp/assimp/pull/4567
* [Experimental] New skeleton container for bones by @kimkulling in https://github.com/assimp/assimp/pull/4552
* Add support for GCC v12 by @PercentBoat4164 in https://github.com/assimp/assimp/pull/4578
* Remove unused variable. by @kovacsv in https://github.com/assimp/assimp/pull/4584
* Infinite loop on bad import files by @tanolino in https://github.com/assimp/assimp/pull/4534
* Utilize AI_CONFIG_IMPORT_REMOVE_EMPTY_BONES flag for Collada meshes. by @onurtore in https://github.com/assimp/assimp/pull/4585
* Fix Windows 32-bit builds by @Gargaj in https://github.com/assimp/assimp/pull/4581
* Fix GNUC check on Windows by @hgdagon in https://github.com/assimp/assimp/pull/4542
* Update the name of the package by @kimkulling in https://github.com/assimp/assimp/pull/4595
* Kimkulling/fix invalid opengex token match by @kimkulling in https://github.com/assimp/assimp/pull/4596
* Disable build for tools per default by @kimkulling in https://github.com/assimp/assimp/pull/4598
* Use mingw.include by @kimkulling in https://github.com/assimp/assimp/pull/4601
* Fix a memory leak by @kimkulling in https://github.com/assimp/assimp/pull/4605
* Try to fix issue 4238 by @kimkulling in https://github.com/assimp/assimp/pull/4609
* Fix nested animation name being lost in Collada by @luca-della-vedova in https://github.com/assimp/assimp/pull/4597
* chore: Included GitHub actions in the dependabot config by @naveensrinivasan in https://github.com/assimp/assimp/pull/4569
* Fix possible bad_alloc exception for an invalid file by @kimkulling in https://github.com/assimp/assimp/pull/4614
* Bump JesseTG/rm from 1.0.2 to 1.0.3 by @dependabot in https://github.com/assimp/assimp/pull/4613
* Bump actions/cache from 2 to 3 by @dependabot in https://github.com/assimp/assimp/pull/4612
* Kimkulling/fix texture loading 3MF, reladed issue-4568 by @kimkulling in https://github.com/assimp/assimp/pull/4619
* Bump actions/upload-artifact from 2 to 3 by @dependabot in https://github.com/assimp/assimp/pull/4610
* Bump actions/checkout from 2 to 3 by @dependabot in https://github.com/assimp/assimp/pull/4611
* I ran into an error while processing colored binary stl. Just a type but it better be fixed by @blackhorse-reddog in https://github.com/assimp/assimp/pull/4541
* Remove assertion test by @kimkulling in https://github.com/assimp/assimp/pull/4627
* Fix memory leak in D3MFOpcPackage by @kimkulling in https://github.com/assimp/assimp/pull/4629
* Fix typo in installation instructions for ubuntu. by @hectorpiteau in https://github.com/assimp/assimp/pull/4621
* Build fix for compiling against minizip. by @robertosfield in https://github.com/assimp/assimp/pull/4631
* Fix stl for over 4 GB by @tanolino in https://github.com/assimp/assimp/pull/4630
* Fix uninitialized variable. by @kimkulling in https://github.com/assimp/assimp/pull/4642
* Fixes Crash in Assimp::ObjFileMtlImporter::getFloatValue by @sashashura in https://github.com/assimp/assimp/pull/4647
* Fixes Heap-buffer-overflow in Assimp::ObjFileParser::getFace by @sashashura in https://github.com/assimp/assimp/pull/4646
* Fixes Heap-buffer-overflow in std::__1::basic_string<char, std::__1::… by @sashashura in https://github.com/assimp/assimp/pull/4645
* Fixes Heap-use-after-free in Assimp::DXFImporter::ExpandBlockReferences by @sashashura in https://github.com/assimp/assimp/pull/4644
* Fixes Heap-buffer-overflow in SuperFastHash by @sashashura in https://github.com/assimp/assimp/pull/4643
* ColladaParser - Store sid in mSID field by @luca-della-vedova in https://github.com/assimp/assimp/pull/4538
* Fix mingw include in assimp_cmd.rc by @Koekto-code in https://github.com/assimp/assimp/pull/4635
* Fix warnings that are causing build fails with specific build flags by @enginmanap in https://github.com/assimp/assimp/pull/4632
* Update version tag by @waebbl in https://github.com/assimp/assimp/pull/4656
* Improvements and optimizations for the obj-parsers. by @kimkulling in https://github.com/assimp/assimp/pull/4666
* Experiment: try to enable parallel build by @kimkulling in https://github.com/assimp/assimp/pull/4669
* Fixed typo by @Fiskmans in https://github.com/assimp/assimp/pull/4668
* Use  [[fallthrough]]; to mark whished fallthroughs by @kimkulling in https://github.com/assimp/assimp/pull/4673
* Kimkulling/do not add dot when the extension is empty issue 4670 by @kimkulling in https://github.com/assimp/assimp/pull/4674
* Fixes Heap-buffer-overflow READ in Assimp::ASE::Parser::ParseLV1SoftSkinBlock by @sashashura in https://github.com/assimp/assimp/pull/4680
* Use unqualified uint32_t everywhere in FBXBinaryTokenizer by @villevoutilainen in https://github.com/assimp/assimp/pull/4678
* Fix problems setting DirectX_LIBRARY by @Koekto-code in https://github.com/assimp/assimp/pull/4681
* Added support for more bone weights in GLTF2  by @Promit in https://github.com/assimp/assimp/pull/4453
* (Mostly) Blender fixes by @turol in https://github.com/assimp/assimp/pull/4679
* [WIP] Use ai_Real to write correct accuracy by @kimkulling in https://github.com/assimp/assimp/pull/4697
* SMD fixes by @turol in https://github.com/assimp/assimp/pull/4699
* Remove exception on glTF 2.0 loading by @vkaytsanov in https://github.com/assimp/assimp/pull/4693
* Fix out-of-bounds reads in X3D importer by @turol in https://github.com/assimp/assimp/pull/4701
* Apply the modernize-use-emplace clang-tidy rule by @Skylion007 in https://github.com/assimp/assimp/pull/4700
* The Wrong object is created here! by @JG-Adams in https://github.com/assimp/assimp/pull/4704
* [WIP] Code cleanup and some new unittests for edge-cases. by @kimkulling in https://github.com/assimp/assimp/pull/4705
* clang-tidy: explicitly default all empty ctors and dtors by @Skylion007 in https://github.com/assimp/assimp/pull/4703
* fix vertices being joined duplicating weights by @Gargaj in https://github.com/assimp/assimp/pull/4707
* add missing light data to assbin import/export by @Gargaj in https://github.com/assimp/assimp/pull/4702
* Fix aiBone.mOffsetMatrix documentation by @ChrisBlueStone in https://github.com/assimp/assimp/pull/4713
* Minor obj export bugfix by @HiMemX in https://github.com/assimp/assimp/pull/4716
* Kimkulling/cleanup after reviewing by @kimkulling in https://github.com/assimp/assimp/pull/4715

## New Contributors
* @Underdisc made their first contribution in https://github.com/assimp/assimp/pull/4555
* @Nor-s made their first contribution in https://github.com/assimp/assimp/pull/4554
* @ethaninfinity made their first contribution in https://github.com/assimp/assimp/pull/4564
* @matthewclendening made their first contribution in https://github.com/assimp/assimp/pull/4546
* @PercentBoat4164 made their first contribution in https://github.com/assimp/assimp/pull/4578
* @onurtore made their first contribution in https://github.com/assimp/assimp/pull/4585
* @luca-della-vedova made their first contribution in https://github.com/assimp/assimp/pull/4597
* @naveensrinivasan made their first contribution in https://github.com/assimp/assimp/pull/4569
* @dependabot made their first contribution in https://github.com/assimp/assimp/pull/4613
* @blackhorse-reddog made their first contribution in https://github.com/assimp/assimp/pull/4541
* @hectorpiteau made their first contribution in https://github.com/assimp/assimp/pull/4621
* @robertosfield made their first contribution in https://github.com/assimp/assimp/pull/4631
* @sashashura made their first contribution in https://github.com/assimp/assimp/pull/4647
* @Koekto-code made their first contribution in https://github.com/assimp/assimp/pull/4635
* @enginmanap made their first contribution in https://github.com/assimp/assimp/pull/4632
* @waebbl made their first contribution in https://github.com/assimp/assimp/pull/4656
* @Fiskmans made their first contribution in https://github.com/assimp/assimp/pull/4668
* @vkaytsanov made their first contribution in https://github.com/assimp/assimp/pull/4693
* @JG-Adams made their first contribution in https://github.com/assimp/assimp/pull/4704
* @ChrisBlueStone made their first contribution in https://github.com/assimp/assimp/pull/4713
* @HiMemX made their first contribution in https://github.com/assimp/assimp/pull/4716

**Full Changelog**: https://github.com/assimp/assimp/compare/v5.2.4...v5.2.5

# 5.2.4
## What's Changed
* Use static runtime only when the option is selected by @kimkulling in https://github.com/assimp/assimp/pull/4464
* Fix compile error: When enabling macro ASSIMP_DOUBLE_PRECISION by @xiaohunqupo in https://github.com/assimp/assimp/pull/4473
* Detect Roughness factor exported by Blender by @ifiddynine in https://github.com/assimp/assimp/pull/4466
* Updated Android build script by @mpcarlos87 in https://github.com/assimp/assimp/pull/4454
* Prevent nullptr access to normals-array in bitangent computation by @jakrams in https://github.com/assimp/assimp/pull/4463
* Avoid setting PBR properties when they are not found on mtl file by @sacereda in https://github.com/assimp/assimp/pull/4440
* Fix ogre xml serializer by @HadesD in https://github.com/assimp/assimp/pull/4488
* Fix draco building error when import assimp with cmake add_subdirectory #3663 by @lgmcode in https://github.com/assimp/assimp/pull/4483
* FbxConverter: update bone mOffsetMatrix  by @Mykhailo418 in https://github.com/assimp/assimp/pull/4490
* Some Windows/MSYS2-specific fixes  by @hgdagon in https://github.com/assimp/assimp/pull/4481
* Document fuzz folder by @kimkulling in https://github.com/assimp/assimp/pull/4501
* Fix out-of-range access in ASE-Parser by @kimkulling in https://github.com/assimp/assimp/pull/4502
* Disable assertion tests floating point against inf for Intel oneAPI by @kimkulling in https://github.com/assimp/assimp/pull/4507
* Delete README by @kimkulling in https://github.com/assimp/assimp/pull/4506
* Rename TextureTypeToString() to aiTextureTypeToString() by @umlaeute in https://github.com/assimp/assimp/pull/4512
* Fixed library names for MinGW/MSYS2 by @hgdagon in https://github.com/assimp/assimp/pull/4508
* Update pugixml dependency to  v1.12.1 by @ALittleStardust in https://github.com/assimp/assimp/pull/4514
* Add an option to treat warnings as errors by @hgdagon in https://github.com/assimp/assimp/pull/4509
* Minor updates to ASSIMP Viewer by @hgdagon in https://github.com/assimp/assimp/pull/4511
* Add badge to show open issue in percentage by @kimkulling in https://github.com/assimp/assimp/pull/4524
* Clang-Tidy performance fixes (make values const-ref where needed). by @Skylion007 in https://github.com/assimp/assimp/pull/4525
* MMD (pmx) fixes by @RedSkittleFox in https://github.com/assimp/assimp/pull/4484
* Resource script updates by @hgdagon in https://github.com/assimp/assimp/pull/4510
* Accelerate the Merge vertex post processing step by @motazmuhammad in https://github.com/assimp/assimp/pull/4527

## New Contributors
* @mpcarlos87 made their first contribution in https://github.com/assimp/assimp/pull/4454
* @HadesD made their first contribution in https://github.com/assimp/assimp/pull/4488
* @hgdagon made their first contribution in https://github.com/assimp/assimp/pull/4481
* @ALittleStardust made their first contribution in https://github.com/assimp/assimp/pull/4514
* @RedSkittleFox made their first contribution in https://github.com/assimp/assimp/pull/4484
* @motazmuhammad made their first contribution in https://github.com/assimp/assimp/pull/4527

**Full Changelog**: https://github.com/assimp/assimp/compare/v5.2.3...v5.2.4

# 5.2.3
## What's Changed
* Show warning when assimp_viewer cannot be build on target platform by @kimkulling in https://github.com/assimp/assimp/pull/4402
* Fix ordering of member initialization by @kimkulling in https://github.com/assimp/assimp/pull/4408
* Fix possible negative array access by @kimkulling in https://github.com/assimp/assimp/pull/4415
* Expose the original OBJ "illum" value by @TerenceRussell in https://github.com/assimp/assimp/pull/4409
* Optimize the problem of excessive memory allocation in FBX import by @SolaToucher in https://github.com/assimp/assimp/pull/4416
* Update version of Hunter to v0.24.0 that supports VS 2022 by @rbsheth in https://github.com/assimp/assimp/pull/4417
* Smartday master by @kimkulling in https://github.com/assimp/assimp/pull/4427
* update LWO importer(available lwo3) by @smartday in https://github.com/assimp/assimp/pull/4020
* Reinstate a deprecated gltfpbr macro: AI_MATKEY_GLTF_PBRSPECULARGLOSSINESS by @RichardTea in https://github.com/assimp/assimp/pull/4203
* Fix parsing OBJ with free-form curve/surface body statements by @JaroslavPribyl in https://github.com/assimp/assimp/pull/4403
* Fix missing members and do some small refactorings. by @kimkulling in https://github.com/assimp/assimp/pull/4432
* Fix 'i >= 0' always true bug by @karjonas in https://github.com/assimp/assimp/pull/4434
* Update AI_TEXTURE_TYPE_MAX by @RichardTea in https://github.com/assimp/assimp/pull/4421
* Fix getting anisotropy in obj by @youkeyao in https://github.com/assimp/assimp/pull/4423
* glTF2: Metallic roughness split by @sacereda in https://github.com/assimp/assimp/pull/4425
* Add properties information on assimp info command line by @sacereda in https://github.com/assimp/assimp/pull/4426
* Added missing ObjMaterial.h to CMakeLists by @TerenceRussell in https://github.com/assimp/assimp/pull/4431
* Update version in doxy-config by @kimkulling in https://github.com/assimp/assimp/pull/4441
* add ifndef guard for resolve to fails to compile by @jaefunk in https://github.com/assimp/assimp/pull/4437
* Add USE_STATIC_CRT option by @EYHN in https://github.com/assimp/assimp/pull/4444
* Fix nullptr dereferencing by @kimkulling in https://github.com/assimp/assimp/pull/4446
* Fix stack-overflow in MDLLoader by @kimkulling in https://github.com/assimp/assimp/pull/4448
* GLTF2 attribute name/parse bug fix by @Promit in https://github.com/assimp/assimp/pull/4451

## New Contributors
* @SolaToucher made their first contribution in https://github.com/assimp/assimp/pull/4416
* @smartday made their first contribution in https://github.com/assimp/assimp/pull/4020
* @JaroslavPribyl made their first contribution in https://github.com/assimp/assimp/pull/4403
* @karjonas made their first contribution in https://github.com/assimp/assimp/pull/4434
* @EYHN made their first contribution in https://github.com/assimp/assimp/pull/4444

**Full Changelog**: https://github.com/assimp/assimp/compare/v5.2.2...v5.2.3

# 5.2.2
## What's Changed
* Fix missing include for size_t by @malytomas in https://github.com/assimp/assimp/pull/4380
* Kimkulling/introduce compression by @kimkulling in https://github.com/assimp/assimp/pull/4381
* Refactoring: add usage of ai_epsilon to FBX-Importer. by @kimkulling in https://github.com/assimp/assimp/pull/4387
* CMake: Fix Assimp target install rule fully specifying component by @jcfr in https://github.com/assimp/assimp/pull/4391
* Fix stat for 32-bit Linux by @kimkulling in https://github.com/assimp/assimp/pull/4398
* Update build script to fit "Visual Studio 16 2019" Generator by @BA7LYA in https://github.com/assimp/assimp/pull/4394
* Update the calculation and orthogonalization for bitangent by @youkeyao in https://github.com/assimp/assimp/pull/4397
* Update CMakeLists.txt by @kimkulling in https://github.com/assimp/assimp/pull/4399
* Added support for "map_Bump -bm" by @TerenceRussell in https://github.com/assimp/assimp/pull/4393

## New Contributors
* @jcfr made their first contribution in https://github.com/assimp/assimp/pull/4391
* @BA7LYA made their first contribution in https://github.com/assimp/assimp/pull/4394
* @youkeyao made their first contribution in https://github.com/assimp/assimp/pull/4397
* @TerenceRussell made their first contribution in https://github.com/assimp/assimp/pull/4393

**Full Changelog**: https://github.com/assimp/assimp/compare/v5.2.0...v5.2.2

# 5.2.1
## What's Changed
* ASE: Fix warning unreachable code by @kimkulling in https://github.com/assimp/assimp/pull/4355
* HMP: Fix override during copying position data by @kimkulling in https://github.com/assimp/assimp/pull/4359
* use fully qualified namespace in byteswap macros by @umlaeute in https://github.com/assimp/assimp/pull/4367
* fix compilation with clangcl on windows by @malytomas in https://github.com/assimp/assimp/pull/4366
* Delete .travis.sh by @kimkulling in https://github.com/assimp/assimp/pull/4371
* Update ccpp.yml by @kimkulling in https://github.com/assimp/assimp/pull/4378
* LWO: validate enum value before parsing it by @kimkulling in https://github.com/assimp/assimp/pull/4376


**Full Changelog**: https://github.com/assimp/assimp/compare/5.2.0...v5.2.1

# 5.2.0
## What's Changed
* Update copyrights by @kimkulling in https://github.com/assimp/assimp/pull/4335
* Fix imported target include directory by @lopsided98 in https://github.com/assimp/assimp/pull/4337
* Assimp Patch Android LTS NDK 23 Fix by @danoli3 in https://github.com/assimp/assimp/pull/4330
* Allow dlclose of so library by avoiding unique symbols. by @TThulesen in https://github.com/assimp/assimp/pull/4204
* Move Base64 encode/decode functionality to the common folder by @kovacsv in https://github.com/assimp/assimp/pull/4336
* Locale independent meter scale by @tanolino in https://github.com/assimp/assimp/pull/4323
* add Inter-Quake Model (IQM) Importer by @Garux in https://github.com/assimp/assimp/pull/4265
* Collada: Read all instance_material child nodes by @jsigrist in https://github.com/assimp/assimp/pull/4339
* Krishty new file detection by @kimkulling in https://github.com/assimp/assimp/pull/4342
* ASE: Fix material parsing by @kimkulling in https://github.com/assimp/assimp/pull/4346
* IFC Reading: Fix opening reading. by @bensewell in https://github.com/assimp/assimp/pull/4344
* CMAKE: Respect top-level CMAKE_*_OUTPUT_DIRECTORY variables by @leonvictor in https://github.com/assimp/assimp/pull/4338
* Udate version to 5.2.0 by @kimkulling in https://github.com/assimp/assimp/pull/4353

## New Contributors
* @lopsided98 made their first contribution in https://github.com/assimp/assimp/pull/4337
* @danoli3 made their first contribution in https://github.com/assimp/assimp/pull/4330
* @TThulesen made their first contribution in https://github.com/assimp/assimp/pull/4204
* @jsigrist made their first contribution in https://github.com/assimp/assimp/pull/4339
* @bensewell made their first contribution in https://github.com/assimp/assimp/pull/4344
* @leonvictor made their first contribution in https://github.com/assimp/assimp/pull/4338

**Full Changelog**: https://github.com/assimp/assimp/compare/v5.1.6...5.2.0

# 5.1.6
## What's Changed
* Add link to used enum for a better understandability by @kimkulling in https://github.com/assimp/assimp/pull/4321
* Fix fuzzer crashes by @alpire in https://github.com/assimp/assimp/pull/4324
* Fix nullptr-dereferencing by @kimkulling in https://github.com/assimp/assimp/pull/4328
* Fix bone fitted check in gltf2 exporter by @vpzomtrrfrt in https://github.com/assimp/assimp/pull/4318
* Update to 5.1.6 by @kimkulling in https://github.com/assimp/assimp/pull/4333

## New Contributors
* @vpzomtrrfrt made their first contribution in https://github.com/assimp/assimp/pull/4318

**Full Changelog**: https://github.com/assimp/assimp/compare/v5.1.5...v5.1.6

# 5.1.5
## What's Changed
* Make sure no overflow can happen by @kimkulling in https://github.com/assimp/assimp/pull/4282
* LWS-Import: Avoid access to empty string token by @kimkulling in https://github.com/assimp/assimp/pull/4283
* MDL: Do not try to copy empty embedded texture by @kimkulling in https://github.com/assimp/assimp/pull/4284
* Add console progresshandler by @kimkulling in https://github.com/assimp/assimp/pull/4293
* CMake: Replace CMAKE_COMPILER_IS_MINGW by MINGW by @SirLynix in https://github.com/assimp/assimp/pull/4311
* fix fbx import metalness by @VyacheslavVanin in https://github.com/assimp/assimp/pull/4259
* RFC: BlenderScene: use explicit namespace instead of using namespace by @pseiderer in https://github.com/assimp/assimp/pull/4314
* Support PBR properties/maps in Obj importer by @errissa in https://github.com/assimp/assimp/pull/4272

## New Contributors
* @SirLynix made their first contribution in https://github.com/assimp/assimp/pull/4311
* @VyacheslavVanin made their first contribution in https://github.com/assimp/assimp/pull/4259
* @errissa made their first contribution in https://github.com/assimp/assimp/pull/4272

**Full Changelog**: https://github.com/assimp/assimp/compare/v5.1.4...v5.1.5

# 5.1.4
## What's Changed
* Fix division by zero in PointInTriangle2D by @kimkulling in https://github.com/assimp/assimp/pull/4245
* Fix nullptr dereferencing from std::shared_ptr by @kimkulling in https://github.com/assimp/assimp/pull/4249
* Update HMPLoader.cpp by @kimkulling in https://github.com/assimp/assimp/pull/4250
* Revert "FBXConverter : Fix timescales of FBX animations" by @inhosens in https://github.com/assimp/assimp/pull/4247
* Use correct XmlParser-methods and add some missing casts by @kimkulling in https://github.com/assimp/assimp/pull/4253
* Bug: Export crashes when any of the meshes contains texture coordinate names #4243 by @kovacsv in https://github.com/assimp/assimp/pull/4244
* Bugfix/import crashes by @umlaeute in https://github.com/assimp/assimp/pull/4226
* Fix a typo in the Visual-Studio Dll-Versions by @kimkulling in https://github.com/assimp/assimp/pull/4260
* Enable C++11 and C99 by @kimkulling in https://github.com/assimp/assimp/pull/4261
* Fixed cmake error: No known features for C compiler when using the assimp library from another project by @rumgot in https://github.com/assimp/assimp/pull/4256
* fix test/models/3DS/IMAGE1.bmp: is jpg by @Garux in https://github.com/assimp/assimp/pull/4264
* Fix compile error when ASSIMP_BUILD_NO_X3D_IMPORTER is define. by @RivIs-sssa01 in https://github.com/assimp/assimp/pull/4263
* Update version to 5.1.4 by @kimkulling in https://github.com/assimp/assimp/pull/4266

## New Contributors
* @rumgot made their first contribution in https://github.com/assimp/assimp/pull/4256
* @RivIs-sssa01 made their first contribution in https://github.com/assimp/assimp/pull/4263

**Full Changelog**: https://github.com/assimp/assimp/compare/v5.1.3...v5.1.4

# 5.1.3
## What's Changed
* Update blender importer to work with Blender 2.8+ files by @PencilAmazing in https://github.com/assimp/assimp/pull/4193
* Added checks for out of bounds data access/writing by @ms-maxvollmer in https://github.com/assimp/assimp/pull/4211
* Interpolate euler rotations for quaternion animations by @inhosens in https://github.com/assimp/assimp/pull/4216
* Fix file-extension check for X3D-files by @umlaeute in https://github.com/assimp/assimp/pull/4217
* Update CMakeLists.txt by @kimkulling in https://github.com/assimp/assimp/pull/4219

## New Contributors
* @PencilAmazing made their first contribution in https://github.com/assimp/assimp/pull/4193

**Full Changelog**: https://github.com/assimp/assimp/compare/v5.1.2...v5.1.3

# 5.1.2
## What's Changed
* Use adviced c++ flag to supress warning on mingw by @Rodousse in https://github.com/assimp/assimp/pull/4196
* Fixed an incorrect indeiciesType in the glTF2 sparse accessor. by @ruyo in https://github.com/assimp/assimp/pull/4195
* Prevent out-of-range memory writes by sparse accessors by @jakrams in https://github.com/assimp/assimp/pull/4207
* Delete test/models/3DS/UVTransformTest directory by @kimkulling in https://github.com/assimp/assimp/pull/4212

## New Contributors
* @Rodousse made their first contribution in https://github.com/assimp/assimp/pull/4196
* @jakrams made their first contribution in https://github.com/assimp/assimp/pull/4207

**Full Changelog**: https://github.com/assimp/assimp/compare/v5.1.1...v5.1.2

# 5.1.1
## What's Changed
* Delete irrXML_note.txt by @kimkulling in https://github.com/assimp/assimp/pull/4180
* Update script_x64.iss by @kimkulling in https://github.com/assimp/assimp/pull/4181
* Do not build ziplib when 3MF exporter is disabled. by @jdumas in https://github.com/assimp/assimp/pull/4173
* Collada: Read value, not attribute by @RichardTea in https://github.com/assimp/assimp/pull/4187
* Redefine deprecated glTF-specific PBR material macros by @RichardTea in https://github.com/assimp/assimp/pull/4184
* On Windows/mingw in shared build mode append '-SOVERSION' to DLL base file name by @rhabacker in https://github.com/assimp/assimp/pull/4185

## New Contributors
* @jdumas made their first contribution in https://github.com/assimp/assimp/pull/4173
* @rhabacker made their first contribution in https://github.com/assimp/assimp/pull/4185

**Full Changelog**: https://github.com/assimp/assimp/compare/v5.1.0...v5.1.1

# 5.1.0
## What's Changed
* Fix: Mismatched new/free in gltf2 importer (fixes #2668) by @theakman2 in https://github.com/assimp/assimp/pull/2669
* fix issue textureCoords are messed up for multiple uvset  by @thomasbiang in https://github.com/assimp/assimp/pull/2663
* Add vcpkg installation instructions by @grdowns in https://github.com/assimp/assimp/pull/2680
* Fix: Wrong aiAnimation::mTicksPerSecond for gltf2 imports (fixes #2662) by @theakman2 in https://github.com/assimp/assimp/pull/2666
* gltf2.0 importer - Support for mesh morph animations added. by @vcebollada in https://github.com/assimp/assimp/pull/2675
* Add AppVeyor build VS2019 by @escherstair in https://github.com/assimp/assimp/pull/2692
* Enginmanap issue 2693 by @kimkulling in https://github.com/assimp/assimp/pull/2706
* Findassimp.cmake: add hint for lib search path for Linux by @feniksa in https://github.com/assimp/assimp/pull/2699
* Support Apple naming conventions - shared library by @mdinim in https://github.com/assimp/assimp/pull/2677
* Cleanup headers by @kimkulling in https://github.com/assimp/assimp/pull/2708
* ColladaExporter: improve name/id handling by @TGEnigma in https://github.com/assimp/assimp/pull/2690
* Fix CMake import by @jherico in https://github.com/assimp/assimp/pull/2722
* fix vs2013 build by @ardenpm in https://github.com/assimp/assimp/pull/2715
* Fix gltf importer crash by @MalcolmTyrrell in https://github.com/assimp/assimp/pull/2704
* Update appveyor.yml by @kimkulling in https://github.com/assimp/assimp/pull/2723
* Fix FBXConverter: use proper 64-bit constant by @ffontaine in https://github.com/assimp/assimp/pull/2697
* Fix #2685 - library compiled by MinGW is unusable in MinGW. by @Zalewa in https://github.com/assimp/assimp/pull/2698
* Update assimp legal and version by @RichardTea in https://github.com/assimp/assimp/pull/2709
* Collada ZAE texture loading by @RichardTea in https://github.com/assimp/assimp/pull/2711
* Fix multiple deallocation of memory for texture data. by @ardenpm in https://github.com/assimp/assimp/pull/2717
* glTF2 fix glossinessFactor being put into the wrong object on export by @ardenpm in https://github.com/assimp/assimp/pull/2725
* Fix lower casing material names in 3DS importer by @quanterion in https://github.com/assimp/assimp/pull/2726
* Implemented armature lookup and updated FBX importer to properly support this by @RevoluPowered in https://github.com/assimp/assimp/pull/2731
* Update CXMLReaderImpl.h by @tanolino in https://github.com/assimp/assimp/pull/2744
* FBX orphan embedded textures fix by @muxanickms in https://github.com/assimp/assimp/pull/2741
* Clang format added for code reformatting by @RevoluPowered in https://github.com/assimp/assimp/pull/2728
* Added M3D format support by @bztsrc in https://github.com/assimp/assimp/pull/2736
* Fix for exporting fbx bigger than 2GB by @muxanickms in https://github.com/assimp/assimp/pull/2751
* closes https://github.com/assimp/assimp/issues/2684: normalize path by @kimkulling in https://github.com/assimp/assimp/pull/2757
* closes https://github.com/assimp/assimp/issues/1320: make sure build … by @kimkulling in https://github.com/assimp/assimp/pull/2759
* BUG - use noexcept only for C++11 and more by @jcarpent in https://github.com/assimp/assimp/pull/2758
* Kimkullig dev by @kimkulling in https://github.com/assimp/assimp/pull/2764
* Update .gitignore by @thewoz in https://github.com/assimp/assimp/pull/2748
* closes https://github.com/assimp/assimp/issues/2733: update of zlip t… by @kimkulling in https://github.com/assimp/assimp/pull/2771
* Update Readme.md by @kimkulling in https://github.com/assimp/assimp/pull/2775
* Updated M3D SDK and some fixes by @bztsrc in https://github.com/assimp/assimp/pull/2766
* ColladaParser: fix handling of empty XML-elements by @da-raf in https://github.com/assimp/assimp/pull/2727
* Add function aiGetVersionPatch() to be able to display Assimp version as in Git tags by @LoicFr in https://github.com/assimp/assimp/pull/2780
* MinGW support, profiling and signed error code by @bztsrc in https://github.com/assimp/assimp/pull/2783
* Migenius fix doubleexport by @ardenpm in https://github.com/assimp/assimp/pull/2782
* Migenius fix khrtexturetransform by @ardenpm in https://github.com/assimp/assimp/pull/2787
* Update CMakeLists.txt by @kimkulling in https://github.com/assimp/assimp/pull/2789
* Migenius fix dracocrash by @ardenpm in https://github.com/assimp/assimp/pull/2792
* avoid weighting vertex repeatedly when joining identical vertices by @thomasbiang in https://github.com/assimp/assimp/pull/2752
* Fix for memory leak in glTF2 Importer if an exception has been thrown by @muxanickms in https://github.com/assimp/assimp/pull/2770
* Error string of Importer should contain a message in case of an exception by @muxanickms in https://github.com/assimp/assimp/pull/2769
* Fix glTF validation error related to accessor min and max values by @coryf in https://github.com/assimp/assimp/pull/2779
* Remove cout calls from FBX, LWO and B3D by @RichardTea in https://github.com/assimp/assimp/pull/2802
* Modeller meta data by @MalcolmTyrrell in https://github.com/assimp/assimp/pull/2810
* MSVC: Disable optimisations in debug build by @RichardTea in https://github.com/assimp/assimp/pull/2801
* Some cleanup of M3D support by @RichardTea in https://github.com/assimp/assimp/pull/2805
* closes https://github.com/assimp/assimp/issues/2809: fix crash for sp… by @kimkulling in https://github.com/assimp/assimp/pull/2814
* Backfacing odd negative scale 2383 by @RichardTea in https://github.com/assimp/assimp/pull/2818
* Add a support for 3DSMax Physically Based Materials for FBX format by @muxanickms in https://github.com/assimp/assimp/pull/2827
* Fix texcoord by @MalcolmTyrrell in https://github.com/assimp/assimp/pull/2829
* Update M3D ASCII exporter ident and extension by @RichardTea in https://github.com/assimp/assimp/pull/2819
* Added CMake option to set the compiler warning to max (-Wall, /W4). Off by default by @dylankenneally in https://github.com/assimp/assimp/pull/2776
* Fix typos by @MalcolmTyrrell in https://github.com/assimp/assimp/pull/2841
* Python fixes: ctypes declarations and cosmetics by @umlaeute in https://github.com/assimp/assimp/pull/2813
* Made the CMake config more submodule friendly by @apanteleev in https://github.com/assimp/assimp/pull/2839
* MSVC workarounds by @bztsrc in https://github.com/assimp/assimp/pull/2825
* Update SMDLoader.cpp by @9739654 in https://github.com/assimp/assimp/pull/2843
* Added support to load Half-Life 1 MDL files. by @malortie in https://github.com/assimp/assimp/pull/2838
* Gltf import bug fix by @yunqiangshanbill in https://github.com/assimp/assimp/pull/2853
* Fixed UV coordinate swapped twice in big endian. by @malortie in https://github.com/assimp/assimp/pull/2858
* Collada and glTF modeller metadata by @RichardTea in https://github.com/assimp/assimp/pull/2820
* fix: Don't combine Collada animations when channels are shared by @felipeek in https://github.com/assimp/assimp/pull/2855
* Fix PlyExporter to support faces with 0 vertices by @Dunni in https://github.com/assimp/assimp/pull/2863
* Fix possible null pointer exception on scene metadata when exporting a glTF2 file by @LoicFr in https://github.com/assimp/assimp/pull/2865
* Update VertexTriangleAdjacency.cpp by @kimkulling in https://github.com/assimp/assimp/pull/2867
* Update glTF2Importer.cpp by @kimkulling in https://github.com/assimp/assimp/pull/2866
* Revert 3_bananas.amf.7z to working version by @turol in https://github.com/assimp/assimp/pull/2870
* Update Readme.md by @kimkulling in https://github.com/assimp/assimp/pull/2868
* Revert more broken test models to working versions by @turol in https://github.com/assimp/assimp/pull/2875
* Add MD2 importer unit tests by @turol in https://github.com/assimp/assimp/pull/2877
* Revert broken dwarf.cob test file and add MD5 and COB unit tests by @turol in https://github.com/assimp/assimp/pull/2891
* Fix: gltf exporting memory leak by @runforu in https://github.com/assimp/assimp/pull/2892
* Revert broken test files and improve error messages in Half-Life MDL loader by @turol in https://github.com/assimp/assimp/pull/2904
* Suppressed MSVC++ warnings C4244 and C4267 by @matt77hias in https://github.com/assimp/assimp/pull/2905
* [MDL] Updated header to use when checking file size. (Redone) by @malortie in https://github.com/assimp/assimp/pull/2908
* Update .travis.yml by @kimkulling in https://github.com/assimp/assimp/pull/2887
* Fixed memory leak in MDL importer. by @malortie in https://github.com/assimp/assimp/pull/2927
* Removed name of unreferenced local variable in catch block by @matt77hias in https://github.com/assimp/assimp/pull/2910
* Add all shipped .blend files to unit tests by @turol in https://github.com/assimp/assimp/pull/2907
* Revert broken test files by @turol in https://github.com/assimp/assimp/pull/2932
* Revert broken .X test model to working version by @turol in https://github.com/assimp/assimp/pull/2934
* Add more unit tests by @turol in https://github.com/assimp/assimp/pull/2936
* Fix Assimp patch version to match the last bug fix release by @LoicFr in https://github.com/assimp/assimp/pull/2884
* Fix memory leak in .X importer by @turol in https://github.com/assimp/assimp/pull/2940
* Updated copyright dates. by @malortie in https://github.com/assimp/assimp/pull/2933
* [MDL HL1] Fixed children nodes not deleted. by @malortie in https://github.com/assimp/assimp/pull/2928
* Updated places where read/write for achFormatHint (Redone). by @malortie in https://github.com/assimp/assimp/pull/2948
* Added support to load HL1 MDL external texture files directly. by @malortie in https://github.com/assimp/assimp/pull/2952
* Remove explicit setting of macos install_name by @RichardTea in https://github.com/assimp/assimp/pull/2962
* Revert image files corrupted by a8a1ca9 by @tellypresence in https://github.com/assimp/assimp/pull/2960
* Uniformized error codes (return values) in assimp_cmd. by @malortie in https://github.com/assimp/assimp/pull/2958
* [MDL HL1] Fixed wrong texture format used. by @malortie in https://github.com/assimp/assimp/pull/2943
* Refactored Assbin exporter and assimp_cmd binary serialization functions. by @malortie in https://github.com/assimp/assimp/pull/2967
* closes https://github.com/assimp/assimp/issues/1592:  by @kimkulling in https://github.com/assimp/assimp/pull/2968
* Fix version revision formatting in glTF metadata by @LoicFr in https://github.com/assimp/assimp/pull/2941
* Update EmbedTexturesProcess.cpp by @kimkulling in https://github.com/assimp/assimp/pull/2880
* Refactored Assxml exporter and dump xml writer. by @malortie in https://github.com/assimp/assimp/pull/2972
* Added missing checks for tempData and uvIndices sizes in all cases by @ms-maxvollmer in https://github.com/assimp/assimp/pull/2970
* Fixed mValues allocated twice in SceneCombiner.cpp. by @malortie in https://github.com/assimp/assimp/pull/2978
* Fixed a bunch of clang warnings by @hbina in https://github.com/assimp/assimp/pull/2976
* Removed unnecessary checks that may result in false positives rejecting valid models. by @ms-maxvollmer in https://github.com/assimp/assimp/pull/2984
* Renamed WriteDumb.cpp to WriteDump.cpp by @malortie in https://github.com/assimp/assimp/pull/2974
* [MDL HL1] Removed texture dimensions limitation. by @malortie in https://github.com/assimp/assimp/pull/2942
* Update FUNDING.yml by @kimkulling in https://github.com/assimp/assimp/pull/2990
* Fixed TextureTypeToString defined multiple times. by @malortie in https://github.com/assimp/assimp/pull/2989
* Fixed population of bone armature and node fields for some meshes by @Frooxius in https://github.com/assimp/assimp/pull/2987
* Small changes in the CMake file in https://github.com/assimp/assimp/pull/2994
* Import/export of embedded texture names for the glTF/glTF2 format by @LoicFr in https://github.com/assimp/assimp/pull/2969
* Check input token length before copy by @ms-maxvollmer in https://github.com/assimp/assimp/pull/2971
* Additional checks on invalid input by @bztsrc in https://github.com/assimp/assimp/pull/2973
* Fixed size check to use correct value by @ms-maxvollmer in https://github.com/assimp/assimp/pull/2985
* Fix: GLTF animation works on RTS not matrix; fix matrix related bug; … by @runforu in https://github.com/assimp/assimp/pull/2995
* Use the translation matrix in gltf2 cameras for aiCamera.mPosition by @bubba in https://github.com/assimp/assimp/pull/2986
* Added missing texture types when searching for invalid textures. by @malortie in https://github.com/assimp/assimp/pull/3000
* minor code improvements for the obj code in https://github.com/assimp/assimp/pull/3003
* Update .clang-format by @kimkulling in https://github.com/assimp/assimp/pull/3016
* Fixed SimpleTexturedOpenGL sample. by @malortie in https://github.com/assimp/assimp/pull/3015
* Keep post-processor intentions by @kimkulling in https://github.com/assimp/assimp/pull/3020
* Fixed memory leaks in SimpleTexturedOpenGL sample. by @malortie in https://github.com/assimp/assimp/pull/3021
* small changes in https://github.com/assimp/assimp/pull/3014
* Made changes to write compiled binaries to a common directory. by @malortie in https://github.com/assimp/assimp/pull/3013
* A bug when importing multiple gltf files by @inhosens in https://github.com/assimp/assimp/pull/3009
* Removed uneeded expression in else() and endif() constructs. by @malortie in https://github.com/assimp/assimp/pull/3024
* pkg-config: fix include path by @jcarpent in https://github.com/assimp/assimp/pull/3010
* small improvements in the CMakeLists.txt file in https://github.com/assimp/assimp/pull/3022
* Fix for #3037 [FATAL] SplitByBoneCountProcess::SplitMesh goes into infinite loop by @Nimer-88 in https://github.com/assimp/assimp/pull/3040
* Fix for #3037 cause glTF2Importer creating bunch of bones with 0 for vertex with index 0 by @Nimer-88 in https://github.com/assimp/assimp/pull/3039
* Fixed SimpleTexturedDirectX11 sample. by @malortie in https://github.com/assimp/assimp/pull/3053
* Fixed wrong matrix type used in aiMatrix3x3t comparison operators. by @malortie in https://github.com/assimp/assimp/pull/3056
* Only try to initialize members whose name starts with 'm' followed by an uppercase character by @shawwn in https://github.com/assimp/assimp/pull/3057
* Fix zip issue by @kimkulling in https://github.com/assimp/assimp/pull/3058
* Raised minimum CMake version to 3.0 for assimp_cmd and assimp_view. by @malortie in https://github.com/assimp/assimp/pull/3033
* Added missing std namespace prefix to std types. by @malortie in https://github.com/assimp/assimp/pull/3055
* cmake: double quotes around the <path> by @maquefel in https://github.com/assimp/assimp/pull/3034
* Minor changes in CMakeLists files. by @malortie in https://github.com/assimp/assimp/pull/3060
* Blendshape Support in Assimp Gltf2/Glb2 Exporter (positions, normals) by @thomasbiang in https://github.com/assimp/assimp/pull/3063
* Added test case for fix earlier submitted by @ms-maxvollmer in https://github.com/assimp/assimp/pull/3067
* Create ccpp.yml by @kimkulling in https://github.com/assimp/assimp/pull/3078
* [WIP] Enable all warnings for Visual Studio. by @kimkulling in https://github.com/assimp/assimp/pull/3012
* {cmake} Prefix options to avoid pollution when included as a submodule by @asmaloney in https://github.com/assimp/assimp/pull/3083
* fix FBX no preservePivots bug by @aimoonchen in https://github.com/assimp/assimp/pull/3075
* GLTF2: Fixed behavior of glTF2Importer::ImportNodes by @ms-maxvollmer in https://github.com/assimp/assimp/pull/3069
* Added missing functionalities to C API. by @malortie in https://github.com/assimp/assimp/pull/3089
* Fixed FBX simple anim pre/post rotation handling by @napina in https://github.com/assimp/assimp/pull/3086
* Add mac by @kimkulling in https://github.com/assimp/assimp/pull/3096
* add windows build by @kimkulling in https://github.com/assimp/assimp/pull/3098
* closes https://github.com/assimp/assimp/pull/3104:  by @kimkulling in https://github.com/assimp/assimp/pull/3112
* Fix funding by @kimkulling in https://github.com/assimp/assimp/pull/3114
* Removed unneeded SceneDiffer.h includes. by @malortie in https://github.com/assimp/assimp/pull/3117
* Fix gltf2 exporter memory crash by @thomasbiang in https://github.com/assimp/assimp/pull/3113
* Fixed /W4 compile warnings in sample SimpleOpenGL. by @malortie in https://github.com/assimp/assimp/pull/3124
* Fixed /W4 compile warnings in sample SimpleTexturedDirectx11. by @malortie in https://github.com/assimp/assimp/pull/3127
* closes https://github.com/assimp/assimp/issues/2166:  by @kimkulling in https://github.com/assimp/assimp/pull/3137
* use GNUInstallDirs where possible (master branch) by @vmatare in https://github.com/assimp/assimp/pull/3126
* Fixed /W4 compile warnings in Assimp viewer. by @malortie in https://github.com/assimp/assimp/pull/3129
* Fixed /W4 compile warnings in sample SimpleTexturedOpenGL. by @malortie in https://github.com/assimp/assimp/pull/3125
* Replaced NULL with nullptr for pointers in Assimp viewer. by @malortie in https://github.com/assimp/assimp/pull/3139
* Replaced NULL with nullptr for pointers in sample SimpleTexturedOpenGL. by @malortie in https://github.com/assimp/assimp/pull/3141
* Testcoverage improvements. by @kimkulling in https://github.com/assimp/assimp/pull/2885
* Use checkoutv2 by @kimkulling in https://github.com/assimp/assimp/pull/3150
* Added tests to C API missing functionalities in #3091 by @malortie in https://github.com/assimp/assimp/pull/3147
* [RFC] cmake: targets: check lib or lib64 path by @maquefel in https://github.com/assimp/assimp/pull/3035
* Minor fixes and improvements in sample SimpleOpenGL. by @malortie in https://github.com/assimp/assimp/pull/3036
* ifdef the exporters as specifying stricter linker flags than what's in default CMake causes linking issues by @Nimer-88 in https://github.com/assimp/assimp/pull/3049
* GLTF2: ExtractData now throws exception instead of returning false if data is invalid by @ms-maxvollmer in https://github.com/assimp/assimp/pull/3068
* GLTF2: Detect and abort recursive references by @ms-maxvollmer in https://github.com/assimp/assimp/pull/3070
* Replaced NULL with nullptr for pointers in sample SimpleTexturedDirectx11. by @malortie in https://github.com/assimp/assimp/pull/3131
* 3MF fix  by @Hehongyuanlove in https://github.com/assimp/assimp/pull/3158
* Erase the remaining _INSTALL_PREFIX and LIBSUFFIX in CMake files by @Zalewa in https://github.com/assimp/assimp/pull/3155
* Add support for glTF2 targetNames by @timuber in https://github.com/assimp/assimp/pull/3149
* Update to 5.0.1 by @kimkulling in https://github.com/assimp/assimp/pull/3161
* Optimized LimitBoneWeightsProcess. by @napina in https://github.com/assimp/assimp/pull/3085
* Migenius migenius rsws53 mig 2 by @kimkulling in https://github.com/assimp/assimp/pull/3164
* Fix to read orthographic camera data by @ardenpm in https://github.com/assimp/assimp/pull/3029
* closes https://github.com/assimp/assimp/issues/3103: always enable wa… by @kimkulling in https://github.com/assimp/assimp/pull/3116
* Iisue 3165 by @kimkulling in https://github.com/assimp/assimp/pull/3167
* Use clang on Unix, msvc on Windows, Use Ninja everywhere by @lukka in https://github.com/assimp/assimp/pull/3168
* Enable gcc on Linux on CI, fix warnings by @lukka in https://github.com/assimp/assimp/pull/3173
* Add sanitizer support by @kimkulling in https://github.com/assimp/assimp/pull/3171
* Opencollective opencollective by @kimkulling in https://github.com/assimp/assimp/pull/3170
* Kimkulling dev by @kimkulling in https://github.com/assimp/assimp/pull/3196
* Kimkulling dev by @kimkulling in https://github.com/assimp/assimp/pull/3197
* closes https://github.com/assimp/assimp/issues/3190 : fix leak. by @kimkulling in https://github.com/assimp/assimp/pull/3202
* Aaronfranke file formatting by @kimkulling in https://github.com/assimp/assimp/pull/3204
* Fix for issue# 3206: GLTF2 blendshape import missing shapes https://github.com/assimp/assimp/issues/3206 by @thomasbiang in https://github.com/assimp/assimp/pull/3207
* Remove duplicate flag by @kimkulling in https://github.com/assimp/assimp/pull/3208
* fix incorrect header path on framework build by @sercand in https://github.com/assimp/assimp/pull/3209
* Fixed bone splitting with excessive amount of bones with 0 weight by @rudybear in https://github.com/assimp/assimp/pull/3105
* remove step prototype: does not work this way. by @kimkulling in https://github.com/assimp/assimp/pull/3212
* integrate first fuzzer target. by @kimkulling in https://github.com/assimp/assimp/pull/3211
* contrib/zlib: disable dynamic library building by @pseiderer in https://github.com/assimp/assimp/pull/3146
* Fix fbx rotation ; by @hoshiryu in https://github.com/assimp/assimp/pull/3175
* Collada: Ensure export uses unique Mesh Ids by @RichardTea in https://github.com/assimp/assimp/pull/3188
* Zyndor master by @kimkulling in https://github.com/assimp/assimp/pull/3223
* Migenius migenius fix ortho by @kimkulling in https://github.com/assimp/assimp/pull/3224
* Inhosens master by @kimkulling in https://github.com/assimp/assimp/pull/3225
* Collada unit test cleanup by @RichardTea in https://github.com/assimp/assimp/pull/3194
* Any interest in Rust '18 port? by @dmgolembiowski in https://github.com/assimp/assimp/pull/3195
* Export Collada Meshes on root aiNode by @RichardTea in https://github.com/assimp/assimp/pull/3205
* Qarmin added check before using by @kimkulling in https://github.com/assimp/assimp/pull/3229
* [gltf2 Export] More robust handling for non-finites and 0-length normals by @jercytryn in https://github.com/assimp/assimp/pull/3214
* Add IMPORTED_CONFIGURATIONS property to cmake target. by @kalyan-kumar in https://github.com/assimp/assimp/pull/3215
* [GLTF2] Fix infinite recursion in skin/node parsing by @M4T1A5 in https://github.com/assimp/assimp/pull/3226
* Fix double free caused in FindInvalidDataProcess by @rmstyrczula in https://github.com/assimp/assimp/pull/3235
* Verbose logging by @MalcolmTyrrell in https://github.com/assimp/assimp/pull/3232
* Fixed rotation order bug in BVH Loader by @kshepherd2013 in https://github.com/assimp/assimp/pull/3233
* File is duplicated by @kimkulling in https://github.com/assimp/assimp/pull/3241
* Remove duplicated code by @kimkulling in https://github.com/assimp/assimp/pull/3242
* Update utRemoveComponent.cpp by @kimkulling in https://github.com/assimp/assimp/pull/3243
* Gltf2 Support Importing sparse accessor  by @thomasbiang in https://github.com/assimp/assimp/pull/3219
* Evaluated expressions and clean up some code in tests by @hbina in https://github.com/assimp/assimp/pull/3249
* closes https://github.com/assimp/assimp/issues/3256: Remove redundand… by @kimkulling in https://github.com/assimp/assimp/pull/3262
* Added rapidjson define to avoid warnings in c++17 by @AlecLafita in https://github.com/assimp/assimp/pull/3263
* Perform sanity check only in debug by @kimkulling in https://github.com/assimp/assimp/pull/3265
* Pyassimp - contextmanager for load function by @DavidBerger98 in https://github.com/assimp/assimp/pull/3271
* Migenius migenius fix texcoord by @kimkulling in https://github.com/assimp/assimp/pull/3277
* Fbx Import: support channel name in blendshape name by @thomasbiang in https://github.com/assimp/assimp/pull/3268
* add a unittest. by @kimkulling in https://github.com/assimp/assimp/pull/3279
* Gltf2 Export Target Names for Blendshapes by @thomasbiang in https://github.com/assimp/assimp/pull/3267
* closes https://github.com/assimp/assimp/issues/3165: fix gcc build. by @kimkulling in https://github.com/assimp/assimp/pull/3248
* Update issue templates by @kimkulling in https://github.com/assimp/assimp/pull/3284
* closes https://github.com/assimp/assimp/issues/3253 : remove useless … by @kimkulling in https://github.com/assimp/assimp/pull/3287
* Check invalid vertex id for bone weight by @infosia in https://github.com/assimp/assimp/pull/3288
* Repo-Cleanup by @kimkulling in https://github.com/assimp/assimp/pull/3296
* Allow users to customize the behavior of assert violations by @MalcolmTyrrell in https://github.com/assimp/assimp/pull/3297
* FindInvalidDataProcess: Mark removed meshes as "out" by @rmstyrczula in https://github.com/assimp/assimp/pull/3294
* Added arbitrary recursive metadata to allow for glTF2's extensions to… by @Evangel63 in https://github.com/assimp/assimp/pull/3298
* rename fuzzer target by @kimkulling in https://github.com/assimp/assimp/pull/3299
* Fix Maya PBR & stingray texture detection by @ifiddynine in https://github.com/assimp/assimp/pull/3293
* Fixed variable in loop - HL1MDLLoader.cpp by @malortie in https://github.com/assimp/assimp/pull/3303
* Move patreon to contribution by @kimkulling in https://github.com/assimp/assimp/pull/3302
* Fix build when ASSIMP_DOUBLE_PRECISION is on. by @mahiuchun in https://github.com/assimp/assimp/pull/3301
* closes https://github.com/assimp/assimp/issues/3305: remove merge issue. by @kimkulling in https://github.com/assimp/assimp/pull/3306
* Improve ToBinary() for double precision. by @mahiuchun in https://github.com/assimp/assimp/pull/3309
* Ensure asserts are defined where expected. by @MalcolmTyrrell in https://github.com/assimp/assimp/pull/3307
* closes https://github.com/assimp/assimp/issues/3252: fix build. by @kimkulling in https://github.com/assimp/assimp/pull/3314
* Use strrchr() when finding the '.' that begins the file extension. by @mahiuchun in https://github.com/assimp/assimp/pull/3300
* use c-style cast in a c-file by @kimkulling in https://github.com/assimp/assimp/pull/3321
* Customize Extras in Gltf2 Exporter with ExporterProperty Callback by @thomasbiang in https://github.com/assimp/assimp/pull/3280
* Gltf2 Sparse Accessor Export (blendshape export using sparse accessor) by @thomasbiang in https://github.com/assimp/assimp/pull/3227
* Loic fr master by @kimkulling in https://github.com/assimp/assimp/pull/3323
* Fix Blender .fbx metalness detection by @ifiddynine in https://github.com/assimp/assimp/pull/3289
* FBXExporter: Use scene metadata for global settings by @rmstyrczula in https://github.com/assimp/assimp/pull/3292
* add triangle strip support to AC file loader by @IOBYTE in https://github.com/assimp/assimp/pull/3320
* fix invalid pointer for bone animation by @infosia in https://github.com/assimp/assimp/pull/3330
* Added macros to enable/disable GLTF1 and GLTF2 independently by @ms-maxvollmer in https://github.com/assimp/assimp/pull/3311
* Build viewer and publish artifacts on windows-msvc. by @AndrewJDR in https://github.com/assimp/assimp/pull/3322
* DefaultIOStream: Remove assert on empty count by @rmstyrczula in https://github.com/assimp/assimp/pull/3326
* Hunter-related build fixes by @rbsheth in https://github.com/assimp/assimp/pull/3327
* clang with msvc backend by @MeyerFabian in https://github.com/assimp/assimp/pull/3337
* Use #ifdef _MSC_VER for pragma warnings (Issue 3332) by @RichardTea in https://github.com/assimp/assimp/pull/3336
* Issue 3334 cl d9025 by @RichardTea in https://github.com/assimp/assimp/pull/3335
* ACLoader: Use Surface type enums by @RichardTea in https://github.com/assimp/assimp/pull/3333
* Handle Gltf2 files where a value in a mesh index buffer is out of range. by @MalcolmTyrrell in https://github.com/assimp/assimp/pull/3329
* Remove travis + assveyor. by @kimkulling in https://github.com/assimp/assimp/pull/3342
* fix namespace issue in fuzzer. by @kimkulling in https://github.com/assimp/assimp/pull/3343
* use correct include. by @kimkulling in https://github.com/assimp/assimp/pull/3344
* add missing include for logging. by @kimkulling in https://github.com/assimp/assimp/pull/3345
* Fix warning: comparison between unsigned and signed. by @kimkulling in https://github.com/assimp/assimp/pull/3346
* Fix MinGW builds (issues related to pragmas and format strings) by @awr1 in https://github.com/assimp/assimp/pull/3328
* Fixing more build warnings by @rbsheth in https://github.com/assimp/assimp/pull/3347
* FBXExport: Fix crash if scene->mMetaData is null by @rmstyrczula in https://github.com/assimp/assimp/pull/3349
* FBX Version/Size Check by @MalcolmTyrrell in https://github.com/assimp/assimp/pull/3350
* Export opacity is 3DS by @kimkulling in https://github.com/assimp/assimp/pull/3357
* Fix wrong function defines by @kimkulling in https://github.com/assimp/assimp/pull/3355
* Added more undefined sanitizer flags by @qarmin in https://github.com/assimp/assimp/pull/3356
* closes https://github.com/assimp/assimp/issues/2992: detect double support  by @kimkulling in https://github.com/assimp/assimp/pull/3360
* Fix nncorrectly named Assimp .dll by @kimkulling in https://github.com/assimp/assimp/pull/3362
* Fixed runtime output directory overridden. by @malortie in https://github.com/assimp/assimp/pull/3363
* Fix incorrect index by @kimkulling in https://github.com/assimp/assimp/pull/3369
* NFF importer double precision support by @lsliegeo in https://github.com/assimp/assimp/pull/3372
* Update utf8cpp to fix use of C++17 deprecated feature by @fuj1n in https://github.com/assimp/assimp/pull/3374
* Update Jassimp's AiTextureType.java by @flowtsohg in https://github.com/assimp/assimp/pull/3386
* Fix Bad Ownership Acquisition by @jnhyatt in https://github.com/assimp/assimp/pull/3385
* add missing define to glTF importer by @Gargaj in https://github.com/assimp/assimp/pull/3387
* Fix an unreferenced formal parameter warning on MSVC when no exporter… by @Naios in https://github.com/assimp/assimp/pull/3391
* Make internal errors accessible by @MalcolmTyrrell in https://github.com/assimp/assimp/pull/3375
* collada: set aiAnimation->mTicksPerSecond to 1000.0 by @crocdialer in https://github.com/assimp/assimp/pull/3383
* Fix RapidJSON defines and add Hunter builds to CI by @rbsheth in https://github.com/assimp/assimp/pull/3382
* Update Readme.md by @kimkulling in https://github.com/assimp/assimp/pull/3400
* GLTF2: Throw instead of assert when input file is invalid. by @ms-maxvollmer in https://github.com/assimp/assimp/pull/3394
* Fix for build break due to warnings-as-errors. by @sherief in https://github.com/assimp/assimp/pull/3405
* Pugi xml by @kimkulling in https://github.com/assimp/assimp/pull/2966
* closes https://github.com/assimp/assimp/issues/3379: reproduce error … by @kimkulling in https://github.com/assimp/assimp/pull/3415
* [Bug-Fix] Fixed Strict Aliasing Level 3 Warnings by @Optimizer0 in https://github.com/assimp/assimp/pull/3413
* fix for fbx files using stingray materials; by @stromaster in https://github.com/assimp/assimp/pull/3446
* Update Hunter for pugixml by @rbsheth in https://github.com/assimp/assimp/pull/3452
* Add handling for source for params by @kimkulling in https://github.com/assimp/assimp/pull/3458
* Fix for issue #3445 by @jsmaatta in https://github.com/assimp/assimp/pull/3454
* Hotfix for Hunter builds by @rbsheth in https://github.com/assimp/assimp/pull/3456
* CMake: Fix FindRT warning by @xantares in https://github.com/assimp/assimp/pull/3451
* glTF1's orthgraphic camera & glTF2's skinning by @inhosens in https://github.com/assimp/assimp/pull/3461
* optimize CMakeLists.txt by @xiaozhuai in https://github.com/assimp/assimp/pull/3231
* Collada cleanup by @kimkulling in https://github.com/assimp/assimp/pull/3466
* Update FUNDING.yml by @kimkulling in https://github.com/assimp/assimp/pull/3470
* Fbx report asset issues properly by @MalcolmTyrrell in https://github.com/assimp/assimp/pull/3412
* Fixes for crashes in GLTF2 Importer by @ms-maxvollmer in https://github.com/assimp/assimp/pull/3476
* openddl: latest greatest. by @kimkulling in https://github.com/assimp/assimp/pull/3478
* MSVC crash while importing fbx model workaround by @MomoDeve in https://github.com/assimp/assimp/pull/3471
* AI_CONFIG_IMPORT_FBX_READ_WEIGHTS by @MalcolmTyrrell in https://github.com/assimp/assimp/pull/3460
* fix xcode compile bug by @maiqingqiang in https://github.com/assimp/assimp/pull/3463
* Fix CMake config generation problems by @traversaro in https://github.com/assimp/assimp/pull/3455
* closes https://github.com/assimp/assimp/issues/3448 by @kimkulling in https://github.com/assimp/assimp/pull/3493
* closes https://github.com/assimp/assimp/issues/3198: make aiMaterial:… by @kimkulling in https://github.com/assimp/assimp/pull/3494
* Delete AMFImporter_Postprocess.cpp by @kimkulling in https://github.com/assimp/assimp/pull/3486
* closes https://github.com/assimp/assimp/issues/1044 by @kimkulling in https://github.com/assimp/assimp/pull/3497
* closes https://github.com/assimp/assimp/issues/3187 by @kimkulling in https://github.com/assimp/assimp/pull/3498
* GLTF2: Null bufferview crash fix by @MalcolmTyrrell in https://github.com/assimp/assimp/pull/3490
* Prevent crash with malformed texture reference by @MalcolmTyrrell in https://github.com/assimp/assimp/pull/3502
* Fixing issue 3500, invalid outer cone angle readed from gltf2 file on  machines which defines M_PI as a double value by @Nodrev in https://github.com/assimp/assimp/pull/3501
* Optimize FindDegenerates so it doesn't explode by @MalcolmTyrrell in https://github.com/assimp/assimp/pull/3484
* Added mName to aiScene. by @Evangel63 in https://github.com/assimp/assimp/pull/3510
* FBXParser.cpp - handle buffer over-read correctly by @Neil-Clifford-FB in https://github.com/assimp/assimp/pull/3504
* Prevent to generate redundant morph targets for glTF2 by @inhosens in https://github.com/assimp/assimp/pull/3487
* Simplification: textures_converted keys can just be pointers by @MalcolmTyrrell in https://github.com/assimp/assimp/pull/3511
* Update Android port README.md with ABI doc by @natanaeljr in https://github.com/assimp/assimp/pull/3503
* Sceneprecessor - potential memory leak by @Neil-Clifford-FB in https://github.com/assimp/assimp/pull/3505
* Fbx exception safety by @MalcolmTyrrell in https://github.com/assimp/assimp/pull/3506
* Fix for 3489 | Preserve morph targets when splitting by bone count by @boguscoder in https://github.com/assimp/assimp/pull/3512
* Update Readme.md by @kimkulling in https://github.com/assimp/assimp/pull/3516
* GLTF: Fix crash on invalid base64 data + improved error messages by @ms-maxvollmer in https://github.com/assimp/assimp/pull/3514
* Provide option for rapidjson no-member-iterator define by @tellypresence in https://github.com/assimp/assimp/pull/3528
* glTF2Exporter: fix crash when exporting a scene with several meshes p… by @LoicFr in https://github.com/assimp/assimp/pull/3515
* Additional Compiler Options for mips64el by @huiji12321 in https://github.com/assimp/assimp/pull/3521
* Check _MSC_VER for MSVC specific pragma directives. by @Biswa96 in https://github.com/assimp/assimp/pull/3518
* 3ds Max 2021 PBR Materials in FBX by @MalcolmTyrrell in https://github.com/assimp/assimp/pull/3519
* FBXConverter : Fix timescales of FBX animations by @inhosens in https://github.com/assimp/assimp/pull/3524
* Fixed a crash of the Gltf 2 exporter in the case of an animation without scale animation key. by @JLouis-B in https://github.com/assimp/assimp/pull/3531
* Common: Fix GCC error invalid conversion in MINGW. by @Biswa96 in https://github.com/assimp/assimp/pull/3533
* _dest may be destructed twice if _dest is not null in MergeScenes() by @wasd845 in https://github.com/assimp/assimp/pull/3540
* Fix #3222 by @someonewithpc in https://github.com/assimp/assimp/pull/3555
* GLTF2 fixes by @ms-maxvollmer in https://github.com/assimp/assimp/pull/3541
* FBXBinaryTokenizer: Check length of property by @ms-maxvollmer in https://github.com/assimp/assimp/pull/3542
* Fix glTF from memory loading .bin with a custom IOHandler by @makitsune in https://github.com/assimp/assimp/pull/3562
* Only consider continuation tokens at end of line by @gris-martin in https://github.com/assimp/assimp/pull/3543
* add operator* in aiQuaterniont by @wasd845 in https://github.com/assimp/assimp/pull/3546
* [gltf2] Add support for extensions KHR_materials by @Danny-Kint in https://github.com/assimp/assimp/pull/3552
* Update unzip contrib by @JLouis-B in https://github.com/assimp/assimp/pull/3556
* 3mf improvements by @JLouis-B in https://github.com/assimp/assimp/pull/3558
* fix of an unattainable condition. by @ihsinme in https://github.com/assimp/assimp/pull/3569
* contrib/zlib/CMakeLists.txt: don't install zlib by @ffontaine in https://github.com/assimp/assimp/pull/3561
* Issue 3570 (CMake Policy violations on MSVC) by @JacksonM8 in https://github.com/assimp/assimp/pull/3571
* Use const instead of constexpr by @kimkulling in https://github.com/assimp/assimp/pull/3581
* Fixes for GLTF2 buffers by @ms-maxvollmer in https://github.com/assimp/assimp/pull/3575
* Remove dependency to posix-extension function by @kimkulling in https://github.com/assimp/assimp/pull/3583
* remove install of zlibstatic by @kimkulling in https://github.com/assimp/assimp/pull/3585
* aiMaterial::GetTexture() - fix misleading description of path argument. by @gkv311 in https://github.com/assimp/assimp/pull/3578
* Fix Divide-by-zero in vector3 by @kimkulling in https://github.com/assimp/assimp/pull/3586
* Fix overflow in aiString by @kimkulling in https://github.com/assimp/assimp/pull/3589
* Fix nullptr dereference in scenepreprocessor by @kimkulling in https://github.com/assimp/assimp/pull/3590
* Fix build failure on Linux by @h3xx in https://github.com/assimp/assimp/pull/3591
* fixed memory leak by @ihsinme in https://github.com/assimp/assimp/pull/3579
* Fix nullptr access by @kimkulling in https://github.com/assimp/assimp/pull/3592
* Win32-refactorings by @kimkulling in https://github.com/assimp/assimp/pull/3587
* pbrt-v4 exporter by @mmp in https://github.com/assimp/assimp/pull/3580
* Update Build.md by @kimkulling in https://github.com/assimp/assimp/pull/3600
* Fix glTF vertex colors with types other than float by @makitsune in https://github.com/assimp/assimp/pull/3582
* Fix STL Expoter error. by @xiaohunqupo in https://github.com/assimp/assimp/pull/3594
* Update 3DSLoader.cpp by @kimkulling in https://github.com/assimp/assimp/pull/3602
* Remove redundant statement in if by @kimkulling in https://github.com/assimp/assimp/pull/3603
* Update gitignore for default VS2019 behaviour by @RichardTea in https://github.com/assimp/assimp/pull/3596
* Fix typo in collada parser by @kimkulling in https://github.com/assimp/assimp/pull/3611
* Add missing skip by @kimkulling in https://github.com/assimp/assimp/pull/3612
* Delete appveyor.yml by @kimkulling in https://github.com/assimp/assimp/pull/3610
* Replace patreon by opencollective by @kimkulling in https://github.com/assimp/assimp/pull/3615
* Increase float and double string export precision by @RichardTea in https://github.com/assimp/assimp/pull/3597
* Fix compiler bug for VS2019 by @kimkulling in https://github.com/assimp/assimp/pull/3616
* GCC 11 build fixes by @villevoutilainen in https://github.com/assimp/assimp/pull/3608
* GLTF2: Allow Export Node in TRS format by @thomasbiang in https://github.com/assimp/assimp/pull/3598
* Fixes a mem leak in aiMetadata::Set by @kimkulling in https://github.com/assimp/assimp/pull/3622
* Update all minimum cmake req to 3.10 by @kimkulling in https://github.com/assimp/assimp/pull/3623
* Changed morph anim error to warning when validating by @bsekura in https://github.com/assimp/assimp/pull/3604
* ColladaLoader now assigns individual material indices to submeshes as needed by @contriteobserver in https://github.com/assimp/assimp/pull/3607
* cleaned up sign-compare unittest build warnings by @contriteobserver in https://github.com/assimp/assimp/pull/3625
* Fix incorrect xml-parsing in collada importer. by @kimkulling in https://github.com/assimp/assimp/pull/3635
* Fix compiler warning: warning: argument to ... call is the same expre… by @kimkulling in https://github.com/assimp/assimp/pull/3642
* fix issue: 3482: invalid gltf2 properties by @thomasbiang in https://github.com/assimp/assimp/pull/3636
* fix compile warning-turned-error on x86 by @Gargaj in https://github.com/assimp/assimp/pull/3643
* Collada importer now identifies animations by @contriteobserver in https://github.com/assimp/assimp/pull/3619
* Eliminate maybe-uninitialized warnings which are treated as errors by @lgmcode in https://github.com/assimp/assimp/pull/3644
* Update defs.h by @kimkulling in https://github.com/assimp/assimp/pull/3649
* Fix apha value by @kimkulling in https://github.com/assimp/assimp/pull/3652
* Implements access to files bundled with Android Applications by @contriteobserver in https://github.com/assimp/assimp/pull/3634
* Rust bindings by @jkvargas in https://github.com/assimp/assimp/pull/3653
* Implement import of Draco-encoded glTFv2 models by @RichardTea in https://github.com/assimp/assimp/pull/3614
* Export zlib if it's built outside by @gongminmin in https://github.com/assimp/assimp/pull/3620
* Eliminate MSVC warning C4819 caused by source files encoded in UTF-8 without BOM by @lgmcode in https://github.com/assimp/assimp/pull/3650
* Export the animation name to gltf2 by @gongminmin in https://github.com/assimp/assimp/pull/3659
* Silence uninitialized variable warning in 3MF importer by @turol in https://github.com/assimp/assimp/pull/3665
* Remove buggy assert by @kimkulling in https://github.com/assimp/assimp/pull/3674
* Check that normal count and tangent count match vertex count. by @ms-maxvollmer in https://github.com/assimp/assimp/pull/3670
* Workaround for VS2019 by @kimkulling in https://github.com/assimp/assimp/pull/3680
* Update copyrights to 2021. by @kimkulling in https://github.com/assimp/assimp/pull/3681
* CMake fix for Android build with enabled JNI io system by @ademets in https://github.com/assimp/assimp/pull/3660
* workaround for ASSIMP_DOUBLE_PRECISION compile errors by @contriteobserver in https://github.com/assimp/assimp/pull/3683
* Export Min/Max for Sparse Accessor by @thomasbiang in https://github.com/assimp/assimp/pull/3667
* applied extern "C" guards to importerdesc.h by @contriteobserver in https://github.com/assimp/assimp/pull/3687
* closes https://github.com/assimp/assimp/issues/3678: ensure lowercase by @kimkulling in https://github.com/assimp/assimp/pull/3694
* Fix Step Expoter Error.  by @xiaohunqupo in https://github.com/assimp/assimp/pull/3661
* Fix compiling issues in clang-cl by @gongminmin in https://github.com/assimp/assimp/pull/3688
* Compile fix for MSVC 2019 by @AndyShawQt in https://github.com/assimp/assimp/pull/3689
* Update crypt.c by @Paul-Austria in https://github.com/assimp/assimp/pull/3691
* change file encoding by @jaefunk in https://github.com/assimp/assimp/pull/3697
* Fix a memory leak in glTF2. by @mahiuchun in https://github.com/assimp/assimp/pull/3709
* export with rotation by @jaefunk in https://github.com/assimp/assimp/pull/3696
* Fix a set of glTF2 crashes on bad input by @MalcolmTyrrell in https://github.com/assimp/assimp/pull/3707
* Provide a dockerfile by @kimkulling in https://github.com/assimp/assimp/pull/3716
* Fixing CHUNK_TRMATRIX translation sub chunk by @bekraft in https://github.com/assimp/assimp/pull/3722
* Fixing 3DS import for CHUNK_TRMATRIX translation vector. by @bekraft in https://github.com/assimp/assimp/pull/3724
* Added Blendshape Support to FBX Export by @vfxgordon in https://github.com/assimp/assimp/pull/3721
* Update Readme.md by @kimkulling in https://github.com/assimp/assimp/pull/3727
* Update FUNDING.yml by @kimkulling in https://github.com/assimp/assimp/pull/3728
* Add zlibstatic to the list of exported targets by @eliasdaler in https://github.com/assimp/assimp/pull/3723
* Issue 3678 by @kimkulling in https://github.com/assimp/assimp/pull/3736
* 615 io ios port update by @kimkulling in https://github.com/assimp/assimp/pull/3737
* Malcolm tyrrell/tangent check by @MalcolmTyrrell in https://github.com/assimp/assimp/pull/3718
* Update INSTALL by @kimkulling in https://github.com/assimp/assimp/pull/3739
* Renaming PI to aiPi. by @BaldricS in https://github.com/assimp/assimp/pull/3746
* Fix direct leak by @kimkulling in https://github.com/assimp/assimp/pull/3748
* [gltf2-exporter] Adding FB_ngon_encoding support by @clems71 in https://github.com/assimp/assimp/pull/3695
* Fix incorrect indices in the MilkShape 3D loader by @pcwalton in https://github.com/assimp/assimp/pull/3749
* Fix import of FBX files with last UV duplicated (caused by bug in FBX SDK 2019.0+) by @urschanselmann in https://github.com/assimp/assimp/pull/3708
* Not resize empty vectors. by @kimkulling in https://github.com/assimp/assimp/pull/3755
* Update repo for assimp-net by @kimkulling in https://github.com/assimp/assimp/pull/3758
* Fix MDC loader by @Garux in https://github.com/assimp/assimp/pull/3742
* Flip the check on _MSC_VER for using TR1 containers. by @mahiuchun in https://github.com/assimp/assimp/pull/3757
* Version string fix (if anyone cares) by @krishty in https://github.com/assimp/assimp/pull/3774
* fixed export exceptions on import by @krishty in https://github.com/assimp/assimp/pull/3776
* fixed glTF export stuff being pulled into the EXE even if building wi… by @krishty in https://github.com/assimp/assimp/pull/3763
* Add Codacy Badge by @kimkulling in https://github.com/assimp/assimp/pull/3793
* Update Readme.md by @kimkulling in https://github.com/assimp/assimp/pull/3794
* Update aiProcess_PreTransformVertices docs to match behavior. by @JC3 in https://github.com/assimp/assimp/pull/3821
* Remove newline from name of Blender importer. by @JC3 in https://github.com/assimp/assimp/pull/3822
* Fix crash when reading 0 bytes by @kimkulling in https://github.com/assimp/assimp/pull/3833
* Importer improvements by @ms-maxvollmer in https://github.com/assimp/assimp/pull/3787
* closes https://github.com/assimp/assimp/issues/3831 : update zip by @kimkulling in https://github.com/assimp/assimp/pull/3846
* style fix - initializing and assigning empty std::string properly by @krishty in https://github.com/assimp/assimp/pull/3764
* moved MD2/MDC tables from BSS to const data by @krishty in https://github.com/assimp/assimp/pull/3765
* Add PBRT to exportable file formats list by @diiigle in https://github.com/assimp/assimp/pull/3775
* Fix: Removing double delete of texture items after MergeScene. by @BaldricS in https://github.com/assimp/assimp/pull/3788
* reverted regression in 3DS transformation (issue #3802) by @krishty in https://github.com/assimp/assimp/pull/3826
* Fix formatter. by @kimkulling in https://github.com/assimp/assimp/pull/3795
* style fix: namespace instead of class with public static members by @krishty in https://github.com/assimp/assimp/pull/3852
* small cleanup in file format detection by @krishty in https://github.com/assimp/assimp/pull/3796
* fixed malformatted message by @krishty in https://github.com/assimp/assimp/pull/3805
* use const chars by @kimkulling in https://github.com/assimp/assimp/pull/3869
* Fixed weighting issue with blendShapeChannels by @vfxgordon in https://github.com/assimp/assimp/pull/3819
* Use POINTER(char) for binary data. For pyassimp issue #2339: Can't load OBJ by @olsoneric in https://github.com/assimp/assimp/pull/3877
* ASSIMP_ENABLE_DEV_IMPORTERS env var, applied to X3D importer by @JC3 in https://github.com/assimp/assimp/pull/3834
* Optimize 3mf strings by @kimkulling in https://github.com/assimp/assimp/pull/3882
* removed dead code from 0d29203e24a8bc2c75278931a6bd25b2ae5848de by @krishty in https://github.com/assimp/assimp/pull/3806
* SimpleTexturedDirectx11 sample: support embedded uncompressed textures by @ericwa in https://github.com/assimp/assimp/pull/3808
* added .step extension to IFC loader by @krishty in https://github.com/assimp/assimp/pull/3837
* consider aiProcess_FlipWindingOrder in aiProcess_GenNormals & aiProcess_GenSmoothNormals by @Garux in https://github.com/assimp/assimp/pull/3838
* consider pScene->mRootNode->mTransformation set by some importers while using AI_CONFIG_PP_PTV_ROOT_TRANSFORMATION by @Garux in https://github.com/assimp/assimp/pull/3839
* Fix crash in CanRead when file can not be opened. by @JC3 in https://github.com/assimp/assimp/pull/3850
* Make sure ctype calls use unsigned chars. by @JC3 in https://github.com/assimp/assimp/pull/3880
* orient mdc correctly by @Garux in https://github.com/assimp/assimp/pull/3841
* Fix importer ReadFile issues on file open error or when opening empty files by @JC3 in https://github.com/assimp/assimp/pull/3890
* Misc. log output and message fixes by @JC3 in https://github.com/assimp/assimp/pull/3881
* support missing closing brace in material list after Ascii Scene Exporter v2.51 by @Garux in https://github.com/assimp/assimp/pull/3844
* Reapply [amf] Fix crash when file could not be parsed. by @JC3 in https://github.com/assimp/assimp/pull/3898
* fix hl1 mdl orientation, tex coords, face windings order by @Garux in https://github.com/assimp/assimp/pull/3842
* updated C4D importer to use the Cineware SDK by @krishty in https://github.com/assimp/assimp/pull/3851
* fix md2 orientation by @Garux in https://github.com/assimp/assimp/pull/3843
* build M3D ASCII support by default by @contriteobserver in https://github.com/assimp/assimp/pull/3848
* [blender] Disable creation of "dna.txt" by @JC3 in https://github.com/assimp/assimp/pull/3891
* Update Readme.md by @kimkulling in https://github.com/assimp/assimp/pull/3902
* Add support for arm 64 bit by @impala454 in https://github.com/assimp/assimp/pull/3901
* Utilize decltype for slightly improved syntax by @Saalvage in https://github.com/assimp/assimp/pull/3900
* [Logger] Unify log formatting by @MalcolmTyrrell in https://github.com/assimp/assimp/pull/3905
* Xml doc by @kimkulling in https://github.com/assimp/assimp/pull/3907
* Fix possible nullptr dereferences. by @kimkulling in https://github.com/assimp/assimp/pull/3908
* fixed bloat in SIB importer by @krishty in https://github.com/assimp/assimp/pull/3856
* Md3 tuning by @Garux in https://github.com/assimp/assimp/pull/3847
* PBR material support by @spotaws in https://github.com/assimp/assimp/pull/3858
* preserve UV Stream names in FBX files by @spotaws in https://github.com/assimp/assimp/pull/3859
* Follow up to PR #3787 by @ms-maxvollmer in https://github.com/assimp/assimp/pull/3883
* Support basis universal to GLTF2 format by @thomasbiang in https://github.com/assimp/assimp/pull/3893
* Less string bloat by @krishty in https://github.com/assimp/assimp/pull/3878
* [Logger] Log a notification instead of silently dropping long log messages. by @JC3 in https://github.com/assimp/assimp/pull/3896
* Fix camera fov comment since full fov is stored by @dlyr in https://github.com/assimp/assimp/pull/3912
* Add export property to control blob names. by @crud89 in https://github.com/assimp/assimp/pull/3894
* Fix gcc4.9 compilation by @Nodrev in https://github.com/assimp/assimp/pull/3917
* Create tech_debt.md by @kimkulling in https://github.com/assimp/assimp/pull/3923
* Update tech_debt.md by @kimkulling in https://github.com/assimp/assimp/pull/3925
* Update bug_report.md by @kimkulling in https://github.com/assimp/assimp/pull/3926
* Update feature_request.md by @kimkulling in https://github.com/assimp/assimp/pull/3927
* Fix non skipped CR in header parsing for binary PLY by @TinyTinni in https://github.com/assimp/assimp/pull/3929
* Fix bug in aiMetadata constructor that overwrites an array of one of … by @Evangel63 in https://github.com/assimp/assimp/pull/3922
* Change the Assimp output directory vars to cached vars by @ogjamesfranco in https://github.com/assimp/assimp/pull/3903
* Update unity plugin to trilib2 by @kimkulling in https://github.com/assimp/assimp/pull/3939
* Use correct attribute name by @kimkulling in https://github.com/assimp/assimp/pull/3940
* update pugi_xml to 1.11 by @kimkulling in https://github.com/assimp/assimp/pull/3941
* fix viewer in case of unknown primitives. by @kimkulling in https://github.com/assimp/assimp/pull/3934
* Fix fbx exporter bug if root node contains meshes. by @jagoon in https://github.com/assimp/assimp/pull/3916
* enable debug information in MSVC release build by @krishty in https://github.com/assimp/assimp/pull/3873
* Update Draco to upstream e4103dc by @RichardTea in https://github.com/assimp/assimp/pull/3911
* Adding basic support for lights in FBX exporter by @Nodrev in https://github.com/assimp/assimp/pull/3918
* SceneCombiner memory issues when re-indexing textures. by @BaldricS in https://github.com/assimp/assimp/pull/3938
* Fix issue #2873 by @mahiuchun in https://github.com/assimp/assimp/pull/3958
* Add GetEmbeddedTextureAndIndex() to aiScene. by @mahiuchun in https://github.com/assimp/assimp/pull/3945
* glTF2: Make handling of embedded textures safer. by @mahiuchun in https://github.com/assimp/assimp/pull/3946
* Replace swear words in IFCBoolean.cpp by @andreasbuhr in https://github.com/assimp/assimp/pull/3961
* First pass at simplifying PBR by @RichardTea in https://github.com/assimp/assimp/pull/3952
* the expression does not throw an exception. by @ihsinme in https://github.com/assimp/assimp/pull/3954
* include/material.h: Fixed broken C support by @jerstlouis in https://github.com/assimp/assimp/pull/3966
* Add scene metadata for glTF2 files as allowed by the glTF2 specification by @Evangel63 in https://github.com/assimp/assimp/pull/3955
* glTF2: zero out extra space created by padding. by @mahiuchun in https://github.com/assimp/assimp/pull/3959
* Performance: Apply various performance fixes from clang-tidy by @Skylion007 in https://github.com/assimp/assimp/pull/3964
* FBX module unable to read uv rotation angle and write all the uv transformation data. by @Pankaj003 in https://github.com/assimp/assimp/pull/3965
* closes https://github.com/assimp/assimp/issues/3971: fix wrong depend… by @kimkulling in https://github.com/assimp/assimp/pull/3973
* glTF2: Improved support for AI_MATKEY_OPACITY by @jerstlouis in https://github.com/assimp/assimp/pull/3967
* Added support for custom properties ("extras") in glTF2 importer by @Promit in https://github.com/assimp/assimp/pull/3969
* Update Gitignore exclude x64 folder generated by build by @irajsb in https://github.com/assimp/assimp/pull/3980
* Manage /R/N lines ends correctly on binary files, tested with solidworks PLY export by @arkeon7 in https://github.com/assimp/assimp/pull/3981
* Fix stb_image dependency by @rbsheth in https://github.com/assimp/assimp/pull/3985
* Stb image updated by @krishty in https://github.com/assimp/assimp/pull/3889
* PyAssimp fix: don't always search anaconda paths by @mlopezantequera in https://github.com/assimp/assimp/pull/3986
* Find stb for Assimp by @rbsheth in https://github.com/assimp/assimp/pull/3989
* Collada: Read <matrix> tags properly, assume <input set="0"/> when not present by @RichardTea in https://github.com/assimp/assimp/pull/3988
* Fix version, remove deprecated doc files, fix some path errors by @kimkulling in https://github.com/assimp/assimp/pull/3993
* Ensure glTFv2 scene name is unique by @RichardTea in https://github.com/assimp/assimp/pull/3990
* Doxygen: Disable html and enable xml by @kimkulling in https://github.com/assimp/assimp/pull/3994
* FBX: fix double precision build. by @mahiuchun in https://github.com/assimp/assimp/pull/3991
* Add hpp to doxygen filter by @kimkulling in https://github.com/assimp/assimp/pull/3995
* Fix issues encountered during integration attempt by @AdrianAtGoogle in https://github.com/assimp/assimp/pull/3992
* closes https://github.com/assimp/assimp/issues/3957: checkj for empty… by @kimkulling in https://github.com/assimp/assimp/pull/3997
* closes https://github.com/assimp/assimp/issues/3975:  by @kimkulling in https://github.com/assimp/assimp/pull/3998
* Fix fuzzer issue in m3d-importer by @kimkulling in https://github.com/assimp/assimp/pull/3999
* Fix euler angles by @kimkulling in https://github.com/assimp/assimp/pull/4000
* Fix Issue3760 by @kimkulling in https://github.com/assimp/assimp/pull/4002
* more range-based for by @krishty in https://github.com/assimp/assimp/pull/4011
* removed useless code by @krishty in https://github.com/assimp/assimp/pull/4006
* Use strlen() rather than fixed length in fast_atof.h by @mahiuchun in https://github.com/assimp/assimp/pull/4016
* StepExporter support polygon mesh  by @xiaohunqupo in https://github.com/assimp/assimp/pull/4001
* removed trailing spaces and tabs from source and text by @krishty in https://github.com/assimp/assimp/pull/4007
* style fix – initializing and assigning empty std::string properly by @krishty in https://github.com/assimp/assimp/pull/4008
* style fix: indentation by @krishty in https://github.com/assimp/assimp/pull/4009
* fix comments by @krishty in https://github.com/assimp/assimp/pull/4010
* Obj: make a predicate more robust. by @mahiuchun in https://github.com/assimp/assimp/pull/4017
* Crash fixes by @ms-maxvollmer in https://github.com/assimp/assimp/pull/4032
* Add missing diagnostic pragmas and remove unused code by @uerobert in https://github.com/assimp/assimp/pull/4027
* Fix: incorrect reading of PBR properties in FBX by @Mykhailo418 in https://github.com/assimp/assimp/pull/4026
* Mingw build fix by @kovacsv in https://github.com/assimp/assimp/pull/4037
* fix sample build error by @yzthr in https://github.com/assimp/assimp/pull/4036
* Build fixes by @kimkulling in https://github.com/assimp/assimp/pull/4040
* Fix M3D import crash and memory leak. by @kovacsv in https://github.com/assimp/assimp/pull/4044
* Enable Viewer only for VS-Builds by @kimkulling in https://github.com/assimp/assimp/pull/4045
* Handle  empty keys by @kimkulling in https://github.com/assimp/assimp/pull/4049
* Fix possible overrun by @kimkulling in https://github.com/assimp/assimp/pull/4050
* Delete FindIrrXML.cmake by @kimkulling in https://github.com/assimp/assimp/pull/4051
* Add support for M3F Embedded textures by @kimkulling in https://github.com/assimp/assimp/pull/4029
* Add export property for assimp json exporter to write compressed json by @kovacsv in https://github.com/assimp/assimp/pull/4053
* Fixes issues our internal compliance and code quality tool found by @ms-maxvollmer in https://github.com/assimp/assimp/pull/4055
* XGLImporter: Compiler warning fix by @Dig-Doug in https://github.com/assimp/assimp/pull/4056
* Double Precision Issue by @Madrich in https://github.com/assimp/assimp/pull/4057
* Update .gitignore by @Spectrum76 in https://github.com/assimp/assimp/pull/4012
* Add patreon by @kimkulling in https://github.com/assimp/assimp/pull/4070
* Fix MinGW build by @kirillsurkov in https://github.com/assimp/assimp/pull/4054
* fixed incorrect/misleading comment at end of scene.h by @ingowald in https://github.com/assimp/assimp/pull/4081
* Rework format + introdule missing C++11 features by @kimkulling in https://github.com/assimp/assimp/pull/4072
* removed useless code by @krishty in https://github.com/assimp/assimp/pull/4077
* including <exception> by @markoffline in https://github.com/assimp/assimp/pull/4083
* Use Safe Constants Idioms for ObjFileParser::DEFAULT_MATERIAL. by @mahiuchun in https://github.com/assimp/assimp/pull/4076
* Fix possible nullptr dereferencing in material parsing by @kimkulling in https://github.com/assimp/assimp/pull/4085
* Update ObjTools.h by @kimkulling in https://github.com/assimp/assimp/pull/4086
* more const in format detection by @krishty in https://github.com/assimp/assimp/pull/4078
* Fix Q1 MDL group frame loading, e.g. Q1 progs/flame2.mdl by @Garux in https://github.com/assimp/assimp/pull/3743
* Add support for normal maps, the classic way by @kimkulling in https://github.com/assimp/assimp/pull/4106
* Fix aiString length not updated in the EmbedTextures postprocess task by @davidepi in https://github.com/assimp/assimp/pull/4108
* Added missing include  by @lerppana in https://github.com/assimp/assimp/pull/4115
* Fix no export build by @kimkulling in https://github.com/assimp/assimp/pull/4123
* Delete fast_atof.h by @kimkulling in https://github.com/assimp/assimp/pull/4124
* Spelling fixes by @umlaeute in https://github.com/assimp/assimp/pull/4109
* Fix a warning about deprecated array comparison by @marcappelsmeier in https://github.com/assimp/assimp/pull/4110
* [GLTF2] Add read and write support for KHR_materials_volume and KHR_materials_ior extensions. by @diharaw in https://github.com/assimp/assimp/pull/4112
* Remove dead code. by @kimkulling in https://github.com/assimp/assimp/pull/4140
* SpatialSort improvements by @MalcolmTyrrell in https://github.com/assimp/assimp/pull/4130
* Added another constructor to avoid requiring a full ANativeActivity by @Daniel-Genkin in https://github.com/assimp/assimp/pull/4142
* Fixed building of Android port by @Daniel-Genkin in https://github.com/assimp/assimp/pull/4145
* Fix fuzzer crashes by @alpire in https://github.com/assimp/assimp/pull/4146
* Update D3MFOpcPackage.cpp by @kimkulling in https://github.com/assimp/assimp/pull/4154
* Remove optimization fence preventing automatic move by @Skylion007 in https://github.com/assimp/assimp/pull/4164
* Change version to 5.1.0 by @kimkulling in https://github.com/assimp/assimp/pull/4166
* Update anim.h by @kimkulling in https://github.com/assimp/assimp/pull/4167
* Added UE4 plugin by @irajsb in https://github.com/assimp/assimp/pull/4165
* Artenuvielle x3d pugi migration artenuvielle by @kimkulling in https://github.com/assimp/assimp/pull/4170
* X3D importer pugi migration by @Artenuvielle in https://github.com/assimp/assimp/pull/4079
* Better aiMesh ABI compatibility with 5.0.1, make smaller by @RichardTea in https://github.com/assimp/assimp/pull/4163
* Check and limit maximum size of glTF by @RichardTea in https://github.com/assimp/assimp/pull/4160
* 3DS Export: Add support for aiShadingMode_PBR_BRDF by @RichardTea in https://github.com/assimp/assimp/pull/4159
* Add assimpjs link to the ports in the readme file by @kovacsv in https://github.com/assimp/assimp/pull/4150
* Fix out-of-bounds read in RemoveLineComments by @alpire in https://github.com/assimp/assimp/pull/4147
* Allow the gltf2 Importer to optionally use glTF 2.0 JSON schemas for initial validation by @MalcolmTyrrell in https://github.com/assimp/assimp/pull/4111
* Disable m3d for 5.1 by @kimkulling in https://github.com/assimp/assimp/pull/4172
* glTF2 skinning related fixes by @ifree in https://github.com/assimp/assimp/pull/4174

## New Contributors
* @grdowns made their first contribution in https://github.com/assimp/assimp/pull/2680
* @vcebollada made their first contribution in https://github.com/assimp/assimp/pull/2675
* @feniksa made their first contribution in https://github.com/assimp/assimp/pull/2699
* @mdinim made their first contribution in https://github.com/assimp/assimp/pull/2677
* @TGEnigma made their first contribution in https://github.com/assimp/assimp/pull/2690
* @jherico made their first contribution in https://github.com/assimp/assimp/pull/2722
* @ffontaine made their first contribution in https://github.com/assimp/assimp/pull/2697
* @Zalewa made their first contribution in https://github.com/assimp/assimp/pull/2698
* @quanterion made their first contribution in https://github.com/assimp/assimp/pull/2726
* @tanolino made their first contribution in https://github.com/assimp/assimp/pull/2744
* @bztsrc made their first contribution in https://github.com/assimp/assimp/pull/2736
* @jcarpent made their first contribution in https://github.com/assimp/assimp/pull/2758
* @thewoz made their first contribution in https://github.com/assimp/assimp/pull/2748
* @da-raf made their first contribution in https://github.com/assimp/assimp/pull/2727
* @coryf made their first contribution in https://github.com/assimp/assimp/pull/2779
* @dylankenneally made their first contribution in https://github.com/assimp/assimp/pull/2776
* @apanteleev made their first contribution in https://github.com/assimp/assimp/pull/2839
* @9739654 made their first contribution in https://github.com/assimp/assimp/pull/2843
* @malortie made their first contribution in https://github.com/assimp/assimp/pull/2838
* @yunqiangshanbill made their first contribution in https://github.com/assimp/assimp/pull/2853
* @felipeek made their first contribution in https://github.com/assimp/assimp/pull/2855
* @Dunni made their first contribution in https://github.com/assimp/assimp/pull/2863
* @runforu made their first contribution in https://github.com/assimp/assimp/pull/2892
* @tellypresence made their first contribution in https://github.com/assimp/assimp/pull/2960
* @ms-maxvollmer made their first contribution in https://github.com/assimp/assimp/pull/2970
* @hbina made their first contribution in https://github.com/assimp/assimp/pull/2976
* @inhosens made their first contribution in https://github.com/assimp/assimp/pull/3009
* @shawwn made their first contribution in https://github.com/assimp/assimp/pull/3057
* @maquefel made their first contribution in https://github.com/assimp/assimp/pull/3034
* @aimoonchen made their first contribution in https://github.com/assimp/assimp/pull/3075
* @napina made their first contribution in https://github.com/assimp/assimp/pull/3086
* @Hehongyuanlove made their first contribution in https://github.com/assimp/assimp/pull/3158
* @timuber made their first contribution in https://github.com/assimp/assimp/pull/3149
* @lukka made their first contribution in https://github.com/assimp/assimp/pull/3168
* @sercand made their first contribution in https://github.com/assimp/assimp/pull/3209
* @rudybear made their first contribution in https://github.com/assimp/assimp/pull/3105
* @pseiderer made their first contribution in https://github.com/assimp/assimp/pull/3146
* @hoshiryu made their first contribution in https://github.com/assimp/assimp/pull/3175
* @dmgolembiowski made their first contribution in https://github.com/assimp/assimp/pull/3195
* @jercytryn made their first contribution in https://github.com/assimp/assimp/pull/3214
* @kalyan-kumar made their first contribution in https://github.com/assimp/assimp/pull/3215
* @M4T1A5 made their first contribution in https://github.com/assimp/assimp/pull/3226
* @rmstyrczula made their first contribution in https://github.com/assimp/assimp/pull/3235
* @kshepherd2013 made their first contribution in https://github.com/assimp/assimp/pull/3233
* @AlecLafita made their first contribution in https://github.com/assimp/assimp/pull/3263
* @DavidBerger98 made their first contribution in https://github.com/assimp/assimp/pull/3271
* @infosia made their first contribution in https://github.com/assimp/assimp/pull/3288
* @Evangel63 made their first contribution in https://github.com/assimp/assimp/pull/3298
* @ifiddynine made their first contribution in https://github.com/assimp/assimp/pull/3293
* @mahiuchun made their first contribution in https://github.com/assimp/assimp/pull/3301
* @IOBYTE made their first contribution in https://github.com/assimp/assimp/pull/3320
* @AndrewJDR made their first contribution in https://github.com/assimp/assimp/pull/3322
* @MeyerFabian made their first contribution in https://github.com/assimp/assimp/pull/3337
* @awr1 made their first contribution in https://github.com/assimp/assimp/pull/3328
* @qarmin made their first contribution in https://github.com/assimp/assimp/pull/3356
* @lsliegeo made their first contribution in https://github.com/assimp/assimp/pull/3372
* @fuj1n made their first contribution in https://github.com/assimp/assimp/pull/3374
* @flowtsohg made their first contribution in https://github.com/assimp/assimp/pull/3386
* @jnhyatt made their first contribution in https://github.com/assimp/assimp/pull/3385
* @Naios made their first contribution in https://github.com/assimp/assimp/pull/3391
* @crocdialer made their first contribution in https://github.com/assimp/assimp/pull/3383
* @Optimizer0 made their first contribution in https://github.com/assimp/assimp/pull/3413
* @stromaster made their first contribution in https://github.com/assimp/assimp/pull/3446
* @jsmaatta made their first contribution in https://github.com/assimp/assimp/pull/3454
* @xiaozhuai made their first contribution in https://github.com/assimp/assimp/pull/3231
* @MomoDeve made their first contribution in https://github.com/assimp/assimp/pull/3471
* @maiqingqiang made their first contribution in https://github.com/assimp/assimp/pull/3463
* @traversaro made their first contribution in https://github.com/assimp/assimp/pull/3455
* @Nodrev made their first contribution in https://github.com/assimp/assimp/pull/3501
* @Neil-Clifford-FB made their first contribution in https://github.com/assimp/assimp/pull/3504
* @natanaeljr made their first contribution in https://github.com/assimp/assimp/pull/3503
* @boguscoder made their first contribution in https://github.com/assimp/assimp/pull/3512
* @huiji12321 made their first contribution in https://github.com/assimp/assimp/pull/3521
* @Biswa96 made their first contribution in https://github.com/assimp/assimp/pull/3518
* @wasd845 made their first contribution in https://github.com/assimp/assimp/pull/3540
* @someonewithpc made their first contribution in https://github.com/assimp/assimp/pull/3555
* @makitsune made their first contribution in https://github.com/assimp/assimp/pull/3562
* @gris-martin made their first contribution in https://github.com/assimp/assimp/pull/3543
* @Danny-Kint made their first contribution in https://github.com/assimp/assimp/pull/3552
* @ihsinme made their first contribution in https://github.com/assimp/assimp/pull/3569
* @JacksonM8 made their first contribution in https://github.com/assimp/assimp/pull/3571
* @gkv311 made their first contribution in https://github.com/assimp/assimp/pull/3578
* @h3xx made their first contribution in https://github.com/assimp/assimp/pull/3591
* @mmp made their first contribution in https://github.com/assimp/assimp/pull/3580
* @xiaohunqupo made their first contribution in https://github.com/assimp/assimp/pull/3594
* @villevoutilainen made their first contribution in https://github.com/assimp/assimp/pull/3608
* @bsekura made their first contribution in https://github.com/assimp/assimp/pull/3604
* @contriteobserver made their first contribution in https://github.com/assimp/assimp/pull/3607
* @lgmcode made their first contribution in https://github.com/assimp/assimp/pull/3644
* @jkvargas made their first contribution in https://github.com/assimp/assimp/pull/3653
* @ademets made their first contribution in https://github.com/assimp/assimp/pull/3660
* @AndyShawQt made their first contribution in https://github.com/assimp/assimp/pull/3689
* @Paul-Austria made their first contribution in https://github.com/assimp/assimp/pull/3691
* @jaefunk made their first contribution in https://github.com/assimp/assimp/pull/3697
* @bekraft made their first contribution in https://github.com/assimp/assimp/pull/3722
* @vfxgordon made their first contribution in https://github.com/assimp/assimp/pull/3721
* @eliasdaler made their first contribution in https://github.com/assimp/assimp/pull/3723
* @BaldricS made their first contribution in https://github.com/assimp/assimp/pull/3746
* @clems71 made their first contribution in https://github.com/assimp/assimp/pull/3695
* @pcwalton made their first contribution in https://github.com/assimp/assimp/pull/3749
* @urschanselmann made their first contribution in https://github.com/assimp/assimp/pull/3708
* @Garux made their first contribution in https://github.com/assimp/assimp/pull/3742
* @krishty made their first contribution in https://github.com/assimp/assimp/pull/3774
* @JC3 made their first contribution in https://github.com/assimp/assimp/pull/3821
* @diiigle made their first contribution in https://github.com/assimp/assimp/pull/3775
* @ericwa made their first contribution in https://github.com/assimp/assimp/pull/3808
* @impala454 made their first contribution in https://github.com/assimp/assimp/pull/3901
* @Saalvage made their first contribution in https://github.com/assimp/assimp/pull/3900
* @spotaws made their first contribution in https://github.com/assimp/assimp/pull/3858
* @dlyr made their first contribution in https://github.com/assimp/assimp/pull/3912
* @crud89 made their first contribution in https://github.com/assimp/assimp/pull/3894
* @ogjamesfranco made their first contribution in https://github.com/assimp/assimp/pull/3903
* @jagoon made their first contribution in https://github.com/assimp/assimp/pull/3916
* @andreasbuhr made their first contribution in https://github.com/assimp/assimp/pull/3961
* @jerstlouis made their first contribution in https://github.com/assimp/assimp/pull/3966
* @Skylion007 made their first contribution in https://github.com/assimp/assimp/pull/3964
* @Pankaj003 made their first contribution in https://github.com/assimp/assimp/pull/3965
* @Promit made their first contribution in https://github.com/assimp/assimp/pull/3969
* @irajsb made their first contribution in https://github.com/assimp/assimp/pull/3980
* @mlopezantequera made their first contribution in https://github.com/assimp/assimp/pull/3986
* @uerobert made their first contribution in https://github.com/assimp/assimp/pull/4027
* @Mykhailo418 made their first contribution in https://github.com/assimp/assimp/pull/4026
* @kovacsv made their first contribution in https://github.com/assimp/assimp/pull/4037
* @yzthr made their first contribution in https://github.com/assimp/assimp/pull/4036
* @Dig-Doug made their first contribution in https://github.com/assimp/assimp/pull/4056
* @Spectrum76 made their first contribution in https://github.com/assimp/assimp/pull/4012
* @kirillsurkov made their first contribution in https://github.com/assimp/assimp/pull/4054
* @ingowald made their first contribution in https://github.com/assimp/assimp/pull/4081
* @markoffline made their first contribution in https://github.com/assimp/assimp/pull/4083
* @davidepi made their first contribution in https://github.com/assimp/assimp/pull/4108
* @marcappelsmeier made their first contribution in https://github.com/assimp/assimp/pull/4110
* @diharaw made their first contribution in https://github.com/assimp/assimp/pull/4112
* @Daniel-Genkin made their first contribution in https://github.com/assimp/assimp/pull/4142
* @alpire made their first contribution in https://github.com/assimp/assimp/pull/4146
* @Artenuvielle made their first contribution in https://github.com/assimp/assimp/pull/4079
* @ifree made their first contribution in https://github.com/assimp/assimp/pull/4174

**Full Changelog**: https://github.com/assimp/assimp/compare/v5.0.0...v5.1.0

#  5.0.1
- Fix wrong version 
- Fix MacOS compile issue.
- Add pdf-docs

# 5.0.0 
 - Bugfixes:
    - https://github.com/assimp/assimp/issues/2551: Collada output path is worng when that is exported.
    - https://github.com/assimp/assimp/issues/2603: Corrupted normals loaded from x-file.
    - https://github.com/assimp/assimp/issues/2598: introduce getEpsilon
    - https://github.com/assimp/assimp/issues/2613: merge glTF2 patch
    - https://github.com/assimp/assimp/issues/2653: Introduce 2 tests to reproduce fbx-tokenize issue.
    - https://github.com/assimp/assimp/issues/2627: Remove code from ai_assert test, will be removed in release versions.
    - https://github.com/assimp/assimp/issues/2618: Compilation fails with latest MinGW
    - https://github.com/assimp/assimp/issues/2614: FBX Crash on import
    - https://github.com/assimp/assimp/issues/2596: Stop JoinVerticiesProcess removing bones from mesh 
    - https://github.com/assimp/assimp/issues/2599: Multiconfig debug postfix
    - https://github.com/assimp/assimp/issues/2570: Update config.h.in
    - https://github.com/assimp/assimp/issues/1623: Crash when loading multiple PLY files
    - https://github.com/assimp/assimp/issues/2571: Extra layer for multi uv sets
    - https://github.com/assimp/assimp/issues/2557: Fix CMake exporter macro
    - https://github.com/assimp/assimp/issues/1623: Crash when loading multiple PLY files
    - https://github.com/assimp/assimp/issues/2548: Check if weight are set or set the weight to 1.0f
    - https://github.com/assimp/assimp/issues/1612: Make wstaring handling depend from encoding of the filename.
    - https://github.com/assimp/assimp/issues/1642: Fix build on Hurd
    - https://github.com/assimp/assimp/issues/1460: Skip uv- and color-components if these are not defined.
    - https://github.com/assimp/assimp/issues/1638: Use memcpy instead of dynamic_cast.
    - https://github.com/assimp/assimp/issues/1574: Add API to get name of current branch.
    - https://github.com/assimp/assimp/issues/2439: Add null ptr test before calling hasAttr.
    - https://github.com/assimp/assimp/issues/2527: Use correct macro for Assimp-exporter.
    - https://github.com/assimp/assimp/issues/2368: Just fix it.
    - https://github.com/assimp/assimp/issues/2507: Collada_metadata' branch breaks loading of collada files
    - https://github.com/assimp/assimp/issues/2368: Add missign anim mesh for multimaterial meshes in fbx.
    - https://github.com/assimp/assimp/issues/2431: Use irrXml directly for mingw compiler.
    - https://github.com/assimp/assimp/issues/1660: Use memcpy instead of a c-style dynamic cast to parse a aiVector3D
    - https://github.com/assimp/assimp/issues/1722: Use a const reference to fix issue with ppc.
    - https://github.com/assimp/assimp/issues/1390: aiScene now stores metadata as well.
    - https://github.com/assimp/assimp/issues/1721: set camera parameters instead of nonsense.
    - https://github.com/assimp/assimp/issues/1728: check if mesh is a null instance before dereferencing it.
    - https://github.com/assimp/assimp/issues/1721: set camera param…
    - https://github.com/assimp/assimp/issues/1743: introduce /bigobj compile flag.
    - https://github.com/assimp/assimp/issues/905:  Add missing contrib source from zlib.
    - https://github.com/assimp/assimp/issues/1758: Fix compiler warning.
    - https://github.com/assimp/assimp/issues/1752: Move guard over include statement.
    - https://github.com/assimp/assimp/issues/1583: Update doc.
    - https://github.com/assimp/assimp/issues/774:  Use correct type for unitscale in fbx.
    - https://github.com/assimp/assimp/issues/1729: check for bit flip when unsigned int overflow happens in x-file parsing.
    - https://github.com/assimp/assimp/issues/1386: fix undefined behaviour in compare function.
    - https://github.com/assimp/assimp/issues/567:  prevend dependency cycle.
    - https://github.com/assimp/assimp/issues/1564: Remove copy constructor.
    - https://github.com/assimp/assimp/issues/1773: Make FileSystemFilter forward all virtual functions to wrapped IOSystem instance
    - https://github.com/assimp/assimp/issues/1120: use euler angles for pre- and post-rotation.
    - https://github.com/assimp/assimp/issues/1796: return correct value on detach logger.
    - https://github.com/assimp/assimp/issues/1850: remove buggy setup in cmake.
    - https://github.com/assimp/assimp/issues/1836: make documentation much more clear how to apply global scaling.
    - https://github.com/assimp/assimp/issues/1855: fix correction of node names.
    - https://github.com/assimp/assimp/issues/1831: make config CMAKE_LIBRARY_CONFIG overridable.
    - https://github.com/assimp/assimp/issues/1881: make template-based get and put in streamreader/writer public.
    - https://github.com/assimp/assimp/issues/1621: add file check for dxf file without extensions.
    - https://github.com/assimp/assimp/issues/1894: use mesh name to name exported obj node.
    - https://github.com/assimp/assimp/issues/1893: fix mem leak in glft2Importer.
    - https://github.com/assimp/assimp/issues/1784: change so.name to keep track of the minor version of the lib.
    - https://github.com/assimp/assimp/issues/842:  experimental suppor for ascii stl pointcloud export.
    - https://github.com/assimp/assimp/issues/919:  add missing cast for vs2015.
    - https://github.com/assimp/assimp/issues/1952: check for postprocessing parameter before try to parse -f
    - https://github.com/assimp/assimp/issues/1780: check against nullptr before accessing normal data in aiMesh instance.
    - https://github.com/assimp/assimp/issues/1970: stl with empty solid.
    - https://github.com/assimp/assimp/issues/1587: Add validation to LWS unit test.
    - https://github.com/assimp/assimp/issues/1973: Added support for CustomData(Layer) to support multiple (texture) UV mappings.
    - https://github.com/assimp/assimp/issues/2016: Only add material uv mappings if set, ignore when no uvmapping set.
    - https://github.com/assimp/assimp/issues/2019: fix the qt-viewer without export.
    - https://github.com/assimp/assimp/issues/2024: make code more readable.
    - https://github.com/assimp/assimp/issues/2011: add reference to free model.
    - https://github.com/assimp/assimp/issues/2001: Make glTF2 the default exporter for glft
    - https://github.com/assimp/assimp/issues/1340: Fix handling of empty nodes in openddl-parser.
    - https://github.com/assimp/assimp/issues/2449: fix uwp handling
    - https://github.com/assimp/assimp/issues/2459: fix duplicated fbx-type propertry.
    - https://github.com/assimp/assimp/issues/2334: run vc_redist in passive + quiet mode.
    - https://github.com/assimp/assimp/issues/2335: add cmake-modules to the installer.
    - https://github.com/assimp/assimp/issues/2054: make empty bone validation optional.
    - https://github.com/assimp/assimp/issues/2056: use correc exception type in MMD-loader.
    - https://github.com/assimp/assimp/issues/1724: add default material access to the material API.
    - https://github.com/assimp/assimp/issues/2075: Make inject of debug postfix an option, so you can swich it off.
    - https://github.com/assimp/assimp/issues/2088: fix possible out-of-bound access in fbx-lerp operaation.
    - https://github.com/assimp/assimp/issues/1451: break when assimp-bin format was exported with a different version.
    - https://github.com/assimp/assimp/issues/212: introduce unittest for line-splitter, will validate that the current behaviour is correct.
    - https://github.com/assimp/assimp/issues/2154: remove redundant file from source folder.
    - https://github.com/assimp/assimp/issues/2067: introduce /bigobj compiler flag
    - https://github.com/assimp/assimp/issues/2206: make bone error in verification more verbose.
    - https://github.com/assimp/assimp/issues/2199: introduce first version for exporter.
    - https://github.com/assimp/assimp/issues/2229: fix count of polylines when only one vertex was indexed.
    - https://github.com/assimp/assimp/issues/2210: use different enum value.
    - https://github.com/assimp/assimp/issues/2202: put STEPParser out of IFC importer.
    - https://github.com/assimp/assimp/issues/2247: change include folder from debian package from /usr/lib/include to /usr/include
    - https://github.com/assimp/assimp/issues/817: use emmisive factor instead of color.
    - https://github.com/assimp/assimp/issues/2251: introduce AI_CONFIG_PP_FID_IGNORE_TEXTURECOORDS to avoid removing textures.
    - https://github.com/assimp/assimp/issues/2297: introduce obj-unittest to validate working importer.
    - https://github.com/assimp/assimp/issues/2115: rollback setup of FBX-camera.
    - https://github.com/assimp/assimp/issues/1593: fix computation of percentf for 3DS.
    - https://github.com/assimp/assimp/issues/934: introduce material keys for shader types.
    - https://github.com/assimp/assimp/issues/1650: build irrXml as shared lib.
    - https://github.com/assimp/assimp/issues/2411: Revert parts of dynamic ixxxml linkage
    - https://github.com/assimp/assimp/issues/2336: use new cmp_048 policy even for zlib in the assimp build.
    - A crash in the aiMesh destructor
    - Unicode fix ( experimental, feedback is welcome )
    - alloc-dealloc-mismatch
    - fix for div by zero reported by address sanitizer
 - assimp_cmd: 
    - Add --verbose flag to 'info' command, to print node transforms.
    - assimp_cmd info: list meshes and print basic mesh stats.
    - print error message on failure.
    - Parse post process arguments when using info tool.
    - prettier and better-compressed node hierarchy.
 - Common Stuff:
    - Exporter::ExportToBlob() Pass on preprocessing and properties. Issue #2302
    - Use correct escape sequence for unsigned.
    - Ensure that the aiString lenght is 4 bytes independent which platform
    - Fix compiler warnings
    - Fix codacy issues
    - Implemented basic PBR materials into assimp.
      This adds the following texture types:
      - BASE_COLOR
      - NORMAL_CAMERA
      - EMISSION_COLOR
      - METALNESS
      - DIFFUSE_ROUGHNESS
    - Make IsVerbose accessible outside the exporter
    - Create FUNDING.yml
    - Add copyright headers to ZipArchiveIOSystem
    - diable unaigned pointer access temprary.
    - Fix signed unsigned comparison warnings.
    - Fixed anim meshes generated from blendshapes not being copied to output for multi-material meshes
    - All textures use relative path except embedded, this is fix for it.
    - I want to see what GetErrorString() blurts out on the Travis failure real quick
    - including <unzip.h> instead of <contrib/unzip/unzip.h>
    - Fix Matrix4x4t Decompose to rotation vector.
    - Add missing assignment operator to aiString.
    - fast_atof: Remove unused variable.
    - BaseImporter: Remove dead condition.
    - Some StreamWriter improvements / additions.
    - added support for embedded textures defined with buffer views.
    - fixed embedded texture reading.
    - BaseImporter: fix lookup for tokens during inmemory imports.
    - Reorg of code: Each importer / exporter / domain has its own folder.
    - Export: Copy metadata to be able to export it properly.
    - Some MSVC/Windows corrections and updates, issue 2302.
    - Introduce new log macros.
    -_stat64 doesn't seem to exist. use __stat64!
    - Fix strict aliasing violation in MaterialSystem
    - Added check to BaseImporter::SearchFileHeaderForToken making sure that a detected token is not 
      in fact just a fraction of a longer token.
    - added internal to_string.
    - Use delete[] instead of delete to clear mMeshes
    - Fix memory leak in assimp_loader
    - Fix VS2013: array initialization does not work.
    -  aiMatrix4x4t<TReal>::FromEulerAnglesXYZ modified to row order
    - Various additions/fixes (FBX blend-shapes support added)
      - Added animMesh name assignment at ColladaLoader
      - Fixed animMesh post-processing on ConvertToLhProcess (blend-shapes weren't being affected by post-processing)
      - Added WindowsStore define. This is used to change some incompatible WinRT methods
      - Added FBX blend-shapes and blend-shapes animations support
      - Added Maya FBX specific texture slots parsing
      - Added extra FBX metadata parsing
      - Added GLTF2 vertex color parsing
      - Fixed IFC-Loader zip-buffer reading rountine
      - Fixed OBJ file parsing line-breaker bug
      - Fixed IOStreamBuffer cache over-read bug
      - Added mName field to aiAnimMesh
      - Reverted EmissiveFactor, TransparencyFactor and Specular
 - Doc: 
    - Move to https://assimp-docs.readthedocs.io/en/latest/
    - Add more detailed information about the source code structure
    - Clarify the matrix layout
    - Document AI_MATKEY_REFLECTIVITY and AI_MATKEY_COLOR_REFLECTIVE.
    - Correct matrix layout documentation
    - Fix aiBone->mOffsetMatrix documentation, which was incorrect.
    - Fix the assimp.net link.
    - Expand the current documentation about loading of embedded textures.
    - Improve the contribution guide.
    - Add the HAXE-port.
 - Build:
    - Add example build script.
    - Update DLL PE details: Copyright, git commit hash and original filename.
    - Depreciated compiler which doesn't support standard features.
    - Fix error when building assimp on older Mac OS X version.
    - Update scene.h to use #include <cstdlib> only if the compiler is used to compile c++ code.
    - Fixed MSVC toolset versions >140.
    - Fixed android zlib compile error.
    - Export static libaries as CMake package.
    - Set directory and name properties for installing static lib PDB.
    - Use CMAKE_INSTALL_PREFIX as ASSIMP_ROOT_DIR.
    - ios-build-script
    - Add Inno setup path to PATH variable.
    - Assimp will now be correctly built with -O3 or -Og based on build type.
    - Add TARGET_INCLUDE_DIRECTORIES for assimp target
    - Use a more accurate way of checking if the build is 64 bit
    - Improved cmake configs for Windows and Linux.
    - Default CMAKE_DEBUG_POSTFIX to 'd' on multiconfig
    - Adds a way to select which exporters you want to compile
 - CI:
    - Fix Travis CI sign-compare warning
    - Travis updated to clang 5.0 and there are new issues
    - Appveyor + Travis: use caching
    - Use clange adress sanitizer + memory leak detection
    - Handle warnings as errors
    - Use Hunter for pulling in dependencies.
 - 3DS: 
    - Reformat initializer list
    - Add explicit default constructors and assignment operators to Material.
    - Add Material constructor which takes material name.
    - Add Mesh constructor with takes name.
    - Explicitly pass "UNNAMED" as 3DS root node name.
    - Fix more thread-safety issue in 3DS loader.
 - 3MF:
    - Introduce first prototype for basematerial support.
    - Fix parsing of base-material color.
    - Fix order of init list.
    - Use correct material assignment in case of multi-materials.
    - Add missig tags for meta data.
    - Fix model folder desc.
    - Fix CanRead-method for the 3MF-Importer.
 - ASE:
   - Reformat initializer list.
   - Add explicit default constructors and assignment operators to Material.
   - Add Material constructor which takes material name.
   - Pass a default material name when resizing materials buffer.
   - Fix more thread-safety issue in ASE loader.
 - AssJSon:
   - Add json export.
 - B3D
    - Use std::unique_ptr
 - BlenderLoader: 
    - Fix memory leak.
    - Update BlenderDNA.h.
 - Collada
    - Add Collada zae import support
    - fix possible memleak when throwing an exception.
    - Don't use SkipElement() to skip empty Text.
    - Correction on Collada parser missing textures when the image is in CDATA
    - Richard tea collada metadata
    - collada export: Use Camera local coordinate system
    - Save/Load Collada 1.4 Root Asset Metadata
    - Fix automatic name assignment for ColladaLoader when using name based assignment
 - DXF: 
    - Fix macro issues.
 - glFT
    - Add ortho camera support
    - Fix incorrect NO_GLTF_IMPORTER define name in glTFExporter.h
    - Fix delete / delete[] mismatch
    - glTFAsset: Use std:unique_ptr for Image data
    - Update o3dgcTimer.h
    - Added import of material properties (double sided and transparency) in glTF 1.0 importer.
    - Forced 4-bits alignment for glTF buffers
    - Fixed some gltf files being detected as OBJ
    - Added support for non-indexed meshes in glTF importer. Addresses issue #2046.
    - GLTF segfault using triangle strip
    - buffer grow changes and large files support
 - glFT2
    - Add ortho camera support.
    - Lights import.
    - Added support for generating glb2.
    - Not using external bin file for glb2.
    - Correctly export images with bufferView.
    - Using relative buffers URI.
    - Set camera "look at" to (0.0, 0.0, -1.0).
    - Assign default material to meshes with no material reference.
    - Import scale for normal textures and strength for occlusion textures.
    - Add vertex color support to glTF2 export.
    - Fix export gltf2, The JOINTS_0 componentType is incorrect.
    - Move creation of vars to avoid useless creation in case of an error.
    - Fix gltf2 export component type error
    - Change glTF2 file extensions from gltf2/glb2 to gltf/glb in the exporter so that it matches the importer and respect the standard specifications
    - Read and write the KHR_materials_unlit glTF/2.0 extension.
    - Pick scene zero as scene to recursively load if no "scene" property is specified.
    - Properly reads in glTF/2.0 sampler address modes.
    - Fix inconsistency between animation and node name in glTF2 Importer
    - Add test for glTF2  lines, line strip, lines loop and points
    - Fixes crash when importing invalid glTF/2.0 files
    - Skips some glTF/2.0 uv processing if the count of uvs in the attribute stream doesn't match the vertex count.
    - Make gltf2's roughnessAsShininess matches between importer and exporter.
    - Add support for importing GLTF2 animations.
    - glTF2 importer multiple primitives and 16-bit index buffer skinmesh support.
    - Fix glTF2 export with no texture coordinates
    - Fix inconsistency between animation and node name in glTF2 Importer
 - FBX
    - FBX Import: Properly clean up post_nodes_chain in case of exception.
    - Implemented basic PBR material textures
    - Added maya stingray support for textures
    - Added VertexColor to FBX exporter (one channel)
    - Fix FBX units not being converted from CM to application scale
    - FBX node chain assert fix
    - FBX importer armature fixes and root bone fixes - animations should now work for more models.
    - Fix Issue: group node in fbx being exported as bone node
    - Fix non-ascii encoding in comments in FBXMaterial.cpp
    - FBX files may use a texture reference from an embedded texture that hasn't been loaded yet. 
      This patch fixes this issue, storing all texture filenames, that can be acessed later via "scene::GetEmbeddedTexture", when all textures are already loaded.
      Some warnings have been added to other file formats that uses embedded data.
    - Fix export custom bindpose error
    - Some FBX multi-material mesh fixes
    - Fixed first vertex of each blendshape on a multi-material mesh having all unmapped vertice offsets being added to it
    - Fixed blendshapes not importing for multi-material FBX meshes with no bones
    - Store UnitScaleFactor for fbx-files.
    - Global settings use float instead of double.
    - Initial FBX Export Support, sponsored by MyDidimo (mydidimo.com).
    - FBX Importer double precision fix.
    - Apply inverse of geometric transform to child nodes.
    - Node names optimization and fixing non-unique name
    - Support for FBX file sizes more than 4GB.
    - Fragmented FBX ASCII emdedded resource.
    - Fix parse error for uv-coordinates.
    - Exception spam fix for FBXMaterial.
    - Fix empty fbx mesh names
    - fix for geometric transform nodes with multiple children.
    - FBX Export: Geometric transformations always create transformation chain.
    - FBX Export: fix logic for determining if scale transformation is identity.
    - FBX geometric transforms fix
    - FBX Export: handle newly-added geometric transform inverse nodes.
    - FBX Export: reconstruct full skeleton for any FBX deformers.
    - Fbx export skeleton improvements
    - FBX Export: add missing 0 value to file footer.
    - Fix Texture_Alpha_soutce typo.
    - LayeredTextures now work with embedded texture data
    - Assimp animation time is already in seconds. Just convert to FBX time.
    - Added check for NULL Compound in Properties70 element (fixes DeadlyImportError on some FBX files)
    - FBXImporter: Fix GetUniqueName to return names properly
    - Fix for crash in StreamWriter::PutString when exporting ASCII FBX
    - Don't call PutString with an empty string. Both DumpChildrenAscii and EndAscii can return without modifyting the string, 
      so we need to check the string before calling PutString. This used to cause a crash.
    - Fbx convert to unit 
    - Optimisation of FBX node name uniqueness
    - FBX import: fix import of direct data by vertices + unify node renaming
    - fixed ordering of skin indices and weights, to be consistent between systems
    - Add FBX Line Element support.
    - Preserve all the material parameters from FBX models
    - Fix for FBX binary tokenization of arrays of type 'c'
    - Generate attenuation constants if non are privded in the Blender file. Using: https://imdoingitwrong.wordpress.com/2011/01/31/light-attenuation/
    - Fix FBX face materials not being properly loaded if the face count mismatches the material count
 - LWO:
    - Initialize unnamed node uniqueness index outside of loop.
    - Use C++11 auto for easier refactoring.
    - Move some assignments to make it clearer when the thing should be moved.
 - IFC-2x3:
    - Fixed bug in IFC when dimensional exponent parameters in IfcSIUnits is not defined.
    - In IFC, fixed parser to avoid interpretation of '##' in string as identifiers
 - MD5:
    - MD5-Loader: set meshnames.
 - MDC:
    - Fix horrible pointer casting hack
    - fix a possible nullptr access.
 - MDLLoader: 
    - Replace raw pointer with vector to fix a memory leak
 - MMD:
    - Add  virtual destructor to a class which already has virtual functions
    - Fix memory leak
 - MDLImporter: 
    - Use unique_ptr
    - Fixes a double free
 - Obj:
    - Use unique_ptr
    - Fix possible memory leak
    - Fix line continuations in OBJ files
    - Fix buffer overflow in obj loader
    - Replace assertion by an exception in Obj parsing.
    - Fix material index off-by-one error in some OBJ files (seen in a C4D export).
    - Fixed test .obj file: OBJ Vertex Colors are expected to be floats (0-1).
    - Fix Issue #1923: OBJ Exporter can't correctly export vertex colors.
    - bad OBJ file that can still be read
    - Fix .obj displacement texture parsing
    - Fix expensive memory allocation and memory copying occurring in obj files with a large number of meshes.
    - Pointcloud support
    - OBJ: Coerce texture coords of nan or infinity to zero
    - a test for bad OBJ file format and code fix that handles it
    - Fix progress reporting in ObjFileParser. Remove old unused code which is claiming to still take up "1/3" of the total progress.
    - Obj: we can still import partially incorrect files
    - OBJ coerce invalid nan or inf texture coords to 0
 - Ogre: 
    - Change OgreXmlSerializer::HasAttribute parameter from std::string to pointer.
    - Avoid creating static std::strings.
    - Avoid creating function-scoped static constants.
    - Prevent crash when Ogre skeleton file is missing
 - OpenGEX: 
    - Replace raw pointer with vector to fix a memory leak.
    - Replace std::copy with explicit loop.
    - Use std::unique_ptr to fix some memory leaks.
 - Q3BSP: 
    - Convert Q3BSP Importer to use ZipArchiveIOSystem
    - Add assertion to silence a static analyzer warning
 - PLY:
    - float-color.ply is broken.
    - Fix for undefined behavior when loading binary PLY.
    - PLY importer should not create faces.
    - Set primitive_type to point when PLY is a point cloud.
    - Add support for texture file in PLY exports.
    - PLY importer should not automatically create faces.
    - Fix ply exporter to be conformant to spec respecting vertex colors.
    - Fix ply exporter to be conformant to spec respecting vertex colors.
 - Smd 
    - Cannot read bone names containing spaces
 - STL:
    - Fix white ambient in STL loader
    - Fix import of binary STL files in double-precision builds.
    - STL binary Export should write 4-byte floats for vertex and normal coordinates.
    - Add mesh names to imported ASCII STL.
    - STL-Exporter: fix division by zero in normalize method during update
 - XGLLoader: 
    - Fix const issue when seeting vec2.
    - Fix a memory leak
 - X3DImporter: 
    - Add virtual destructors to some classes which already have virtual functions
 - X:
    - fix out of bound access.
    - Release x-file-based scene when throwing an exception.
    - Fix exception.
    - Fix typo.
    - Add misisng bracket.
 - Postprocessing    
    - Fix UV texture coords generation along Z Axis
    - Thomasbiang fix issue join identical vertices not needed.
    - Fix missing File Scale and Application scale fixes for all conversions.
    - ScaleProcess overhauled to improve compatibility with animations and unit conversion.
    - All textures use relative path except embedded textures, this is a fix for it.
    - Add gen-aabb process to postprocessing.
    - Embedding images post-process.
    - PretransformVertices: Rearrange some assignments to clarify things.
    - LimitBoneWeightsProcess: Initialize all members of Weight in constructor.
    - FindInstancesProcessOptim: Optim FindInstancesProcess.
    - Optim FindInstancesProcess: ComputePositionEpsilon() is a costly function which was called in the inner
      loop although it only uses data from upper loop.
    - Degenerate triangles with small area removing fix
    - ImproveCacheLocality crashes if non triangular faces
    -  Add mesh name to ValidateDataStructure log
    - Fix GenVertexNormals
    - Added forced generation of normals with extra flag.
    - Force generating mesh normals.
    - Deactivate area based rejection of triangles in triangulation
    - JoinVerticesProcess should only try to deduplicate used vertices.
    - Feature/join vertices processor kill unused vertices
    - FlipUVsProcess should also process AnimMeshes (if any)
    - Allow findDegenerate and SplitLargeMesh to pass point clouds models.
    - integrated DropFaceNormals process (cmake, poststepregistry)
    - ValidateDataStructure.cpp:
      - Fixed warnings introduced by last commit (hopefully)
      - Fixed case fallthrough (due to exception flow, it didn't make a practical difference, but hopefully will remove a warning)
      - Minor formatting consistency improvements
 - Tools:
    - Remove the qt-viewer from the build system and move it into its own repo.
    - Fix AssimpView build.
    - Fix Assimp viewer build with MinGW
 - Examples:
    - Update Sample_SimpleOpenGL.c
 - Java-Port:
     - JAssimp: fix simple code analysis issues.
     - Fix another misinterpretation from the JNI-interface.
     - Add progresshandler support jassimp
 - .Net-Port:
    - Fix documentation for assimp.net.
    - Update AssimpNet redirect
 - Python-port:
    - Version bump pyassimp.
    - Fix indentation error in python bindings.
    - Remove check for 'assimp' in name of directories to be searched for library in python port.
    - Add pyassimp code generation script for materials.
    - Ensure obj is not null before using hasattr.
    - Minor changes to setup.py to make it slightly more compliant
    - Solved pyassimp.errors.AssimpError in conda
    - Build Python representation for metadata.
    - Fix "bytes aketrans" issues in Python>=3.1
    - Fix: except `SyntaxError` for py3 viewer
 - Android:
    - Added ASSIMP_ANDROID_JNIIOSYSTEM precheck to only remain set to ON in proper ANDROID enabled toolchain environment
 - zlib & unzip
    - unzip: fix build with older zlib
 - irrXml:
    - IrrXML doesn't recognise the construction: `<author></author>` as being an empty element, and 
      so ColladaParser::TestTextContent advances the element stream into the `</author>` element.

# 4.1.0 (2017-12):
## FEATURES:
 - Export 3MF ( experimental )
 - Import / Export glTF 2
 - Introduce new zib-lib to eb able to export zip-archives
- FIXES/HOUSEKEEPING:
 - Added missing include to stdlib.h and remove load library call
 - Fix install for builds with MSVC compiler and NMake.
 - Update list of supported file formats.
 - Add TriLib to the official list of supported ports.
 - Re-enabling PACK_STRUCT for MDL files.
 - Use std.::unique_ptr
 - Update D3MFExporter.h
 - Update MD3Loader.cpp, using index
 - Fix all warnings on MSVC14
 - Copy assimp dll to unit folder on windows
 - Update jvm port supported formats
 - Add support for building Mac OS X Framework bundles
 - Check for nullptr dereferencing before copying scene data
 - Update ValidateDataStructure.h, typo
 - Enable data structure validation in cases where it doesn't cause failures
 - Remove some dead assignments
 - fast_atof: Silence some uninitialized variable warnings
 - Check for area test if the face is a triangle.
 - Set mNumUVComponents to 0 when deleting texture coordinate sets
 - Only scale the root node because this will rescale all children nodes as well.
 - Issue 1514: Fix frame pointer arithmetic
 - Prevent failing stringstream to crash the export process
 - powf -> pow
 - add Defines.h to include folder for install.
 - Android:
  - Fix android build
  - Fix assimp for cross compile for android
  - Use define for D_FILE_OFFSET_BITS only for not-android systems.
 - FBX:
  - Fix handling with embedded textures
  - FBX 7500 Binary reading
  - Remove dead assignment
  - Fix export of deleted meshes; Add LazyDict::Remove method
  - Log an error instead of letting the fbx-importer crash. ( issue 213 )
  - Replace bad pointer casting with memcpy
  - Remove useless const qualifier from return value
  - Add explicit instantiation of log_prefix so other FBX source files can see it
  - add missing inversion of postrotation matrix for fbx.
  - FIReader: Silence uninitialized variable warning
  - Update version check in FBX reader to check for version >= 7500
  - Use actual min/max of anim keys when start/stop time is missing
- GLTF1:
 - Fix output of glTF 1 version string
 - Fix delete / delete[] mismatch in glTFAsset
 - Don’t ignore rgba(1,1,1,1) color properties
 - glTF2 primitives fixes
 - Don’t ignore rgba(1,1,1,1) color properties
 - Fix delete / delete[] mismatch in glTFAsset
 - Remove KHR_binary_glTF code
 - glTF nodes can only hold one mesh. this simply assigns to and check’s a Node’s Mesh
 - version in glb header is stored as uint32_t
- GLTF2:
 - node name conflict fix
 - Fix transform matrices multiplication order
 - Preserve node names when importing
 - Add support for tangents in import
 - Fix typo on gltf2 camera parameters
 - Moved byteStride from accessor to bufferView
 - Implemented reading binary glTF2 (glb) files
 - Fix signed/unsigned warning
 - Add postprocess step for scaling
 - Fix shininess to roughness conversion
 - Prefer “BLEND” over “MASK” as an alphaMode default
 - Approximate specularity / glossiness in metallicRoughness materials
 - Diffuse color and diffuse texture import and export improvements
 - Addressed some mismatched news/deletes caused by the new glTF2 sources.
 - Fix delete / delete[] mismatches in glTF2 importer
 - use correct name of exporter to gltf2
 - Fix possible infinite loop when exporting to gltf2
 - Fix glTF2::Asset::FindUniqueID() when the input string is >= 256 chars
 - Fix glTF2 alphaMode storage and reading
 - Fix glTF 2.0 multi-primitive support
 - Load gltf .bin files from correct directory
 - Add support for importing both glTF and glTF2 files
 - ampler improvements; Add new LazyDict method
 - Changes to GLTF2 materials
 - Remove Light, Technique references
 - Start removing materials common, and adding pbrSpecularGlossiness
 - Use !ObjectEmpty() vs. MemberCount() > 0
 - Working read, import, export, and write of gltf2 (pbr) material
 - Check in gltf2 models to test directory
 - Remove un-needed test models
 - Start managing and importing gltf2 pbr materials
 - Update glTF2 Asset to use indexes
 - Duplicate gltfImporter as gltf2Importer; Include glTF2 importer in CMake List
 - glTF2: Fix animation export
 - use opacity for diffuse alpha + alphaMode
- STL:
 - Restore import of multi mesh binary STLs
- Blender:
 - Silence warning about uninitialized member
- MDLImporter:
 - Don't take address of packed struct member
- assimp_cmd:
 - Fix strict-aliasing warnings
- Open3DGC:
 - Fix strict-aliasing warnings
 - Add assertions to silence static analyzer warnings
 - Remove redundant const qualifiers from return types
 - Fix some uninitialized variable warnings
 - Remove OPEN3DGC and compression references
- unzip:
 - Remove dead assignment
 - Bail on bad compression method
 - Fix possibly uninitialized variables
- clipper:
 - Add assertion to silence a static analyzer warning
- OpenDDLExport:
 - Reduce scope of a variable
 - Remove dead variable
 - Remove dead assignment
 - Fix another potential memory leak
- X3DImporter:
 - Add assertions to silence static analyzer warnings
 - Add missing unittest
 - Workaround for buggy Android NDK (issue #1361)
- TerragenLoader:
 - Remove unused variable
- SIBImporter:
 - Add assertions to silence static analyzer warnings
- IFC:
 - Remove dead code
 - Add explicit instantiation of log_prefix so IFCMaterial.cpp can see it
- PLY:
 - Remove dead assignment and reduce scope of a variable
 - fix vertex attribute lookup.
- OpenGEX:
 - Add assertion to silence a static analyzer warning
 - Fix for TextureFile with number in file name
 - Return early when element is TextureFile
- NFF:
 - Add assertions to silence static analyzer warnings
 - Split up some complicated assignments
- Raw: Fix misleading indentation warning
 - Reduce scope of a variable
- LWO
 - Reduce scope of a variable
- IRRLoader:
 - Fix confusing boolean casting
- AssbinExporter:
 - Add assertion to silence a static analyzer warning
- ASE:
 - Add assertion to silence a static analyzer warning
- AMFImporter:
 - Add assertion to silence a static analyzer warning
 - Add a block
- OptimizeGraph:
 - Fix possible null pointer dereference
 - RemoveRedundantMaterials:
 - Add assertion to silence a static analyzer warning
- ImproveCacheLocality:
 - Add assertion to silence a static analyzer warning
- RemoveRedundantMaterials:
 - Set pointer to nullptr after deleting it
- Travis:
 - Disable unit tests in scan-build config
 - Move slower builds earlier to improve parallelization
 - Add static analysis to build
 - Remove unused branch rule for travis.
 - Add Clang UBSan build configuration
 - Treat warnings as errors, without typos this time
- Unittests:
 - Add VS-based source groups for the unittests.
- Collada:
 - export <library_animations> tag
 - Update ColladaExporter.cpp
 - Silence uninitialized variable warning
 - Add support for line strip primitives
- Obj Wavefront:
 - check in exporting against out-of-bounds-access .
 - Issue 1351: use correct name for obj-meshname export for groups.
 - fix mem-lead: face will be not released in case of an error.
 - Anatoscope obj exporter nomtl
 - Raise exception when obj file contains invalid face indices
 - Added alternative displacement texture token in OBJ MTL material.
 - Obj: rename attribute from exporter.
 - Fix OBJ discarding all material names if the material library is missing
- Step:
 - use correct lookup for utf32
- MD2:
 - Fix MD2 frames containing garbage
- STL
 - add missing const.
 - Fix memory-alignment bug.
 - Fix issue 104: deal with more solids in one STL file.
- CMake
 - Fix issue 213: use correct include folder for assimp
- Doxygen
 - Fix issue 1513: put irrXML onto exclucde list for doxygen run
- PyAssimp:
 - Search for libassimp.so in LD_LIBRARY_PATH if available.
 - Fix operator precedence issue in header check
 - Split setup.py into multiple lines
 - Detect if Anaconda and fixed 3d_viewer for Python 3
 - created a python3 version of the 3dviewer and fixed the / = float in py3
- Blender:
 - Fix invalid access to mesh array when the array is empty.
 - Fix short overflow.
 - Silence warning about inline function which is declared but not defined
- JAssimp
 - Changed license header for IHMC contributions from Apache 2.0 to BSD
 - Add Node metadata to the Jassmip Java API
 - Added supported for custom IO Systems in Java. Implemented ClassLoader IO System
 - Added a link to pure jvm assimp port
- Clang sanitizer:
 - Undefined Behavior sanitizer
 - Fixed a divide by zero error in IFCBoolean that was latent, but nevertheless a bug
- B3DImporter:
 - Replace bad pointer casting with memcpy
- AppVeyor:
 - Cleanup and Addition of VS 2017 and running Tests
 - Fixed File Size reported as 0 in tests that use temporary files
 - x86 isn't a valid VS platform. Win32 it is, then.
 - Replaced the worker image name, which doesn't work as generator name, with a manually created generator name.
 - Cleaned up appveyor setup, added VS 2017 to the build matrix and attempted to add running of tests.
 - Treat warnings as errors on Appveyor
 - Disable warning 4351 on MSVC 2013
- OpenGEXImporter:
 - Copy materials to scene
 - Store RefInfo in unique_ptr so they get automatically cleaned up
 - Fix IOStream leak
 - Store ChildInfo in unique_ptr so they get automatically cleaned up
 - improve logging to be able to detect error-prone situations.
-  AMFImporter:
 - Fix memory leak
- UnrealLoader:
 - Fix IOStream leak
- Upgrade RapidJSON to get rid of a clang warning
- zlib:
 - Update zlib contribution
 - Removed unnecessary files from zlib contribution
 - Replaced unsigned long for the crc table to z_crc_t, to match what is returned by get-crc_table
- MakeVerboseFormat:
  - Fix delete / delete[] mismatches in MakeVerboseFormat
- MaterialSystem:
 - Fix out-of-bounds read in MaterialSystem unit test
- SIB:
 - Added support for SIB models from Silo 2.5
- AssbinExporter:
 - Fix strict aliasing violation
 - Add Write specialization for aiColor3D
- DefaultLogger:
 - Whitespace cleanup to fix GCC misleading indentation warning
- MDP:
 - Fix encoding issues.
 - PreTransformVertices:
 - fix name lost in mesh and nodes when load with flag
- C4D:
 - Fixes for C4D importer
- Unzip:
 - Latest greatest.

# 4.0.1 (2017-07-28)
- FIXES/HOUSEKEEPING:
- fix version test.
- Not compiling when using ASSIMP_DOUBLE_PRECISION
- Added support for python3
- Check if cmake is installed with brew
- Low performance in OptimizeMeshesProcess::ProcessNode with huge numbers of meshes
- Elapsed seconds not shown correctly
- StreamReader: fix out-of-range exception
- PPdPmdParser: fix compilation for clang

# 4.0.0 (2017-07-18)
## FEATURES:
- Double precision support provided ( available via cmake option )
- QT-Widget based assimp-viewer ( works for windows, linux, osx )
- Open3DGC codec supported by glFT-importer
- glTF: Read and write transparency values
- Add Triangulate post-processing step to glTF exporters
- Update rapidjson to v1.0.2
- Added method to append new metadata to structure
- Unittests: intoduce a prototype model differ
- X3D support
- AMF support
- Lugdunum3D support
- Obj-Importer: obj-homogeneous_coords support
- Obj-Importer: new streaming handling
	- Added support for 64 bit version header introduced in FbxSdk2016
	- Travis: enable coverall support.
	- PyAssimp: New version of the pyASSIMP 3D viewer, with much improved 3D controls
    - Morph animation support for collada
	- Added support for parameters Ni and Tf in OBJ/MTL file format
	- aiScene: add method to add children
	- Added new option to IFC importer to control tessellation angle + removed unused IFC option
	- aiMetaData: introduce aiMetaData::Dealloc
	- Samples: add a DX11 example
	- travis ci: test on OXS ( XCode 6.3 ) as well
	- travis ci: enable sudo support.
	- openddlparser: integrate release v0.4.0
	- aiMetaData: Added support for metadata in assbin format

## FIXES/HOUSEKEEPING:
- Introduce usage of #pragma statement
- Put cmake-scripts into their own folder
- Fix install pathes ( issue 938 )
- Fix object_compare in blender importer( issue 946 )
- Fix OSX compilation error
- Fix unzip path when no other version was found ( issue 967 )
- Set _FILE_OFFSET_BITS=64 for 32-bit linux ( issue 975 )
- Fix constructor for radjson on OSX
- Use Assimp namespace to fix build for big-endian architectures
- Add -fPIC to C Flags for 64bit linux Shared Object builds
- MDLLoader: fix resource leak.
- MakeVerboseFormat: fix invalid delete statement
- IFC: fix possible use after free access bug
- ComputeUVMappingprocess: add missing initialization for scalar value
- Fix invalid release of mat + mesh
- IrrImporter: Fix release functions
- Split mesh before exporting gltf ( issue 995 )
- 3MFImporter: add source group for visual studio
- IFC: Switch generated file to 2 files to fix issue related to <mingw4.9 ( Thanks Qt! )
- ObjImporter: fix test for vertices import
- Export scene combiner ( issues177 )
- FBX: make lookup test less strict ( issues 994 )
- OpenGEX-Importer: add import of vertex colors ( issue 954 )
- Fix bug when exporting mRotationKeys data
- Fix mingw build (mingw supports stat64 nowadays)
- cfileio: fix leaks by not closing files in the destructor
- Fix OBJ parser mtllib statement parsing bug.
- Q3BSP-Importer: remove dead code
- Fix BlenderDNA for clang cross compiler.
- ScenePreprocessor: fix invalid index counter.
- Fix compiler warnings ( issue 957 )
- Fix obj .mtl file loading
- Fixed a compile error on MSVC14 x64 caused by the /bigobj flag failing to be set for the 1 and 2-suffixed versions introduced in commit 0a25b076b8968b7ea2aa96d7d1b4381be2d72ce6
- Fixed build warnings on MSVC14 x64
- Remove scaling of specular exponent in OBJFileImporter.cpp
- use ai_assert instead of assert ( issue 1076 )
- Added a preprocessor definition for MSVC to silence safety warnings regarding C library functions. This addresses all warnings for MSVC x86 and x64 when building zlib, tools and viewer as a static lib
- fix parsing of texture name ( issue 899 )
- add warning when detecting invalid mat definition ( issue 1111 )
- copy aiTexture type declaration instead of using decltype for declaration to fix iOS build( issue 1101 )
- FBX: Add additional material properties
- FBX: Correct camera position and clip planes
- FBX: Add correct light locations and falloff values
- fix typo ( issue 1141 )
- Fix collada export. Don't duplicate TEXCOORD/NORMALS/COLORS in <vertices> and <polylist> ( issue 1084 )
- OBJParser: set material index when changing current material
- OBJ: check for null mesh before updating material index
- add vertex color export support ( issue 809 )
- Fix memory leak in Collada importer ( issue 1169 )
- add stp to the list of supported extensions for step-files ( issue 1183 )
- fix clang build ( Issue-1169 )
- fix for FreeBSD
- Import FindPkgMacros to main CMake Configuration
- Extended support for tessellation parameter to more IFC shapes
- defensice handling of utf-8 decode issues ( issue 1211 )
- Fixed compiler error on clang 4.0 running on OSX
- use test extension for exported test files ( issue 1228 )
- Set UVW index material properties for OBJ files
- Fixed no member named 'atop' in global namespace issue for Android NDK compilation
- Apply mechanism to decide use for IrrXML external or internal
- Fix static init ordering bug in OpenGEX importer
- GLTF exporter: ensure animation accessors have same count
- GLTF exporter: convert animation time from ticks to seconds
- Add support for reading texture coordinates from PLY meshes with properties named 'texture_u' and 'texture_v'
- Added TokensForSearch in BlenderLoader to allow CanRead return true for in-memory files.
- Fix wrong delete ( issue 1266 )
- OpenGEX: fix invalid handling with color4 token ( issue 1262 )
- LWOLoader: fix link in loader description
- Fix error when custom CMAKE_C_FLAGS is specified
- Fast-atof: log overflow errors
- Obj-Importer: do not break when detecting an overflow ( issue 1244 )
- Obj-Importer: fix parsing of multible line data definitions
- Fixed bug where IFC models with multiple IFCSite only loaded 1 site instead of the complete model
- PLYImporter: - optimize memory and speed on ply importer / change parser to use a file stream - manage texture path in ply
- Import - manage texture coords on faces in ply import - correction on point cloud faces generation
- Utf8: integrate new lib ( issue 1158 )
- Fixed CMAKE_MODULE_PATH overwriting previous values
- OpenGEX: Fixed bug in material color processing ( issue 1271 )
- SceneCombiner: move header for scenecombiner to public folder.
- GLTF exporter: ensure buffer view byte offsets are correctly aligned
- X3D importer: Added EXPORT and IMPORT to the list of ignored XML tags
- X3D Exporter: fixed missing attributes
- X3D importer: Fixed import of normals for the single index / normal per vertex case
- X3D importer: Fixed handling of inlined files
- X3D importer: fixed whitespace handling (issue 1202)
- X3D importer: Fixed iterator on MSVC 2015
- X3D importer: Fixed problems with auto, override and regex on older compilers
- X3D importer: Fixed missing header file
- X3D importer: Fixed path handling
- X3D importer: Implemented support for binary X3D files
- Fix build without 3DS ( issue 1319 )
- pyassimp: Fixed indices for IndexedTriangleFanSet, IndexedTriangleSet and IndexedTriangleStripSet
- Fixes parameters to pyassimp.load
- Obj-Importe: Fixed texture bug due simultaneously using 'usemtl' and 'usemap' attributes
- check if all exporters are disabled ( issue 1320 )
- Remove std functions deprecated by C++11.
- X-Importer: make it deal with lines
- use correct path for compilers ( issue 1335 )
- Collada: add workaround to deal with polygon with holes
- update python readme
- Use unique node names when loading Collada files
- Fixed many FBX bugs

## API COMPATIBILITY:
- Changed ABI-compatibility to v3.3.1, please rebuild your precompiled libraries ( see issue 1182 )
- VS2010 outdated

# 3.3.1 (2016-07-08)
## FIXES/HOUSEKEEPING:
- Setup of default precision for 17 exporters
- Fix xcode project files
- Fix BlenderTesselator: offsetof operator
- Invalid version in cmake file
- Update pstdint.h to latest greatest


# 3.3.0 (2016-07-05)
## FEATURES:
- C++11 support enabled
- New regression-test-UI
- Experimental glTF-importer support
- OpenGEX: add support for cameras and lights
- C4D: update to latest Melange-SDK
- Add a gitter channel
- Coverity check enabled
- Switch to <...> include brackets for public headers
- Enable export by pyAssimp
- CI: check windows build
- Add functionality to perform a singlepost-processing step
- Many more, just check the history

## FIXES/HOUSEKEEPING:
- Fix of many resource leaks in unittests and main lib
- Fix iOS-buildfor X64
- Choosing zlib manually for cmake
- many more, just check the history

# 3.2.1 (2016-010-10)
## FEATURES:
- Updated glTF exporter to meet 1.0 specification.

## FIXES/HOUSEKEEPING:
- Fixed glTF Validator errors for exported glTF format.

# ISSUES:
- Hard coded sampler setting for
  - magFilter
  - minFilter
- void* in ExportData for accessor max and min.

# 3.2.0 (2015-11-03)
## FEATURES:
- OpenDDL-Parser is part of contrib-source.
- Experimental OpenGEX-support
- CI-check for linux and windows
- Coverity check added
- New regression testsuite.
## FIXES/HOUSEKEEPING:
- Hundreds of bugfixes  in all parts of the library
- Unified line endings
## API COMPATIBILITY:
- Removed precompiled header to increase build speed for linux

# 3.1.1 (2014-06-15)
## FEATURES:
- Support for FBX 2013 and newer, binary and ASCII (this is partly work from Google Summer of Code 2012)
- Support for OGRE binary mesh and skeleton format
- Updated BLEND support for newer Blender versions
- Support for arbitrary meta data, used to hold FBX and DAE metadata
- OBJ Export now produces smaller files
- Meshes can now have names, this is supported by the major importers
- Improved IFC geometry generation
- M3 support has been removed
## FIXES/HOUSEKEEPING:
- Hundreds of bugfixes in all parts of the library
- CMake is now the primary build system
## API COMPATIBILITY:
- 3.1.1 is not binary compatible to 3.0 due to aiNode::mMetaData and aiMesh::mName
- Export interface has been cleaned up and unified
- Other than that no relevant changes

# 3.0.0 (2012-07-07)
## FEATURES:
- New export interface similar to the import API.
- Supported export formats: Collada, OBJ, PLY and STL
- Added new import formats: XGL/ZGL, M3 (experimental)
- New postprocessing steps: Debone
- Vastly improved IFC (Industry Foundation Classes) support
- Introduced API to query importer meta information (such as supported format versions, full name, maintainer info).
- Reworked Ogre XML import
- C-API now supports per-import properties
## FIXES/HOUSEKEEPING:
- Hundreds of bugfixes in all parts of the library
- Unified naming and cleanup of public headers
- Improved CMake build system
- Templatized math library
- Reduce dependency on boost.thread, only remaining spot is synchronization for the C logging API
## API COMPATIBILITY:
- renamed headers, export interface, C API properties and meta data prevent compatibility with code written for 2.0, but in most cases these can be easily resolved
- Note: 3.0 is not binary compatible with 2.0

# 2.0 (2010-11-21)
## FEATURES:
- Add support for static Blender (*.blend) scenes
- Add support for Q3BSP scenes
- Add a windows-based OpenGL sample featuring texturing & basic materials
- Add an experimental progress feedback interface.
- Vastly improved performance (up to 500%, depending on mesh size and spatial structure) in some expensive postprocessing steps
- AssimpView now uses a reworked layout which leaves more space to the scene hierarchy window
- Add C# bindings ('Assimp.NET')
- Keep BSD-licensed and otherwise free test files in separate folders (./test/models and ./test/models-nonbsd).
## FIXES:
- Many Collada bugfixes, improve fault tolerance
- Fix possible crashes in the Obj loader
- Improve the Ogre XML loader
- OpenGL-sample now works with MinGW
- Fix Importer::FindLoader failing on uppercase file extensions
- Fix flawed path handling when locating external files
- Limit the maximum number of vertices, faces, face indices and weights that Assimp is able to handle. This is to avoid crashes due to overflowing counters.
- Updated XCode project files
- Further CMAKE build improvements
## API CHANGES:
- Add data structures for vertex-based animations (These are not currently used, however ...)
- Some Assimp::Importer methods are const now.

# 1.1 (2010-04-17)
## FEATURES:
- Vastly improved Collada support
- Add MS3D (Milkshape 3D) support
- Add support for Ogre XML static meshes
- Add experimental COB (TrueSpace) support
- Automatic test suite to quickly locate regressions
- D bindings (`dAssimp`)
- Python 2.n bindings (`PyAssimp`)
- Add basic support for Unicode input files (utf8, utf16 and utf32)
- Add further utilities to the `assimp` tool (xml/binary dumps, quick file stats)
- Switch to a CMAKE-based build system including an install target for unix'es
- Automatic evaluation of subdivision surfaces for some formats.
- Add `Importer::ReadFileFromMemory` and the corresponding C-API `aiReadFileFromMemory`
- Expose further math utilities via the C-API (i.e. `aiMultiplyMatrix4`)
- Move noboost files away from the public include directory
- Many, many bugfixes and improvements in existing loaders and postprocessing steps
- Documentation improved and clarified in many places.
- Add a sample on using Assimp in conjunction with OpenGL
- Distribution/packaging: comfortable SDK installer for Windows
- Distribution/packaging: improved release packages for other architectures
## CRITICAL FIXES:
- Resolve problems with clashing heap managers, STL ABIs and runtime libraries (win32)
- Fix automatic detection of file type if no file extension is given
- Improved exception safety and robustness, prevent leaking of exceptions through the C interface
- Fix possible heap corruption due to material properties pulled in incorrectly
- Avoid leaking in certain error scenarios
- Fix 64 bit compatibility problems in some loaders (i.e. MDL)
## BREAKING API CHANGES:
  - None -
## MINOR API BEHAVIOUR CHANGES:
- Change quaternion orientation to suit to the more common convention (-w).
- aiString is utf8 now. Not yet consistent, however.
