unit aiCFileIO;

interface

uses aiTypes;

type
   PaiFileIO = ^TaiFileIO;
   PaiFile = ^TaiFile;

   TaiFileWriteProc = function(pFile: PaiFile; pBuffer: PAnsiChar; size, count: NativeUInt): NativeUInt; cdecl;
   TaiFileReadProc = function(pFile: PaiFile; pBuffer: PAnsiChar; size, count: NativeUInt): NativeUInt; cdecl;
   TaiFileTellProc = function(pFile: PaiFile): NativeUInt; cdecl;
   TaiFileFlushProc = procedure(pFile: PaiFile); cdecl;
   TaiFileSeek = function(pFile: PaiFile; offset: NativeUInt; origin: TaiOrigin): aiReturn; cdecl;

   TaiFileOpenProc = function(pIO: PaiFileIO; pFileName, pMode: PAnsiChar): PaiFile; cdecl;
   TaiFileCloseProc = procedure(pIO: PaiFileIO; pFile: PaiFile); cdecl;

   TaiUserData = PAnsiChar;

   TaiFileIO = record
      OpenProc: TaiFileOpenProc;
      CloseProc: TaiFileCloseProc;
      UserData: TaiUserData;
   end;

   TaiFile = record
      ReadProc: TaiFileReadProc;
      WriteProc: TaiFileWriteProc;
      TellProc: TaiFileTellProc;
      FileSizeProc: TaiFileTellProc;
      SeekProc: TaiFileSeek;
      FlushProc: TaiFileFlushProc;
      UserData: TaiUserData;
   end;

implementation

end.
