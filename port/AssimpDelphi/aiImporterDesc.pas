unit aiImporterDesc;

interface

uses aiDefs;

{$Z4} // 32 bit enums, as in C

type TaiImporterFlags = (
   aiImporterFlags_SupportTextFlavour = $1,
   aiImporterFlags_SupportBinaryFlavour = $2,
   aiImporterFlags_SupportCompressedFlavour = $4,
   aiImporterFlags_LimitedSupport = $8,
   aiImporterFlags_Experimental = $10
);

type TaiImporterDesc = record
   mName: PAnsiChar;
   mAuthor: PAnsiChar;
   mMaintainer: PAnsiChar;
   mComments: PAnsiChar;
   mFlags: Cardinal;
   mMinMajor: Cardinal;
   mMinMinor: Cardinal;
   mMaxMajor: Cardinal;
   mMaxMinor: Cardinal;
   mFileExtensions: PAnsiChar;
end;
type PaiImporterDesc = ^TaiImporterDesc;

function aiGetImporterDesc(extension: PAnsiChar): PaiImporterDesc; cdecl; external ASSIMP_DLL;

implementation

end.
