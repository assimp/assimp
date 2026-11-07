This is a set of Delphi units for using the Assimp C DLL. They bind the whole of the C API: every
struct, enum and function declared in the public C headers, and the AI_CONFIG_XXX property names.

There is one unit for each C header:

  aiDefs          defs.h (also holds ASSIMP_DLL, the name of the DLL)
  aiTypes         types.h
  aiVector2D      vector2.h
  aiVector3D      vector3.h
  aiColor4D       color4.h
  aiMatrix3x3     matrix3x3.h
  aiMatrix4x4     matrix4x4.h
  aiQuaternion    quaternion.h
  aiAABB          aabb.h
  aiMesh          mesh.h
  aiMaterial      material.h
  aiTexture       texture.h
  aiAnim          anim.h
  aiCamera        camera.h
  aiLight         light.h
  aiMetadata      metadata.h
  aiScene         scene.h
  assimp          cimport.h
  aiCExport       cexport.h
  aiCFileIO       cfileio.h
  aiImporterDesc  importerdesc.h
  aiPostProcess   postprocess.h
  aiVersion       version.h
  aiConfig        config.h

The units need a Unicode Delphi (2009 or later) and work for both Win32 and Win64. Strings passed
to and from the DLL are UTF-8, so use PAnsiChar(UTF8String(S)) to pass a Delphi string.

The units import from assimp.dll (ASSIMP_DLL in aiDefs.pas). Assimp's default CMake build names the
DLL after the compiler, for example assimp-vc143-mt.dll; build with -DLIBRARY_SUFFIX= to produce
assimp.dll instead, or rename the DLL.

Two of Assimp's build options change the record layouts. If the DLL was built with either, define
the same symbol when compiling these units: ASSIMP_DOUBLE_PRECISION makes ai_real a double, and
ASSIMP_BUILD_NO_ARMATUREPOPULATE_PROCESS removes the mArmature and mNode fields of aiBone and
aiSkeletonBone.

Code written for the previous version of these units needs these changes:

  - Delphi 2009 or later, since aiString.data is now AnsiChar and strings are PAnsiChar
  - uses aiMaterial for the aiGetMaterialXXX functions and aiDefs for ASSIMP_DLL, which is now
    assimp.dll rather than assimp32.dll
  - the C names TaiFace.mNumIndices and TaiMesh.mNumAnimMeshes and mAnimMeshes, for mNumIndicies,
    mNumAniMeshes and mAniMeshes
  - TaiNode.mParent is a PaiNode, and TaiMesh.mTangents and mBitangents are PTaiVector3DArray
  - pMax of aiGetMaterialFloatArray and aiGetMaterialIntegerArray, and the outputs of
    aiGetMaterialTexture after path, are pointers, so that they can be nil

Units cannot refer to each other circularly, so the aiNode pointers in aiBone and aiSkeletonBone
(aiMesh) are declared as Pointer; cast them to PaiNode from aiScene.

See http://sourceforge.net/tracker/?func=detail&aid=3212646&group_id=226462&atid=1067634 for the original patch
