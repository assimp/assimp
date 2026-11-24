unit aiTexture;

interface

uses aiTypes;

const AI_EMBEDDED_TEXNAME_PREFIX = '*';

type TaiTexel = record
   b, g, r, a: byte;
end;
PaiTexel = ^TaiTexel;
TaiTexelArray = array[0..0] of TaiTexel;
PaiTexelArray = ^TaiTexelArray;

const HINTMAXTEXTURELEN = 9;

type TaiTexture = record
   mWidth: Cardinal; //width in pixels, OR total embedded file size if texture is a jpg/png/etc
   mHeight: Cardinal; //0 if texture is an embedded file
   achFormatHint: array[0..HINTMAXTEXTURELEN-1] of AnsiChar;
   pcData: PaiTexelArray;
   mFilename: aiString;
end;
PaiTexture = ^TaiTexture;
PaiTextureArray = array [0..0] of PaiTexture;
PPaiTextureArray = ^PaiTextureArray;



implementation

end.
