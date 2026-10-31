unit aiMesh;

interface

uses aiDefs, aiTypes, aiMatrix4x4, aiVector3D, aiColor4D, aiAABB;

{$Z4} // 32 bit enums, as in C

const
   AI_MAX_FACE_INDICES = $7fff;
   AI_MAX_BONE_WEIGHTS = $7fffffff;
   AI_MAX_VERTICES = $7fffffff;
   AI_MAX_FACES = $7fffffff;
   AI_MAX_NUMBER_OF_COLOR_SETS = $8;
   AI_MAX_NUMBER_OF_TEXTURECOORDS = $8;

type TaiFace = record
   mNumIndices: cardinal;
   mIndices: PCardinalArray;
end;
type PaiFace = ^TaiFace;
type PaiFaceArray = array [0..0] of PaiFace;

type TaiFaceArray = array [0..0] of TaiFace;
type PTaiFaceArray = ^TaiFaceArray;

type TaiVertexWeight = record
   mVertexId: cardinal;
   mWeight: ai_real;
end;
type PaiVertexWeight = ^TaiVertexWeight;
type TaiVertexWeightArray = array[0..0] of TaiVertexWeight;
type PTaiVertexWeightArray = ^TaiVertexWeightArray;

// mArmature and mNode are aiNode pointers (PaiNode in aiScene), which this unit cannot name
// without a circular unit reference. Define ASSIMP_BUILD_NO_ARMATUREPOPULATE_PROCESS when
// compiling these units if the DLL was built with it.
type TaiBone = record
   mName: aiString;
   mNumWeights: cardinal;
{$IFNDEF ASSIMP_BUILD_NO_ARMATUREPOPULATE_PROCESS}
   mArmature: Pointer;
   mNode: Pointer;
{$ENDIF}
   mWeights: PTaiVertexWeightArray;
   mOffsetMatrix: TaiMatrix4x4;
end;
type PaiBone = ^TaiBone;
type PaiBoneArray = array[0..0] of PaiBone;
type PPaiBoneArray = ^PaiBoneArray;

type TaiPrimitiveType =
   (
   	aiPrimitiveType_POINT       = $1,
   	aiPrimitiveType_LINE        = $2,
   	aiPrimitiveType_TRIANGLE    = $4,
   	aiPrimitiveType_POLYGON     = $8,
   	aiPrimitiveType_NGONEncodingFlag = $10
   );

type TaiAnimMesh = record
   mName: aiString;
   mVertices: PTaiVector3DArray;
   mNormals: PTaiVector3DArray;
   mTangents: PTaiVector3DArray;
   mBitangents: PTaiVector3DArray;
   mColors: array[0..AI_MAX_NUMBER_OF_COLOR_SETS-1] of PTaiColor4DArray;
   mTextureCoords: array[0..AI_MAX_NUMBER_OF_TEXTURECOORDS-1] of PTaiVector3DArray;
   mNumVertices: cardinal;
   mWeight: single;
end;
type PaiAnimMesh = ^TaiAnimMesh;
type PaiAnimMeshArray = array[0..0] of PaiAnimMesh;
type PPaiAnimMeshArray = ^PaiAnimMeshArray;

type TaiMorphingMethod = (
    aiMorphingMethod_UNKNOWN          = $0,
    aiMorphingMethod_VERTEX_BLEND     = $1,
    aiMorphingMethod_MORPH_NORMALIZED = $2,
    aiMorphingMethod_MORPH_RELATIVE   = $3
  );

type TaiMesh = record
   mPrimitiveTypes: cardinal;
   mNumVertices: cardinal;
   mNumFaces: cardinal;
   mVertices: PTaiVector3DArray;
   mNormals: PTaiVector3DArray;
   mTangents: PTaiVector3DArray;
   mBitangents: PTaiVector3DArray;
   mColors: array[0..AI_MAX_NUMBER_OF_COLOR_SETS-1] of PTaiColor4DArray;
   mTextureCoords: array [0..AI_MAX_NUMBER_OF_TEXTURECOORDS-1] of PTaiVector3DArray;
   mNumUVComponents: array[0..AI_MAX_NUMBER_OF_TEXTURECOORDS-1] of cardinal;
   mFaces: PTaiFaceArray;
   mNumBones: cardinal;
   mBones: PPaiBoneArray;
   mMaterialIndex: cardinal;
   mName: aiString;
   mNumAnimMeshes: cardinal;
   mAnimMeshes: PPaiAnimMeshArray;
   mMethod: TaiMorphingMethod;
   mAABB: TaiAABB;
   mTextureCoordsNames: PPaiStringArray;
end;
type PaiMesh = ^TaiMesh;
type PPaiMesh = ^PaiMesh;
type PaiMeshArray = array [0..0] of PaiMesh;
type PPaiMeshArray = ^PaiMeshArray;

// mArmature and mNode are aiNode pointers, as in TaiBone, and conditional in the same way
type TaiSkeletonBone = record
   mParent: integer;
{$IFNDEF ASSIMP_BUILD_NO_ARMATUREPOPULATE_PROCESS}
   mArmature: Pointer;
   mNode: Pointer;
{$ENDIF}
   mNumnWeights: cardinal;
   mMeshId: PaiMesh;
   mWeights: PTaiVertexWeightArray;
   mOffsetMatrix: TaiMatrix4x4;
   mLocalMatrix: TaiMatrix4x4;
end;
type PaiSkeletonBone = ^TaiSkeletonBone;
type PaiSkeletonBoneArray = array[0..0] of PaiSkeletonBone;
type PPaiSkeletonBoneArray = ^PaiSkeletonBoneArray;

type TaiSkeleton = record
   mName: aiString;
   mNumBones: cardinal;
   mBones: PPaiSkeletonBoneArray;
end;
type PaiSkeleton = ^TaiSkeleton;
type PaiSkeletonArray = array[0..0] of PaiSkeleton;
type PPaiSkeletonArray = ^PaiSkeletonArray;



implementation

end.
