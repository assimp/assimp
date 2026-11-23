unit aiMaterial;

interface

uses aiDefs, aiTypes, aiVector2D, aiColor4D;

{$Z4} // 32 bit enums, as in C

const AI_DEFAULT_MATERIAL_NAME = 'DefaultMaterial';

type TaiTextureOp = (
	aiTextureOp_Multiply = $0,
	aiTextureOp_Add = $1,
	aiTextureOp_Subtract = $2,
	aiTextureOp_Divide = $3,
	aiTextureOp_SmoothAdd = $4,
	aiTextureOp_SignedAdd = $5
);
type PaiTextureOp = ^TaiTextureOp;

type TaiTextureMapMode = (
    aiTextureMapMode_Wrap = $0,
    aiTextureMapMode_Clamp = $1,
    aiTextureMapMode_Decal = $3,
    aiTextureMapMode_Mirror = $2
);
type PaiTextureMapMode = ^TaiTextureMapMode;

type TaiTextureMapping = (
    aiTextureMapping_UV = $0,
    aiTextureMapping_SPHERE = $1,
    aiTextureMapping_CYLINDER = $2,
    aiTextureMapping_BOX = $3,
    aiTextureMapping_PLANE = $4,
    aiTextureMapping_OTHER = $5
);
type PaiTextureMapping = ^TaiTextureMapping;

type TaiTextureType = (
    aiTextureType_NONE = 0,
    aiTextureType_DIFFUSE = 1,
    aiTextureType_SPECULAR = 2,
    aiTextureType_AMBIENT = 3,
    aiTextureType_EMISSIVE = 4,
    aiTextureType_HEIGHT = 5,
    aiTextureType_NORMALS = 6,
    aiTextureType_SHININESS = 7,
    aiTextureType_OPACITY = 8,
    aiTextureType_DISPLACEMENT = 9,
    aiTextureType_LIGHTMAP = 10,
    aiTextureType_REFLECTION = 11,
    aiTextureType_BASE_COLOR = 12,
    aiTextureType_NORMAL_CAMERA = 13,
    aiTextureType_EMISSION_COLOR = 14,
    aiTextureType_METALNESS = 15,
    aiTextureType_DIFFUSE_ROUGHNESS = 16,
    aiTextureType_AMBIENT_OCCLUSION = 17,
    aiTextureType_UNKNOWN = 18,
    aiTextureType_SHEEN = 19,
    aiTextureType_CLEARCOAT = 20,
    aiTextureType_TRANSMISSION = 21,
    aiTextureType_MAYA_BASE = 22,
    aiTextureType_MAYA_SPECULAR = 23,
    aiTextureType_MAYA_SPECULAR_COLOR = 24,
    aiTextureType_MAYA_SPECULAR_ROUGHNESS = 25,
    aiTextureType_ANISOTROPY = 26,
    aiTextureType_GLTF_METALLIC_ROUGHNESS = 27
);

const AI_TEXTURE_TYPE_MAX = aiTextureType_GLTF_METALLIC_ROUGHNESS;

type TaiShadingMode = (
    aiShadingMode_Flat = $1,
    aiShadingMode_Gouraud =  $2,
    aiShadingMode_Phong = $3,
    aiShadingMode_Blinn	= $4,
    aiShadingMode_Toon = $5,
    aiShadingMode_OrenNayar = $6,
    aiShadingMode_Minnaert = $7,
    aiShadingMode_CookTorrance = $8,
    aiShadingMode_NoShading = $9,
    aiShadingMode_Fresnel = $A,
    aiShadingMode_PBR_BRDF = $B
);

const aiShadingMode_Unlit = aiShadingMode_NoShading;

type TaiTextureFlags = (
	aiTextureFlags_Invert = $1,
	aiTextureFlags_UseAlpha = $2,
	aiTextureFlags_IgnoreAlpha = $4
);

type TaiBlendMode = (
	aiBlendMode_Default = $0,
	aiBlendMode_Additive = $1
);

type TaiUVTransform = record
   mTranslation: TaiVector2D;
   mScaling: TaiVector2D;
   mRotation: ai_real;
end;
type PaiUVTransform = ^TaiUVTransform;

type TaiPropertyTypeInfo = (
   aiPTI_Float   = $1,
   aiPTI_Double  = $2,
   aiPTI_String  = $3,
   aiPTI_Integer = $4,
   aiPTI_Buffer  = $5
);

type TaiMaterialProperty = record
   mKey: aiString;
   mSemantic: Cardinal;
   mIndex: Cardinal;
   mDataLength: Cardinal;
   mType: TaiPropertyTypeInfo;
   mData: PAnsiChar;
end;
type PaiMaterialProperty = ^TaiMaterialProperty;
type PaiMaterialPropertyArray = array[0..0] of PaiMaterialProperty;
type PPaiMaterialPropertyArray = ^PaiMaterialPropertyArray;

type TaiMaterial = record
   mProperties: PPaiMaterialPropertyArray;
   mNumProperties: Cardinal;
   mNumAllocated: Cardinal;
end;
type PaiMaterial = ^TaiMaterial;
type PaiMaterialArray = array[0..0] of PaiMaterial;
type PPaiMaterialArray = ^PaiMaterialArray;

// Each AI_MATKEY_XXX constant is the key of the C macro of the same name, which also supplies
// the type and index arguments: pass 0, 0 for those. The AI_MATKEY_XXX_TEXTURE constants are
// instead the texture type of the C macro, whose texture index is 0 unless noted.
const AI_MATKEY_NAME = '?mat.name';
const AI_MATKEY_TWOSIDED = '$mat.twosided';
const AI_MATKEY_SHADING_MODEL = '$mat.shadingm';
const AI_MATKEY_ENABLE_WIREFRAME = '$mat.wireframe';
const AI_MATKEY_BLEND_FUNC = '$mat.blend';
const AI_MATKEY_OPACITY = '$mat.opacity';
const AI_MATKEY_TRANSPARENCYFACTOR = '$mat.transparencyfactor';
const AI_MATKEY_BUMPSCALING = '$mat.bumpscaling';
const AI_MATKEY_SHININESS = '$mat.shininess';
const AI_MATKEY_REFLECTIVITY = '$mat.reflectivity';
const AI_MATKEY_SHININESS_STRENGTH = '$mat.shinpercent';
const AI_MATKEY_REFRACTI = '$mat.refracti';
const AI_MATKEY_COLOR_DIFFUSE = '$clr.diffuse';
const AI_MATKEY_COLOR_AMBIENT = '$clr.ambient';
const AI_MATKEY_COLOR_SPECULAR = '$clr.specular';
const AI_MATKEY_COLOR_EMISSIVE = '$clr.emissive';
const AI_MATKEY_COLOR_TRANSPARENT = '$clr.transparent';
const AI_MATKEY_COLOR_REFLECTIVE = '$clr.reflective';
const AI_MATKEY_GLOBAL_BACKGROUND_IMAGE = '?bg.global';
const AI_MATKEY_GLOBAL_SHADERLANG = '?sh.lang';
const AI_MATKEY_SHADER_VERTEX = '?sh.vs';
const AI_MATKEY_SHADER_FRAGMENT = '?sh.fs';
const AI_MATKEY_SHADER_GEO = '?sh.gs';
const AI_MATKEY_SHADER_TESSELATION = '?sh.ts';
const AI_MATKEY_SHADER_PRIMITIVE = '?sh.ps';
const AI_MATKEY_SHADER_COMPUTE = '?sh.cs';
const AI_MATKEY_USE_COLOR_MAP = '$mat.useColorMap';
const AI_MATKEY_BASE_COLOR = '$clr.base';
const AI_MATKEY_BASE_COLOR_TEXTURE = aiTextureType_BASE_COLOR;
const AI_MATKEY_USE_METALLIC_MAP = '$mat.useMetallicMap';
const AI_MATKEY_METALLIC_FACTOR = '$mat.metallicFactor';
const AI_MATKEY_METALLIC_TEXTURE = aiTextureType_METALNESS;
const AI_MATKEY_USE_ROUGHNESS_MAP = '$mat.useRoughnessMap';
const AI_MATKEY_ROUGHNESS_FACTOR = '$mat.roughnessFactor';
const AI_MATKEY_ROUGHNESS_TEXTURE = aiTextureType_DIFFUSE_ROUGHNESS;
const AI_MATKEY_ANISOTROPY_FACTOR = '$mat.anisotropyFactor';
const AI_MATKEY_ANISOTROPY_ROTATION = '$mat.anisotropyRotation';
const AI_MATKEY_ANISOTROPY_TEXTURE = aiTextureType_ANISOTROPY;
const AI_MATKEY_SPECULAR_FACTOR = '$mat.specularFactor';
const AI_MATKEY_GLOSSINESS_FACTOR = '$mat.glossinessFactor';
const AI_MATKEY_SHEEN_COLOR_FACTOR = '$clr.sheen.factor';
const AI_MATKEY_SHEEN_ROUGHNESS_FACTOR = '$mat.sheen.roughnessFactor';
const AI_MATKEY_SHEEN_COLOR_TEXTURE = aiTextureType_SHEEN;
const AI_MATKEY_SHEEN_ROUGHNESS_TEXTURE = aiTextureType_SHEEN; // texture index 1
const AI_MATKEY_CLEARCOAT_FACTOR = '$mat.clearcoat.factor';
const AI_MATKEY_CLEARCOAT_ROUGHNESS_FACTOR = '$mat.clearcoat.roughnessFactor';
const AI_MATKEY_CLEARCOAT_TEXTURE = aiTextureType_CLEARCOAT;
const AI_MATKEY_CLEARCOAT_ROUGHNESS_TEXTURE = aiTextureType_CLEARCOAT; // texture index 1
const AI_MATKEY_CLEARCOAT_NORMAL_TEXTURE = aiTextureType_CLEARCOAT; // texture index 2
const AI_MATKEY_TRANSMISSION_FACTOR = '$mat.transmission.factor';
const AI_MATKEY_TRANSMISSION_TEXTURE = aiTextureType_TRANSMISSION;
const AI_MATKEY_VOLUME_THICKNESS_FACTOR = '$mat.volume.thicknessFactor';
const AI_MATKEY_VOLUME_THICKNESS_TEXTURE = aiTextureType_TRANSMISSION; // texture index 1
const AI_MATKEY_VOLUME_ATTENUATION_DISTANCE = '$mat.volume.attenuationDistance';
const AI_MATKEY_VOLUME_ATTENUATION_COLOR = '$mat.volume.attenuationColor';
const AI_MATKEY_USE_EMISSIVE_MAP = '$mat.useEmissiveMap';
const AI_MATKEY_EMISSIVE_INTENSITY = '$mat.emissiveIntensity';
const AI_MATKEY_USE_AO_MAP = '$mat.useAOMap';

// Keys of the per-texture properties; the type argument is the texture type, the index the
// texture index
const _AI_MATKEY_TEXTURE_BASE = '$tex.file';
const _AI_MATKEY_UVWSRC_BASE = '$tex.uvwsrc';
const _AI_MATKEY_TEXOP_BASE = '$tex.op';
const _AI_MATKEY_MAPPING_BASE = '$tex.mapping';
const _AI_MATKEY_TEXBLEND_BASE = '$tex.blend';
const _AI_MATKEY_MAPPINGMODE_U_BASE = '$tex.mapmodeu';
const _AI_MATKEY_MAPPINGMODE_V_BASE = '$tex.mapmodev';
const _AI_MATKEY_TEXMAP_AXIS_BASE = '$tex.mapaxis';
const _AI_MATKEY_UVTRANSFORM_BASE = '$tex.uvtrafo';
const _AI_MATKEY_TEXFLAGS_BASE = '$tex.flags';

function aiTextureTypeToString(nType: TaiTextureType): PAnsiChar; cdecl; external ASSIMP_DLL;
function aiGetMaterialProperty( pMat: PaiMaterial; pKey: PAnsiChar; nType: Cardinal; nIndex: Cardinal; pPropOut: pointer): aiReturn; cdecl; external ASSIMP_DLL;
function aiGetMaterialFloatArray( var pMat: TaiMaterial; pKey: PAnsiChar; nType: Cardinal; nIndex: Cardinal; var pOut: ai_real; pMax: PCardinal): aiReturn; cdecl; external ASSIMP_DLL;
function aiGetMaterialFloat( var pMat: TaiMaterial; pKey: PAnsiChar; nType: Cardinal; nIndex: Cardinal; var pOut: ai_real): aiReturn;
function aiGetMaterialIntegerArray(var pMat: TaiMaterial; pKey: PAnsiChar; nType: Cardinal; nIndex: Cardinal; var pOut: Integer; pMax: PCardinal): aiReturn; cdecl; external ASSIMP_DLL;
function aiGetMaterialInteger(var pMat: TaiMaterial; pKey: PAnsiChar; nType: Cardinal; nIndex: Cardinal; var pOut: Integer): aiReturn;
function aiGetMaterialColor(var pMat: TaiMaterial; pKey: PAnsiChar; nType: Cardinal; nIndex: Cardinal; var pOut: TaiColor4D): aiReturn; cdecl; external ASSIMP_DLL;
function aiGetMaterialUVTransform(var pMat: TaiMaterial; pKey: PAnsiChar; nType: Cardinal; nIndex: Cardinal; var pOut: TaiUVTransform): aiReturn; cdecl; external ASSIMP_DLL;
function aiGetMaterialString(var pMat: TaiMaterial; pKey: PAnsiChar; nType: Cardinal; nIndex: Cardinal; var pOut: aiString): aiReturn; cdecl; external ASSIMP_DLL;
function aiGetMaterialTextureCount(var pMat: TaiMaterial; nType: TaiTextureType): Cardinal; cdecl; external ASSIMP_DLL;
function aiGetMaterialTexture(var mat: TaiMaterial; nType: TaiTextureType; nIndex: Cardinal; var path: aiString; mapping: PaiTextureMapping; uvindex: PCardinal; blend: Pai_real; op: PaiTextureOp; mapmode: PaiTextureMapMode; flags: PCardinal): aiReturn; cdecl; external ASSIMP_DLL;


implementation

// aiGetMaterialFloat and aiGetMaterialInteger are inline functions in C, so the DLL does not export them

function aiGetMaterialFloat( var pMat: TaiMaterial; pKey: PAnsiChar; nType: Cardinal; nIndex: Cardinal; var pOut: ai_real): aiReturn;
begin
   result := aiGetMaterialFloatArray( pMat, pKey, nType, nIndex, pOut, nil);
end;

function aiGetMaterialInteger(var pMat: TaiMaterial; pKey: PAnsiChar; nType: Cardinal; nIndex: Cardinal; var pOut: integer): aiReturn;
begin
   result := aiGetMaterialIntegerArray( pMat, pKey, nType, nIndex, pOut, nil);
end;

end.
