# Open Asset Import Library (assimp)
# ----------------------------------------------------------------------
# Copyright (c) 2006-2024, assimp team
#
# All rights reserved.
#
# Redistribution and use of this software in source and binary forms,
# with or without modification, are permitted provided that the
# following conditions are met:
#
# * Redistributions of source code must retain the above
#   copyright notice, this list of conditions and the
#   following disclaimer.
#
# * Redistributions in binary form must reproduce the above
#   copyright notice, this list of conditions and the
#   following disclaimer in the documentation and/or other
#   materials provided with the distribution.
#
# * Neither the name of the assimp team, nor the names of its
#   contributors may be used to endorse or promote products
#   derived from this software without specific prior
#   written permission of the assimp team.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#----------------------------------------------------------------------

# Listing and grouping of all the source files.
# 1) Set the file lists for each component
# 2) Create a Source Group for each component, for IDE project orginization
# 3) Add libassimp using the file lists (eliminates duplication of file names between
#    source groups and library command)
#
cmake_minimum_required( VERSION 3.22 )
SET( HEADER_PATH ../include/assimp )

if(NOT ANDROID AND ASSIMP_ANDROID_JNIIOSYSTEM)
    message(WARNING "Requesting Android JNI I/O-System in non-Android toolchain. Resetting ASSIMP_ANDROID_JNIIOSYSTEM to OFF.")
    set(ASSIMP_ANDROID_JNIIOSYSTEM OFF)
endif()

SET( COMPILER_HEADERS
  ${HEADER_PATH}/Compiler/pushpack1.h
  ${HEADER_PATH}/Compiler/poppack1.h
  ${HEADER_PATH}/Compiler/pstdint.h
)
SOURCE_GROUP( Compiler FILES ${COMPILER_HEADERS})

SET( PUBLIC_HEADERS
  ${HEADER_PATH}/anim.h
  ${HEADER_PATH}/aabb.h
  ${HEADER_PATH}/ai_assert.h
  ${HEADER_PATH}/camera.h
  ${HEADER_PATH}/color4.h
  ${HEADER_PATH}/color4.inl
  ${CMAKE_CURRENT_BINARY_DIR}/../include/assimp/config.h
  ${HEADER_PATH}/ColladaMetaData.h
  ${HEADER_PATH}/commonMetaData.h
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
  ${HEADER_PATH}/ObjMaterial.h
  ${HEADER_PATH}/pbrmaterial.h
  ${HEADER_PATH}/GltfMaterial.h
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
  ${HEADER_PATH}/AssertHandler.h
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
  ${HEADER_PATH}/DefaultIOStream.h
  ${HEADER_PATH}/DefaultIOSystem.h
  ${HEADER_PATH}/ZipArchiveIOSystem.h
  ${HEADER_PATH}/SceneCombiner.h
  ${HEADER_PATH}/fast_atof.h
  ${HEADER_PATH}/qnan.h
  ${HEADER_PATH}/BaseImporter.h
  ${HEADER_PATH}/Hash.h
  ${HEADER_PATH}/MemoryIOWrapper.h
  ${HEADER_PATH}/ParsingUtils.h
  ${HEADER_PATH}/StreamReader.h
  ${HEADER_PATH}/StreamWriter.h
  ${HEADER_PATH}/StringComparison.h
  ${HEADER_PATH}/StringUtils.h
  ${HEADER_PATH}/SGSpatialSort.h
  ${HEADER_PATH}/GenericProperty.h
  ${HEADER_PATH}/SpatialSort.h
  ${HEADER_PATH}/SkeletonMeshBuilder.h
  ${HEADER_PATH}/SmallVector.h
  ${HEADER_PATH}/SmoothingGroups.h
  ${HEADER_PATH}/SmoothingGroups.inl
  ${HEADER_PATH}/StandardShapes.h
  ${HEADER_PATH}/RemoveComments.h
  ${HEADER_PATH}/Subdivision.h
  ${HEADER_PATH}/Vertex.h
  ${HEADER_PATH}/LineSplitter.h
  ${HEADER_PATH}/TinyFormatter.h
  ${HEADER_PATH}/Profiler.h
  ${HEADER_PATH}/LogAux.h
  ${HEADER_PATH}/Bitmap.h
  ${HEADER_PATH}/XMLTools.h
  ${HEADER_PATH}/IOStreamBuffer.h
  ${HEADER_PATH}/CreateAnimMesh.h
  ${HEADER_PATH}/XmlParser.h
  ${HEADER_PATH}/BlobIOSystem.h
  ${HEADER_PATH}/MathFunctions.h
  ${HEADER_PATH}/Exceptional.h
  ${HEADER_PATH}/ByteSwapper.h
  ${HEADER_PATH}/Base64.hpp
)

SET( Core_SRCS
  Common/Assimp.cpp
)

IF(MSVC OR (MINGW AND BUILD_SHARED_LIBS))
  list(APPEND Core_SRCS "res/assimp.rc")
ENDIF()

SET( Logging_SRCS
  ${HEADER_PATH}/DefaultLogger.hpp
  ${HEADER_PATH}/LogStream.hpp
  ${HEADER_PATH}/Logger.hpp
  ${HEADER_PATH}/NullLogger.hpp
  Common/Win32DebugLogStream.h
  Common/DefaultLogger.cpp
  Common/FileLogStream.h
  Common/StdOStreamLogStream.h
)
SOURCE_GROUP(Logging FILES ${Logging_SRCS})

SET( Common_SRCS
  Common/StbCommon.h
  Common/Compression.cpp
  Common/Compression.h
  Common/BaseImporter.cpp
  Common/BaseProcess.cpp
  Common/BaseProcess.h
  Common/Importer.h
  Common/ScenePrivate.h
  Common/PostStepRegistry.cpp
  Common/ImporterRegistry.cpp
  Common/DefaultProgressHandler.h
  Common/DefaultIOStream.cpp
  Common/IOSystem.cpp
  Common/DefaultIOSystem.cpp
  Common/ZipArchiveIOSystem.cpp
  Common/PolyTools.h
  Common/Maybe.h
  Common/Importer.cpp
  Common/IFF.h
  Common/SGSpatialSort.cpp
  Common/VertexTriangleAdjacency.cpp
  Common/VertexTriangleAdjacency.h
  Common/SpatialSort.cpp
  Common/SceneCombiner.cpp
  Common/ScenePreprocessor.cpp
  Common/ScenePreprocessor.h
  Common/SkeletonMeshBuilder.cpp
  Common/StackAllocator.h
  Common/StackAllocator.inl
  Common/StandardShapes.cpp
  Common/TargetAnimation.cpp
  Common/TargetAnimation.h
  Common/RemoveComments.cpp
  Common/Subdivision.cpp
  Common/scene.cpp
  Common/Bitmap.cpp
  Common/Version.cpp
  Common/CreateAnimMesh.cpp
  Common/simd.h
  Common/simd.cpp
  Common/material.cpp
  Common/AssertHandler.cpp
  Common/Exceptional.cpp
  Common/Base64.cpp
)
SOURCE_GROUP(Common FILES ${Common_SRCS})

SET( CApi_SRCS
  CApi/CInterfaceIOWrapper.cpp
  CApi/CInterfaceIOWrapper.h
)
SOURCE_GROUP(CApi FILES ${CApi_SRCS})

SET(Geometry_SRCS
  Geometry/GeometryUtils.h
  Geometry/GeometryUtils.cpp
)
SOURCE_GROUP(Geometry FILES ${Geometry_SRCS})

SET( STEPParser_SRCS
  AssetLib/STEPParser/STEPFileReader.h
  AssetLib/STEPParser/STEPFileReader.cpp
  AssetLib/STEPParser/STEPFileEncoding.cpp
  AssetLib/STEPParser/STEPFileEncoding.h
)
SOURCE_GROUP(STEPParser FILES ${STEPParser_SRCS})

IF ( ASSIMP_BUILD_NONFREE_C4D_IMPORTER )
  SET( C4D_SRCS
    AssetLib/C4D/C4DImporter.cpp
    AssetLib/C4D/C4DImporter.h
  )
  SOURCE_GROUP( C4D FILES ${C4D_SRCS})
ENDIF ()

# if this variable is set to TRUE, the user can manually disable importers by setting
# ASSIMP_BUILD_XXX_IMPORTER to FALSE for each importer
# if this variable is set to FALSE, the user can manually enable importers by setting
# ASSIMP_BUILD_XXX_IMPORTER to TRUE for each importer
OPTION(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT "default value of all ASSIMP_BUILD_XXX_IMPORTER values" TRUE)

# macro to add the CMake Option ADD_ASSIMP_IMPORTER_<name> which enables compile of loader
# this way selective loaders can be compiled (reduces filesize + compile time)
MACRO(ADD_ASSIMP_IMPORTER name)
  IF (ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT)
    set(ASSIMP_IMPORTER_ENABLED TRUE)
    IF (DEFINED ASSIMP_BUILD_${name}_IMPORTER AND NOT ASSIMP_BUILD_${name}_IMPORTER)
      set(ASSIMP_IMPORTER_ENABLED FALSE)
    ENDIF ()
  ELSE ()
    set(ASSIMP_IMPORTER_ENABLED ${ASSIMP_BUILD_${name}_IMPORTER})
  ENDIF ()
  IF (ASSIMP_IMPORTER_ENABLED)
    LIST(APPEND ASSIMP_LOADER_SRCS ${ARGN})
    SET(ASSIMP_IMPORTERS_ENABLED "${ASSIMP_IMPORTERS_ENABLED} ${name}")
    SOURCE_GROUP(AssetLib\\${name} FILES ${ARGN})
  ELSE()
    SET(${name}_SRC "")
    SET(ASSIMP_IMPORTERS_DISABLED "${ASSIMP_IMPORTERS_DISABLED} ${name}")
    add_definitions(-DASSIMP_BUILD_NO_${name}_IMPORTER)
  ENDIF()
ENDMACRO()

if (NOT ASSIMP_NO_EXPORT)

  # if this variable is set to TRUE, the user can manually disable exporters by setting
  # ASSIMP_BUILD_XXX_EXPORTER to FALSE for each exporter
  # if this variable is set to FALSE, the user can manually enable exporters by setting
  # ASSIMP_BUILD_XXX_EXPORTER to TRUE for each exporter
  OPTION(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT "default value of all ASSIMP_BUILD_XXX_EXPORTER values" TRUE)

  # macro to add the CMake Option ADD_ASSIMP_EXPORTER_<name> which enables compilation of an exporter
  # this way selective exporters can be compiled (reduces filesize + compile time)
  MACRO(ADD_ASSIMP_EXPORTER name)
    IF (ASSIMP_NO_EXPORT)
      set(ASSIMP_EXPORTER_ENABLED FALSE)
    ELSEIF (ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT)
      set(ASSIMP_EXPORTER_ENABLED TRUE)
      IF (DEFINED ASSIMP_BUILD_${name}_EXPORTER AND NOT ASSIMP_BUILD_${name}_EXPORTER)
        set(ASSIMP_EXPORTER_ENABLED FALSE)
      ENDIF ()
    ELSE ()
      set(ASSIMP_EXPORTER_ENABLED ${ASSIMP_BUILD_${name}_EXPORTER})
    ENDIF ()

    IF (ASSIMP_EXPORTER_ENABLED)
      SET(ASSIMP_EXPORTERS_ENABLED "${ASSIMP_EXPORTERS_ENABLED} ${name}")
      LIST(APPEND ASSIMP_EXPORTER_SRCS ${ARGN})
      SOURCE_GROUP(AssetLib\\${name} FILES ${ARGN})
    ELSE()
      SET(ASSIMP_EXPORTERS_DISABLED "${ASSIMP_EXPORTERS_DISABLED} ${name}")
      add_definitions(-DASSIMP_BUILD_NO_${name}_EXPORTER)
    ENDIF()
  ENDMACRO()

endif()

SET(ASSIMP_LOADER_SRCS "")
SET(ASSIMP_IMPORTERS_ENABLED "") # list of enabled importers
SET(ASSIMP_IMPORTERS_DISABLED "") # disabled importers list (used to print)

SET(ASSIMP_EXPORTER_SRCS "")
SET(ASSIMP_EXPORTERS_ENABLED "") # list of enabled exporters
SET(ASSIMP_EXPORTERS_DISABLED "") # disabled exporters list (used to print)

ADD_ASSIMP_IMPORTER( AMF
  AssetLib/AMF/AMFImporter.hpp
  AssetLib/AMF/AMFImporter_Node.hpp
  AssetLib/AMF/AMFImporter.cpp
  AssetLib/AMF/AMFImporter_Geometry.cpp
  AssetLib/AMF/AMFImporter_Material.cpp
  AssetLib/AMF/AMFImporter_Postprocess.cpp
)

ADD_ASSIMP_IMPORTER( 3DS
  AssetLib/3DS/3DSConverter.cpp
  AssetLib/3DS/3DSHelper.h
  AssetLib/3DS/3DSLoader.cpp
  AssetLib/3DS/3DSLoader.h
)

ADD_ASSIMP_IMPORTER( AC
  AssetLib/AC/ACLoader.cpp
  AssetLib/AC/ACLoader.h
)

ADD_ASSIMP_IMPORTER( ASE
  AssetLib/ASE/ASELoader.cpp
  AssetLib/ASE/ASELoader.h
  AssetLib/ASE/ASEParser.cpp
  AssetLib/ASE/ASEParser.h
)

ADD_ASSIMP_IMPORTER( ASSBIN
  AssetLib/Assbin/AssbinLoader.h
  AssetLib/Assbin/AssbinLoader.cpp
)

ADD_ASSIMP_IMPORTER( B3D
  AssetLib/B3D/B3DImporter.cpp
  AssetLib/B3D/B3DImporter.h
)

ADD_ASSIMP_IMPORTER( BVH
  AssetLib/BVH/BVHLoader.cpp
  AssetLib/BVH/BVHLoader.h
)

ADD_ASSIMP_IMPORTER( COLLADA
  AssetLib/Collada/ColladaHelper.cpp
  AssetLib/Collada/ColladaHelper.h
  AssetLib/Collada/ColladaLoader.cpp
  AssetLib/Collada/ColladaLoader.h
  AssetLib/Collada/ColladaParser.cpp
  AssetLib/Collada/ColladaParser.h
)

ADD_ASSIMP_IMPORTER( DXF
  AssetLib/DXF/DXFLoader.cpp
  AssetLib/DXF/DXFLoader.h
  AssetLib/DXF/DXFHelper.h
)

ADD_ASSIMP_IMPORTER( CSM
  AssetLib/CSM/CSMLoader.cpp
  AssetLib/CSM/CSMLoader.h
)

ADD_ASSIMP_IMPORTER( HMP
  AssetLib/HMP/HMPFileData.h
  AssetLib/HMP/HMPLoader.cpp
  AssetLib/HMP/HMPLoader.h
  AssetLib/HMP/HalfLifeFileData.h
)

ADD_ASSIMP_IMPORTER( IRRMESH
  AssetLib/Irr/IRRMeshLoader.cpp
  AssetLib/Irr/IRRMeshLoader.h
  AssetLib/Irr/IRRShared.cpp
  AssetLib/Irr/IRRShared.h
)

ADD_ASSIMP_IMPORTER( IQM
  AssetLib/IQM/IQMImporter.cpp
  AssetLib/IQM/iqm.h
  AssetLib/IQM/IQMImporter.h
)

ADD_ASSIMP_IMPORTER( IRR
  AssetLib/Irr/IRRLoader.cpp
  AssetLib/Irr/IRRLoader.h
  AssetLib/Irr/IRRShared.cpp
  AssetLib/Irr/IRRShared.h
)

ADD_ASSIMP_IMPORTER( LWO
  AssetLib/LWO/LWOAnimation.cpp
  AssetLib/LWO/LWOAnimation.h
  AssetLib/LWO/LWOBLoader.cpp
  AssetLib/LWO/LWOFileData.h
  AssetLib/LWO/LWOLoader.cpp
  AssetLib/LWO/LWOLoader.h
  AssetLib/LWO/LWOMaterial.cpp
)

ADD_ASSIMP_IMPORTER( LWS
  AssetLib/LWS/LWSLoader.cpp
  AssetLib/LWS/LWSLoader.h
)

ADD_ASSIMP_IMPORTER( M3D
  AssetLib/M3D/M3DMaterials.h
  AssetLib/M3D/M3DImporter.h
  AssetLib/M3D/M3DImporter.cpp
  AssetLib/M3D/M3DWrapper.h
  AssetLib/M3D/M3DWrapper.cpp
  AssetLib/M3D/m3d.h
)

ADD_ASSIMP_IMPORTER( MD2
  AssetLib/MD2/MD2FileData.h
  AssetLib/MD2/MD2Loader.cpp
  AssetLib/MD2/MD2Loader.h
  AssetLib/MD2/MD2NormalTable.h
)

ADD_ASSIMP_IMPORTER( MD3
  AssetLib/MD3/MD3FileData.h
  AssetLib/MD3/MD3Loader.cpp
  AssetLib/MD3/MD3Loader.h
)

ADD_ASSIMP_IMPORTER( MD5
  AssetLib/MD5/MD5Loader.cpp
  AssetLib/MD5/MD5Loader.h
  AssetLib/MD5/MD5Parser.cpp
  AssetLib/MD5/MD5Parser.h
)

ADD_ASSIMP_IMPORTER( MDC
  AssetLib/MDC/MDCFileData.h
  AssetLib/MDC/MDCLoader.cpp
  AssetLib/MDC/MDCLoader.h
  AssetLib/MDC/MDCNormalTable.h
)

ADD_ASSIMP_IMPORTER( MDL
  AssetLib/MDL/MDLDefaultColorMap.h
  AssetLib/MDL/MDLFileData.h
  AssetLib/MDL/MDLLoader.cpp
  AssetLib/MDL/MDLLoader.h
  AssetLib/MDL/MDLMaterialLoader.cpp
  AssetLib/MDL/HalfLife/HalfLifeMDLBaseHeader.h
  AssetLib/MDL/HalfLife/HL1FileData.h
  AssetLib/MDL/HalfLife/HL1MDLLoader.cpp
  AssetLib/MDL/HalfLife/HL1MDLLoader.h
  AssetLib/MDL/HalfLife/HL1ImportDefinitions.h
  AssetLib/MDL/HalfLife/HL1ImportSettings.h
  AssetLib/MDL/HalfLife/HL1MeshTrivert.h
  AssetLib/MDL/HalfLife/LogFunctions.h
  AssetLib/MDL/HalfLife/UniqueNameGenerator.cpp
  AssetLib/MDL/HalfLife/UniqueNameGenerator.h
)

IF(((CMAKE_CXX_COMPILER_ID MATCHES "GNU") AND NOT MINGW AND NOT HAIKU) AND CMAKE_CXX_COMPILER_VERSION GREATER_EQUAL 13)
  message(STATUS "GCC13 detected disabling \"-Warray-bounds and -Wstringop-overflow\" for"
      " AssetLib/MDL/MDLLoader.cpp as it appears to be a false positive")
  set_source_files_properties(AssetLib/MDL/MDLLoader.cpp PROPERTIES
      COMPILE_FLAGS "-Wno-array-bounds -Wno-stringop-overflow"
  )
endif()

SET( MaterialSystem_SRCS
  Material/MaterialSystem.cpp
  Material/MaterialSystem.h
)
SOURCE_GROUP( MaterialSystem FILES ${MaterialSystem_SRCS})

ADD_ASSIMP_IMPORTER( NFF
  AssetLib/NFF/NFFLoader.cpp
  AssetLib/NFF/NFFLoader.h
)

ADD_ASSIMP_IMPORTER( NDO
  AssetLib/NDO/NDOLoader.cpp
  AssetLib/NDO/NDOLoader.h
)

ADD_ASSIMP_IMPORTER( OFF
  AssetLib/OFF/OFFLoader.cpp
  AssetLib/OFF/OFFLoader.h
)

ADD_ASSIMP_IMPORTER( OBJ
  AssetLib/Obj/ObjFileData.h
  AssetLib/Obj/ObjFileImporter.cpp
  AssetLib/Obj/ObjFileImporter.h
  AssetLib/Obj/ObjFileMtlImporter.cpp
  AssetLib/Obj/ObjFileMtlImporter.h
  AssetLib/Obj/ObjFileParser.cpp
  AssetLib/Obj/ObjFileParser.h
  AssetLib/Obj/ObjTools.h
)

IF(((CMAKE_CXX_COMPILER_ID MATCHES "GNU") AND NOT MINGW AND NOT HAIKU) AND CMAKE_CXX_COMPILER_VERSION EQUAL 14)
  message(STATUS "GCC14 detected disabling \"-Wmaybe-uninitialized\" for"
      " AssetLib/Obj/ObjFileParser.cpp as it appears to be a false positive")
  set_source_files_properties(AssetLib/Obj/ObjFileParser.cpp PROPERTIES
      COMPILE_FLAGS "-Wno-maybe-uninitialized"
  )
endif()

ADD_ASSIMP_IMPORTER( OGRE
  AssetLib/Ogre/OgreImporter.h
  AssetLib/Ogre/OgreStructs.h
  AssetLib/Ogre/OgreParsingUtils.h
  AssetLib/Ogre/OgreBinarySerializer.h
  AssetLib/Ogre/OgreXmlSerializer.h
  AssetLib/Ogre/OgreImporter.cpp
  AssetLib/Ogre/OgreStructs.cpp
  AssetLib/Ogre/OgreBinarySerializer.cpp
  AssetLib/Ogre/OgreXmlSerializer.cpp
  AssetLib/Ogre/OgreMaterial.cpp
)

ADD_ASSIMP_IMPORTER( OPENGEX
  AssetLib/OpenGEX/OpenGEXImporter.cpp
  AssetLib/OpenGEX/OpenGEXImporter.h
  AssetLib/OpenGEX/OpenGEXStructs.h
)

ADD_ASSIMP_IMPORTER( PLY
  AssetLib/Ply/PlyLoader.cpp
  AssetLib/Ply/PlyLoader.h
  AssetLib/Ply/PlyParser.cpp
  AssetLib/Ply/PlyParser.h
)

ADD_ASSIMP_IMPORTER( MS3D
  AssetLib/MS3D/MS3DLoader.cpp
  AssetLib/MS3D/MS3DLoader.h
)

ADD_ASSIMP_IMPORTER( COB
  AssetLib/COB/COBLoader.cpp
  AssetLib/COB/COBLoader.h
  AssetLib/COB/COBScene.h
)

ADD_ASSIMP_IMPORTER( BLEND
  AssetLib/Blender/BlenderLoader.cpp
  AssetLib/Blender/BlenderLoader.h
  AssetLib/Blender/BlenderDNA.cpp
  AssetLib/Blender/BlenderDNA.h
  AssetLib/Blender/BlenderDNA.inl
  AssetLib/Blender/BlenderScene.cpp
  AssetLib/Blender/BlenderScene.h
  AssetLib/Blender/BlenderSceneGen.h
  AssetLib/Blender/BlenderIntermediate.h
  AssetLib/Blender/BlenderModifier.h
  AssetLib/Blender/BlenderModifier.cpp
  AssetLib/Blender/BlenderBMesh.h
  AssetLib/Blender/BlenderBMesh.cpp
  AssetLib/Blender/BlenderTessellator.h
  AssetLib/Blender/BlenderTessellator.cpp
  AssetLib/Blender/BlenderCustomData.h
  AssetLib/Blender/BlenderCustomData.cpp
)

ADD_ASSIMP_IMPORTER( IFC
  AssetLib/IFC/IFCLoader.cpp
  AssetLib/IFC/IFCLoader.h
  AssetLib/IFC/IFCReaderGen1_2x3.cpp
  AssetLib/IFC/IFCReaderGen2_2x3.cpp
  AssetLib/IFC/IFCReaderGen_2x3.h
  AssetLib/IFC/IFCUtil.h
  AssetLib/IFC/IFCUtil.cpp
  AssetLib/IFC/IFCGeometry.cpp
  AssetLib/IFC/IFCMaterial.cpp
  AssetLib/IFC/IFCProfile.cpp
  AssetLib/IFC/IFCCurve.cpp
  AssetLib/IFC/IFCBoolean.cpp
  AssetLib/IFC/IFCOpenings.cpp
)

if (ASSIMP_BUILD_IFC_IMPORTER)
  if (MSVC)
    set_source_files_properties(Importer/IFC/IFCReaderGen1_2x3.cpp Importer/IFC/IFCReaderGen2_2x3.cpp PROPERTIES COMPILE_FLAGS "/bigobj")
  elseif(MINGW)
    set_source_files_properties(Importer/IFC/IFCReaderGen1_2x3.cpp Importer/IFC/IFCReaderGen2_2x3.cpp PROPERTIES COMPILE_FLAGS "-O2 -Wa,-mbig-obj")
  endif()
endif ()

ADD_ASSIMP_IMPORTER( XGL
  AssetLib/XGL/XGLLoader.cpp
  AssetLib/XGL/XGLLoader.h
)

ADD_ASSIMP_IMPORTER( FBX
  AssetLib/FBX/FBXImporter.cpp
  AssetLib/FBX/FBXCompileConfig.h
  AssetLib/FBX/FBXImporter.h
  AssetLib/FBX/FBXParser.cpp
  AssetLib/FBX/FBXParser.h
  AssetLib/FBX/FBXTokenizer.cpp
  AssetLib/FBX/FBXTokenizer.h
  AssetLib/FBX/FBXImportSettings.h
  AssetLib/FBX/FBXConverter.h
  AssetLib/FBX/FBXConverter.cpp
  AssetLib/FBX/FBXUtil.h
  AssetLib/FBX/FBXUtil.cpp
  AssetLib/FBX/FBXDocument.h
  AssetLib/FBX/FBXDocument.cpp
  AssetLib/FBX/FBXProperties.h
  AssetLib/FBX/FBXProperties.cpp
  AssetLib/FBX/FBXMeshGeometry.h
  AssetLib/FBX/FBXMeshGeometry.cpp
  AssetLib/FBX/FBXMaterial.cpp
  AssetLib/FBX/FBXModel.cpp
  AssetLib/FBX/FBXAnimation.cpp
  AssetLib/FBX/FBXNodeAttribute.cpp
  AssetLib/FBX/FBXDeformer.cpp
  AssetLib/FBX/FBXBinaryTokenizer.cpp
  AssetLib/FBX/FBXDocumentUtil.cpp
  AssetLib/FBX/FBXCommon.h
)

if (NOT ASSIMP_NO_EXPORT)

  ADD_ASSIMP_EXPORTER( OBJ
    AssetLib/Obj/ObjExporter.h
    AssetLib/Obj/ObjExporter.cpp)

  ADD_ASSIMP_EXPORTER( OPENGEX
    AssetLib/OpenGEX/OpenGEXExporter.cpp
    AssetLib/OpenGEX/OpenGEXExporter.h)

  ADD_ASSIMP_EXPORTER( PLY
    AssetLib/Ply/PlyExporter.cpp
    AssetLib/Ply/PlyExporter.h)

  ADD_ASSIMP_EXPORTER( 3DS
    AssetLib/3DS/3DSExporter.h
    AssetLib/3DS/3DSExporter.cpp)

  ADD_ASSIMP_EXPORTER( ASSBIN
    AssetLib/Assbin/AssbinExporter.h
    AssetLib/Assbin/AssbinExporter.cpp
    AssetLib/Assbin/AssbinFileWriter.h
    AssetLib/Assbin/AssbinFileWriter.cpp)

  ADD_ASSIMP_EXPORTER( ASSXML
    AssetLib/Assxml/AssxmlExporter.h
    AssetLib/Assxml/AssxmlExporter.cpp
    AssetLib/Assxml/AssxmlFileWriter.h
    AssetLib/Assxml/AssxmlFileWriter.cpp)

  ADD_ASSIMP_EXPORTER(M3D
    AssetLib/M3D/M3DExporter.h
    AssetLib/M3D/M3DExporter.cpp)

  ADD_ASSIMP_EXPORTER(COLLADA
    AssetLib/Collada/ColladaExporter.h
    AssetLib/Collada/ColladaExporter.cpp)

  ADD_ASSIMP_EXPORTER( FBX
    AssetLib/FBX/FBXExporter.h
    AssetLib/FBX/FBXExporter.cpp
    AssetLib/FBX/FBXExportNode.h
    AssetLib/FBX/FBXExportNode.cpp
    AssetLib/FBX/FBXExportProperty.h
    AssetLib/FBX/FBXExportProperty.cpp)

  ADD_ASSIMP_EXPORTER( STL
    AssetLib/STL/STLExporter.h
    AssetLib/STL/STLExporter.cpp)

  ADD_ASSIMP_EXPORTER( X
    AssetLib/X/XFileExporter.h
    AssetLib/X/XFileExporter.cpp)

  ADD_ASSIMP_EXPORTER( X3D
    AssetLib/X3D/X3DExporter.cpp
    AssetLib/X3D/X3DExporter.hpp)

  ADD_ASSIMP_EXPORTER( GLTF
    AssetLib/glTF/glTFExporter.h
    AssetLib/glTF/glTFExporter.cpp
    AssetLib/glTF2/glTF2Exporter.h
    AssetLib/glTF2/glTF2Exporter.cpp)

  ADD_ASSIMP_EXPORTER( 3MF
    AssetLib/3MF/D3MFExporter.h
    AssetLib/3MF/D3MFExporter.cpp)

  ADD_ASSIMP_EXPORTER( PBRT
    Pbrt/PbrtExporter.h
    Pbrt/PbrtExporter.cpp)

  ADD_ASSIMP_EXPORTER( ASSJSON
    AssetLib/Assjson/cencode.c
    AssetLib/Assjson/cencode.h
    AssetLib/Assjson/json_exporter.cpp
    AssetLib/Assjson/mesh_splitter.cpp
    AssetLib/Assjson/mesh_splitter.h)

  ADD_ASSIMP_EXPORTER( STEP
    AssetLib/Step/StepExporter.h
    AssetLib/Step/StepExporter.cpp)

endif()

SET( PostProcessing_SRCS
  PostProcessing/CalcTangentsProcess.cpp
  PostProcessing/CalcTangentsProcess.h
  PostProcessing/ComputeUVMappingProcess.cpp
  PostProcessing/ComputeUVMappingProcess.h
  PostProcessing/ConvertToLHProcess.cpp
  PostProcessing/ConvertToLHProcess.h
  PostProcessing/EmbedTexturesProcess.cpp
  PostProcessing/EmbedTexturesProcess.h
  PostProcessing/FindDegenerates.cpp
  PostProcessing/FindDegenerates.h
  PostProcessing/FindInstancesProcess.cpp
  PostProcessing/FindInstancesProcess.h
  PostProcessing/FindInvalidDataProcess.cpp
  PostProcessing/FindInvalidDataProcess.h
  PostProcessing/FixNormalsStep.cpp
  PostProcessing/FixNormalsStep.h
  PostProcessing/DropFaceNormalsProcess.cpp
  PostProcessing/DropFaceNormalsProcess.h
  PostProcessing/GenFaceNormalsProcess.cpp
  PostProcessing/GenFaceNormalsProcess.h
  PostProcessing/GenVertexNormalsProcess.cpp
  PostProcessing/GenVertexNormalsProcess.h
  PostProcessing/PretransformVertices.cpp
  PostProcessing/PretransformVertices.h
  PostProcessing/ImproveCacheLocality.cpp
  PostProcessing/ImproveCacheLocality.h
  PostProcessing/JoinVerticesProcess.cpp
  PostProcessing/JoinVerticesProcess.h
  PostProcessing/LimitBoneWeightsProcess.cpp
  PostProcessing/LimitBoneWeightsProcess.h
  PostProcessing/RemoveRedundantMaterials.cpp
  PostProcessing/RemoveRedundantMaterials.h
  PostProcessing/RemoveVCProcess.cpp
  PostProcessing/RemoveVCProcess.h
  PostProcessing/SortByPTypeProcess.cpp
  PostProcessing/SortByPTypeProcess.h
  PostProcessing/SplitLargeMeshes.cpp
  PostProcessing/SplitLargeMeshes.h
  PostProcessing/TextureTransform.cpp
  PostProcessing/TextureTransform.h
  PostProcessing/TriangulateProcess.cpp
  PostProcessing/TriangulateProcess.h
  PostProcessing/ValidateDataStructure.cpp
  PostProcessing/ValidateDataStructure.h
  PostProcessing/OptimizeGraph.cpp
  PostProcessing/OptimizeGraph.h
  PostProcessing/OptimizeMeshes.cpp
  PostProcessing/OptimizeMeshes.h
  PostProcessing/DeboneProcess.cpp
  PostProcessing/DeboneProcess.h
  PostProcessing/ProcessHelper.h
  PostProcessing/ProcessHelper.cpp
  PostProcessing/MakeVerboseFormat.cpp
  PostProcessing/MakeVerboseFormat.h
  PostProcessing/ScaleProcess.cpp
  PostProcessing/ScaleProcess.h
  PostProcessing/ArmaturePopulate.cpp
  PostProcessing/ArmaturePopulate.h
  PostProcessing/GenBoundingBoxesProcess.cpp
  PostProcessing/GenBoundingBoxesProcess.h
  PostProcessing/SplitByBoneCountProcess.cpp
  PostProcessing/SplitByBoneCountProcess.h
)
SOURCE_GROUP( PostProcessing FILES ${PostProcessing_SRCS})

ADD_ASSIMP_IMPORTER( Q3D
  AssetLib/Q3D/Q3DLoader.cpp
  AssetLib/Q3D/Q3DLoader.h
)

ADD_ASSIMP_IMPORTER( Q3BSP
  AssetLib/Q3BSP/Q3BSPFileData.h
  AssetLib/Q3BSP/Q3BSPFileParser.h
  AssetLib/Q3BSP/Q3BSPFileParser.cpp
  AssetLib/Q3BSP/Q3BSPFileImporter.h
  AssetLib/Q3BSP/Q3BSPFileImporter.cpp
)

ADD_ASSIMP_IMPORTER( RAW
  AssetLib/Raw/RawLoader.cpp
  AssetLib/Raw/RawLoader.h
)

ADD_ASSIMP_IMPORTER( SIB
  AssetLib/SIB/SIBImporter.cpp
  AssetLib/SIB/SIBImporter.h
)

ADD_ASSIMP_IMPORTER( SMD
  AssetLib/SMD/SMDLoader.cpp
  AssetLib/SMD/SMDLoader.h
)

ADD_ASSIMP_IMPORTER( STL
  AssetLib/STL/STLLoader.cpp
  AssetLib/STL/STLLoader.h
)

ADD_ASSIMP_IMPORTER( TERRAGEN
  AssetLib/Terragen/TerragenLoader.cpp
  AssetLib/Terragen/TerragenLoader.h
)

ADD_ASSIMP_IMPORTER( 3D
  AssetLib/Unreal/UnrealLoader.cpp
  AssetLib/Unreal/UnrealLoader.h
)

ADD_ASSIMP_IMPORTER( X
  AssetLib/X/XFileHelper.h
  AssetLib/X/XFileImporter.cpp
  AssetLib/X/XFileImporter.h
  AssetLib/X/XFileParser.cpp
  AssetLib/X/XFileParser.h
)

ADD_ASSIMP_IMPORTER( X3D
  AssetLib/X3D/X3DImporter.cpp
  AssetLib/X3D/X3DImporter_Geometry2D.cpp
  AssetLib/X3D/X3DImporter_Geometry3D.cpp
  AssetLib/X3D/X3DImporter_Group.cpp
  AssetLib/X3D/X3DImporter_Light.cpp
  AssetLib/X3D/X3DImporter_Metadata.cpp
  AssetLib/X3D/X3DImporter_Networking.cpp
  AssetLib/X3D/X3DImporter_Postprocess.cpp
  AssetLib/X3D/X3DImporter_Rendering.cpp
  AssetLib/X3D/X3DImporter_Shape.cpp
  AssetLib/X3D/X3DImporter_Texturing.cpp
  AssetLib/X3D/X3DImporter.hpp
  AssetLib/X3D/X3DImporter_Macro.hpp
  AssetLib/X3D/X3DImporter_Node.hpp
  AssetLib/X3D/X3DGeoHelper.cpp
  AssetLib/X3D/X3DGeoHelper.h
  AssetLib/X3D/X3DXmlHelper.cpp
  AssetLib/X3D/X3DXmlHelper.h
)

ADD_ASSIMP_IMPORTER( GLTF
  AssetLib/glTF/glTFCommon.h
  AssetLib/glTF/glTFCommon.cpp
  AssetLib/glTF/glTFAsset.h
  AssetLib/glTF/glTFAsset.inl
  AssetLib/glTF/glTFAssetWriter.h
  AssetLib/glTF/glTFAssetWriter.inl
  AssetLib/glTF/glTFImporter.cpp
  AssetLib/glTF/glTFImporter.h
  AssetLib/glTF2/glTF2Asset.h
  AssetLib/glTF2/glTF2Asset.inl
  AssetLib/glTF2/glTF2AssetWriter.h
  AssetLib/glTF2/glTF2AssetWriter.inl
  AssetLib/glTF2/glTF2Importer.cpp
  AssetLib/glTF2/glTF2Importer.h
)

ADD_ASSIMP_IMPORTER(3MF
  AssetLib/3MF/3MFTypes.h
  AssetLib/3MF/XmlSerializer.h
  AssetLib/3MF/XmlSerializer.cpp
  AssetLib/3MF/D3MFImporter.h
  AssetLib/3MF/D3MFImporter.cpp
  AssetLib/3MF/D3MFOpcPackage.h
  AssetLib/3MF/D3MFOpcPackage.cpp
  AssetLib/3MF/3MFXmlTags.h
)

ADD_ASSIMP_IMPORTER( MMD
  AssetLib/MMD/MMDCpp14.h
  AssetLib/MMD/MMDImporter.cpp
  AssetLib/MMD/MMDImporter.h
  AssetLib/MMD/MMDPmdParser.h
  AssetLib/MMD/MMDPmxParser.h
  AssetLib/MMD/MMDPmxParser.cpp
  AssetLib/MMD/MMDVmdParser.h
)

# Workaround for issue #2406 - force problematic large file to be optimized to prevent string table overflow error
# Used -Os instead of -O2 as previous issues had mentioned, since -Os is roughly speaking -O2, excluding any
# optimizations that take up extra space. Given that the issue is a string table overflowing, -Os seemed appropriate
# Also, I'm not positive if both link & compile flags are needed, but this hopefully ensures that the issue should not
# recur for edge cases such as static builds.
if ((MINGW) AND (CMAKE_BUILD_TYPE MATCHES Debug))
  message("-- Applying MinGW StepFileGen1.cpp Debug Workaround")
  SET_SOURCE_FILES_PROPERTIES(Importer/StepFile/StepFileGen1.cpp PROPERTIES COMPILE_FLAGS -Os )
  SET_SOURCE_FILES_PROPERTIES(Importer/StepFile/StepFileGen1.cpp PROPERTIES LINK_FLAGS -Os )
  SET_SOURCE_FILES_PROPERTIES(Importer/StepFile/StepFileGen1.cpp PROPERTIES STATIC_LIBRARY_FLAGS -Os )
endif()

if ((NOT ASSIMP_NO_EXPORT) OR (NOT ASSIMP_EXPORTERS_ENABLED STREQUAL ""))
	SET( Exporter_SRCS
	  Common/Exporter.cpp
	  CApi/AssimpCExport.cpp
	  ${HEADER_PATH}/BlobIOSystem.h
	)
	SOURCE_GROUP( Exporter FILES ${Exporter_SRCS})
endif()

SET( Extra_SRCS
  MD4FileData.h
)
SOURCE_GROUP( Extra FILES ${Extra_SRCS})

# pugixml
IF(ASSIMP_HUNTER_ENABLED)
  hunter_add_package(pugixml)
  find_package(pugixml CONFIG REQUIRED)
ELSE()
  SET( Pugixml_SRCS
    ../contrib/pugixml/src/pugiconfig.hpp
    ../contrib/pugixml/src/pugixml.hpp
  )
  INCLUDE_DIRECTORIES("../contrib/pugixml/src")
  SOURCE_GROUP( Contrib\\Pugixml FILES ${Pugixml_SRCS})
ENDIF()

# utf8
IF(ASSIMP_HUNTER_ENABLED)
  hunter_add_package(utf8)
  find_package(utf8cpp CONFIG REQUIRED)
ELSE()
  INCLUDE_DIRECTORIES("../contrib/utf8cpp/source")
ENDIF()

# polyclipping
#IF(ASSIMP_HUNTER_ENABLED)
#  hunter_add_package(polyclipping)
#  find_package(polyclipping CONFIG REQUIRED)
#ELSE()
  SET( Clipper_SRCS
    ../contrib/clipper/clipper.hpp
    ../contrib/clipper/clipper.cpp
  )
  SOURCE_GROUP( Contrib\\Clipper FILES ${Clipper_SRCS})
#ENDIF()

# poly2tri
#IF(ASSIMP_HUNTER_ENABLED)
#  hunter_add_package(poly2tri)
#  find_package(poly2tri CONFIG REQUIRED)
#ELSE()
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
  SOURCE_GROUP( Contrib\\Poly2Tri FILES ${Poly2Tri_SRCS})
#ENDIF()

# minizip/unzip
IF(ASSIMP_HUNTER_ENABLED)
  hunter_add_package(minizip)
  find_package(minizip CONFIG REQUIRED)
ELSE()
  SET( unzip_SRCS
    ../contrib/unzip/crypt.h
    ../contrib/unzip/ioapi.c
    ../contrib/unzip/ioapi.h
    ../contrib/unzip/unzip.c
    ../contrib/unzip/unzip.h
  )
  SOURCE_GROUP(Contrib\\unzip FILES ${unzip_SRCS})
ENDIF()

# zip (https://github.com/kuba--/zip)
separate_arguments(ASSIMP_EXPORTERS_LIST UNIX_COMMAND ${ASSIMP_EXPORTERS_ENABLED})
IF(3MF IN_LIST ASSIMP_EXPORTERS_LIST)
  IF(ASSIMP_HUNTER_ENABLED)
    hunter_add_package(zip)
    find_package(zip CONFIG REQUIRED)
  ELSE()
    SET( ziplib_SRCS
      ../contrib/zip/src/miniz.h
      ../contrib/zip/src/zip.c
      ../contrib/zip/src/zip.h
    )

    # TODO if cmake required version has been updated to >3.12.0, collapse this to the second case only
    if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
      add_definitions(-DMINIZ_USE_UNALIGNED_LOADS_AND_STORES=0)
    else()
      add_compile_definitions(MINIZ_USE_UNALIGNED_LOADS_AND_STORES=0)
    endif()

    SOURCE_GROUP( ziplib FILES ${ziplib_SRCS} )
  ENDIF()
ENDIF()

# openddlparser
IF(ASSIMP_HUNTER_ENABLED)
  hunter_add_package(openddlparser)
  find_package(openddlparser CONFIG REQUIRED)
ELSE()
  SET ( openddl_parser_SRCS
    ../contrib/openddlparser/code/OpenDDLParser.cpp
    ../contrib/openddlparser/code/DDLNode.cpp
    ../contrib/openddlparser/code/OpenDDLCommon.cpp
    ../contrib/openddlparser/code/OpenDDLExport.cpp
    ../contrib/openddlparser/code/Value.cpp
    ../contrib/openddlparser/code/OpenDDLStream.cpp
    ../contrib/openddlparser/include/openddlparser/OpenDDLParser.h
    ../contrib/openddlparser/include/openddlparser/OpenDDLParserUtils.h
    ../contrib/openddlparser/include/openddlparser/OpenDDLCommon.h
    ../contrib/openddlparser/include/openddlparser/OpenDDLExport.h
    ../contrib/openddlparser/include/openddlparser/OpenDDLStream.h
    ../contrib/openddlparser/include/openddlparser/DDLNode.h
    ../contrib/openddlparser/include/openddlparser/Value.h
  )
  SOURCE_GROUP( Contrib\\openddl_parser FILES ${openddl_parser_SRCS})
ENDIF()

# Open3DGC
IF(ASSIMP_HUNTER_ENABLED)
  # Nothing to do, not available in Hunter yet.
ELSE()
  SET ( open3dgc_SRCS
    ../contrib/Open3DGC/o3dgcAdjacencyInfo.h
    ../contrib/Open3DGC/o3dgcArithmeticCodec.cpp
    ../contrib/Open3DGC/o3dgcArithmeticCodec.h
    ../contrib/Open3DGC/o3dgcBinaryStream.h
    ../contrib/Open3DGC/o3dgcCommon.h
    ../contrib/Open3DGC/o3dgcDVEncodeParams.h
    ../contrib/Open3DGC/o3dgcDynamicVectorDecoder.cpp
    ../contrib/Open3DGC/o3dgcDynamicVectorDecoder.h
    ../contrib/Open3DGC/o3dgcDynamicVectorEncoder.cpp
    ../contrib/Open3DGC/o3dgcDynamicVectorEncoder.h
    ../contrib/Open3DGC/o3dgcDynamicVector.h
    ../contrib/Open3DGC/o3dgcFIFO.h
    ../contrib/Open3DGC/o3dgcIndexedFaceSet.h
    ../contrib/Open3DGC/o3dgcIndexedFaceSet.inl
    ../contrib/Open3DGC/o3dgcSC3DMCDecoder.h
    ../contrib/Open3DGC/o3dgcSC3DMCDecoder.inl
    ../contrib/Open3DGC/o3dgcSC3DMCEncodeParams.h
    ../contrib/Open3DGC/o3dgcSC3DMCEncoder.h
    ../contrib/Open3DGC/o3dgcSC3DMCEncoder.inl
    ../contrib/Open3DGC/o3dgcTimer.h
    ../contrib/Open3DGC/o3dgcTools.cpp
    ../contrib/Open3DGC/o3dgcTriangleFans.cpp
    ../contrib/Open3DGC/o3dgcTriangleFans.h
    ../contrib/Open3DGC/o3dgcTriangleListDecoder.h
    ../contrib/Open3DGC/o3dgcTriangleListDecoder.inl
    ../contrib/Open3DGC/o3dgcTriangleListEncoder.h
    ../contrib/Open3DGC/o3dgcTriangleListEncoder.inl
    ../contrib/Open3DGC/o3dgcVector.h
    ../contrib/Open3DGC/o3dgcVector.inl
  )
  SOURCE_GROUP( Contrib\\open3dgc FILES ${open3dgc_SRCS})
ENDIF()

# Check dependencies for glTF importer with Open3DGC-compression.
# RT-extensions is used in "contrib/Open3DGC/o3dgcTimer.h" for collecting statistics. Pointed file
# has implementation for different platforms: WIN32, __MACH__ and other ("else" block).
IF (NOT WIN32)
  FIND_PACKAGE(RT QUIET)
ENDIF ()
IF (NOT ASSIMP_HUNTER_ENABLED AND (RT_FOUND OR WIN32))
  SET( ASSIMP_IMPORTER_GLTF_USE_OPEN3DGC 1 )
  ADD_DEFINITIONS( -DASSIMP_IMPORTER_GLTF_USE_OPEN3DGC=1 )
ELSE ()
  SET (open3dgc_SRCS "")
  MESSAGE (INFO " Hunter enabled or RT-extension not found. glTF import/export will be built without Open3DGC-compression.")
  #!TODO: off course is better to remove statistics timers from o3dgc codec. Or propose to choose what to use.
ENDIF ()

# RapidJSON
IF(ASSIMP_HUNTER_ENABLED)
  hunter_add_package(RapidJSON)
  find_package(RapidJSON CONFIG REQUIRED)
ELSE()
  INCLUDE_DIRECTORIES("../contrib/rapidjson/include")
  ADD_DEFINITIONS( -DRAPIDJSON_HAS_STDSTRING=1)
  option( ASSIMP_RAPIDJSON_NO_MEMBER_ITERATOR "Suppress rapidjson warning on MSVC (NOTE: breaks android build)" ON )
  if(ASSIMP_RAPIDJSON_NO_MEMBER_ITERATOR)
    ADD_DEFINITIONS( -DRAPIDJSON_NOMEMBERITERATORCLASS )
  endif()
ENDIF()

# stb
IF(ASSIMP_HUNTER_ENABLED)
  hunter_add_package(stb)
  find_package(stb CONFIG REQUIRED)
ELSE()
  SET( stb_SRCS
    ../contrib/stb/stb_image.h
  )
  INCLUDE_DIRECTORIES("../contrib")
  SOURCE_GROUP( Contrib\\stb FILES ${stb_SRCS})
ENDIF()

# VC2010 fixes
if(MSVC10)
  option( VC10_STDINT_FIX "Fix for VC10 Compiler regarding pstdint.h redefinition errors" OFF )
  if( VC10_STDINT_FIX )
    ADD_DEFINITIONS( -D_STDINT )
  endif()
endif()

IF( MSVC OR "${CMAKE_CXX_SIMULATE_ID}" MATCHES "MSVC") # clang with MSVC ABI
  ADD_DEFINITIONS( -D_SCL_SECURE_NO_WARNINGS )
  ADD_DEFINITIONS( -D_CRT_SECURE_NO_WARNINGS )
endif ()

IF(NOT ASSIMP_HUNTER_ENABLED)
  if (UNZIP_FOUND)
    SET (unzip_compile_SRCS "")
  else ()
    SET (unzip_compile_SRCS ${unzip_SRCS})
    INCLUDE_DIRECTORIES( "../contrib/unzip/" )
  endif ()
ENDIF()

MESSAGE(STATUS "Enabled importer formats:${ASSIMP_IMPORTERS_ENABLED}")
MESSAGE(STATUS "Disabled importer formats:${ASSIMP_IMPORTERS_DISABLED}")

if (NOT ASSIMP_NO_EXPORT)
  MESSAGE(STATUS "Enabled exporter formats:${ASSIMP_EXPORTERS_ENABLED}")
  MESSAGE(STATUS "Disabled exporter formats:${ASSIMP_EXPORTERS_DISABLED}")
endif()

SOURCE_GROUP( include\\assimp    FILES ${PUBLIC_HEADERS} )

SET( assimp_src
  # Assimp Files
  ${Core_SRCS}
  ${CApi_SRCS}
  ${Common_SRCS}
  ${Geometry_SRCS}
  ${Logging_SRCS}
  ${Exporter_SRCS}
  ${PostProcessing_SRCS}
  ${MaterialSystem_SRCS}
  ${STEPParser_SRCS}
#  ${Step_SRCS} check if we need a different approach

  # Model Support
  ${ASSIMP_LOADER_SRCS}
  ${ASSIMP_EXPORTER_SRCS}

  # Third-party libraries
  ${unzip_compile_SRCS}
  ${Poly2Tri_SRCS}
  ${Clipper_SRCS}
  ${openddl_parser_SRCS}
  ${open3dgc_SRCS}
  ${ziplib_SRCS}
  ${Pugixml_SRCS}
  ${stb_SRCS}
  # Necessary to show the headers in the project when using the VC++ generator:

  ${PUBLIC_HEADERS}
  ${COMPILER_HEADERS}
)
ADD_DEFINITIONS( -DOPENDDLPARSER_BUILD )

IF(NOT ASSIMP_HUNTER_ENABLED)
  INCLUDE_DIRECTORIES(
      ${IRRXML_INCLUDE_DIR}
      ../contrib/openddlparser/include
  )
ENDIF()

IF (ASSIMP_BUILD_NONFREE_C4D_IMPORTER)
  SET( assimp_src ${assimp_src} ${C4D_SRCS})
  INCLUDE_DIRECTORIES(${C4D_INCLUDES})
ENDIF ()

IF (ASSIMP_BUILD_DRACO)
  INCLUDE_DIRECTORIES(${draco_INCLUDE_DIRS})
  ADD_DEFINITIONS( -DASSIMP_ENABLE_DRACO )
ENDIF()

ADD_LIBRARY( assimp ${assimp_src} )
ADD_LIBRARY(assimp::assimp ALIAS assimp)

# Add or remove dllexport tags depending on the library type.
IF (BUILD_SHARED_LIBS)
  TARGET_COMPILE_DEFINITIONS(assimp PRIVATE ASSIMP_BUILD_DLL_EXPORT)
ELSE ()
  TARGET_COMPILE_DEFINITIONS(assimp PRIVATE OPENDDL_STATIC_LIBARY)
ENDIF ()

TARGET_USE_COMMON_OUTPUT_DIRECTORY(assimp)

add_compile_options(
  "$<$<CONFIG:DEBUG>:-O0;-g3;-ggdb>"
)

IF (ASSIMP_WARNINGS_AS_ERRORS)
  MESSAGE(STATUS "Treating all warnings as errors (for assimp library only)")
  IF (MSVC)

    IF(CMAKE_CXX_COMPILER_ID MATCHES "Clang" ) # clang-cl
      TARGET_COMPILE_OPTIONS(assimp PRIVATE -Wall -Werror
        -Wno-microsoft-enum-value
        -Wno-switch-enum
        -Wno-covered-switch-default
        -Wno-reserved-identifier
        -Wno-c++98-compat-pedantic
        -Wno-c++98-compat
        -Wno-documentation
        -Wno-documentation-unknown-command
        -Wno-deprecated-dynamic-exception-spec
        -Wno-undef
        -Wno-suggest-destructor-override
        -Wno-suggest-override
        -Wno-zero-as-null-pointer-constant
        -Wno-global-constructors
        -Wno-exit-time-destructors
        -Wno-extra-semi-stmt
        -Wno-missing-prototypes
        -Wno-old-style-cast
        -Wno-cast-align
        -Wno-cast-qual
        -Wno-float-equal
        -Wno-implicit-int-float-conversion
        -Wno-sign-conversion
        -Wno-implicit-float-conversion
        -Wno-implicit-int-conversion
        -Wno-float-conversion
        -Wno-double-promotion
        -Wno-unused-macros
        -Wno-disabled-macro-expansion
        -Wno-shadow-field
        -Wno-shadow
        -Wno-language-extension-token
        -Wno-header-hygiene
        -Wno-tautological-value-range-compare
        -Wno-tautological-type-limit-compare
        -Wno-missing-variable-declarations
        -Wno-extra-semi
        -Wno-nonportable-system-include-path
        -Wno-undefined-reinterpret-cast
        -Wno-shift-sign-overflow
        -Wno-deprecated
        -Wno-format-nonliteral
        -Wno-comma
        -Wno-implicit-fallthrough
        -Wno-unused-template
        -Wno-undefined-func-template
        -Wno-declaration-after-statement
      )
    ELSE()
      TARGET_COMPILE_OPTIONS(assimp PRIVATE /W4 /WX)
    ENDIF()
  ELSE()
    TARGET_COMPILE_OPTIONS(assimp PRIVATE -Wall -Werror)
  ENDIF()
ENDIF()

# adds C_FLAGS required to compile zip.c on old GCC 4.x compiler
TARGET_COMPILE_FEATURES(assimp PRIVATE c_std_99)

TARGET_INCLUDE_DIRECTORIES ( assimp PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>
  $<INSTALL_INTERFACE:${ASSIMP_INCLUDE_INSTALL_DIR}>
)

IF(ASSIMP_HUNTER_ENABLED)
  TARGET_LINK_LIBRARIES(assimp
      PUBLIC
      #polyclipping::polyclipping
      openddlparser::openddl_parser
      #poly2tri::poly2tri
      minizip::minizip
      ZLIB::zlib
      RapidJSON::rapidjson
      utf8cpp
      pugixml
      stb::stb
  )
  if(TARGET zip::zip)
    target_link_libraries(assimp PUBLIC zip::zip)
  endif()

  if (ASSIMP_BUILD_DRACO)
    target_link_libraries(assimp PRIVATE ${draco_LIBRARIES})
  endif()
ELSE()
  TARGET_LINK_LIBRARIES(assimp ${ZLIB_LIBRARIES} ${OPENDDL_PARSER_LIBRARIES})
  if (ASSIMP_BUILD_DRACO)
    target_link_libraries(assimp ${draco_LIBRARIES})
  endif()
ENDIF()

if(ASSIMP_ANDROID_JNIIOSYSTEM)
  set(ASSIMP_ANDROID_JNIIOSYSTEM_PATH port/AndroidJNI)
  add_subdirectory(../${ASSIMP_ANDROID_JNIIOSYSTEM_PATH}/ ../${ASSIMP_ANDROID_JNIIOSYSTEM_PATH}/)
  target_link_libraries(assimp android_jniiosystem)
endif()

IF (ASSIMP_BUILD_NONFREE_C4D_IMPORTER)
  TARGET_LINK_LIBRARIES(assimp optimized ${C4D_RELEASE_LIBRARIES})
  TARGET_LINK_LIBRARIES(assimp debug ${C4D_DEBUG_LIBRARIES})
  TARGET_LINK_LIBRARIES(assimp ${C4D_EXTRA_LIBRARIES})
ENDIF ()

if( MSVC )
  # in order to prevent DLL hell, each of the DLLs have to be suffixed with the major version and msvc prefix
  # CMake 3.12 added a variable for this
  if(MSVC_TOOLSET_VERSION)
    set(MSVC_PREFIX "vc${MSVC_TOOLSET_VERSION}")
  else()
    if( MSVC70 OR MSVC71 )
      set(MSVC_PREFIX "vc70")
    elseif( MSVC80 )
      set(MSVC_PREFIX "vc80")
    elseif( MSVC90 )
      set(MSVC_PREFIX "vc90")
    elseif( MSVC10 )
      set(MSVC_PREFIX "vc100")
    elseif( MSVC11 )
      set(MSVC_PREFIX "vc110")
    elseif( MSVC12 )
      set(MSVC_PREFIX "vc120")
    elseif( MSVC_VERSION LESS 1910)
      set(MSVC_PREFIX "vc140")
    elseif( MSVC_VERSION LESS 1920)
      set(MSVC_PREFIX "vc141")
    elseif( MSVC_VERSION LESS 1930)
      set(MSVC_PREFIX "vc142")
    else()
      MESSAGE(WARNING "unknown msvc version ${MSVC_VERSION}")
      set(MSVC_PREFIX "vc150")
    endif()
  endif()
  set(LIBRARY_SUFFIX "${ASSIMP_LIBRARY_SUFFIX}-${MSVC_PREFIX}-mt" CACHE STRING "the suffix for the assimp windows library")
endif()

if (MINGW)
  set(LIBRARY_SUFFIX "-${ASSIMP_SOVERSION}" CACHE STRING "the suffix for the assimp MinGW shared library")
  SET_TARGET_PROPERTIES( assimp PROPERTIES
    ARCHIVE_OUTPUT_NAME assimp
  )
  if (NOT BUILD_SHARED_LIBS)
    TARGET_LINK_LIBRARIES ( assimp -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lwinpthread ) # winpthread is for libminizip.
  endif ()
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "WindowsStore")
    target_compile_definitions(assimp PUBLIC WindowsStore)
    TARGET_LINK_LIBRARIES(assimp advapi32)
endif()

SET_TARGET_PROPERTIES( assimp PROPERTIES
  VERSION ${ASSIMP_VERSION}
  SOVERSION ${ASSIMP_SOVERSION} # use full version
  OUTPUT_NAME assimp${LIBRARY_SUFFIX}
)

if (APPLE)
  if (ASSIMP_BUILD_FRAMEWORK)
    SET_TARGET_PROPERTIES( assimp PROPERTIES
      FRAMEWORK TRUE
      FRAMEWORK_VERSION C
      MACOSX_FRAMEWORK_IDENTIFIER net.sf.assimp
      PUBLIC_HEADER "${PUBLIC_HEADERS}"
    )

    # PUBLIC_HEADER option does not support directory structure creation
    # add ./Compiler/*.h to assimp.framework via copy command
    ADD_CUSTOM_COMMAND(TARGET assimp POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy_directory
         "${HEADER_PATH}/Compiler"
         assimp.framework/Headers/Compiler
      COMMENT "Copying public ./Compiler/ header files to framework bundle's Headers/Compiler/")
  ENDIF()
ENDIF()

# Build against external unzip, or add ../contrib/unzip so
# assimp can #include "unzip.h"
IF(NOT ASSIMP_HUNTER_ENABLED)
  if (UNZIP_FOUND)
    INCLUDE_DIRECTORIES(${UNZIP_INCLUDE_DIRS})
    # TODO if cmake required version has been updated to >3.12.0, collapse this to the second case only
    if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
      TARGET_LINK_LIBRARIES(assimp ${UNZIP_LIBRARIES})
    else()
      TARGET_LINK_LIBRARIES(assimp ${UNZIP_LINK_LIBRARIES})
    endif()
  else ()
    INCLUDE_DIRECTORIES("../")
  endif ()
ENDIF()

# Add RT-extension library for glTF importer with Open3DGC-compression.
IF (RT_FOUND AND ASSIMP_IMPORTER_GLTF_USE_OPEN3DGC)
  TARGET_LINK_LIBRARIES(assimp rt)
ENDIF ()

IF(ASSIMP_INSTALL)
  INSTALL( TARGETS assimp
    EXPORT "${TARGETS_EXPORT_NAME}"
    LIBRARY DESTINATION ${ASSIMP_LIB_INSTALL_DIR} COMPONENT ${LIBASSIMP_COMPONENT}
    ARCHIVE DESTINATION ${ASSIMP_LIB_INSTALL_DIR} COMPONENT ${LIBASSIMP-DEV_COMPONENT}
    RUNTIME DESTINATION ${ASSIMP_BIN_INSTALL_DIR} COMPONENT ${LIBASSIMP_COMPONENT}
    FRAMEWORK DESTINATION ${ASSIMP_LIB_INSTALL_DIR} COMPONENT ${LIBASSIMP_COMPONENT}
    INCLUDES DESTINATION ${ASSIMP_INCLUDE_INSTALL_DIR}
  )
  INSTALL( FILES ${PUBLIC_HEADERS} DESTINATION ${ASSIMP_INCLUDE_INSTALL_DIR}/assimp COMPONENT assimp-dev)
  INSTALL( FILES ${COMPILER_HEADERS} DESTINATION ${ASSIMP_INCLUDE_INSTALL_DIR}/assimp/Compiler COMPONENT assimp-dev)
ENDIF()

if (ASSIMP_ANDROID_JNIIOSYSTEM AND ASSIMP_INSTALL)
  INSTALL(FILES ${HEADER_PATH}/${ASSIMP_ANDROID_JNIIOSYSTEM_PATH}/AndroidJNIIOSystem.h
    DESTINATION ${ASSIMP_INCLUDE_INSTALL_DIR}
    COMPONENT assimp-dev)
ENDIF()

if(MSVC AND ASSIMP_INSTALL_PDB)
  # When only the static library is built, these properties must
  # be set to ensure the static lib .pdb is staged for installation.
  IF(NOT BUILD_SHARED_LIBS)
    SET_TARGET_PROPERTIES( assimp PROPERTIES
      COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMPILE_PDB_NAME assimp${LIBRARY_SUFFIX}
      COMPILE_PDB_NAME_DEBUG assimp${LIBRARY_SUFFIX}${CMAKE_DEBUG_POSTFIX}
    )

    IF(GENERATOR_IS_MULTI_CONFIG)
      install(FILES ${Assimp_BINARY_DIR}/code/Debug/assimp${LIBRARY_SUFFIX}${CMAKE_DEBUG_POSTFIX}.pdb
        DESTINATION ${ASSIMP_LIB_INSTALL_DIR}
        CONFIGURATIONS Debug
      )
      install(FILES ${Assimp_BINARY_DIR}/code/RelWithDebInfo/assimp${LIBRARY_SUFFIX}.pdb
        DESTINATION ${ASSIMP_LIB_INSTALL_DIR}
        CONFIGURATIONS RelWithDebInfo
      )
    ELSE()
      install(FILES ${Assimp_BINARY_DIR}/code/assimp${LIBRARY_SUFFIX}${CMAKE_DEBUG_POSTFIX}.pdb
        DESTINATION ${ASSIMP_LIB_INSTALL_DIR}
        CONFIGURATIONS Debug
      )
      install(FILES ${Assimp_BINARY_DIR}/code/assimp${LIBRARY_SUFFIX}.pdb
        DESTINATION ${ASSIMP_LIB_INSTALL_DIR}
        CONFIGURATIONS RelWithDebInfo
      )
    ENDIF()
  ELSE()
    install(FILES $<TARGET_PDB_FILE:assimp>
      DESTINATION ${ASSIMP_LIB_INSTALL_DIR}
    )
  ENDIF()
ENDIF ()

if (ASSIMP_COVERALLS)
    include(Coveralls)

    set(COVERAGE_SRCS ${assimp_src} ${TEST_SRCS} )

    # Create the coveralls target.
    coveralls_setup(
        "${COVERAGE_SRCS}" # The source files.
        ON                 # If we should upload.
        "${PROJECT_SOURCE_DIR}/cmake-modules/") # (Optional) Alternate project cmake module path.
ENDIF()
