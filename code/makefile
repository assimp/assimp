#rough makefile for linux/gnu systems.  requires that boost libs are installed
#produces shared library output in bin/libassimp.so


SOURCES = AssimpPCH.cpp \
	3DSConverter.cpp \
	3DSLoader.cpp \
	aiAssert.cpp \
	ASELoader.cpp \
	ASEParser.cpp \
	Assimp.cpp BaseImporter.cpp BaseProcess.cpp \
	CalcTangentsProcess.cpp \
	ConvertToLHProcess.cpp \
	DefaultIOStream.cpp \
	DefaultIOSystem.cpp \
	DefaultLogger.cpp \
	DXFLoader.cpp \
	FixNormalsStep.cpp \
	GenFaceNormalsProcess.cpp \
	GenVertexNormalsProcess.cpp \
	HMPLoader.cpp \
	Importer.cpp \
	ImproveCacheLocality.cpp \
	JoinVerticesProcess.cpp \
	RemoveVCProcess.cpp \
	LimitBoneWeightsProcess.cpp \
	LWOBLoader.cpp \
	LWOLoader.cpp \
	LWOMaterial.cpp \
	MaterialSystem.cpp \
	MD2Loader.cpp \
	MD3Loader.cpp \
	MD5Loader.cpp \
	MD5Parser.cpp \
	MDCLoader.cpp \
	MDLLoader.cpp \
	MDLMaterialLoader.cpp \
	NFFLoader.cpp \
	ObjFileImporter.cpp \
	ObjFileMtlImporter.cpp \
	ObjFileParser.cpp \
	PlyLoader.cpp \
	PlyParser.cpp \
	PretransformVertices.cpp \
	RemoveComments.cpp \
	RemoveRedundantMaterials.cpp \
	SGSpatialSort.cpp \
	SMDLoader.cpp \
	SpatialSort.cpp \
	SplitLargeMeshes.cpp \
	StandardShapes.cpp \
	STLLoader.cpp \
	TextureTransform.cpp \
	TriangulateProcess.cpp \
	ValidateDataStructure.cpp \
	VertexTriangleAdjacency.cpp \
	XFileImporter.cpp \
	RawLoader.cpp \
	OFFLoader.cpp \
	SortByPTypeProcess.cpp \
	FindInvalidDataProcess.cpp \
	ACLoader.cpp \
	LWSLoader.cpp \
	BVHLoader.cpp \
	SceneCombiner.cpp \
	SkeletonMeshBuilder.cpp \
	FindDegenerates.cpp \
	XFileParser.cpp \
	./irrXML/irrXML.cpp \
	IRRMeshLoader.cpp \
	IRRLoader.cpp \
	Q3DLoader.cpp \
	ScenePreprocessor.cpp \
	B3DImporter.cpp \
	TargetAnimation.cpp \
	ComputeUVMappingProcess.cpp  \
	ColladaLoader.cpp \
	ColladaParser.cpp \
	TerragenLoader.cpp 

OBJECTS = $(SOURCES:.cpp=.o)

TARGET = ./../bin/libassimp.so

all:	$(TARGET)

$(TARGET):	$(OBJECTS)
	gcc -o $@ $(OBJECTS) -shared -lstdc++ 
%.o:%.cpp
	$(CXX) -g -Wall -c $? -o $@ -I../include -I"C:\Program Files\boost\boost_1_35_0" -fPIC
clean:
	rm -f $(OBJECTS) $(TARGET)


STATIC = ./../bin/libassimp.a
static:    $(STATIC)
$(STATIC):    $(OBJECTS)
	ar rcs $@ $(OBJECTS)

