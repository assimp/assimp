
SET( HEADER_PATH ../include/ )

SOURCE_GROUP( Logging FILES 
	${HEADER_PATH}/DefaultLogger.h 
	${HEADER_PATH}/IOStream.h
	${HEADER_PATH}/LogStream.h
	${HEADER_PATH}/Logger.h
	${HEADER_PATH}/NullLogger.h
	Win32DebugLogStream.h
	DefaultLogger.cpp
	FileLogStream.h
)
SOURCE_GROUP( Common FILES 
	aiAssert.cpp
	fast_atof.h	
	qnan.h
	BaseImporter.cpp
	BaseImporter.h
	BaseProcess.cpp
	BaseProcess.h
	ByteSwap.h
	ProcessHelper.h
	DefaultIOStream.cpp
	DefaultIOStream.h
	DefaultIOSystem.cpp
	DefaultIOSystem.h
	Hash.h
	Importer.cpp
	IFF.h
	ParsingUtils.h
	StdOStreamLogStream.h
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
	SmoothingGroups.h
	StandardShapes.cpp
	StandardShapes.h
	TargetAnimation.cpp
	TargetAnimation.h
	RemoveComments.cpp
	RemoveComments.h
)

SOURCE_GROUP( 3DS FILES 
	3DSConverter.cpp
	3DSHelper.h
	3DSLoader.cpp
	3DSLoader.h
)

SOURCE_GROUP( AC FILES
	ACLoader.cpp
	ACLoader.h
)

SOURCE_GROUP( ASE FILES
	ASELoader.cpp
	ASELoader.h
	ASEParser.cpp
	ASEParser.h
)
SOURCE_GROUP( B3D FILES
	B3DImporter.cpp
	B3DImporter.h
)

SOURCE_GROUP( BVH FILES
	BVHLoader.cpp
	BVHLoader.h
)

SOURCE_GROUP(Collada FILES
	ColladaHelper.h
	ColladaLoader.cpp
	ColladaLoader.h
	ColladaParser.cpp
	ColladaParser.h
)

SOURCE_GROUP(DXF FILES
	DXFLoader.cpp
	DXFLoader.h
)

SOURCE_GROUP(CSM FILES
	CSMLoader.cpp
	CSMLoader.h
)

SOURCE_GROUP(HMP FILES
	HMPFileData.h
	HMPLoader.cpp
	HMPLoader.h
	HalfLifeFileData.h
)

SOURCE_GROUP(Irr FILES
	IRRLoader.cpp
	IRRLoader.h
	IRRMeshLoader.cpp
	IRRMeshLoader.h
	IRRShared.cpp
	IRRShared.h
)

SOURCE_GROUP(LWO FILES
	LWOAnimation.cpp
	LWOAnimation.h
	LWOBLoader.cpp
	LWOFileData.h
	LWOLoader.cpp
	LWOLoader.h
	LWOMaterial.cpp
)

SOURCE_GROUP(LWS FILES
	LWSLoader.cpp
	LWSLoader.h
)

SOURCE_GROUP(MD2 FILES
	MD2FileData.h
	MD2Loader.cpp
	MD2Loader.h
	MD2NormalTable.h
)

SOURCE_GROUP( MD3 FILES
	MD3FileData.h
	MD3Loader.cpp
	MD3Loader.h
)

SOURCE_GROUP( MD5 FILES
	MD5Loader.cpp
	MD5Loader.h
	MD5Parser.cpp
	MD5Parser.h
)

SOURCE_GROUP( MDC FILES
	MDCFileData.h
	MDCLoader.cpp
	MDCLoader.h
	MDCNormalTable.h
)

SOURCE_GROUP( MDL FILES
	MDLDefaultColorMap.h
	MDLFileData.h
	MDLLoader.cpp
	MDLLoader.h
	MDLMaterialLoader.cpp
)

SOURCE_GROUP( MaterialSystem FILES
	MaterialSystem.cpp
	MaterialSystem.h
)

SOURCE_GROUP( NFF FILES
	NFFLoader.cpp
	NFFLoader.h
)

SOURCE_GROUP( OFFFormat FILES
	OFFLoader.cpp
	OFFLoader.h
)

SOURCE_GROUP( Obj FILES
	ObjFileData.h
	ObjFileImporter.cpp
	ObjFileImporter.h
	ObjFileMtlImporter.cpp
	ObjFileMtlImporter.h
	ObjFileParser.cpp
	ObjFileParser.h
	ObjTools.h
)

SOURCE_GROUP( Ply FILES
	PlyLoader.cpp
	PlyLoader.h
	PlyParser.cpp
	PlyParser.h
)

SOURCE_GROUP( PostProcessing FILES
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
	TerragenLoader.cpp
	TerragenLoader.h
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
)

SOURCE_GROUP( Q3D FILES
	Q3DLoader.cpp
	Q3DLoader.h
)

SOURCE_GROUP( Raw FILES
	RawLoader.cpp
	RawLoader.h
)

SOURCE_GROUP( SMD FILES
	SMDLoader.cpp
	SMDLoader.h
)

SOURCE_GROUP( STL FILES
	STLLoader.cpp
	STLLoader.h
)

SOURCE_GROUP( Unreal FILES
	UnrealLoader.cpp
	UnrealLoader.h
)

SOURCE_GROUP( XFile FILES
	XFileHelper.h
	XFileImporter.cpp
	XFileImporter.h
	XFileParser.cpp
	XFileParser.h
)

SOURCE_GROUP( Extra FILES
	extra/MakeVerboseFormat.cpp
	extra/MakeVerboseFormat.h
	extra/MD4FileData.h
)

SOURCE_GROUP( IrrXML FILES
	irrXMLWrapper.h
	../contrib/irrXML/CXMLReaderImpl.h
	../contrib/irrXML/heapsort.h
	../contrib/irrXML/irrArray.h
	../contrib/irrXML/irrString.h
	../contrib/irrXML/irrTypes.h
	../contrib/irrXML/irrXML.cpp
	../contrib/irrXML/irrXML.h
)

SOURCE_GROUP( zlib FILES
	../contrib/zlib/adler32.c
	../contrib/zlib/compress.c
	../contrib/zlib/crc32.c
	../contrib/zlib/crc32.h
	../contrib/zlib/deflate.c
	../contrib/zlib/deflate.h
	../contrib/zlib/inffast.c
	../contrib/zlib/inffast.h
	../contrib/zlib/inffixed.h
	../contrib/zlib/inflate.c
	../contrib/zlib/inflate.h
	../contrib/zlib/inftrees.c
	../contrib/zlib/inftrees.h
	../contrib/zlib/trees.c
	../contrib/zlib/trees.h
	../contrib/zlib/zconf.h
	../contrib/zlib/zconf.in.h
	../contrib/zlib/zlib.h
	../contrib/zlib/zutil.c
	../contrib/zlib/zutil.h
)

ADD_LIBRARY( assimp SHARED
	${HEADER_PATH}/DefaultLogger.h
	${HEADER_PATH}/IOStream.h
	${HEADER_PATH}/IOSystem.h
	${HEADER_PATH}/LogStream.h
	${HEADER_PATH}/Logger.h
	${HEADER_PATH}/NullLogger.h
	${HEADER_PATH}/TDepGraphNode.h
	${HEADER_PATH}/aiAnim.h
	${HEADER_PATH}/aiAssert.h
	${HEADER_PATH}/aiCamera.h
	${HEADER_PATH}/aiConfig.h
	${HEADER_PATH}/aiDefines.h
	${HEADER_PATH}/aiFileIO.h
	${HEADER_PATH}/aiLight.h
	${HEADER_PATH}/aiMaterial.h
	${HEADER_PATH}/aiMatrix3x3.h
	${HEADER_PATH}/aiMatrix4x4.h
	${HEADER_PATH}/aiMesh.h
	${HEADER_PATH}/aiPostProcess.h
	${HEADER_PATH}/aiQuaternion.h
	${HEADER_PATH}/aiScene.h
	${HEADER_PATH}/aiTexture.h
	${HEADER_PATH}/aiTypes.h
	${HEADER_PATH}/aiVector2D.h
	${HEADER_PATH}/aiVector3D.h
	${HEADER_PATH}/aiVersion.h
	${HEADER_PATH}/assimp.h

	3DSConverter.cpp
	3DSHelper.h
	3DSLoader.cpp
	3DSLoader.h
	ACLoader.cpp
	ACLoader.h
	ASELoader.cpp
	ASELoader.h
	ASEParser.cpp
	ASEParser.h
	Assimp.cpp
	AssimpPCH.cpp
	AssimpPCH.h
	B3DImporter.cpp
	B3DImporter.h
	BVHLoader.cpp
	BVHLoader.h
	BaseImporter.cpp
	BaseImporter.h
	BaseProcess.cpp
	BaseProcess.h
	ByteSwap.h
	CalcTangentsProcess.cpp
	CalcTangentsProcess.h
	ColladaHelper.h
	ColladaLoader.cpp
	ColladaLoader.h
	ColladaParser.cpp
	ColladaParser.h
	ComputeUVMappingProcess.cpp
	ComputeUVMappingProcess.h
	ConvertToLHProcess.cpp
	ConvertToLHProcess.h
	DXFLoader.cpp
	DXFLoader.h
	CSMLoader.cpp
	CSMLoader.h
	DefaultIOStream.cpp
	DefaultIOStream.h
	DefaultIOSystem.cpp
	DefaultIOSystem.h
	DefaultLogger.cpp
	FileLogStream.h
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
	GenericProperty.h
	HMPFileData.h
	HMPLoader.cpp
	HMPLoader.h
	HalfLifeFileData.h
	Hash.h
	IFF.h
	IRRLoader.cpp
	IRRLoader.h
	IRRMeshLoader.cpp
	IRRMeshLoader.h
	IRRShared.cpp
	IRRShared.h
	Importer.cpp
	ImproveCacheLocality.cpp
	ImproveCacheLocality.h
	JoinVerticesProcess.cpp
	JoinVerticesProcess.h
	LWOAnimation.cpp
	LWOAnimation.h
	LWOBLoader.cpp
	LWOFileData.h
	LWOLoader.cpp
	LWOLoader.h
	LWOMaterial.cpp
	LWSLoader.cpp
	LWSLoader.h
	LimitBoneWeightsProcess.cpp
	LimitBoneWeightsProcess.h
	MD2FileData.h
	MD2Loader.cpp
	MD2Loader.h
	MD2NormalTable.h
	MD3FileData.h
	MD3Loader.cpp
	MD3Loader.h
	MD5Loader.cpp
	MD5Loader.h
	MD5Parser.cpp
	MD5Parser.h
	MDCFileData.h
	MDCLoader.cpp
	MDCLoader.h
	MDCNormalTable.h
	MDLDefaultColorMap.h
	MDLFileData.h
	MDLLoader.cpp
	MDLLoader.h
	MDLMaterialLoader.cpp
	MaterialSystem.cpp
	MaterialSystem.h
	NFFLoader.cpp
	NFFLoader.h
	OFFLoader.cpp
	OFFLoader.h
	ObjFileData.h
	ObjFileImporter.cpp
	ObjFileImporter.h
	ObjFileMtlImporter.cpp
	ObjFileMtlImporter.h
	ObjFileParser.cpp
	ObjFileParser.h
	ObjTools.h
	OptimizeGraph.cpp
	OptimizeGraph.h
	OptimizeMeshes.cpp
	OptimizeMeshes.h
	ParsingUtils.h
	PlyLoader.cpp
	PlyLoader.h
	PlyParser.cpp
	PlyParser.h
	PretransformVertices.cpp
	PretransformVertices.h
	ProcessHelper.h
	Q3DLoader.cpp
	Q3DLoader.h
	RawLoader.cpp
	RawLoader.h
	RemoveComments.cpp
	RemoveComments.h
	RemoveRedundantMaterials.cpp
	RemoveRedundantMaterials.h
	RemoveVCProcess.cpp
	RemoveVCProcess.h
	SGSpatialSort.cpp
	SGSpatialSort.h
	SMDLoader.cpp
	SMDLoader.h
	STLLoader.cpp
	STLLoader.h
	SceneCombiner.cpp
	SceneCombiner.h
	ScenePreprocessor.cpp
	ScenePreprocessor.h
	SkeletonMeshBuilder.cpp
	SkeletonMeshBuilder.h
	SmoothingGroups.h
	SortByPTypeProcess.cpp
	SortByPTypeProcess.h
	SpatialSort.cpp
	SpatialSort.h
	SplitLargeMeshes.cpp
	SplitLargeMeshes.h
	StandardShapes.cpp
	StandardShapes.h
	StdOStreamLogStream.h
	StreamReader.h
	StringComparison.h
	TargetAnimation.cpp
	TargetAnimation.h
	TerragenLoader.cpp
	TerragenLoader.h
	TextureTransform.cpp
	TextureTransform.h
	TriangulateProcess.cpp
	TriangulateProcess.h
	UnrealLoader.cpp
	UnrealLoader.h
	ValidateDataStructure.cpp
	ValidateDataStructure.h
	VertexTriangleAdjacency.cpp
	VertexTriangleAdjacency.h
	Win32DebugLogStream.h
	XFileHelper.h
	XFileImporter.cpp
	XFileImporter.h
	XFileParser.cpp
	XFileParser.h
	aiAssert.cpp
	fast_atof.h
	irrXMLWrapper.h
	qnan.h
	extra/MakeVerboseFormat.cpp
	extra/MakeVerboseFormat.h
	extra/MD4FileData.h
	../contrib/irrXML/CXMLReaderImpl.h
	../contrib/irrXML/heapsort.h
	../contrib/irrXML/irrArray.h
	../contrib/irrXML/irrString.h
	../contrib/irrXML/irrTypes.h
	../contrib/irrXML/irrXML.cpp
	../contrib/irrXML/irrXML.h
	../contrib/zlib/adler32.c
	../contrib/zlib/compress.c
	../contrib/zlib/crc32.c
	../contrib/zlib/crc32.h
	../contrib/zlib/deflate.c
	../contrib/zlib/deflate.h
	../contrib/zlib/inffast.c
	../contrib/zlib/inffast.h
	../contrib/zlib/inffixed.h
	../contrib/zlib/inflate.c
	../contrib/zlib/inflate.h
	../contrib/zlib/inftrees.c
	../contrib/zlib/inftrees.h
	../contrib/zlib/trees.c
	../contrib/zlib/trees.h
	../contrib/zlib/zconf.h
	../contrib/zlib/zconf.in.h
	../contrib/zlib/zlib.h
	../contrib/zlib/zutil.c
	../contrib/zlib/zutil.h
)
ADD_DEFINITIONS(-DASSIMP_BUILD_DLL_EXPORT)

if (WIN32)
	if ( MSVC80 )
		ADD_DEFINITIONS( -D_SCL_SECURE_NO_WARNINGS )
		ADD_DEFINITIONS( -D_CRT_SECURE_NO_WARNINGS )
	endif( MSVC80 )
endif (WIN32)
