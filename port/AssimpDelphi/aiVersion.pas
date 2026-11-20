unit aiVersion;

interface

uses aiDefs;

const ASSIMP_CFLAGS_SHARED = $1;
const ASSIMP_CFLAGS_STLPORT = $2;
const ASSIMP_CFLAGS_DEBUG = $4;
const ASSIMP_CFLAGS_NOBOOST = $8;
const ASSIMP_CFLAGS_SINGLETHREADED = $10;
const ASSIMP_CFLAGS_DOUBLE_SUPPORT = $20;

function aiGetLegalString: PAnsiChar; cdecl; external ASSIMP_DLL;
function aiGetVersionPatch: Cardinal; cdecl; external ASSIMP_DLL;
function aiGetVersionMinor: Cardinal; cdecl; external ASSIMP_DLL;
function aiGetVersionMajor: Cardinal; cdecl; external ASSIMP_DLL;
function aiGetVersionRevision: Cardinal; cdecl; external ASSIMP_DLL;
function aiGetBranchName: PAnsiChar; cdecl; external ASSIMP_DLL;
function aiGetCompileFlags: Cardinal; cdecl; external ASSIMP_DLL;

implementation

end.
