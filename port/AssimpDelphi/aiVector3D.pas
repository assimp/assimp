unit aiVector3D;

interface

uses aiDefs;

type TaiVector3D = record
   x, y, z: ai_real;
end;
type PaiVector3D = ^TaiVector3D;
type PaiVector3DArray = array [0..0] of PaiVector3D;

type TaiVector3DArray = array[0..0] of TaiVector3D;
type PTaiVector3DArray = ^TaiVector3DArray;

implementation

end.
