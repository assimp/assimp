unit aiScene;

interface

uses aiTypes, aiMatrix4x4, aiMesh, aiMaterial, aiTexture, aiAnim, aiCamera, aiLight, aiMetadata;

const AI_SCENE_FLAGS_INCOMPLETE = $1;
const AI_SCENE_FLAGS_VALIDATED = $2;
const AI_SCENE_FLAGS_VALIDATION_WARNING = $4;
const AI_SCENE_FLAGS_NON_VERBOSE_FORMAT = $8;
const AI_SCENE_FLAGS_TERRAIN = $10;
const AI_SCENE_FLAGS_ALLOW_SHARED = $20;

type
  PaiNode = ^TaiNode;
  PPaiNode = ^PaiNode;
  PaiNodeArray = array[0..0] of PaiNode;
  PPaiNodeArray = ^PaiNodeArray;

  TaiNode = record
   mName: aiString;
   mTransformation: TaiMatrix4x4;
   mParent: PaiNode;
   mNumChildren: cardinal;
   mChildren: PPaiNodeArray;
   mNumMeshes: cardinal;
   mMeshes: PCardinalArray;
   mMetaData: PaiMetadata;
  end;

type TaiScene = record
   mFlags: cardinal;
   mRootNode: PaiNode;
   mNumMeshes: Cardinal;
   mMeshes: PPaiMeshArray;
   mNumMaterials: Cardinal;
   mMaterials: PPaiMaterialArray;
   mNumAnimations: Cardinal;
   mAnimations: PPaiAnimationArray;
   mNumTextures: Cardinal;
   mTextures: PPaiTextureArray;
   mNumLights: Cardinal;
   mLights: PPaiLightArray;
   mNumCameras: Cardinal;
   mCameras: PPaiCameraArray;
   mMetaData: PaiMetadata;
   mName: aiString;
   mNumSkeletons: Cardinal;
   mSkeletons: PPaiSkeletonArray;
   mPrivate: PAnsiChar;
end;
type PaiScene = ^TaiScene;
type PPaiScene = ^PaiScene;

implementation

end.
