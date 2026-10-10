unit aiLight;

interface

uses aiTypes, aiVector2D, aiVector3D;

{$Z4} // 32 bit enums, as in C

type TaiLightSourceType = (
   aiLightSource_UNDEFINED = $0,
   aiLightSource_DIRECTIONAL = $1,
   aiLightSource_POINT = $2,
   aiLightSource_SPOT = $3,
   aiLightSource_AMBIENT = $4,
   aiLightSource_AREA = $5
);

type TaiLight = record
   mName: aiString;
   mType: TaiLightSourceType;
   mPosition: TaiVector3D;
   mDirection: TaiVector3D;
   mUp: TaiVector3D;
   mAttenuationConstant: single;
   mAttenuationLinear: single;
   mAttenuationQuadratic: single;
   mColorDiffuse: TaiColor3D;
   mColorSpecular: TaiColor3D;
   mColorAmbient: TaiColor3D;
   mAngleInnerCone: single;
   mAngleOuterCone: single;
   mSize: TaiVector2D;
end;
type PaiLight = ^TaiLight;
type PaiLightArray = array[0..0] of PaiLight;
type PPaiLightArray = ^PaiLightArray;

implementation

end.
