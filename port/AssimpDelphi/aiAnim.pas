unit aiAnim;

interface

uses aiTypes, aiVector3D, aiQuaternion;

{$Z4} // 32 bit enums, as in C

type TaiAnimInterpolation = (
   aiAnimInterpolation_Step,
   aiAnimInterpolation_Linear,
   aiAnimInterpolation_Spherical_Linear,
   aiAnimInterpolation_Cubic_Spline
);

type TaiVectorKey = record
   mTime: double;
   mValue: TaiVector3D;
   mInterpolation: TaiAnimInterpolation;
end;
type PaiVectorKey = ^TaiVectorKey;
type TaiVectorKeyArray = array[0..0] of TaiVectorKey;
type PTaiVectorKeyArray = ^TaiVectorKeyArray;

type TaiQuatKey = record
   mTime: double;
   mValue: TaiQuaternion;
   mInterpolation: TaiAnimInterpolation;
end;
type PaiQuatKey = ^TaiQuatKey;
type TaiQuatKeyArray = array[0..0] of TaiQuatKey;
type PTaiQuatKeyArray = ^TaiQuatKeyArray;

type TaiMeshKey = record
   mTime: double;
   mValue: cardinal;
end;
type PaiMeshKey = ^TaiMeshKey;
type TaiMeshKeyArray = array[0..0] of TaiMeshKey;
type PTaiMeshKeyArray = ^TaiMeshKeyArray;

type TaiMeshMorphKey = record
   mTime: double;
   mValues: PCardinalArray;
   mWeights: PDoubleArray;
   mNumValuesAndWeights: cardinal;
end;
type PaiMeshMorphKey = ^TaiMeshMorphKey;
type TaiMeshMorphKeyArray = array[0..0] of TaiMeshMorphKey;
type PTaiMeshMorphKeyArray = ^TaiMeshMorphKeyArray;

type TaiAnimBehaviour = (
   aiAnimBehaviour_DEFAULT = $0,
   aiAnimBehaviour_CONSTANT = $1,
   aiAnimBehaviour_LINEAR = $2,
   aiAnimBehaviour_REPEAT = $3
);

type TaiNodeAnim = record
   mNodeName: aiString;
   mNumPositionKeys: cardinal;
   mPositionKeys: PTaiVectorKeyArray;
   mNumRotationKeys: cardinal;
   mRotationKeys: PTaiQuatKeyArray;
   mNumScalingKeys: cardinal;
   mScalingKeys: PTaiVectorKeyArray;
   mPreState: TaiAnimBehaviour;
   mPostState: TaiAnimBehaviour;
end;
type PaiNodeAnim = ^TaiNodeAnim;
type PaiNodeAnimArray = array[0..0] of PaiNodeAnim;
type PPaiNodeAnimArray = ^PaiNodeAnimArray;

type TaiMeshAnim = record
   mName: aiString;
   mNumKeys: cardinal;
   mKeys: PTaiMeshKeyArray;
end;
type PaiMeshAnim = ^TaiMeshAnim;
type PaiMeshAnimArray = array[0..0] of PaiMeshAnim;
type PPaiMeshAnimArray = ^PaiMeshAnimArray;

type TaiMeshMorphAnim = record
   mName: aiString;
   mNumKeys: cardinal;
   mKeys: PTaiMeshMorphKeyArray;
end;
type PaiMeshMorphAnim = ^TaiMeshMorphAnim;
type PaiMeshMorphAnimArray = array[0..0] of PaiMeshMorphAnim;
type PPaiMeshMorphAnimArray = ^PaiMeshMorphAnimArray;

type TaiAnimation = record
   mName: aiString;
   mDuration: double;
   mTicksPerSecond: double;
   mNumChannels: cardinal;
   mChannels: PPaiNodeAnimArray;
   mNumMeshChannels: cardinal;
   mMeshChannels: PPaiMeshAnimArray;
   mNumMorphMeshChannels: cardinal;
   mMorphMeshChannels: PPaiMeshMorphAnimArray;
end;
type PaiAnimation = ^TaiAnimation;
type PaiAnimationArray = array[0..0] of PaiAnimation;
type PPaiAnimationArray = ^PaiAnimationArray;

implementation

end.
