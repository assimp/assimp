unit assimp;

interface

uses aiDefs, aiTypes, aiVector2D, aiVector3D, aiQuaternion, aiMatrix3x3, aiMatrix4x4, aiScene, aiTexture,
  aiCFileIO, aiImporterDesc;

type aiBool = Integer;
const AI_FALSE = 0;
const AI_TRUE = 1;

type TaiLogStreamCallback = procedure(message: PAnsiChar; user: PAnsiChar); cdecl;

type TaiLogStream = record
   callback: TaiLogStreamCallback;
   user: PAnsiChar;
end;
type PaiLogStream = ^TaiLogStream;

// Opaque: create with aiCreatePropertyStore and release with aiReleasePropertyStore
type TaiPropertyStore = record
   sentinel: AnsiChar;
end;
type PaiPropertyStore = ^TaiPropertyStore;

function  aiImportFile(filename: PAnsiChar; pFlags: Cardinal): PaiScene; cdecl; external ASSIMP_DLL;
function  aiImportFileEx(pFile: PAnsiChar; pFlags: Cardinal; pFS: PaiFileIO): PaiScene; cdecl; external ASSIMP_DLL;
function  aiImportFileExWithProperties(pFile: PAnsiChar; pFlags: Cardinal; pFS: PaiFileIO; pProps: PaiPropertyStore): PaiScene; cdecl; external ASSIMP_DLL;
function  aiImportFileFromMemory(pBuffer: Pointer; pLength: Cardinal; pFlags: Cardinal; pHint: PAnsiChar): PaiScene; cdecl; external ASSIMP_DLL;
function  aiImportFileFromMemoryWithProperties(pBuffer: Pointer; pLength: Cardinal; pFlags: Cardinal; pHint: PAnsiChar; pProps: PaiPropertyStore): PaiScene; cdecl; external ASSIMP_DLL;
function  aiApplyPostProcessing(pScene: PaiScene; pFlags: Cardinal): PaiScene; cdecl; external ASSIMP_DLL;
function  aiGetPredefinedLogStream(pStreams: TaiDefaultLogStream; pFile: PAnsiChar): TaiLogStream;{$IFNDEF CPUX86} cdecl; external ASSIMP_DLL;{$ENDIF}
procedure aiAttachLogStream(var stream: TaiLogStream); cdecl; external ASSIMP_DLL;
procedure aiEnableVerboseLogging(d: aiBool); cdecl; external ASSIMP_DLL;
function  aiDetachLogStream(var stream: TaiLogStream): aiReturn; cdecl; external ASSIMP_DLL;
procedure aiDetachAllLogStreams; cdecl; external ASSIMP_DLL;
procedure aiReleaseImport( pScene: PaiScene); cdecl; external ASSIMP_DLL;
function  aiGetErrorString(): PAnsiChar; cdecl; external ASSIMP_DLL;
function  aiIsExtensionSupported(szExtension: PAnsiChar): aiBool; cdecl; external ASSIMP_DLL;
procedure aiGetExtensionList(var szOut: aiString); cdecl; external ASSIMP_DLL;
procedure aiGetMemoryRequirements(pIn: PaiScene; var info: TaiMemoryInfo); cdecl; external ASSIMP_DLL;
function  aiGetEmbeddedTexture(pScene: PaiScene; filename: PAnsiChar): PaiTexture; cdecl; external ASSIMP_DLL;
function  aiCreatePropertyStore: PaiPropertyStore; cdecl; external ASSIMP_DLL;
procedure aiReleasePropertyStore(p: PaiPropertyStore); cdecl; external ASSIMP_DLL;
procedure aiSetImportPropertyInteger(store: PaiPropertyStore; szName: PAnsiChar; value: Integer); cdecl; external ASSIMP_DLL;
procedure aiSetImportPropertyFloat(store: PaiPropertyStore; szName: PAnsiChar; value: ai_real); cdecl; external ASSIMP_DLL;
procedure aiSetImportPropertyString(store: PaiPropertyStore; szName: PAnsiChar; var st: aiString); cdecl; external ASSIMP_DLL;
procedure aiSetImportPropertyMatrix(store: PaiPropertyStore; szName: PAnsiChar; var mat: TaiMatrix4x4); cdecl; external ASSIMP_DLL;
function  aiGetImportFormatCount: NativeUInt; cdecl; external ASSIMP_DLL;
function  aiGetImportFormatDescription(pIndex: NativeUInt): PaiImporterDesc; cdecl; external ASSIMP_DLL;

procedure aiCreateQuaternionFromMatrix( var quat: TaiQuaternion; var mat: TaiMatrix3x3); cdecl; external ASSIMP_DLL;
procedure aiDecomposeMatrix( var mat: TaiMatrix4x4; var scaling: TaiVector3D; var rotation: TaiQuaternion; var position: TaiVector3D); cdecl; external ASSIMP_DLL;
procedure aiTransposeMatrix4( var mat: TaiMatrix4x4); cdecl; external ASSIMP_DLL;
procedure aiTransposeMatrix3( var mat: TaiMatrix3x3); cdecl; external ASSIMP_DLL;
procedure aiTransformVecByMatrix3( var vec: TaiVector3D; var mat: TaiMatrix3x3); cdecl; external ASSIMP_DLL;
procedure aiTransformVecByMatrix4( var vec: TaiVector3D; var mat: TaiMatrix4x4); cdecl; external ASSIMP_DLL;
procedure aiMultiplyMatrix4(var dst: TaiMatrix4x4; var src: TaiMatrix4x4); cdecl; external ASSIMP_DLL;
procedure aiMultiplyMatrix3(var dst: TaiMatrix3x3; var src: TaiMatrix3x3); cdecl; external ASSIMP_DLL;
procedure aiIdentityMatrix3(var mat: TaiMatrix3x3); cdecl; external ASSIMP_DLL;
procedure aiIdentityMatrix4(var mat: TaiMatrix4x4); cdecl; external ASSIMP_DLL;

function  aiVector2AreEqual(var a: TaiVector2D; var b: TaiVector2D): Integer; cdecl; external ASSIMP_DLL;
function  aiVector2AreEqualEpsilon(var a: TaiVector2D; var b: TaiVector2D; epsilon: Single): Integer; cdecl; external ASSIMP_DLL;
procedure aiVector2Add(var dst: TaiVector2D; var src: TaiVector2D); cdecl; external ASSIMP_DLL;
procedure aiVector2Subtract(var dst: TaiVector2D; var src: TaiVector2D); cdecl; external ASSIMP_DLL;
procedure aiVector2Scale(var dst: TaiVector2D; s: Single); cdecl; external ASSIMP_DLL;
procedure aiVector2SymMul(var dst: TaiVector2D; var other: TaiVector2D); cdecl; external ASSIMP_DLL;
procedure aiVector2DivideByScalar(var dst: TaiVector2D; s: Single); cdecl; external ASSIMP_DLL;
procedure aiVector2DivideByVector(var dst: TaiVector2D; var v: TaiVector2D); cdecl; external ASSIMP_DLL;
function  aiVector2Length(var v: TaiVector2D): ai_real; cdecl; external ASSIMP_DLL;
function  aiVector2SquareLength(var v: TaiVector2D): ai_real; cdecl; external ASSIMP_DLL;
procedure aiVector2Negate(var dst: TaiVector2D); cdecl; external ASSIMP_DLL;
function  aiVector2DotProduct(var a: TaiVector2D; var b: TaiVector2D): ai_real; cdecl; external ASSIMP_DLL;
procedure aiVector2Normalize(var v: TaiVector2D); cdecl; external ASSIMP_DLL;

function  aiVector3AreEqual(var a: TaiVector3D; var b: TaiVector3D): Integer; cdecl; external ASSIMP_DLL;
function  aiVector3AreEqualEpsilon(var a: TaiVector3D; var b: TaiVector3D; epsilon: Single): Integer; cdecl; external ASSIMP_DLL;
function  aiVector3LessThan(var a: TaiVector3D; var b: TaiVector3D): Integer; cdecl; external ASSIMP_DLL;
procedure aiVector3Add(var dst: TaiVector3D; var src: TaiVector3D); cdecl; external ASSIMP_DLL;
procedure aiVector3Subtract(var dst: TaiVector3D; var src: TaiVector3D); cdecl; external ASSIMP_DLL;
procedure aiVector3Scale(var dst: TaiVector3D; s: Single); cdecl; external ASSIMP_DLL;
procedure aiVector3SymMul(var dst: TaiVector3D; var other: TaiVector3D); cdecl; external ASSIMP_DLL;
procedure aiVector3DivideByScalar(var dst: TaiVector3D; s: Single); cdecl; external ASSIMP_DLL;
procedure aiVector3DivideByVector(var dst: TaiVector3D; var v: TaiVector3D); cdecl; external ASSIMP_DLL;
function  aiVector3Length(var v: TaiVector3D): ai_real; cdecl; external ASSIMP_DLL;
function  aiVector3SquareLength(var v: TaiVector3D): ai_real; cdecl; external ASSIMP_DLL;
procedure aiVector3Negate(var dst: TaiVector3D); cdecl; external ASSIMP_DLL;
function  aiVector3DotProduct(var a: TaiVector3D; var b: TaiVector3D): ai_real; cdecl; external ASSIMP_DLL;
procedure aiVector3CrossProduct(var dst: TaiVector3D; var a: TaiVector3D; var b: TaiVector3D); cdecl; external ASSIMP_DLL;
procedure aiVector3Normalize(var v: TaiVector3D); cdecl; external ASSIMP_DLL;
procedure aiVector3NormalizeSafe(var v: TaiVector3D); cdecl; external ASSIMP_DLL;
procedure aiVector3RotateByQuaternion(var v: TaiVector3D; var q: TaiQuaternion); cdecl; external ASSIMP_DLL;

procedure aiMatrix3FromMatrix4(var dst: TaiMatrix3x3; var mat: TaiMatrix4x4); cdecl; external ASSIMP_DLL;
procedure aiMatrix3FromQuaternion(var mat: TaiMatrix3x3; var q: TaiQuaternion); cdecl; external ASSIMP_DLL;
function  aiMatrix3AreEqual(var a: TaiMatrix3x3; var b: TaiMatrix3x3): Integer; cdecl; external ASSIMP_DLL;
function  aiMatrix3AreEqualEpsilon(var a: TaiMatrix3x3; var b: TaiMatrix3x3; epsilon: Single): Integer; cdecl; external ASSIMP_DLL;
procedure aiMatrix3Inverse(var mat: TaiMatrix3x3); cdecl; external ASSIMP_DLL;
function  aiMatrix3Determinant(var mat: TaiMatrix3x3): ai_real; cdecl; external ASSIMP_DLL;
procedure aiMatrix3RotationZ(var mat: TaiMatrix3x3; angle: Single); cdecl; external ASSIMP_DLL;
procedure aiMatrix3FromRotationAroundAxis(var mat: TaiMatrix3x3; var axis: TaiVector3D; angle: Single); cdecl; external ASSIMP_DLL;
procedure aiMatrix3Translation(var mat: TaiMatrix3x3; var translation: TaiVector2D); cdecl; external ASSIMP_DLL;
procedure aiMatrix3FromTo(var mat: TaiMatrix3x3; var from: TaiVector3D; var &to: TaiVector3D); cdecl; external ASSIMP_DLL;

procedure aiMatrix4FromMatrix3(var dst: TaiMatrix4x4; var mat: TaiMatrix3x3); cdecl; external ASSIMP_DLL;
procedure aiMatrix4FromScalingQuaternionPosition(var mat: TaiMatrix4x4; var scaling: TaiVector3D; var rotation: TaiQuaternion; var position: TaiVector3D); cdecl; external ASSIMP_DLL;
procedure aiMatrix4Add(var dst: TaiMatrix4x4; var src: TaiMatrix4x4); cdecl; external ASSIMP_DLL;
function  aiMatrix4AreEqual(var a: TaiMatrix4x4; var b: TaiMatrix4x4): Integer; cdecl; external ASSIMP_DLL;
function  aiMatrix4AreEqualEpsilon(var a: TaiMatrix4x4; var b: TaiMatrix4x4; epsilon: Single): Integer; cdecl; external ASSIMP_DLL;
procedure aiMatrix4Inverse(var mat: TaiMatrix4x4); cdecl; external ASSIMP_DLL;
function  aiMatrix4Determinant(var mat: TaiMatrix4x4): ai_real; cdecl; external ASSIMP_DLL;
function  aiMatrix4IsIdentity(var mat: TaiMatrix4x4): Integer; cdecl; external ASSIMP_DLL;
procedure aiMatrix4DecomposeIntoScalingEulerAnglesPosition(var mat: TaiMatrix4x4; var scaling: TaiVector3D; var rotation: TaiVector3D; var position: TaiVector3D); cdecl; external ASSIMP_DLL;
procedure aiMatrix4DecomposeIntoScalingAxisAnglePosition(var mat: TaiMatrix4x4; var scaling: TaiVector3D; var axis: TaiVector3D; var angle: ai_real; var position: TaiVector3D); cdecl; external ASSIMP_DLL;
procedure aiMatrix4DecomposeNoScaling(var mat: TaiMatrix4x4; var rotation: TaiQuaternion; var position: TaiVector3D); cdecl; external ASSIMP_DLL;
procedure aiMatrix4FromEulerAngles(var mat: TaiMatrix4x4; x: Single; y: Single; z: Single); cdecl; external ASSIMP_DLL;
procedure aiMatrix4RotationX(var mat: TaiMatrix4x4; angle: Single); cdecl; external ASSIMP_DLL;
procedure aiMatrix4RotationY(var mat: TaiMatrix4x4; angle: Single); cdecl; external ASSIMP_DLL;
procedure aiMatrix4RotationZ(var mat: TaiMatrix4x4; angle: Single); cdecl; external ASSIMP_DLL;
procedure aiMatrix4FromRotationAroundAxis(var mat: TaiMatrix4x4; var axis: TaiVector3D; angle: Single); cdecl; external ASSIMP_DLL;
procedure aiMatrix4Translation(var mat: TaiMatrix4x4; var translation: TaiVector3D); cdecl; external ASSIMP_DLL;
procedure aiMatrix4Scaling(var mat: TaiMatrix4x4; var scaling: TaiVector3D); cdecl; external ASSIMP_DLL;
procedure aiMatrix4FromTo(var mat: TaiMatrix4x4; var from: TaiVector3D; var &to: TaiVector3D); cdecl; external ASSIMP_DLL;

procedure aiQuaternionFromEulerAngles(var q: TaiQuaternion; x: Single; y: Single; z: Single); cdecl; external ASSIMP_DLL;
procedure aiQuaternionFromAxisAngle(var q: TaiQuaternion; var axis: TaiVector3D; angle: Single); cdecl; external ASSIMP_DLL;
procedure aiQuaternionFromNormalizedQuaternion(var q: TaiQuaternion; var normalized: TaiVector3D); cdecl; external ASSIMP_DLL;
function  aiQuaternionAreEqual(var a: TaiQuaternion; var b: TaiQuaternion): Integer; cdecl; external ASSIMP_DLL;
function  aiQuaternionAreEqualEpsilon(var a: TaiQuaternion; var b: TaiQuaternion; epsilon: Single): Integer; cdecl; external ASSIMP_DLL;
procedure aiQuaternionNormalize(var q: TaiQuaternion); cdecl; external ASSIMP_DLL;
procedure aiQuaternionConjugate(var q: TaiQuaternion); cdecl; external ASSIMP_DLL;
procedure aiQuaternionMultiply(var dst: TaiQuaternion; var q: TaiQuaternion); cdecl; external ASSIMP_DLL;
procedure aiQuaternionInterpolate(var dst: TaiQuaternion; var start: TaiQuaternion; var &end: TaiQuaternion; factor: Single); cdecl; external ASSIMP_DLL;


implementation

{$IFDEF CPUX86}
// On Win32 the C compiler returns this 8 byte record in EDX:EAX, where Delphi expects a hidden
// result pointer, so the import returns the record's bytes as a 64 bit integer
function aiGetPredefinedLogStreamBytes(pStreams: TaiDefaultLogStream; pFile: PAnsiChar): UInt64; cdecl; external ASSIMP_DLL name 'aiGetPredefinedLogStream';

function aiGetPredefinedLogStream(pStreams: TaiDefaultLogStream; pFile: PAnsiChar): TaiLogStream;
var
   bytes: UInt64;
begin
   bytes := aiGetPredefinedLogStreamBytes(pStreams, pFile);
   Move(bytes, result, SizeOf(result));
end;
{$ENDIF}

end.
