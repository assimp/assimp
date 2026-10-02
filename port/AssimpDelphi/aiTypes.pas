unit aiTypes;

interface

uses aiDefs, aiVector3D;

{$Z4} // 32 bit enums, as in C

//added for Delphi interface
type
   TCardinalArray = array [0..0] of Cardinal;
   PCardinalArray = ^TCardinalArray;

   TSingleArray = array[0..0] of Single;
   PSingleArray = ^TSingleArray;

   TDoubleArray = array[0..0] of Double;
   PDoubleArray = ^TDoubleArray;

type ai_int32 = Integer;
type ai_uint32 = Cardinal;

const AI_MAXLEN = 1024;

type aiString = record
   length: ai_uint32;
   data: array [0..AI_MAXLEN-1] of AnsiChar;
end;
type PaiString = ^aiString;
type PaiStringArray = array[0..0] of PaiString;
type PPaiStringArray = ^PaiStringArray;

type aiReturn = (
	aiReturn_SUCCESS = $0,
	aiReturn_FAILURE = -$1,
	aiReturn_OUTOFMEMORY = -$3,
	_AI_ENFORCE_ENUM_SIZE = $7fffffff
);

const AI_SUCCESS = aiReturn_SUCCESS;
const AI_FAILURE = aiReturn_FAILURE;
const AI_OUTOFMEMORY = aiReturn_OUTOFMEMORY;

type TaiOrigin = (
   aiOrigin_SET = $0,
   aiOrigin_CUR = $1,
   aiOrigin_END = $2,
   _AI_ORIGIN_ENFORCE_ENUM_SIZE = $7fffffff
);

type TaiDefaultLogStream = (
   aiDefaultLogStream_FILE = $1,
   aiDefaultLogStream_STDOUT = $2,
   aiDefaultLogStream_STDERR = $4,
   aiDefaultLogStream_DEBUGGER = $8,
   _AI_DLS_ENFORCE_ENUM_SIZE = $7fffffff
);

const DLS_FILE = aiDefaultLogStream_FILE;
const DLS_STDOUT = aiDefaultLogStream_STDOUT;
const DLS_STDERR = aiDefaultLogStream_STDERR;
const DLS_DEBUGGER = aiDefaultLogStream_DEBUGGER;

type TaiPlane = record
   a, b, c, d: ai_real;
end;
type PaiPlane = ^TaiPlane;

type TaiRay = record
   pos, dir: TaiVector3D;
end;
type PaiRay = ^TaiRay;

type TaiColor3D = record
   r, g, b: single;
end;
type PaiColor3D = ^TaiColor3D;

type TaiMemoryInfo = record
   textures: Cardinal;
   materials: Cardinal;
   meshes: Cardinal;
   nodes: Cardinal;
   animations: Cardinal;
   cameras: Cardinal;
   lights: Cardinal;
   total: Cardinal;
end;
type PaiMemoryInfo = ^TaiMemoryInfo;

type TaiBuffer = record
   data: PAnsiChar;
   &end: PAnsiChar;
end;
type PaiBuffer = ^TaiBuffer;


function aiStringToDelphiString(const a: aiString): UTF8String;


implementation

function aiStringToDelphiString(const a: aiString): UTF8String;
begin
   SetString(result, PAnsiChar(@a.data), a.length);
end;

end.
