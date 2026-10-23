unit aiColor4D;

interface

type TaiColor4D = record
   r, g, b, a: single;
end;
type PaiColor4D = ^TaiColor4D;

type TaiColor4DArray = array[0..0] of TaiColor4D;
type PTaiColor4DArray = ^TaiColor4DArray;

implementation

end.
