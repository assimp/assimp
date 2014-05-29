# Listing and grouping of all the source files.
# 1) Set the file lists for each component
# 2) Create a Source Group for each component, for IDE project orginization
# 3) Add libassimp using the file lists (eliminates duplication of file names between
#    source groups and library command)
#
cmake_minimum_required( VERSION 2.6 )
SET( HEADER_PATH ../include/assimp )

SET( COMPILER_HEADERS
	${HEADER_PATH}/Compiler/pushpack1.h
	${HEADER_PATH}/Compiler/poppack1.h
	${HEADER_PATH}/Compiler/pstdint.h
)
SOURCE_GROUP( Compiler FILES ${COMPILER_HEADERS})

SET( PUBLIC_HEADERS
	${HEADER_PATH}/anim.h
	${HEADER_PATH}/ai_assert.h
	${HEADER_PATH}/camera.h
	${HEADER_PATH}/color4.h
	${HEADER_PATH}/color4.inl
	${HEADER_PATH}/config.h
	${HEADER_PATH}/defs.h
	${HEADER_PATH}/cfileio.h
	${HEADER_PATH}/light.h
	${HEADER_PATH}/material.h
	${HEADER_PATH}/material.inl
	${HEADER_PATH}/matrix3x3.h
	${HEADER_PATH}/matrix3x3.inl
	${HEADER_PATH}/matrix4x4.h
	${HEADER_PATH}/matrix4x4.inl
	${HEADER_PATH}/mesh.h
	${HEADER_PATH}/postprocess.h
	${HEADER_PATH}/quaternion.h
	${HEADER_PATH}/quaternion.inl
	${HEADER_PATH}/scene.h
	${HEADER_PATH}/metadata.h
	${HEADER_PATH}/texture.h
	${HEADER_PATH}/types.h
	${HEADER_PATH}/vector2.h
	${HEADER_PATH}/vector2.inl
	${HEADER_PATH}/vector3.h
	${HEADER_PATH}/vector3.inl
	${HEADER_PATH}/version.h
	${HEADER_PATH}/cimport.h
	${HEADER_PATH}/importerdesc.h
	${HEADER_PATH}/Importer.hpp
	${HEADER_PATH}/DefaultLogger.hpp
	${HEADER_PATH}/ProgressHandler.hpp
	${HEADER_PATH}/IOStream.hpp
	${HEADER_PATH}/IOSystem.hpp
	${HEADER_PATH}/Logger.hpp
	${HEADER_PATH}/LogStream.hpp
	${HEADER_PATH}/NullLogger.hpp
	${HEADER_PATH}/cexport.h
	${HEADER_PATH}/Exporter.hpp
)

SET( Core_SRCS
	Assimp.cpp
)

SET( Boost_SRCS
	BoostWorkaround/boost/math/common_factor_rt.hpp
	BoostWorkaround/boost/foreach.hpp
	BoostWorkaround/boost/format.hpp
	BoostWorkaround/boost/scoped_array.hpp
	BoostWorkaround/boost/scoped_ptr.hpp
	BoostWorkaround/boost/shared_array.hpp
	BoostWorkaround/boost/shared_ptr.hpp
	BoostWorkaround/boost/make_shared.hpp
	BoostWorkaround/boost/static_assert.hpp
	BoostWorkaround/boost/tuple/tuple.hpp
)
SOURCE_GROUP(Boost FILES ${Boost_SRCS})

SET( Logging_SRCS
	${HEADER_PATH}/DefaultLogger.hpp
	${HEADER_PATH}/LogStream.hpp
	${HEADER_PATH}/Logger.hpp
	${HEADER_PATH}/NullLogger.hpp
	Win32DebugLogStream.h
	DefaultLogger.cpp
	FileLogStream.h
	StdOStreamLogStream.h
)
SOURCE_GROUP(Logging FILES ${Logging_SRCS})

SET( Common_SRCS
	fast_atof.h
	qnan.h
	BaseImporter.cpp
	BaseImporter.h
	BaseProcess.cpp
	BaseProcess.h
	Importer.h
	ScenePrivate.h
	PostStepRegistry.cpp
	ImporterRegistry.cpp
	ByteSwap.h
	DefaultProgressHandler.h
	DefaultIOStream.cpp
	DefaultIOStream.h
	DefaultIOSystem.cpp
	DefaultIOSystem.h
	CInterfaceIOWrapper.h
	Hash.h
	Importer.cpp
	IFF.h
	MemoryIOWrapper.h
	ParsingUtils.h
	StreamReader.h
	StringComparison.h
	SGSpatialSort.cpp
	SGSpatialSort.h
	VertexTriangleAdjacency.cpp
	VertexTriangleAdjacency.h
	GenericProperty.h
	SpatialSort.cpp
	SpatialSort.h
	SceneCombiner.cpp
	SceneCombiner.h
	ScenePreprocessor.cpp
	ScenePreprocessor.h
	SkeletonMeshBuilder.cpp
	SkeletonMeshBuilder.h
	SplitByBoneCountProcess.cpp
	SplitByBoneCountProcess.h
	SmoothingGroups.h
	StandardShapes.cpp
	StandardShapes.h
	TargetAnimation.cpp
	TargetAnimation.h
	RemoveComments.cpp
	RemoveComments.h
	Subdivision.cpp
	Subdivision.h
	Vertex.h
	LineSplitter.h
	TinyFormatter.h
	Profiler.h
	LogAux.h
	Bitmap.cpp
	Bitmap.h
)
SOURCE_GROUP(Common FILES ${Common_SRCS})

SET( 3DS_SRCS
	3DSConverter.cpp
	3DSHelper.h
	3DSLoader.cpp
	3DSLoader.h
)
SOURCE_GROUP(3DS FILES ${3DS_SRCS})

SET( AC_SRCS
	ACLoader.cpp
	ACLoader.h
)
SOURCE_GROUP( AC FILES ${AC_SRCS})

SET( ASE_SRCS
	ASELoader.cpp
	ASELoader.h
	ASEParser.cpp
	ASEParser.h
)
SOURCE_GROUP( ASE FILES ${ASE_SRCS})

SET( B3D_SRCS
	B3DImporter.cpp
	B3DImporter.h
)
SOURCE_GROUP( B3D FILES ${B3D_SRCS})

SET( BVH_SRCS
	BVHLoader.cpp
	BVHLoader.h
)
SOURCE_GROUP( BVH FILES ${BVH_SRCS})

SET( Collada_SRCS
	ColladaHelper.h
	ColladaLoader.cpp
	ColladaLoader.h
	ColladaParser.cpp
	ColladaParser.h
	ColladaExporter.h
	ColladaExporter.cpp
)
SOURCE_GROUP( Collada FILES ${Collada_SRCS})

SET( DXF_SRCS
	DXFLoader.cpp
	DXFLoader.h
	DXFHelper.h
)
SOURCE_GROUP( DXF FILES ${DXF_SRCS})

SET( CSM_SRCS
	CSMLoader.cpp
	CSMLoader.h
)
SOURCE_GROUP( CSM FILES ${CSM_SRCS})

SET( HMP_SRCS
	HMPFileData.h
	HMPLoader.cpp
	HMPLoader.h
	HalfLifeFileData.h
)
SOURCE_GROUP( HMP FILES ${HMP_SRCS})

SET( Irr_SRCS
	IRRLoader.cpp
	IRRLoader.h
	IRRMeshLoader.cpp
	IRRMeshLoader.h
	IRRShared.cpp
	IRRShared.h
)
SOURCE_GROUP( Irr FILES ${Irr_SRCS})

SET( LWO_SRCS
	LWOAnimation.cpp
	LWOAnimation.h
	LWOBLoader.cpp
	LWOFileData.h
	LWOLoader.cpp
	LWOLoader.h
	LWOMaterial.cpp
)
SOURCE_GROUP( LWO FILES ${LWO_SRCS})

SET( LWS_SRCS
	LWSLoader.cpp
	LWSLoader.h
)
SOURCE_GROUP( LWS FILES ${LWS_SRCS})



SET( MD2_SRCS
	MD2FileData.h
	MD2Loader.cpp
	MD2Loader.h
	MD2NormalTable.h
)
SOURCE_GROUP( MD2 FILES ${MD2_SRCS})

SET( MD3_SRCS
	MD3FileData.h
	MD3Loader.cpp
	MD3Loader.h
)
SOURCE_GROUP( MD3 FILES ${MD3_SRCS})

SET( MD5_SRCS
	MD5Loader.cpp
	MD5Loader.h
	MD5Parser.cpp
	MD5Parser.h
)
SOURCE_GROUP( MD5 FILES ${MD5_SRCS})

SET( MDC_SRCS
	MDCFileData.h
	MDCLoader.cpp
	MDCLoader.h
	MDCNormalTable.h
)
SOURCE_GROUP( MDC FILES ${MDC_SRCS})

SET( MDL_SRCS
	MDLDefaultColorMap.h
	MDLFileData.h
	MDLLoader.cpp
	MDLLoader.h
	MDLMaterialLoader.cpp
)
SOURCE_GROUP( MDL FILES ${MDL_SRCS})

SET( MaterialSystem_SRCS
	MaterialSystem.cpp
	MaterialSystem.h
)
SOURCE_GROUP( MaterialSystem FILES ${MaterialSystem_SRCS})

SET( NFF_SRCS
	NFFLoader.cpp
	NFFLoader.h
)
SOURCE_GROUP( NFF FILES ${NFF_SRCS})

SET( NDO_SRCS
	NDOLoader.cpp
	NDOLoader.h
)
SOURCE_GROUP( NDO FILES ${NDO_SRCS})

SET( OFFFormat_SRCS
	OFFLoader.cpp
	OFFLoader.h
)
SOURCE_GROUP( OFFFormat FILES ${OFFFormat_SRCS})

SET( Obj_SRCS
	ObjFileData.h
	ObjFileImporter.cpp
	ObjFileImporter.h
	ObjFileMtlImporter.cpp
	ObjFileMtlImporter.h
	ObjFileParser.cpp
	ObjFileParser.h
	ObjTools.h
	
	ObjExporter.h
	ObjExporter.cpp
)
SOURCE_GROUP( Obj FILES ${Obj_SRCS})

SET( Ogre_SRCS
	OgreImporter.h
	OgreStructs.h
	OgreParsingUtils.h
	OgreBinarySerializer.h
	OgreXmlSerializer.h
	OgreImporter.cpp
	OgreStructs.cpp
	OgreBinarySerializer.cpp
	OgreXmlSerializer.cpp
	OgreMaterial.cpp
)
SOURCE_GROUP( Ogre FILES ${Ogre_SRCS})

SET( Ply_SRCS
	PlyLoader.cpp
	PlyLoader.h
	PlyParser.cpp
	PlyParser.h
	PlyExporter.cpp
	PlyExporter.h
)
SOURCE_GROUP( Ply FILES ${Ply_SRCS})

SET(MS3D_SRCS
	MS3DLoader.cpp
	MS3DLoader.h
)
SOURCE_GROUP( MS3D FILES ${MS3D_SRCS})

SET(COB_SRCS
	COBLoader.cpp
	COBLoader.h
	COBScene.h
)
SOURCE_GROUP( COB FILES ${COB_SRCS})

SET(BLENDER_SRCS
	BlenderLoader.cpp
	BlenderLoader.h
	BlenderDNA.cpp
	BlenderDNA.h
	BlenderDNA.inl
	BlenderScene.cpp
	BlenderScene.h
	BlenderSceneGen.h
	BlenderIntermediate.h
	BlenderModifier.h
	BlenderModifier.cpp
	BlenderBMesh.h
	BlenderBMesh.cpp
	BlenderTessellator.h
	BlenderTessellator.cpp
)
SOURCE_GROUP( BLENDER FILES ${BLENDER_SRCS})

SET(IFC_SRCS
	IFCLoader.cpp
	IFCLoader.h
	IFCReaderGen.cpp
	IFCReaderGen.h
	IFCUtil.h
	IFCUtil.cpp
	IFCGeometry.cpp
	IFCMaterial.cpp
	IFCProfile.cpp
	IFCCurve.cpp
	IFCBoolean.cpp
	IFCOpenings.cpp
	STEPFile.h
	STEPFileReader.h
	STEPFileReader.cpp
	STEPFileEncoding.cpp
	STEPFileEncoding.h
)
SOURCE_GROUP( IFC FILES ${IFC_SRCS})

SET( XGL_SRCS
	XGLLoader.cpp
	XGLLoader.h
)
SOURCE_GROUP( XGL FILES ${XGL_SRCS})


SET(FBX_SRCS
	FBXImporter.cpp
	FBXCompileConfig.h
	FBXImporter.h
	FBXParser.cpp
	FBXParser.h
	FBXTokenizer.cpp
	FBXTokenizer.h
	FBXImportSettings.h
	FBXConverter.h
	FBXConverter.cpp
	FBXUtil.h
	FBXUtil.cpp
	FBXDocument.h
	FBXDocument.cpp
	FBXProperties.h
	FBXProperties.cpp
	FBXMeshGeometry.cpp
	FBXMaterial.cpp
	FBXModel.cpp
	FBXAnimation.cpp
	FBXNodeAttribute.cpp
	FBXDeformer.cpp
	FBXBinaryTokenizer.cpp
	FBXDocumentUtil.cpp
)
SOURCE_GROUP( FBX FILES ${FBX_SRCS})


SET( PostProcessing_SRCS
	CalcTangentsProcess.cpp
	CalcTangentsProcess.h
	ComputeUVMappingProcess.cpp
	ComputeUVMappingProcess.h
	ConvertToLHProcess.cpp
	ConvertToLHProcess.h
	FindDegenerates.cpp
	FindDegenerates.h
	FindInstancesProcess.cpp
	FindInstancesProcess.h
	FindInvalidDataProcess.cpp
	FindInvalidDataProcess.h
	FixNormalsStep.cpp
	FixNormalsStep.h
	GenFaceNormalsProcess.cpp
	GenFaceNormalsProcess.h
	GenVertexNormalsProcess.cpp
	GenVertexNormalsProcess.h
	PretransformVertices.cpp
	PretransformVertices.h
	ImproveCacheLocality.cpp
	ImproveCacheLocality.h
	JoinVerticesProcess.cpp
	JoinVerticesProcess.h
	LimitBoneWeightsProcess.cpp
	LimitBoneWeightsProcess.h
	RemoveRedundantMaterials.cpp
	RemoveRedundantMaterials.h
	RemoveVCProcess.cpp
	RemoveVCProcess.h
	SortByPTypeProcess.cpp
	SortByPTypeProcess.h
	SplitLargeMeshes.cpp
	SplitLargeMeshes.h
	TextureTransform.cpp
	TextureTransform.h
	TriangulateProcess.cpp
	TriangulateProcess.h
	ValidateDataStructure.cpp
	ValidateDataStructure.h
	OptimizeGraph.cpp
	OptimizeGraph.h
	OptimizeMeshes.cpp
	OptimizeMeshes.h
	DeboneProcess.cpp
	DeboneProcess.h
	ProcessHelper.h
	ProcessHelper.cpp
	PolyTools.h
	MakeVerboseFormat.cpp
	MakeVerboseFormat.h
)
SOURCE_GROUP( PostProcessing FILES ${PostProcessing_SRCS})

SET( Q3D_SRCS
	Q3DLoader.cpp
	Q3DLoader.h
)
SOURCE_GROUP( Q3D FILES ${Q3D_SRCS})

SET( Q3BSP_SRCS
	Q3BSPFileData.h
	Q3BSPFileParser.h
	Q3BSPFileParser.cpp
	Q3BSPFileImporter.h
	Q3BSPFileImporter.cpp
	Q3BSPZipArchive.h
	Q3BSPZipArchive.cpp
)
SOURCE_GROUP( Q3BSP FILES ${Q3BSP_SRCS})

SET( Raw_SRCS
	RawLoader.cpp
	RawLoader.h
)
SOURCE_GROUP( Raw FILES ${Raw_SRCS})

SET( SMD_SRCS
	SMDLoader.cpp
	SMDLoader.h
)
SOURCE_GROUP( SMD FILES ${SMD_SRCS})

SET( STL_SRCS
	STLLoader.cpp
	STLLoader.h
	STLExporter.h
	STLExporter.cpp
)
SOURCE_GROUP( STL FILES ${STL_SRCS})

SET( Terragen_SRCS
	TerragenLoader.cpp
	TerragenLoader.h
)
SOURCE_GROUP( Terragen FILES ${Terragen_SRCS})

SET( Unreal_SRCS
	UnrealLoader.cpp
	UnrealLoader.h
)
SOURCE_GROUP( Unreal FILES ${Unreal_SRCS})

SET( XFile_SRCS
	XFileHelper.h
	XFileImporter.cpp
	XFileImporter.h
	XFileParser.cpp
	XFileParser.h
)
SOURCE_GROUP( XFile FILES ${XFile_SRCS})

SET( Exporter_SRCS
	Exporter.cpp
	AssimpCExport.cpp
	BlobIOSystem.h
)
SOURCE_GROUP( Exporter FILES ${Exporter_SRCS})

SET( Extra_SRCS
	MD4FileData.h
)
SOURCE_GROUP( Extra FILES ${Extra_SRCS})

SET( IrrXML_SRCS
	irrXMLWrapper.h
	../contrib/irrXML/CXMLReaderImpl.h
	../contrib/irrXML/heapsort.h
	../contrib/irrXML/irrArray.h
	../contrib/irrXML/irrString.h
	../contrib/irrXML/irrTypes.h
	../contrib/irrXML/irrXML.cpp
	../contrib/irrXML/irrXML.h
)
SOURCE_GROUP( IrrXML FILES ${IrrXML_SRCS})

SET( ConvertUTF_SRCS
	../contrib/ConvertUTF/ConvertUTF.h
	../contrib/ConvertUTF/ConvertUTF.c
)
SOURCE_GROUP( ConvertUTF FILES ${ConvertUTF_SRCS})

SET( Clipper_SRCS 
	../contrib/clipper/clipper.hpp
	../contrib/clipper/clipper.cpp
)
SOURCE_GROUP( Clipper FILES ${Clipper_SRCS})


SET( Poly2Tri_SRCS 
	../contrib/poly2tri/poly2tri/common/shapes.cc
	../contrib/poly2tri/poly2tri/common/shapes.h
	../contrib/poly2tri/poly2tri/common/utils.h
	../contrib/poly2tri/poly2tri/sweep/advancing_front.h
	../contrib/poly2tri/poly2tri/sweep/advancing_front.cc
	../contrib/poly2tri/poly2tri/sweep/cdt.cc
	../contrib/poly2tri/poly2tri/sweep/cdt.h
	../contrib/poly2tri/poly2tri/sweep/sweep.cc
	../contrib/poly2tri/poly2tri/sweep/sweep.h
	../contrib/poly2tri/poly2tri/sweep/sweep_context.cc
	../contrib/poly2tri/poly2tri/sweep/sweep_context.h
)
SOURCE_GROUP( Poly2Tri FILES ${Poly2Tri_SRCS})

SET( unzip_SRCS
	../contrib/unzip/crypt.h
	../contrib/unzip/ioapi.c
	../contrib/unzip/ioapi.h
	../contrib/unzip/unzip.c
	../contrib/unzip/unzip.h
)
SOURCE_GROUP( unzip FILES ${unzip_SRCS})


# VC2010 fixes
if(MSVC10)
	OPTION( VC10_STDINT_FIX "Fix for VC10 Compiler regarding pstdint.h redefinition errors" OFF )
	if( VC10_STDINT_FIX )
		ADD_DEFINITIONS( -D_STDINT )
	endif( VC10_STDINT_FIX )
endif(MSVC10)

ADD_DEFINITIONS( -DASSIMP_BUILD_DLL_EXPORT )

if ( MSVC )
	ADD_DEFINITIONS( -D_SCL_SECURE_NO_WARNINGS )
	ADD_DEFINITIONS( -D_CRT_SECURE_NO_WARNINGS )
endif ( MSVC )

if (UNZIP_FOUND)
	SET (unzip_compile_SRCS "")
else (UNZIP_FOUND)
	SET (unzip_compile_SRCS ${unzip_SRCS})
endif (UNZIP_FOUND)

SET( assimp_src
	# Assimp Files
	${Core_SRCS}
	${Common_SRCS}
	${Logging_SRCS}
	${Exporter_SRCS}
	${PostProcessing_SRCS}

	# Model Support
	${3DS_SRCS}
	${AC_SRCS}
	${ASE_SRCS}
	${B3D_SRCS}
	${BVH_SRCS}
	${Collada_SRCS}
	${DXF_SRCS}
	${CSM_SRCS}
	${HMP_SRCS}
	${Irr_SRCS}
	${LWO_SRCS}
	${LWS_SRCS}
	${MD2_SRCS}
	${MD3_SRCS}
	${MD5_SRCS}
	${MDC_SRCS}
	${MDL_SRCS}
	${MaterialSystem_SRCS}
	${NFF_SRCS}
	${OFFFormat_SRCS}
	${Obj_SRCS}
	${Ogre_SRCS}
	${Ply_SRCS}
	${Q3D_SRCS}
	${Q3BSP_SRCS}
	${Raw_SRCS}
	${SMD_SRCS}
	${STL_SRCS}
	${Terragen_SRCS}
	${Unreal_SRCS}
	${XFile_SRCS}
	${Extra_SRCS}
	${MS3D_SRCS}
	${COB_SRCS}
	${BLENDER_SRCS}
	${NDO_SRCS}
	${IFC_SRCS}
	${XGL_SRCS}
	${FBX_SRCS}
	
	# Third-party libraries
	${IrrXML_SRCS}
	${ConvertUTF_SRCS}
	${unzip_compile_SRCS}
	${Poly2Tri_SRCS}
	${Clipper_SRCS}
	# Necessary to show the headers in the project when using the VC++ generator:
	${Boost_SRCS}

	${PUBLIC_HEADERS}
	${COMPILER_HEADERS}
	
	# Old precompiled header
	# (removed because the precompiled header is not updated when visual studio switch configuration which leads to failed compilation.
	# Moreover it's a drag to recompile assimp entirely each time a modification is made to one of the included header, which is definitely counter-productive.)
	AssimpPCH.cpp
)

#ADD_MSVC_PRECOMPILED_HEADER("AssimpPCH.h" "AssimpPCH.cpp" assimp_src)

ADD_LIBRARY( assimp ${assimp_src} )

SET_PROPERTY(TARGET assimp PROPERTY DEBUG_POSTFIX ${ASSIMP_DEBUG_POSTFIX})

TARGET_LINK_LIBRARIES(assimp ${ZLIB_LIBRARIES})
SET_TARGET_PROPERTIES( assimp PROPERTIES
	VERSION ${ASSIMP_VERSION}
	SOVERSION ${ASSIMP_SOVERSION} # use full version 
    OUTPUT_NAME assimp${ASSIMP_LIBRARY_SUFFIX}
)

if (APPLE)
    SET_TARGET_PROPERTIES( assimp PROPERTIES INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}")
endif()

# Build against external unzip, or add ../contrib/unzip so
# assimp can #include "unzip.h"
if (UNZIP_FOUND)
	INCLUDE_DIRECTORIES(${UNZIP_INCLUDE_DIRS})
	TARGET_LINK_LIBRARIES(assimp ${UNZIP_LIBRARIES})
else (UNZIP_FOUND)
	INCLUDE_DIRECTORIES("../contrib/unzip")
endif (UNZIP_FOUND)

INSTALL( TARGETS assimp
         LIBRARY DESTINATION ${ASSIMP_LIB_INSTALL_DIR}
         ARCHIVE DESTINATION ${ASSIMP_LIB_INSTALL_DIR}
         RUNTIME DESTINATION ${ASSIMP_BIN_INSTALL_DIR}
         COMPONENT ${LIBASSIMP_COMPONENT})
INSTALL( FILES ${PUBLIC_HEADERS} DESTINATION ${ASSIMP_INCLUDE_INSTALL_DIR}/assimp COMPONENT assimp-dev)
INSTALL( FILES ${COMPILER_HEADERS} DESTINATION ${ASSIMP_INCLUDE_INSTALL_DIR}/assimp/Compiler COMPONENT assimp-dev)

if(MSVC AND ASSIMP_INSTALL_PDB)
	install(FILES ${Assimp_BINARY_DIR}/code/Debug/assimp${ASSIMP_DEBUG_POSTFIX}.pdb
		DESTINATION ${ASSIMP_LIB_INSTALL_DIR}
		CONFIGURATIONS Debug
	)
	install(FILES ${Assimp_BINARY_DIR}/code/RelWithDebInfo/assimp.pdb
		DESTINATION ${ASSIMP_LIB_INSTALL_DIR}
		CONFIGURATIONS RelWithDebInfo
	)
endif ()
