AssimpIncludeExporters = false
AssimpEnableNoneFreeC4DImporter = false

HeaderPath = "%{prj.location}/include/assimp"
CodePath = "%{prj.location}/code"
AssimpSourceFiles = {}
AssimpSourceFilesIndex = 0

AssimpImporterSourceFiles = {}
AssimpImporterSourceFilesIndex = 0

AssimpExporterSourceFiles = {}
AssimpExporterSourceFilesIndex = 0



function AddSourceFilesCode(sourceFiles)
    for _, file in ipairs(sourceFiles) do 
        AssimpSourceFiles[AssimpSourceFilesIndex] = file
        AssimpSourceFilesIndex = AssimpSourceFilesIndex + 1
    end
end

function AddAssimpImporter(importerFiles)
    for _, file in ipairs(importerFiles) do 
        AssimpImporterSourceFiles[AssimpImporterSourceFilesIndex] = file
        AssimpImporterSourceFilesIndex = AssimpImporterSourceFilesIndex + 1
    end
end

function AddAssimpExporter(exporterFiles)
    for _, file in table.getn(exporterFiles) do 
        AssimpExporterSourceFiles[AssimpExporterSourceFilesIndex] = file
        AssimpExporterSourceFilesIndex = AssimpExporterSourceFilesIndex + 1
    end
end

AssimpPublicHeaders = {
    "%{HeaderPath}/anim.h",
    "%{HeaderPath}/aabb.h",
    "%{HeaderPath}/ai_assert.h",
    "%{HeaderPath}/camera.h",
    "%{HeaderPath}/color4.h",
    "%{HeaderPath}/color4.inl",
    "%{HeaderPath}/config.h",
    "%{HeaderPath}/ColladaMetaData.h",
    "%{HeaderPath}/commonMetaData.h",
    "%{HeaderPath}/defs.h",
    "%{HeaderPath}/cfileio.h",
    "%{HeaderPath}/light.h",
    "%{HeaderPath}/material.h",
    "%{HeaderPath}/material.inl",
    "%{HeaderPath}/matrix3x3.h",
    "%{HeaderPath}/matrix3x3.inl",
    "%{HeaderPath}/matrix4x4.h",
    "%{HeaderPath}/matrix4x4.inl",
    "%{HeaderPath}/mesh.h",
    "%{HeaderPath}/ObjMaterial.h",
    "%{HeaderPath}/pbrmaterial.h",
    "%{HeaderPath}/GltfMaterial.h",
    "%{HeaderPath}/postprocess.h",
    "%{HeaderPath}/quaternion.h",
    "%{HeaderPath}/quaternion.inl",
    "%{HeaderPath}/scene.h",
    "%{HeaderPath}/metadata.h",
    "%{HeaderPath}/texture.h",
    "%{HeaderPath}/types.h",
    "%{HeaderPath}/vector2.h",
    "%{HeaderPath}/vector2.inl",
    "%{HeaderPath}/vector3.h",
    "%{HeaderPath}/vector3.inl",
    "%{HeaderPath}/version.h",
    "%{HeaderPath}/cimport.h",
    "%{HeaderPath}/AssertHandler.h",
    "%{HeaderPath}/importerdesc.h",
    "%{HeaderPath}/Importer.hpp",
    "%{HeaderPath}/DefaultLogger.hpp",
    "%{HeaderPath}/ProgressHandler.hpp",
    "%{HeaderPath}/IOStream.hpp",
    "%{HeaderPath}/IOSystem.hpp",
    "%{HeaderPath}/Logger.hpp",
    "%{HeaderPath}/LogStream.hpp",
    "%{HeaderPath}/NullLogger.hpp",
    "%{HeaderPath}/cexport.h",
    "%{HeaderPath}/Exporter.hpp",
    "%{HeaderPath}/DefaultIOStream.h",
    "%{HeaderPath}/DefaultIOSystem.h",
    "%{HeaderPath}/ZipArchiveIOSystem.h",
    "%{HeaderPath}/SceneCombiner.h",
    "%{HeaderPath}/fast_atof.h",
    "%{HeaderPath}/qnan.h",
    "%{HeaderPath}/BaseImporter.h",
    "%{HeaderPath}/Hash.h",
    "%{HeaderPath}/MemoryIOWrapper.h",
    "%{HeaderPath}/ParsingUtils.h",
    "%{HeaderPath}/StreamReader.h",
    "%{HeaderPath}/StreamWriter.h",
    "%{HeaderPath}/StringComparison.h",
    "%{HeaderPath}/StringUtils.h",
    "%{HeaderPath}/SGSpatialSort.h",
    "%{HeaderPath}/GenericProperty.h",
    "%{HeaderPath}/SpatialSort.h",
    "%{HeaderPath}/SkeletonMeshBuilder.h",
    "%{HeaderPath}/SmallVector.h",
    "%{HeaderPath}/SmoothingGroups.h",
    "%{HeaderPath}/SmoothingGroups.inl",
    "%{HeaderPath}/StandardShapes.h",
    "%{HeaderPath}/RemoveComments.h",
    "%{HeaderPath}/Subdivision.h",
    "%{HeaderPath}/Vertex.h",
    "%{HeaderPath}/LineSplitter.h",
    "%{HeaderPath}/TinyFormatter.h",
    "%{HeaderPath}/Profiler.h",
    "%{HeaderPath}/LogAux.h",
    "%{HeaderPath}/Bitmap.h",
    "%{HeaderPath}/XMLTools.h",
    "%{HeaderPath}/IOStreamBuffer.h",
    "%{HeaderPath}/CreateAnimMesh.h",
    "%{HeaderPath}/XmlParser.h",
    "%{HeaderPath}/BlobIOSystem.h",
    "%{HeaderPath}/MathFunctions.h",
    "%{HeaderPath}/Exceptional.h",
    "%{HeaderPath}/ByteSwapper.h",
    "%{HeaderPath}/Base64.hpp",

    "%{HeaderPath}/Compiler/pushpack1.h",
    "%{HeaderPath}/Compiler/poppack1.h",
    "%{HeaderPath}/Compiler/pstdint.h"
}
AddSourceFilesCode(AssimpPublicHeaders)

AssimpLoggingSourceFiles = {
    "%{HeaderPath}/DefaultLogger.hpp",
    "%{HeaderPath}/LogStream.hpp",
    "%{HeaderPath}/Logger.hpp",
    "%{HeaderPath}/NullLogger.hpp",
    "%{CodePath}/Common/Win32DebugLogStream.h",
    "%{CodePath}/Common/DefaultLogger.cpp",
    "%{CodePath}/Common/FileLogStream.h",
    "%{CodePath}/Common/StdOStreamLogStream.h"
}
AddSourceFilesCode(AssimpLoggingSourceFiles)

AssimpCommonSourceFiles = {
    "%{CodePath}/Common/Assimp.cpp"
    "%{CodePath}/Common/StbCommon.h",
    "%{CodePath}/Common/Compression.cpp",
    "%{CodePath}/Common/Compression.h",
    "%{CodePath}/Common/BaseImporter.cpp",
    "%{CodePath}/Common/BaseProcess.cpp",
    "%{CodePath}/Common/BaseProcess.h",
    "%{CodePath}/Common/Importer.h",
    "%{CodePath}/Common/ScenePrivate.h",
    "%{CodePath}/Common/PostStepRegistry.cpp",
    "%{CodePath}/Common/ImporterRegistry.cpp",
    "%{CodePath}/Common/DefaultProgressHandler.h",
    "%{CodePath}/Common/DefaultIOStream.cpp",
    "%{CodePath}/Common/IOSystem.cpp",
    "%{CodePath}/Common/DefaultIOSystem.cpp",
    "%{CodePath}/Common/ZipArchiveIOSystem.cpp",
    "%{CodePath}/Common/PolyTools.h",
    "%{CodePath}/Common/Maybe.h",
    "%{CodePath}/Common/Importer.cpp",
    "%{CodePath}/Common/IFF.h",
    "%{CodePath}/Common/SGSpatialSort.cpp",
    "%{CodePath}/Common/VertexTriangleAdjacency.cpp",
    "%{CodePath}/Common/VertexTriangleAdjacency.h",
    "%{CodePath}/Common/SpatialSort.cpp",
    "%{CodePath}/Common/SceneCombiner.cpp",
    "%{CodePath}/Common/ScenePreprocessor.cpp",
    "%{CodePath}/Common/ScenePreprocessor.h",
    "%{CodePath}/Common/SkeletonMeshBuilder.cpp",
    "%{CodePath}/Common/StackAllocator.h",
    "%{CodePath}/Common/StackAllocator.inl",
    "%{CodePath}/Common/StandardShapes.cpp",
    "%{CodePath}/Common/TargetAnimation.cpp",
    "%{CodePath}/Common/TargetAnimation.h",
    "%{CodePath}/Common/RemoveComments.cpp",
    "%{CodePath}/Common/Subdivision.cpp",
    "%{CodePath}/Common/scene.cpp",
    "%{CodePath}/Common/Bitmap.cpp",
    "%{CodePath}/Common/Version.cpp",
    "%{CodePath}/Common/CreateAnimMesh.cpp",
    "%{CodePath}/Common/simd.h",
    "%{CodePath}/Common/simd.cpp",
    "%{CodePath}/Common/material.cpp",
    "%{CodePath}/Common/AssertHandler.cpp",
    "%{CodePath}/Common/Exceptional.cpp",
    "%{CodePath}/Common/Base64.cpp"
}
AddSourceFilesCode(AssimpCommonSourceFiles)

AssimpCAPISourceFiles = {
    "%{CodePath}/CApi/CInterfaceIOWrapper.cpp",
    "%{CodePath}/CApi/CInterfaceIOWrapper.h"
}
AddSourceFilesCode(AssimpCAPISourceFiles)

AssimpGeometrySourceFiles = {
    "%{CodePath}/Geometry/GeometryUtils.h",
    "%{CodePath}/Geometry/GeometryUtils.cpp"
}
AddSourceFilesCode(AssimpGeometrySourceFiles)

AssimpSTEPParserSourceFiles = {
    "%{CodePath}/AssetLib/STEPParser/STEPFileReader.h",
    "%{CodePath}/AssetLib/STEPParser/STEPFileReader.cpp",
    "%{CodePath}/AssetLib/STEPParser/STEPFileEncoding.cpp",
    "%{CodePath}/AssetLib/STEPParser/STEPFileEncoding.h"
}
AddSourceFilesCode(AssimpSTEPParserSourceFiles)

-- C4D Importer not supported
if(AssimpEnableNoneFreeC4DImporter) then
    C4DImporter = {
        "AssetLib/C4D/C4DImporter.cpp",
        "AssetLib/C4D/C4DImporter.h"
    }
    AddSourceFilesCode(C4DImporter)
end


AMFImporter = {
  "%{CodePath}/AssetLib/AMF/AMFImporter.hpp",
  "%{CodePath}/AssetLib/AMF/AMFImporter_Node.hpp",
  "%{CodePath}/AssetLib/AMF/AMFImporter.cpp",
  "%{CodePath}/AssetLib/AMF/AMFImporter_Geometry.cpp",
  "%{CodePath}/AssetLib/AMF/AMFImporter_Material.cpp",
  "%{CodePath}/AssetLib/AMF/AMFImporter_Postprocess.cpp"
}
AddAssimpImporter(AMFImporter)

_3DSImporter = {
    "%{CodePath}/AssetLib/3DS/3DSConverter.cpp",
    "%{CodePath}/AssetLib/3DS/3DSHelper.h",
    "%{CodePath}/AssetLib/3DS/3DSLoader.cpp",
    "%{CodePath}/AssetLib/3DS/3DSLoader.h"
}
AddAssimpImporter(_3DSImporter)

ACImporter = {
    "%{CodePath}/AssetLib/AC/ACLoader.cpp",
    "%{CodePath}/AssetLib/AC/ACLoader.h"
}
AddAssimpImporter(ACImporter)

ASEImporter = {
    "%{CodePath}/AssetLib/ASE/ASELoader.cpp",
    "%{CodePath}/AssetLib/ASE/ASELoader.h",
    "%{CodePath}/AssetLib/ASE/ASEParser.cpp",
    "%{CodePath}/AssetLib/ASE/ASEParser.h"
}
AddAssimpImporter(ASEImporter)

ASSBINImporter = {
    "%{CodePath}/AssetLib/Assbin/AssbinLoader.h",
    "%{CodePath}/AssetLib/Assbin/AssbinLoader.cpp"
}
AddAssimpImporter(ASSBINImporter)

B3DImporter = {
    "%{CodePath}/AssetLib/B3D/B3DImporter.cpp",
    "%{CodePath}/AssetLib/B3D/B3DImporter.h"
}
AddAssimpImporter(B3DImporter)

BVHImporter = {
    "%{CodePath}/AssetLib/BVH/BVHLoader.cpp",
    "%{CodePath}/AssetLib/BVH/BVHLoader.h"
}
AddAssimpImporter(BVHImporter)

ColladaImporter = {
    "%{CodePath}/AssetLib/Collada/ColladaHelper.cpp",
    "%{CodePath}/AssetLib/Collada/ColladaHelper.h",
    "%{CodePath}/AssetLib/Collada/ColladaLoader.cpp",
    "%{CodePath}/AssetLib/Collada/ColladaLoader.h",
    "%{CodePath}/AssetLib/Collada/ColladaParser.cpp",
    "%{CodePath}/AssetLib/Collada/ColladaParser.h"
}
AddAssimpImporter(ColladaImporter)

DXFImporter = {
    "%{CodePath}/AssetLib/DXF/DXFLoader.cpp",
    "%{CodePath}/AssetLib/DXF/DXFLoader.h",
    "%{CodePath}/AssetLib/DXF/DXFHelper.h"
}
AddAssimpImporter(DXFImporter)

CSMImporter = {
    "%{CodePath}/AssetLib/CSM/CSMLoader.cpp",
    "%{CodePath}/AssetLib/CSM/CSMLoader.h"
}
AddAssimpImporter(CSMImporter)

HMPImporter = {
    "%{CodePath}/AssetLib/HMP/HMPFileData.h",
    "%{CodePath}/AssetLib/HMP/HMPLoader.cpp",
    "%{CodePath}/AssetLib/HMP/HMPLoader.h",
    "%{CodePath}/AssetLib/HMP/HalfLifeFileData.h"
}
AddAssimpImporter(HMPImporter)

IRRMESHImporter = {
    "%{CodePath}/AssetLib/Irr/IRRMeshLoader.cpp",
    "%{CodePath}/AssetLib/Irr/IRRMeshLoader.h",
    "%{CodePath}/AssetLib/Irr/IRRShared.cpp",
    "%{CodePath}/AssetLib/Irr/IRRShared.h"
}
AddAssimpImporter(IRRMESHImporter)

IQMImporter = {
    "%{CodePath}/AssetLib/IQM/IQMImporter.cpp",
    "%{CodePath}/AssetLib/IQM/iqm.h",
    "%{CodePath}/AssetLib/IQM/IQMImporter.h"
}
AddAssimpImporter(IQMImporter)

IRRImporter = {
    "%{CodePath}/AssetLib/Irr/IRRLoader.cpp",
    "%{CodePath}/AssetLib/Irr/IRRLoader.h",
    "%{CodePath}/AssetLib/Irr/IRRShared.cpp",
    "%{CodePath}/AssetLib/Irr/IRRShared.h"
}
AddAssimpImporter(IRRImporter)

LWOImporter = {
    "%{CodePath}/AssetLib/LWO/LWOAnimation.cpp",
    "%{CodePath}/AssetLib/LWO/LWOAnimation.h",
    "%{CodePath}/AssetLib/LWO/LWOBLoader.cpp",
    "%{CodePath}/AssetLib/LWO/LWOFileData.h",
    "%{CodePath}/AssetLib/LWO/LWOLoader.cpp",
    "%{CodePath}/AssetLib/LWO/LWOLoader.h",
    "%{CodePath}/AssetLib/LWO/LWOMaterial.cpp"
}
AddAssimpImporter(LWOImporter)

LWSImporter = {
    "%{CodePath}/AssetLib/LWS/LWSLoader.cpp",
    "%{CodePath}/AssetLib/LWS/LWSLoader.h"
}
AddAssimpImporter(LWSImporter)

M3DImporter = {
    "%{CodePath}/AssetLib/M3D/M3DMaterials.h",
    "%{CodePath}/AssetLib/M3D/M3DImporter.h",
    "%{CodePath}/AssetLib/M3D/M3DImporter.cpp",
    "%{CodePath}/AssetLib/M3D/M3DWrapper.h",
    "%{CodePath}/AssetLib/M3D/M3DWrapper.cpp",
    "%{CodePath}/AssetLib/M3D/m3d.h"
}
AddAssimpImporter(M3DImporter)

MD2Importer = {
    "%{CodePath}/AssetLib/MD2/MD2FileData.h",
    "%{CodePath}/AssetLib/MD2/MD2Loader.cpp",
    "%{CodePath}/AssetLib/MD2/MD2Loader.h",
    "%{CodePath}/AssetLib/MD2/MD2NormalTable.h"
}
AddAssimpImporter(MD2Importer)

M3DImporter = {
    "%{CodePath}/AssetLib/MD3/MD3FileData.h",
    "%{CodePath}/AssetLib/MD3/MD3Loader.cpp",
    "%{CodePath}/AssetLib/MD3/MD3Loader.h"
}
AddAssimpImporter(M3DImporter)

MD5Importer = {
    "%{CodePath}/AssetLib/MD5/MD5Loader.cpp",
    "%{CodePath}/AssetLib/MD5/MD5Loader.h",
    "%{CodePath}/AssetLib/MD5/MD5Parser.cpp",
    "%{CodePath}/AssetLib/MD5/MD5Parser.h"
}
AddAssimpImporter(MD5Importer)

MDCImporter = {
    "%{CodePath}/AssetLib/MDC/MDCFileData.h",
    "%{CodePath}/AssetLib/MDC/MDCLoader.cpp",
    "%{CodePath}/AssetLib/MDC/MDCLoader.h",
    "%{CodePath}/AssetLib/MDC/MDCNormalTable.h"
}
AddAssimpImporter(MDCImporter)

MDLImporter = {
    "%{CodePath}/AssetLib/MDL/MDLDefaultColorMap.h",
    "%{CodePath}/AssetLib/MDL/MDLFileData.h",
    "%{CodePath}/AssetLib/MDL/MDLLoader.cpp",
    "%{CodePath}/AssetLib/MDL/MDLLoader.h",
    "%{CodePath}/AssetLib/MDL/MDLMaterialLoader.cpp",
    "%{CodePath}/AssetLib/MDL/HalfLife/HalfLifeMDLBaseHeader.h",
    "%{CodePath}/AssetLib/MDL/HalfLife/HL1FileData.h",
    "%{CodePath}/AssetLib/MDL/HalfLife/HL1MDLLoader.cpp",
    "%{CodePath}/AssetLib/MDL/HalfLife/HL1MDLLoader.h",
    "%{CodePath}/AssetLib/MDL/HalfLife/HL1ImportDefinitions.h",
    "%{CodePath}/AssetLib/MDL/HalfLife/HL1ImportSettings.h",
    "%{CodePath}/AssetLib/MDL/HalfLife/HL1MeshTrivert.h",
    "%{CodePath}/AssetLib/MDL/HalfLife/LogFunctions.h",
    "%{CodePath}/AssetLib/MDL/HalfLife/UniqueNameGenerator.cpp",
    "%{CodePath}/AssetLib/MDL/HalfLife/UniqueNameGenerator.h"
}
AddAssimpImporter(MDLImporter)

MaterialSystemSourceFiles = {
    "%{CodePath}/Material/MaterialSystem.cpp",
    "%{CodePath}/Material/MaterialSystem.h"
}
AddSourceFilesCode(MaterialSystemSourceFiles)


NFFImporter = {
    "%{CodePath}/AssetLib/NFF/NFFLoader.cpp",
    "%{CodePath}/AssetLib/NFF/NFFLoader.h"
}
AddAssimpImporter(NFFImporter)

NDOImporter = {
    "%{CodePath}/AssetLib/NDO/NDOLoader.cpp",
    "%{CodePath}/AssetLib/NDO/NDOLoader.h"
}
AddAssimpImporter(NDOImporter)

OFFImporter = {
    "%{CodePath}/AssetLib/OFF/OFFLoader.cpp",
    "%{CodePath}/AssetLib/OFF/OFFLoader.h"
}
AddAssimpImporter(OFFImporter)

OBJImporter = {
    "%{CodePath}/AssetLib/Obj/ObjFileData.h",
    "%{CodePath}/AssetLib/Obj/ObjFileImporter.cpp",
    "%{CodePath}/AssetLib/Obj/ObjFileImporter.h",
    "%{CodePath}/AssetLib/Obj/ObjFileMtlImporter.cpp",
    "%{CodePath}/AssetLib/Obj/ObjFileMtlImporter.h",
    "%{CodePath}/AssetLib/Obj/ObjFileParser.cpp",
    "%{CodePath}/AssetLib/Obj/ObjFileParser.h",
    "%{CodePath}/AssetLib/Obj/ObjTools.h"
}
AddAssimpImporter(OBJImporter)

OGREImporter = { 
  "%{CodePath}/AssetLib/Ogre/OgreImporter.h",
  "%{CodePath}/AssetLib/Ogre/OgreStructs.h",
  "%{CodePath}/AssetLib/Ogre/OgreParsingUtils.h",
  "%{CodePath}/AssetLib/Ogre/OgreBinarySerializer.h",
  "%{CodePath}/AssetLib/Ogre/OgreXmlSerializer.h",
  "%{CodePath}/AssetLib/Ogre/OgreImporter.cpp",
  "%{CodePath}/AssetLib/Ogre/OgreStructs.cpp",
  "%{CodePath}/AssetLib/Ogre/OgreBinarySerializer.cpp",
  "%{CodePath}/AssetLib/Ogre/OgreXmlSerializer.cpp",
  "%{CodePath}/AssetLib/Ogre/OgreMaterial.cpp"
}
AddAssimpImporter(OGREImporter)

OPENGEXImporter = {
    "%{CodePath}/AssetLib/OpenGEX/OpenGEXImporter.cpp",
    "%{CodePath}/AssetLib/OpenGEX/OpenGEXImporter.h",
    "%{CodePath}/AssetLib/OpenGEX/OpenGEXStructs.h"
}
AddAssimpImporter(OPENGEXImporter)

PLYImporter = {
    "%{CodePath}/AssetLib/Ply/PlyLoader.cpp",
    "%{CodePath}/AssetLib/Ply/PlyLoader.h",
    "%{CodePath}/AssetLib/Ply/PlyParser.cpp",
    "%{CodePath}/AssetLib/Ply/PlyParser.h"
}
AddAssimpImporter(PLYImporter)

MS3DImporter = {
    "%{CodePath}/AssetLib/MS3D/MS3DLoader.cpp",
    "%{CodePath}/AssetLib/MS3D/MS3DLoader.h"
}
AddAssimpImporter(MS3DImporter)

COBImporter = {
    "%{CodePath}/AssetLib/COB/COBLoader.cpp",
    "%{CodePath}/AssetLib/COB/COBLoader.h",
    "%{CodePath}/AssetLib/COB/COBScene.h"
}
AddAssimpImporter(COBImporter)

BLENDImporter = {
    "%{CodePath}/AssetLib/Blender/BlenderLoader.cpp",
    "%{CodePath}/AssetLib/Blender/BlenderLoader.h",
    "%{CodePath}/AssetLib/Blender/BlenderDNA.cpp",
    "%{CodePath}/AssetLib/Blender/BlenderDNA.h",
    "%{CodePath}/AssetLib/Blender/BlenderDNA.inl",
    "%{CodePath}/AssetLib/Blender/BlenderScene.cpp",
    "%{CodePath}/AssetLib/Blender/BlenderScene.h",
    "%{CodePath}/AssetLib/Blender/BlenderSceneGen.h",
    "%{CodePath}/AssetLib/Blender/BlenderIntermediate.h",
    "%{CodePath}/AssetLib/Blender/BlenderModifier.h",
    "%{CodePath}/AssetLib/Blender/BlenderModifier.cpp",
    "%{CodePath}/AssetLib/Blender/BlenderBMesh.h",
    "%{CodePath}/AssetLib/Blender/BlenderBMesh.cpp",
    "%{CodePath}/AssetLib/Blender/BlenderTessellator.h",
    "%{CodePath}/AssetLib/Blender/BlenderTessellator.cpp",
    "%{CodePath}/AssetLib/Blender/BlenderCustomData.h",
    "%{CodePath}/AssetLib/Blender/BlenderCustomData.cpp"
}
AddAssimpImporter(BLENDImporter)

IFCImporter = {
    "%{CodePath}/AssetLib/IFC/IFCLoader.cpp",
    "%{CodePath}/AssetLib/IFC/IFCLoader.h",
    "%{CodePath}/AssetLib/IFC/IFCReaderGen1_2x3.cpp",
    "%{CodePath}/AssetLib/IFC/IFCReaderGen2_2x3.cpp",
    "%{CodePath}/AssetLib/IFC/IFCReaderGen_2x3.h",
    "%{CodePath}/AssetLib/IFC/IFCUtil.h",
    "%{CodePath}/AssetLib/IFC/IFCUtil.cpp",
    "%{CodePath}/AssetLib/IFC/IFCGeometry.cpp",
    "%{CodePath}/AssetLib/IFC/IFCMaterial.cpp",
    "%{CodePath}/AssetLib/IFC/IFCProfile.cpp",
    "%{CodePath}/AssetLib/IFC/IFCCurve.cpp",
    "%{CodePath}/AssetLib/IFC/IFCBoolean.cpp",
    "%{CodePath}/AssetLib/IFC/IFCOpenings.cpp"
}
AddAssimpImporter(IFCImporter)

XGLImporter = {
    "%{CodePath}/AssetLib/XGL/XGLLoader.cpp",
    "%{CodePath}/AssetLib/XGL/XGLLoader.h"
}
AddAssimpImporter(XGLImporter)

FBXImporter = {
    "%{CodePath}/AssetLib/FBX/FBXImporter.cpp",
    "%{CodePath}/AssetLib/FBX/FBXCompileConfig.h",
    "%{CodePath}/AssetLib/FBX/FBXImporter.h",
    "%{CodePath}/AssetLib/FBX/FBXParser.cpp",
    "%{CodePath}/AssetLib/FBX/FBXParser.h",
    "%{CodePath}/AssetLib/FBX/FBXTokenizer.cpp",
    "%{CodePath}/AssetLib/FBX/FBXTokenizer.h",
    "%{CodePath}/AssetLib/FBX/FBXImportSettings.h",
    "%{CodePath}/AssetLib/FBX/FBXConverter.h",
    "%{CodePath}/AssetLib/FBX/FBXConverter.cpp",
    "%{CodePath}/AssetLib/FBX/FBXUtil.h",
    "%{CodePath}/AssetLib/FBX/FBXUtil.cpp",
    "%{CodePath}/AssetLib/FBX/FBXDocument.h",
    "%{CodePath}/AssetLib/FBX/FBXDocument.cpp",
    "%{CodePath}/AssetLib/FBX/FBXProperties.h",
    "%{CodePath}/AssetLib/FBX/FBXProperties.cpp",
    "%{CodePath}/AssetLib/FBX/FBXMeshGeometry.h",
    "%{CodePath}/AssetLib/FBX/FBXMeshGeometry.cpp",
    "%{CodePath}/AssetLib/FBX/FBXMaterial.cpp",
    "%{CodePath}/AssetLib/FBX/FBXModel.cpp",
    "%{CodePath}/AssetLib/FBX/FBXAnimation.cpp",
    "%{CodePath}/AssetLib/FBX/FBXNodeAttribute.cpp",
    "%{CodePath}/AssetLib/FBX/FBXDeformer.cpp",
    "%{CodePath}/AssetLib/FBX/FBXBinaryTokenizer.cpp",
    "%{CodePath}/AssetLib/FBX/FBXDocumentUtil.cpp",
    "%{CodePath}/AssetLib/FBX/FBXCommon.h"
}
AddAssimpImporter(FBXImporter)

if (AssimpIncludeExporters) then
    OBJExporter = {
        "%{CodePath}/AssetLib/Obj/ObjExporter.h",
        "%{CodePath}/AssetLib/Obj/ObjExporter.cpp"
    }
    AddAssimpExporter(OBJExporter)

    OPENGEXExporter = {
        "%{CodePath}/AssetLib/OpenGEX/OpenGEXExporter.cpp",
        "%{CodePath}/AssetLib/OpenGEX/OpenGEXExporter.h"
    }
    AddAssimpExporter(OPENGEXExporter)

    PLYExporter = {
        "%{CodePath}/AssetLib/Ply/PlyExporter.cpp",
        "%{CodePath}/AssetLib/Ply/PlyExporter.h"
    }
    AddAssimpExporter(PLYExporter)

    _3DSExporter = {
        "%{CodePath}/AssetLib/3DS/3DSExporter.h",
        "%{CodePath}/AssetLib/3DS/3DSExporter.cpp"
    }
    AddAssimpExporter(_3DSExporter)

    ASSBINExporter = {
        "%{CodePath}/AssetLib/Assbin/AssbinExporter.h",
        "%{CodePath}/AssetLib/Assbin/AssbinExporter.cpp",
        "%{CodePath}/AssetLib/Assbin/AssbinFileWriter.h",
        "%{CodePath}/AssetLib/Assbin/AssbinFileWriter.cpp"
    }
    AddAssimpExporter(ASSBINExporter)

    ASSXMLExporter = {
        "%{CodePath}/AssetLib/Assxml/AssxmlExporter.h",
        "%{CodePath}/AssetLib/Assxml/AssxmlExporter.cpp",
        "%{CodePath}/AssetLib/Assxml/AssxmlFileWriter.h",
        "%{CodePath}/AssetLib/Assxml/AssxmlFileWriter.cpp"
    }
    AddAssimpExporter(ASSXMLExporter)

    M3DExporter = {
        "%{CodePath}/AssetLib/M3D/M3DExporter.h",
        "%{CodePath}/AssetLib/M3D/M3DExporter.cpp"
    }
    AddAssimpExporter(M3DExporter)

    ColladaExporter = {
        "%{CodePath}/AssetLib/Collada/ColladaExporter.h",
        "%{CodePath}/AssetLib/Collada/ColladaExporter.cpp"
    }
    AddAssimpExporter(ColladaExporter)

    FBXExporter = {
        "%{CodePath}/AssetLib/FBX/FBXExporter.h",
        "%{CodePath}/AssetLib/FBX/FBXExporter.cpp",
        "%{CodePath}/AssetLib/FBX/FBXExportNode.h",
        "%{CodePath}/AssetLib/FBX/FBXExportNode.cpp",
        "%{CodePath}/AssetLib/FBX/FBXExportProperty.h",
        "%{CodePath}/AssetLib/FBX/FBXExportProperty.cpp"
    }
    AddAssimpExporter(FBXExporter)

    STLExporter = {
        "%{CodePath}/AssetLib/STL/STLExporter.h",
        "%{CodePath}/AssetLib/STL/STLExporter.cpp"
    }
    AddAssimpExporter(STLExporter)

    XExporter = {
        "%{CodePath}/AssetLib/X/XFileExporter.h",
        "%{CodePath}/AssetLib/X/XFileExporter.cpp"
    }
    AddAssimpExporter(XExporter)

    X3DExporter = {
        "%{CodePath}/AssetLib/X3D/X3DExporter.cpp",
        "%{CodePath}/AssetLib/X3D/X3DExporter.hpp"
    }
    AddAssimpExporter(X3DExporter)

    GLTFExporter = {
        "%{CodePath}/AssetLib/glTF/glTFExporter.h",
        "%{CodePath}/AssetLib/glTF/glTFExporter.cpp",
        "%{CodePath}/AssetLib/glTF2/glTF2Exporter.h",
        "%{CodePath}/AssetLib/glTF2/glTF2Exporter.cpp"
    }
    AddAssimpExporter(GLTFExporter)

    _3MFExporter = {
        "%{CodePath}/AssetLib/3MF/D3MFExporter.h",
        "%{CodePath}/AssetLib/3MF/D3MFExporter.cpp"
    }
    AddAssimpExporter(_3MFExporter)

    PBRTExporter = {
        "%{CodePath}/Pbrt/PbrtExporter.h",
        "%{CodePath}/Pbrt/PbrtExporter.cpp"
    }
    AddAssimpExporter(PBRTExporter)

    ASSJSON = {
        "%{CodePath}/AssetLib/Assjson/cencode.c",
        "%{CodePath}/AssetLib/Assjson/cencode.h",
        "%{CodePath}/AssetLib/Assjson/json_exporter.cpp",
        "%{CodePath}/AssetLib/Assjson/mesh_splitter.cpp",
        "%{CodePath}/AssetLib/Assjson/mesh_splitter.h"
    }
    AddAssimpExporter(ASSJSON)

    StepExporter = {
        "%{CodePath}/AssetLib/Step/StepExporter.h",
        "%{CodePath}/AssetLib/Step/StepExporter.cpp"
    }
    AddAssimpExporter(StepExporter)
end

PostProcessingSourceFiles = {
    "%{CodePath}/PostProcessing/CalcTangentsProcess.cpp",
    "%{CodePath}/PostProcessing/CalcTangentsProcess.h",
    "%{CodePath}/PostProcessing/ComputeUVMappingProcess.cpp",
    "%{CodePath}/PostProcessing/ComputeUVMappingProcess.h",
    "%{CodePath}/PostProcessing/ConvertToLHProcess.cpp",
    "%{CodePath}/PostProcessing/ConvertToLHProcess.h",
    "%{CodePath}/PostProcessing/EmbedTexturesProcess.cpp",
    "%{CodePath}/PostProcessing/EmbedTexturesProcess.h",
    "%{CodePath}/PostProcessing/FindDegenerates.cpp",
    "%{CodePath}/PostProcessing/FindDegenerates.h",
    "%{CodePath}/PostProcessing/FindInstancesProcess.cpp",
    "%{CodePath}/PostProcessing/FindInstancesProcess.h",
    "%{CodePath}/PostProcessing/FindInvalidDataProcess.cpp",
    "%{CodePath}/PostProcessing/FindInvalidDataProcess.h",
    "%{CodePath}/PostProcessing/FixNormalsStep.cpp",
    "%{CodePath}/PostProcessing/FixNormalsStep.h",
    "%{CodePath}/PostProcessing/DropFaceNormalsProcess.cpp",
    "%{CodePath}/PostProcessing/DropFaceNormalsProcess.h",
    "%{CodePath}/PostProcessing/GenFaceNormalsProcess.cpp",
    "%{CodePath}/PostProcessing/GenFaceNormalsProcess.h",
    "%{CodePath}/PostProcessing/GenVertexNormalsProcess.cpp",
    "%{CodePath}/PostProcessing/GenVertexNormalsProcess.h",
    "%{CodePath}/PostProcessing/PretransformVertices.cpp",
    "%{CodePath}/PostProcessing/PretransformVertices.h",
    "%{CodePath}/PostProcessing/ImproveCacheLocality.cpp",
    "%{CodePath}/PostProcessing/ImproveCacheLocality.h",
    "%{CodePath}/PostProcessing/JoinVerticesProcess.cpp",
    "%{CodePath}/PostProcessing/JoinVerticesProcess.h",
    "%{CodePath}/PostProcessing/LimitBoneWeightsProcess.cpp",
    "%{CodePath}/PostProcessing/LimitBoneWeightsProcess.h",
    "%{CodePath}/PostProcessing/RemoveRedundantMaterials.cpp",
    "%{CodePath}/PostProcessing/RemoveRedundantMaterials.h",
    "%{CodePath}/PostProcessing/RemoveVCProcess.cpp",
    "%{CodePath}/PostProcessing/RemoveVCProcess.h",
    "%{CodePath}/PostProcessing/SortByPTypeProcess.cpp",
    "%{CodePath}/PostProcessing/SortByPTypeProcess.h",
    "%{CodePath}/PostProcessing/SplitLargeMeshes.cpp",
    "%{CodePath}/PostProcessing/SplitLargeMeshes.h",
    "%{CodePath}/PostProcessing/TextureTransform.cpp",
    "%{CodePath}/PostProcessing/TextureTransform.h",
    "%{CodePath}/PostProcessing/TriangulateProcess.cpp",
    "%{CodePath}/PostProcessing/TriangulateProcess.h",
    "%{CodePath}/PostProcessing/ValidateDataStructure.cpp",
    "%{CodePath}/PostProcessing/ValidateDataStructure.h",
    "%{CodePath}/PostProcessing/OptimizeGraph.cpp",
    "%{CodePath}/PostProcessing/OptimizeGraph.h",
    "%{CodePath}/PostProcessing/OptimizeMeshes.cpp",
    "%{CodePath}/PostProcessing/OptimizeMeshes.h",
    "%{CodePath}/PostProcessing/DeboneProcess.cpp",
    "%{CodePath}/PostProcessing/DeboneProcess.h",
    "%{CodePath}/PostProcessing/ProcessHelper.h",
    "%{CodePath}/PostProcessing/ProcessHelper.cpp",
    "%{CodePath}/PostProcessing/MakeVerboseFormat.cpp",
    "%{CodePath}/PostProcessing/MakeVerboseFormat.h",
    "%{CodePath}/PostProcessing/ScaleProcess.cpp",
    "%{CodePath}/PostProcessing/ScaleProcess.h",
    "%{CodePath}/PostProcessing/ArmaturePopulate.cpp",
    "%{CodePath}/PostProcessing/ArmaturePopulate.h",
    "%{CodePath}/PostProcessing/GenBoundingBoxesProcess.cpp",
    "%{CodePath}/PostProcessing/GenBoundingBoxesProcess.h",
    "%{CodePath}/PostProcessing/SplitByBoneCountProcess.cpp",
    "%{CodePath}/PostProcessing/SplitByBoneCountProcess.h"
}
AddSourceFilesCode(PostProcessingSourceFiles)

Q3DImporter = {
    "%{CodePath}/AssetLib/Q3D/Q3DLoader.cpp",
    "%{CodePath}/AssetLib/Q3D/Q3DLoader.h"
}
AddAssimpImporter(Q3DImporter)

Q3BSPImporter = {
    "%{CodePath}/AssetLib/Q3BSP/Q3BSPFileData.h",
    "%{CodePath}/AssetLib/Q3BSP/Q3BSPFileParser.h",
    "%{CodePath}/AssetLib/Q3BSP/Q3BSPFileParser.cpp",
    "%{CodePath}/AssetLib/Q3BSP/Q3BSPFileImporter.h",
    "%{CodePath}/AssetLib/Q3BSP/Q3BSPFileImporter.cpp"
}
AddAssimpImporter(Q3BSPImporter)

RAWImporter = {
    "%{CodePath}/AssetLib/Raw/RawLoader.cpp",
    "%{CodePath}/AssetLib/Raw/RawLoader.h"
}
AddAssimpImporter(RAWImporter)

SIBImporter = {
    "%{CodePath}/AssetLib/SIB/SIBImporter.cpp",
    "%{CodePath}/AssetLib/SIB/SIBImporter.h"
}
AddAssimpImporter(SIBImporter)

SMDImporter = {
    "%{CodePath}/AssetLib/SMD/SMDLoader.cpp",
    "%{CodePath}/AssetLib/SMD/SMDLoader.h"
}
AddAssimpImporter(SMDImporter)

STLImporter = {
    "%{CodePath}/AssetLib/STL/STLLoader.cpp",
    "%{CodePath}/AssetLib/STL/STLLoader.h"
}
AddAssimpImporter(STLImporter)

TerragenImporter = {
    "%{CodePath}/AssetLib/Terragen/TerragenLoader.cpp",
    "%{CodePath}/AssetLib/Terragen/TerragenLoader.h"
}
AddAssimpImporter(TerragenImporter)

_3DImporter = {
    "%{CodePath}/AssetLib/Unreal/UnrealLoader.cpp",
    "%{CodePath}/AssetLib/Unreal/UnrealLoader.h"
}
AddAssimpImporter(_3DImporter)

XImporter = {
    "%{CodePath}/AssetLib/X/XFileHelper.h",
    "%{CodePath}/AssetLib/X/XFileImporter.cpp",
    "%{CodePath}/AssetLib/X/XFileImporter.h",
    "%{CodePath}/AssetLib/X/XFileParser.cpp",
    "%{CodePath}/AssetLib/X/XFileParser.h"
}
AddAssimpImporter(XImporter)

X3DImporter = {
    "%{CodePath}/AssetLib/X3D/X3DImporter.cpp",
    "%{CodePath}/AssetLib/X3D/X3DImporter_Geometry2D.cpp",
    "%{CodePath}/AssetLib/X3D/X3DImporter_Geometry3D.cpp",
    "%{CodePath}/AssetLib/X3D/X3DImporter_Group.cpp",
    "%{CodePath}/AssetLib/X3D/X3DImporter_Light.cpp",
    "%{CodePath}/AssetLib/X3D/X3DImporter_Metadata.cpp",
    "%{CodePath}/AssetLib/X3D/X3DImporter_Networking.cpp",
    "%{CodePath}/AssetLib/X3D/X3DImporter_Postprocess.cpp",
    "%{CodePath}/AssetLib/X3D/X3DImporter_Rendering.cpp",
    "%{CodePath}/AssetLib/X3D/X3DImporter_Shape.cpp",
    "%{CodePath}/AssetLib/X3D/X3DImporter_Texturing.cpp",
    "%{CodePath}/AssetLib/X3D/X3DImporter.hpp",
    "%{CodePath}/AssetLib/X3D/X3DImporter_Macro.hpp",
    "%{CodePath}/AssetLib/X3D/X3DImporter_Node.hpp",
    "%{CodePath}/AssetLib/X3D/X3DGeoHelper.cpp",
    "%{CodePath}/AssetLib/X3D/X3DGeoHelper.h",
    "%{CodePath}/AssetLib/X3D/X3DXmlHelper.cpp",
    "%{CodePath}/AssetLib/X3D/X3DXmlHelper.h"
}
AddAssimpImporter(X3DImporter)

GLTFImporter = {
    "%{CodePath}/AssetLib/glTF/glTFCommon.h",
    "%{CodePath}/AssetLib/glTF/glTFCommon.cpp",
    "%{CodePath}/AssetLib/glTF/glTFAsset.h",
    "%{CodePath}/AssetLib/glTF/glTFAsset.inl",
    "%{CodePath}/AssetLib/glTF/glTFAssetWriter.h",
    "%{CodePath}/AssetLib/glTF/glTFAssetWriter.inl",
    "%{CodePath}/AssetLib/glTF/glTFImporter.cpp",
    "%{CodePath}/AssetLib/glTF/glTFImporter.h",
    "%{CodePath}/AssetLib/glTF2/glTF2Asset.h",
    "%{CodePath}/AssetLib/glTF2/glTF2Asset.inl",
    "%{CodePath}/AssetLib/glTF2/glTF2AssetWriter.h",
    "%{CodePath}/AssetLib/glTF2/glTF2AssetWriter.inl",
    "%{CodePath}/AssetLib/glTF2/glTF2Importer.cpp",
    "%{CodePath}/AssetLib/glTF2/glTF2Importer.h"
}
AddAssimpImporter(GLTFImporter)

_3MFImporter = {
    "%{CodePath}/AssetLib/3MF/3MFTypes.h",
    "%{CodePath}/AssetLib/3MF/XmlSerializer.h",
    "%{CodePath}/AssetLib/3MF/XmlSerializer.cpp",
    "%{CodePath}/AssetLib/3MF/D3MFImporter.h",
    "%{CodePath}/AssetLib/3MF/D3MFImporter.cpp",
    "%{CodePath}/AssetLib/3MF/D3MFOpcPackage.h",
    "%{CodePath}/AssetLib/3MF/D3MFOpcPackage.cpp",
    "%{CodePath}/AssetLib/3MF/3MFXmlTags.h"
}
AddAssimpImporter(_3MFImporter)

MMDImporter = {
    "%{CodePath}/AssetLib/MMD/MMDCpp14.h",
    "%{CodePath}/AssetLib/MMD/MMDImporter.cpp",
    "%{CodePath}/AssetLib/MMD/MMDImporter.h",
    "%{CodePath}/AssetLib/MMD/MMDPmdParser.h",
    "%{CodePath}/AssetLib/MMD/MMDPmxParser.h",
    "%{CodePath}/AssetLib/MMD/MMDPmxParser.cpp",
    "%{CodePath}/AssetLib/MMD/MMDVmdParser.h"
}
AddAssimpImporter(MMDImporter)

if (AssimpIncludeExporters) then
    ExporterSourceFiles = {
        "%{CodePath}/Common/Exporter.cpp",
        "%{CodePath}/CApi/AssimpCExport.cpp",
        "%{HeaderPath}/BlobIOSystem.h"
    }
    AddAssimpExporter(ExporterSourceFiles)
end

