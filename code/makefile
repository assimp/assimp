#rough makefile for linux/gnu systems.  requires that boost libs are installed
#produces shared library output in bin/libassimp.so

SOURCES = \
	3DSConverter.cpp \
	3DSGenNormals.cpp \
	3DSLoader.cpp \
	3DSSpatialSort.cpp \
	aiAssert.cpp \
	ASELoader.cpp \
	ASEParser.cpp \
	Assimp.cpp \
	BaseImporter.cpp \
	BaseProcess.cpp \
	CalcTangentsProcess.cpp \
	ConvertToLHProcess.cpp \
	DefaultIOStream.cpp \
	DefaultIOSystem.cpp \
	DefaultLogger.cpp \
	extra/MakeVerboseFormat.cpp \
	FixNormalsStep.cpp \
	GenFaceNormalsProcess.cpp \
	GenVertexNormalsProcess.cpp \
	HMPLoader.cpp \
	Importer.cpp \
	ImproveCacheLocality.cpp \
	JoinVerticesProcess.cpp \
	KillNormalsProcess.cpp \
	LimitBoneWeightsProcess.cpp \
	LWOMaterial.cpp \
	LWOLoader.cpp \
	MaterialSystem.cpp \
	MD2Loader.cpp \
	MD3Loader.cpp \
	MD5Loader.cpp \
	MD5Parser.cpp \
	MDCLoader.cpp \
	MDLLoader.cpp \
	MDLMaterialLoader.cpp \
	ObjFileImporter.cpp \
	ObjFileMtlImporter.cpp \
	ObjFileParser.cpp \
	PlyLoader.cpp \
	PlyParser.cpp \
	PretransformVertices.cpp \
	RemoveComments.cpp \
	RemoveRedundantMaterials.cpp \
	SMDLoader.cpp \
	SpatialSort.cpp \
	SplitLargeMeshes.cpp \
	STLLoader.cpp \
	TextureTransform.cpp \
	TriangulateProcess.cpp \
	ValidateDataStructure.cpp \
	VertexTriangleAdjacency.cpp \
	XFileImporter.cpp \
	XFileParser.cpp

OBJECTS = $(SOURCES:.cpp=.o)

TARGET = ../bin/libassimp.so

all:	$(TARGET)

$(TARGET):	$(OBJECTS)
	gcc -o $@ $(OBJECTS) -shared -lstdc++ -lboost_thread-mt -lboost_date_time-mt

%.o:%.cpp
	$(CXX) -g -Wall -c $? -o $@ -I../include -fPIC

clean:
	rm -f $(OBJECTS) $(TARGET)
