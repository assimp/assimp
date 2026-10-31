unit aiMetadata;

interface

uses aiTypes;

{$Z4} // 32 bit enums, as in C

type TaiMetadataType = (
   AI_BOOL       = 0,
   AI_INT32      = 1,
   AI_UINT64     = 2,
   AI_FLOAT      = 3,
   AI_DOUBLE     = 4,
   AI_AISTRING   = 5,
   AI_AIVECTOR3D = 6,
   AI_AIMETADATA = 7,
   AI_INT64      = 8,
   AI_UINT32     = 9,
   AI_META_MAX   = 10
);

type TaiMetadataEntry = record
   mType: TaiMetadataType;
   mData: Pointer;
end;
type PaiMetadataEntry = ^TaiMetadataEntry;
type TaiMetadataEntryArray = array[0..0] of TaiMetadataEntry;
type PTaiMetadataEntryArray = ^TaiMetadataEntryArray;

type TaiStringArray = array[0..0] of aiString;
type PTaiStringArray = ^TaiStringArray;

type TaiMetadata = record
   mNumProperties: cardinal;
   mKeys: PTaiStringArray;
   mValues: PTaiMetadataEntryArray;
end;
type PaiMetadata = ^TaiMetadata;

implementation

end.
