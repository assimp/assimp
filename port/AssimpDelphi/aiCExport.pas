unit aiCExport;

interface

uses aiDefs, aiTypes, aiScene, aiCFileIO;

type TaiExportFormatDesc = record
   id: PAnsiChar;
   description: PAnsiChar;
   fileExtension: PAnsiChar;
end;
type PaiExportFormatDesc = ^TaiExportFormatDesc;

type
   PaiExportDataBlob = ^TaiExportDataBlob;
   TaiExportDataBlob = record
      size: NativeUInt;
      data: Pointer;
      name: aiString;
      next: PaiExportDataBlob;
   end;

function aiGetExportFormatCount: NativeUInt; cdecl; external ASSIMP_DLL;
function aiGetExportFormatDescription(pIndex: NativeUInt): PaiExportFormatDesc; cdecl; external ASSIMP_DLL;
procedure aiReleaseExportFormatDescription(desc: PaiExportFormatDesc); cdecl; external ASSIMP_DLL;
procedure aiCopyScene(pIn: PaiScene; pOut: PPaiScene); cdecl; external ASSIMP_DLL;
procedure aiFreeScene(pIn: PaiScene); cdecl; external ASSIMP_DLL;
function aiExportScene(pScene: PaiScene; pFormatId: PAnsiChar; pFileName: PAnsiChar; pPreprocessing: Cardinal): aiReturn; cdecl; external ASSIMP_DLL;
function aiExportSceneEx(pScene: PaiScene; pFormatId: PAnsiChar; pFileName: PAnsiChar; pIO: PaiFileIO; pPreprocessing: Cardinal): aiReturn; cdecl; external ASSIMP_DLL;
function aiExportSceneToBlob(pScene: PaiScene; pFormatId: PAnsiChar; pPreprocessing: Cardinal): PaiExportDataBlob; cdecl; external ASSIMP_DLL;
procedure aiReleaseExportBlob(pData: PaiExportDataBlob); cdecl; external ASSIMP_DLL;

implementation

end.
